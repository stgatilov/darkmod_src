/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

/*

all uncompressed
uncompressed normal maps

downsample images

16 meg Dynamic cache

Anisotropic texturing

Trilinear on all
Trilinear on normal maps, bilinear on others
Bilinear on all


Manager

->List
->Print
->Reload( bool force )

*/

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*

Anywhere that an image name is used (diffusemaps, bumpmaps, specularmaps, lights, etc),
an imageProgram can be specified.

This allows load time operations, like heightmap-to-normalmap conversion and image
composition, to be automatically handled in a way that supports timestamped reloads.

*/

/*
=================
R_HeightmapToNormalMap

it is not possible to convert a heightmap into a normal map
properly without knowing the texture coordinate stretching.
We can assume constant and equal ST vectors for walls, but not for characters.
=================
*/
static void R_HeightmapToNormalMap( byte *data, int width, int height, float scale ) {
	int		i, j;
	byte	*depth;

	scale = scale / 256;

	// copy and convert to grey scale
	j = width * height;
	depth = (byte *)R_StaticAlloc( j );

	for ( i = 0 ; i < j ; i++ ) {
		depth[i] = ( data[i*4] + data[i*4+1] + data[i*4+2] ) / 3;
	}
	idVec3	dir, dir2;

	for ( i = 0 ; i < height ; i++ ) {
		for ( j = 0 ; j < width ; j++ ) {
			int		d1, d2, d3, d4;
			int		a1, a3, a4;

			// FIXME: look at five points?
			// look at three points to estimate the gradient
			a1 = d1 = depth[ ( i * width + j ) ];
			d2 = depth[ ( i * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];
			a3 = d3 = depth[ ( ( ( i + 1 ) & ( height - 1 ) ) * width + j ) ];
			a4 = d4 = depth[ ( ( ( i + 1 ) & ( height - 1 ) ) * width + ( ( j + 1 ) & ( width - 1 ) ) ) ];

			d2 -= d1;
			d3 -= d1;

			dir[0] = -d2 * scale;
			dir[1] = -d3 * scale;
			dir[2] = 1;
			dir.NormalizeFast();

			a1 -= a3;
			a4 -= a3;

			dir2[0] = -a4 * scale;
			dir2[1] = a1 * scale;
			dir2[2] = 1;
			dir2.NormalizeFast();
	
			dir += dir2;
			dir.NormalizeFast();

			a1 = ( i * width + j ) * 4;
			data[ a1 + 0 ] = (byte)(dir[0] * 127 + 128);
			data[ a1 + 1 ] = (byte)(dir[1] * 127 + 128);
			data[ a1 + 2 ] = (byte)(dir[2] * 127 + 128);
			data[ a1 + 3 ] = 255;
		}
	}
	R_StaticFree( depth );
}


/*
=================
R_ImageScale
=================
*/
static void R_ImageScale( byte *data, int width, int height, float scale[4] ) {
	int		i, j;
	int		c;

	c = width * height * 4;

	for ( i = 0 ; i < c ; i++ ) {
		j = (byte)(data[i] * scale[i&3]);
		if ( j < 0 ) {
			j = 0;
		} else if ( j > 255 ) {
			j = 255;
		}
		data[i] = j;
	}
}

/*
=================
R_InvertAlpha
=================
*/
static void R_InvertAlpha( byte *data, int width, int height ) {
	int		i;
	int		c;

	c = width * height* 4;

	for ( i = 0 ; i < c ; i+=4 ) {
		data[i+3] = 255 - data[i+3];
	}
}

/*
=================
R_InvertColor
=================
*/
static void R_InvertColor( byte *data, int width, int height ) {
	int		i;
	int		c;

	c = width * height* 4;

	for ( i = 0 ; i < c ; i+=4 ) {
		data[i+0] = 255 - data[i+0];
		data[i+1] = 255 - data[i+1];
		data[i+2] = 255 - data[i+2];
	}
}


/*
===================
R_AddNormalMaps

===================
*/
static void R_AddNormalMaps( byte *data1, int width1, int height1, byte *data2, int width2, int height2 ) {
	int		i, j;
	byte	*newMap;

	// resample pic2 to the same size as pic1
	if ( width2 != width1 || height2 != height1 ) {
		newMap = R_Dropsample( data2, width2, height2, width1, height1 );
		data2 = newMap;
	} else {
		newMap = nullptr;
	}

	// add the normal change from the second and renormalize
	for ( i = 0 ; i < height1 ; i++ ) {
		for ( j = 0 ; j < width1 ; j++ ) {
			byte	*d1, *d2;
			idVec3	n;
			float   len;

			d1 = data1 + ( i * width1 + j ) * 4;
			d2 = data2 + ( i * width1 + j ) * 4;

			n[0] = ( d1[0] - 128 ) / 127.0;
			n[1] = ( d1[1] - 128 ) / 127.0;
			n[2] = ( d1[2] - 128 ) / 127.0;

			// There are some normal maps that blend to 0,0,0 at the edges
			// this screws up compression, so we try to correct that here by instead fading it to 0,0,1
			len = n.LengthFast();
			if ( len < 1.0f ) {
				n[2] = idMath::Sqrt(1.0 - (n[0]*n[0]) - (n[1]*n[1]));
			}
			n[0] += ( d2[0] - 128 ) / 127.0;
			n[1] += ( d2[1] - 128 ) / 127.0;
			n.Normalize();

			d1[0] = (byte)(n[0] * 127 + 128);
			d1[1] = (byte)(n[1] * 127 + 128);
			d1[2] = (byte)(n[2] * 127 + 128);
			d1[3] = 255;
		}
	}

	if ( newMap ) {
		R_StaticFree( newMap );
	}
}

/*
================
R_SmoothNormalMap
================
*/
static void R_SmoothNormalMap( byte *data, int width, int height ) {
	byte	*orig;
	int		i, j, k, l;
	idVec3	normal;
	byte	*out;
	static float	factors[3][3] = {
		{ 1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, 1, 1 }
	};

	orig = (byte *)R_StaticAlloc( width * height * 4 );
	memcpy( orig, data, width * height * 4 );

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			normal = vec3_origin;
			for ( k = -1 ; k < 2 ; k++ ) {
				for ( l = -1 ; l < 2 ; l++ ) {
					byte	*in;

					in = orig + ( ((j+l)&(height-1))*width + ((i+k)&(width-1)) ) * 4;

					// ignore 000 and -1 -1 -1
					if ( in[0] == 0 && in[1] == 0 && in[2] == 0 ) {
						continue;
					}

					if ( in[0] == 128 && in[1] == 128 && in[2] == 128 ) {
						continue;
					}
					normal[0] += factors[k+1][l+1] * ( in[0] - 128 );
					normal[1] += factors[k+1][l+1] * ( in[1] - 128 );
					normal[2] += factors[k+1][l+1] * ( in[2] - 128 );
				}
			}
			normal.Normalize();
			out = data + ( j * width + i ) * 4;
			out[0] = (byte)(128 + 127 * normal[0]);
			out[1] = (byte)(128 + 127 * normal[1]);
			out[2] = (byte)(128 + 127 * normal[2]);
		}
	}
	R_StaticFree( orig );
}


/*
===================
R_ImageAdd

===================
*/
static void R_ImageAdd( byte *data1, int width1, int height1, byte *data2, int width2, int height2 ) {
	int		i, j;
	int		c;
	byte	*newMap;

	// resample pic2 to the same size as pic1
	if ( width2 != width1 || height2 != height1 ) {
		newMap = R_Dropsample( data2, width2, height2, width1, height1 );
		data2 = newMap;
	} else {
		newMap = nullptr;
	}
	c = width1 * height1 * 4;

	for ( i = 0 ; i < c ; i++ ) {
		j = data1[i] + data2[i];
		if ( j > 255 ) {
			j = 255;
		}
		data1[i] = j;
	}

	if ( newMap ) {
		R_StaticFree( newMap );
	}
}


// we build a canonical token form of the image program here
static char parseBuffer[MAX_IMAGE_NAME];

/*
===================
AppendToken
===================
*/
static void AppendToken( idToken &token ) {
	// add a leading space if not at the beginning
	if ( parseBuffer[0] ) {
		idStr::Append( parseBuffer, MAX_IMAGE_NAME, " " );
	}
	idStr::Append( parseBuffer, MAX_IMAGE_NAME, token.c_str() );
}

/*
===================
MatchAndAppendToken
===================
*/
static void MatchAndAppendToken( idLexer &src, const char *match ) {
	if ( !src.ExpectTokenString( match ) ) {
		return;
	}
	// a matched token won't need a leading space
	idStr::Append( parseBuffer, MAX_IMAGE_NAME, match );
}

/*
===================
R_ParseImageProgram_r

If pic is NULL, the timestamps will be filled in, but no image will be generated
If both pic and timestamps are NULL, it will just advance past it, which can be
used to parse an image program from a text stream.
===================
*/
static bool R_ParseImageProgram_r( idLexer &src, byte **pic, int *width, int *height,
								  ID_TIME_T *timestamps, textureDepth_t *depth ) {
	idToken		token;
	float		scale;
	ID_TIME_T		timestamp;

	src.ReadToken( &token );
	AppendToken( token );

	if ( !token.Icmp( "heightmap" ) ) {
		MatchAndAppendToken( src, "(" );

		if ( !R_ParseImageProgram_r( src, pic, width, height, timestamps, depth ) ) {
			return false;
		}
		MatchAndAppendToken( src, "," );

		src.ReadToken( &token );
		AppendToken( token );
		scale = token.GetFloatValue();
		
		// process it
		if ( pic ) {
			R_HeightmapToNormalMap( *pic, *width, *height, scale );
			if ( depth ) {
				*depth = TD_BUMP;
			}
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "addnormals" ) ) {
		byte	*pic2;
		int		width2, height2;

		MatchAndAppendToken( src, "(" );

		if ( !R_ParseImageProgram_r( src, pic, width, height, timestamps, depth ) ) {
			return false;
		}
		MatchAndAppendToken( src, "," );

		if ( !R_ParseImageProgram_r( src, pic ? &pic2 : nullptr, &width2, &height2, timestamps, depth ) ) {
			if ( pic ) {
				R_StaticFree( *pic );
				*pic = NULL;
			}
			return false;
		}
		
		// process it
		if ( pic ) {
			R_AddNormalMaps( *pic, *width, *height, pic2, width2, height2 );
			R_StaticFree( pic2 );
			if ( depth ) {
				*depth = TD_BUMP;
			}
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "smoothnormals" ) ) {
		MatchAndAppendToken( src, "(" );

		if ( !R_ParseImageProgram_r( src, pic, width, height, timestamps, depth ) ) {
			return false;
		}

		if ( pic ) {
			R_SmoothNormalMap( *pic, *width, *height );
			if ( depth ) {
				*depth = TD_BUMP;
			}
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "add" ) ) {
		byte	*pic2;
		int		width2, height2;

		MatchAndAppendToken( src, "(" );

		if ( !R_ParseImageProgram_r( src, pic, width, height, timestamps, depth ) ) {
			return false;
		}
		MatchAndAppendToken( src, "," );

		if ( !R_ParseImageProgram_r( src, pic ? &pic2 : nullptr, &width2, &height2, timestamps, depth ) ) {
			if ( pic ) {
				R_StaticFree( *pic );
				*pic = NULL;
			}
			return false;
		}
		
		// process it
		if ( pic ) {
			R_ImageAdd( *pic, *width, *height, pic2, width2, height2 );
			R_StaticFree( pic2 );
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "scale" ) ) {
		float	scale[4];
		int		i;

		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, depth );

		for ( i = 0 ; i < 4 ; i++ ) {
			MatchAndAppendToken( src, "," );
			src.ReadToken( &token );
			AppendToken( token );
			scale[i] = token.GetFloatValue();
		}

		// process it
		if ( pic ) {
			R_ImageScale( *pic, *width, *height, scale );
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "invertAlpha" ) ) {
		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, depth );

		// process it
		if ( pic ) {
			R_InvertAlpha( *pic, *width, *height );
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "invertColor" ) ) {
		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, depth );

		// process it
		if ( pic ) {
			R_InvertColor( *pic, *width, *height );
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "makeIntensity" ) ) {
		int		i;

		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, depth );

		// copy red to green, blue, and alpha
		if ( pic ) {
			int		c;
			c = *width * *height * 4;
			for ( i = 0 ; i < c ; i+=4 ) {
				(*pic)[i+1] = 
				(*pic)[i+2] = 
				(*pic)[i+3] = (*pic)[i];
			}
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "makeAlpha" ) ) {
		int		i;

		MatchAndAppendToken( src, "(" );

		R_ParseImageProgram_r( src, pic, width, height, timestamps, depth );

		// average RGB into alpha, then set RGB to white
		if ( pic ) {
			int		c;
			c = *width * *height * 4;
			for ( i = 0; i < c; i += 4 ) {
				(*pic)[i + 3] = ((*pic)[i + 0] + (*pic)[i + 1] + (*pic)[i + 2]) / 3;
				(*pic)[i + 0] =
				(*pic)[i + 1] =
				(*pic)[i + 2] = 255;
			}
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	// if we are just parsing instead of loading or checking,
	// don't do the R_LoadImage
	if ( !timestamps && !pic ) {
		return true;
	}

	// try to load it as uncompressed image
	R_LoadImage( token.c_str(), pic, width, height, &timestamp );

	// try to load it as compressed image
	if ( timestamp == -1 ) {
		idStr filename = "dds/";
		filename += token;
		filename.SetFileExtension(".dds");
		imageCompressedData_t *compData = nullptr;
		R_LoadCompressedImage( filename.c_str(), ( pic ? &compData : nullptr ), &timestamp );
		if ( compData ) {
			assert( pic );
			if ( *pic = compData->ComputeUncompressedData() ) {
				if ( width )
					*width = compData->header.dwWidth;
				if ( height )
					*height = compData->header.dwHeight;
				R_StaticFree( compData );
			}
			else {
				R_StaticFree( compData );
				return false;
			}
		}
	}

	if ( timestamp == -1 ) {
		return false;
	}

	// add this to the timestamp
	if ( timestamps ) {
		if ( timestamp > *timestamps ) {
			*timestamps = timestamp;
		}
	}
	return true;
}


/*
===================
R_LoadImageProgram
===================
*/
void R_LoadImageProgram( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamps, textureDepth_t *depth ) {
	idLexer src;

    src.LoadMemory(name, static_cast<int>(strlen(name)), name);
	src.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );

	parseBuffer[0] = 0;
	if ( timestamps ) {
		*timestamps = 0;
	}
	R_ParseImageProgram_r( src, pic, width, height, timestamps, depth );

	src.FreeSource();
}

/*
===================
R_ParsePastImageProgram
===================
*/
const char *R_ParsePastImageProgram( idLexer &src ) {
	parseBuffer[0] = 0;
	R_ParseImageProgram_r( src, nullptr, nullptr, nullptr, nullptr, nullptr );
	return parseBuffer;
}

/*
===================================================================

CUBE MAPS

===================================================================
*/


static idMat3 cubeAxis[6];

void InitCubeAxis() {
	if ( cubeAxis[0][0][0] == 1 ) {
		return;
	}
	cubeAxis[0][0][0] = 1;
	cubeAxis[0][1][2] = -1;
	cubeAxis[0][2][1] = -1;

	cubeAxis[1][0][0] = -1;
	cubeAxis[1][1][2] = 1;
	cubeAxis[1][2][1] = -1;

	cubeAxis[2][0][1] = 1;
	cubeAxis[2][1][0] = 1;
	cubeAxis[2][2][2] = 1;

	cubeAxis[3][0][1] = -1;
	cubeAxis[3][1][0] = 1;
	cubeAxis[3][2][2] = -1;

	cubeAxis[4][0][2] = 1;
	cubeAxis[4][1][0] = 1;
	cubeAxis[4][2][1] = -1;

	cubeAxis[5][0][2] = -1;
	cubeAxis[5][1][0] = -1;
	cubeAxis[5][2][1] = -1;
}

/*
==================
R_SampleCubeMap
==================
*/
void R_SampleCubeMap( const idVec3 &dir, int size, byte *buffers[6], byte result[4] ) {
	idVec3 adir;
	adir[0] = idMath::Fabs( dir[0] );
	adir[1] = idMath::Fabs( dir[1] );
	adir[2] = idMath::Fabs( dir[2] );

	// select component with maximum absolute value
	float amax = adir.Max();
	int axis;
	if ( adir[0] == amax )
		axis = 0;
	else if ( adir[1] == amax )
		axis = 1;
	else
		axis = 2;
	// look if direction aong it is positive or negative
	float localZ = dir[axis];
	axis = axis * 2 + ( localZ < 0.0f );

	// compute texcoords on the chosen face
	float invZ = idMath::InvSqrt( idMath::Fabs( localZ ) );
	float fx = ( dir * cubeAxis[axis][1] ) * invZ;
	float fy = ( dir * cubeAxis[axis][2] ) * invZ;
	fx = 0.5f * ( fx + 1.0f );
	fy = 0.5f * ( fy + 1.0f );

	// convert to texels
	int x = int( size * fx );
	int y = int( size * fy );
	x = idMath::ClampInt( 0, size - 1, x );
	y = idMath::ClampInt( 0, size - 1, y );

	memcpy( result, &buffers[axis][( y * size + x ) * 4], 4 );
}

/*
=======================
R_MakeAmbientMap
=======================
*/
void R_MakeAmbientMap( const MakeAmbientMapParam &param ) {
	InitCubeAxis();

	TRACE_CPU_SCOPE( "R_MakeAmbientMap" )

	// For each axis direction (surface normal for diffuse, reflected direction for specular),
	// we compute 2D integral over spherical coordinates:
	//   alpha = angle( axis, sample )
	//   phi = rotation of sample around axis
	// 
	// The average incoming irradiance is:
	//   integral( color cos(alpha)^s dS ) / Q =
	//   integral( color cos(alpha)^s sin(alpha) dAlpha dPhi ) / Q
	// For alpha = [0..pi/2], phi = [0..2pi]
	// Where Q is surface of hemisphere:
	//   Q = integral( dS ) = integral( sin(alpha) dAlpha dPhi ) = 2 pi
	// 
	// Note that if color == 1, then the average = 1 / (s + 1)

	int samples = idMath::Imax( param.samples, 16 );
	int resAlp = int( idMath::Sqrt( samples ) * 0.5f );
	int resPhi = int( idMath::Sqrt( samples ) * 2.0f );

	// precompute sin/cos of angles for better performance
	idList<idVec2> alpCosSin, phiCosSin;
	alpCosSin.SetNum(resAlp);
	for ( int segAlp = 0; segAlp < resAlp; segAlp++ ) {
		float alp = ( segAlp + 0.5f ) * ( idMath::HALF_PI / resAlp );
		idMath::SinCos( alp, alpCosSin[segAlp].y, alpCosSin[segAlp].x );
	}
	phiCosSin.SetNum(resPhi);
	for ( int segPhi = 0; segPhi < resPhi; segPhi++ ) {
		float phi = ( segPhi + 0.5f ) * ( idMath::TWO_PI / resPhi );
		idMath::SinCos( phi, phiCosSin[segPhi].y, phiCosSin[segPhi].x );
	}
	// dAlpha dPhi / Q
	const float dAlpPhiQ = ( idMath::HALF_PI / resAlp / resPhi );

	for ( int y = 0; y < param.outSize; y++ ) {
		for ( int x = 0; x < param.outSize; x++ ) {
			float ratioX = ( x + 0.5f ) / param.outSize;
			float ratioY = ( y + 0.5f ) / param.outSize;
			// axis direction precomputed in this cubemap texel
			idVec3 axisZ = (
				cubeAxis[param.side][0] + 
				cubeAxis[param.side][1] * ( 2.0f * ratioX - 1.0f ) +
				cubeAxis[param.side][2] * ( 2.0f * ratioY - 1.0f )
			);
			axisZ.Normalize();

			// we need local coordinate system
			idVec3 axisX, axisY;
			axisZ.NormalVectors( axisX, axisY );

			idVec3 totalColor = idVec3( 0.0f );
			float totalCoeff = 0.0f;

			for ( int segAlp = 0; segAlp < resAlp; segAlp++ ) {
				for ( int segPhi = 0; segPhi < resPhi; segPhi++ ) {
					// convert to direction in world system
					float cosAlp = alpCosSin[segAlp].x;
					float sinAlp = alpCosSin[segAlp].y;
					float cosPhi = phiCosSin[segPhi].x;
					float sinPhi = phiCosSin[segPhi].y;
					idVec3 testDir = (
						axisZ * cosAlp + 
						axisX * (sinAlp * cosPhi) + 
						axisY * (sinAlp * sinPhi)
					);

					// fetch light at sample direction
					byte result[4];
					R_SampleCubeMap( testDir, param.size, param.buffers, result );

					// accumulate integral
					float pwr = cosAlp;
					for ( int t = 1; t < param.cosPower; t++ )
						pwr *= cosAlp;
					float coeff = pwr * sinAlp * dAlpPhiQ;
					totalColor[0] += coeff * result[0];
					totalColor[1] += coeff * result[1];
					totalColor[2] += coeff * result[2];
					totalCoeff += coeff;
				}
			}

			// note: totalCoeff is just for checking that math is correct
			// it's what we'll get for color == 1 constant environment

			idVec3 result( totalColor[0], totalColor[1], totalColor[2] );
			// now that we have average irradiance, we multiply it by:
			//   1. (s + 1) --- in order to normalize output to range [0..1]
			//   2. artist-controlled multiplier
			// ideally, one should remember about 2/5 normalization when consuming the texture
			result *= ( param.cosPower + 1.0f ) * param.multiplier;

			// store result in image
			byte *pixel = param.outBuffer + ( y * param.outSize + x ) * 4;
			pixel[0] = idMath::ClampInt( 0, 255, int( result[0] ) );
			pixel[1] = idMath::ClampInt( 0, 255, int( result[1] ) );
			pixel[2] = idMath::ClampInt( 0, 255, int( result[2] ) );
			pixel[3] = 255;
		}
	}
}

REGISTER_PARALLEL_JOB( R_MakeAmbientMap, "R_MakeAmbientMap_SingleFace" );

/*
=======================
R_MakeAmbientMaps
=======================
*/
void R_MakeAmbientMaps( byte *buffers[6], byte *outBuffers[6], int outSize, int samples, int size, float multiplier, int cosPower ) {
	TRACE_CPU_SCOPE_FORMAT( "R_MakeAmbientMaps", "size %d <- %d, smp %d, pwr %d", outSize, size, samples, cosPower );
	idParallelJobList *jobs = parallelJobManager->AllocJobList( JOBLIST_UTILITY, JOBLIST_PRIORITY_MEDIUM, 6, 0, nullptr );

	MakeAmbientMapParam params[6];
	for ( int i = 0; i < 6; i++ ) {
		MakeAmbientMapParam &p = params[i];
		p.buffers = buffers;
		p.outBuffer = outBuffers[i];
		p.outSize = outSize;
		p.samples = samples;
		p.size = size;
		p.multiplier = multiplier;
		p.cosPower = cosPower;
		p.side = i;
		jobs->AddJob( (jobRun_t)R_MakeAmbientMap, &p );
	}

	jobs->Submit( nullptr, JOBLIST_PARALLELISM_MAX_CORES );
	jobs->Wait();

	parallelJobManager->FreeJobList( jobs );
}

/*
=======================
R_BakeAmbientDiffuse
=======================
*/
void R_BakeAmbient( byte *pics[6], int *size, float multiplier, bool specular ) {
	if ( *size == 0 ) {
		return;
	}

	int outSize = specular ? 64 : 32;
	// note: should match specular power of NdotR in Phong shader
	int cosPower = ( specular ? 4 : 1 );

	byte *outPics[6] = { nullptr };
	// assume cubemaps are RGBA
	for ( int side = 0; side < 6; side++ ) {
		outPics[side] = ( byte * )R_StaticAlloc( 4 * outSize * outSize );
	}

	R_MakeAmbientMaps( pics, outPics, outSize, 256, *size, multiplier, cosPower );

	for ( int side = 0; side < 6; side++ ) {
		R_StaticFree( pics[side] );
		pics[side] = outPics[side];
	}
	*size = outSize;
}

/*
===================
R_ParseImageProgramCubeMap_r

If pic is NULL, the timestamps will be filled in, but no image will be generated
If both pic and timestamps are NULL, it will just advance past it, which can be
used to parse an image program from a text stream.
===================
*/
static bool R_ParseImageProgramCubeMap_r( idLexer &src, cubeFiles_t extensions, byte *pics[6], int *size, ID_TIME_T *timestamps ) {
	idToken		token;
	ID_TIME_T		timestamp;

	src.ReadToken( &token );
	AppendToken( token );

	if ( !token.Icmp( "bakeAmbientDiffuse" ) || !token.Icmp( "bakeAmbientSpecular" ) ) {
		bool specular = !token.Icmp( "bakeAmbientSpecular" );
		MatchAndAppendToken( src, "(" );

		if ( !R_ParseImageProgramCubeMap_r( src, extensions, pics, size, timestamps ) ) {
			return false;
		}

		float multiplier = 1.0f;
		if ( src.PeekTokenString( "," ) ) {
			MatchAndAppendToken( src, "," );

			src.ExpectTokenType( TT_NUMBER, 0, &token );
			AppendToken( token );
			multiplier = token.GetFloatValue();
		}

		// process it
		if ( pics ) {
			R_BakeAmbient( pics, size, multiplier, specular );
			/*// DEBUG OUTPUT..
			for ( int i = 0; i < 6; i++ )
				R_WriteTGA( va("ad%d.tga", i ), pics[i], *size, *size );*/
		}

		MatchAndAppendToken( src, ")" );
		return true;
	}

	if ( !token.Icmp( "nativeLayout" ) || !token.Icmp( "cameraLayout" ) ) {
		// override cubemap layout / orientation in subexpression inside
		cubeFiles_t layout = ( token.Icmp( "nativeLayout" ) == 0 ? CF_NATIVE : CF_CAMERA );
		MatchAndAppendToken( src, "(" );
		if ( !R_ParseImageProgramCubeMap_r( src, layout, pics, size, timestamps ) ) {
			return false;
		}
		MatchAndAppendToken( src, ")" );
		return true;
	}

	// if we are just parsing instead of loading or checking,
	// don't do the actual load
	if ( !timestamps && !pics ) {
		return true;
	}

	// FIXME: precompressed cube map files

	// try to load it as uncompressed image
	R_LoadCubeImages( token.c_str(), extensions, pics, size, &timestamp );

	if ( timestamp == -1 ) {
		return false;
	}

	// add this to the timestamp
	if ( timestamps ) {
		if ( timestamp > *timestamps ) {
			*timestamps = timestamp;
		}
	}
	return true;
}

/*
===================
R_LoadImageProgramCubeMap
===================
*/
void R_LoadImageProgramCubeMap( const char *cname, cubeFiles_t extensions, byte *pic[6], int *size, ID_TIME_T *timestamps ) {
	idLexer src;

	src.LoadMemory(cname, static_cast<int>(strlen(cname)), cname);
	src.SetFlags( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );

	parseBuffer[0] = 0;
	if ( timestamps ) {
		*timestamps = 0;
	}
	R_ParseImageProgramCubeMap_r( src, extensions, pic, size, timestamps );

	src.FreeSource();
}

/*
===================
R_ParsePastImageProgramCubeMap
===================
*/
const char *R_ParsePastImageProgramCubeMap( idLexer &src ) {
	parseBuffer[0] = 0;
	// note: faces layout should not matter
	R_ParseImageProgramCubeMap_r( src, CF_NATIVE, nullptr, nullptr, nullptr );
	return parseBuffer;
}
