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

#include "precompiled.h"
#pragma hdrstop

#include "renderer/resources/Image.h"
#include "tests/testing.h"


bool IsImageFormatCompressed( int internalFormat ) {
	if (
		internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
		internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
		internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
		internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT ||
		internalFormat == GL_COMPRESSED_RG_RGTC2
	) {
		return true;
	}
	return false;
}

static int SizeOfCompressedImage( int w, int h, int internalFormat ) {
	assert( IsImageFormatCompressed( internalFormat ) );
	int numBlocks = ( (w + 3) / 4 ) * ( (h + 3) / 4 );
	int bytesPerBlock = 16;
	if (internalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT || internalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
		bytesPerBlock = 8;
	return numBlocks * bytesPerBlock;
}

static void TestDecompressOnImage(int W, int H, const idList<byte>& inputUnc, GLenum compressedFormat, bool checkPerfo = false) {
	qglGetError();

	idList<byte> openglUnc;
	openglUnc.SetNum(W * H * 4);
	idList<byte> openglComp;
	openglComp.SetNum(SizeOfCompressedImage(W, H, compressedFormat));

	qglBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	qglBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	CHECK(qglGetError() == 0);

	GLuint tex;
	qglGenTextures(1, &tex);
	CHECK(qglGetError() == 0);
	qglBindTexture(GL_TEXTURE_2D, tex);
	qglTexImage2D(GL_TEXTURE_2D, 0, compressedFormat, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, inputUnc.Ptr());
	CHECK(qglGetError() == 0);
	qglGetCompressedTexImage(GL_TEXTURE_2D, 0, openglComp.Ptr());
	CHECK(qglGetError() == 0);
	qglGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, openglUnc.Ptr());
	CHECK(qglGetError() == 0);
	qglBindTexture(GL_TEXTURE_2D, 0);
	qglDeleteTextures(1, &tex);
	CHECK(qglGetError() == 0);

	idList<byte> softUnc;
	softUnc.SetNum(W * H * 4);
	const int TRIES = checkPerfo ? 5 : 1;
	for (int ntry = 0; ntry < TRIES; ntry++) {
		double startClock = Sys_GetClockTicks();
		if (compressedFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
			SIMDProcessor->DecompressRGBA8FromDXT1(openglComp.Ptr(), W, H, softUnc.Ptr(), 4 * W, false);
		else if (compressedFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
			SIMDProcessor->DecompressRGBA8FromDXT1(openglComp.Ptr(), W, H, softUnc.Ptr(), 4 * W, true);
		else if (compressedFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
			SIMDProcessor->DecompressRGBA8FromDXT3(openglComp.Ptr(), W, H, softUnc.Ptr(), 4 * W);
		else if (compressedFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
			SIMDProcessor->DecompressRGBA8FromDXT5(openglComp.Ptr(), W, H, softUnc.Ptr(), 4 * W);
		else if (compressedFormat == GL_COMPRESSED_RG_RGTC2)
			SIMDProcessor->DecompressRGBA8FromRGTC(openglComp.Ptr(), W, H, softUnc.Ptr(), 4 * W);
		else
			CHECK(false);
		double endClock = Sys_GetClockTicks();
		if (checkPerfo && ntry == TRIES-1) {
			MESSAGE(va("Decompressed (%d x %d) image from format %x in %0.3lf ms",
				W, H, compressedFormat, 1e+3 * (endClock - startClock) / Sys_ClockTicksPerSecond()
			));
		}
	}

	idList<int> wrongBytes;
	for (int i = 0; i < W * H * 4; i++) {
		int delta = idMath::Abs(0 + openglUnc[i] - softUnc[i]);
		//note: it is very hard to make image perfectly match due to various quantization/rounding errors
		//I'm not even sure OpenGL defines DXT -> RGBA8 conversion exactly
		if (delta > 1)
			wrongBytes.AddGrow(i);
	}

#ifdef _DEBUG
#define IMSAVE(varname)	idImageWriter().Source(varname.Ptr(), W, H).Dest(fileSystem->OpenFileWrite("temp_" #varname ".tga")).WriteTGA();
	if (wrongBytes.Num()) {
		IMSAVE(inputUnc);
		IMSAVE(openglUnc);
		IMSAVE(softUnc);
	}
#endif

	CHECK(wrongBytes.Num() == 0);
}

static idList<byte> GenImageConstant(int W, int H, int R, int G, int B, int A) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	for (int i = 0; i < W*H; i++) {
		res[4 * i + 0] = R;
		res[4 * i + 1] = G;
		res[4 * i + 2] = B;
		res[4 * i + 3] = A;
	}
	return res;
}

static idList<byte> GenImageGradient(int W, int H) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	int pos = 0;
	for (int i = 0; i < H; i++)
		for (int j = 0; j < W; j++) {
			res[pos++] = i * 255 / (H - 1);
			res[pos++] = (W - 1 - j) * 255 / (W - 1);
			res[pos++] = (i + j) * 255 / (H + W - 2);
			res[pos++] = (H - 1 - i + j) * 255 / (H + W - 2);
		}
	return res;
}

static idList<byte> GenImageRandom(int W, int H, idRandom &rnd) {
	idList<byte> res;
	res.SetNum(W * H * 4);
	for (int i = 0; i < res.Num(); i++)
		res[i] = rnd.RandomInt(256);
	return res;
}

static void TestDecompressDxt(bool checkPerfo) {
	idRandom rnd;
	static const GLenum FORMATS[] = {
		GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
		GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
		GL_COMPRESSED_RG_RGTC2,
		0
	};
	for (int f = 0; FORMATS[f]; f++) {
		GLenum format = FORMATS[f];
		if (checkPerfo) {
			//check performance
			TestDecompressOnImage(1024, 1024, GenImageRandom(1024, 1024, rnd), format, true);
		}
		else {
			//check correctness
			TestDecompressOnImage(16, 16, GenImageConstant(16, 16, 255, 255, 255, 255), format);
			TestDecompressOnImage(16, 16, GenImageConstant(16, 16, 100, 128, 127, 197), format);
			TestDecompressOnImage(16, 16, GenImageConstant(16, 16, 100, 128, 127, 197), format);
			TestDecompressOnImage(16, 16, GenImageGradient(16, 16), format);
			TestDecompressOnImage(512, 512, GenImageGradient(512, 512), format);
			TestDecompressOnImage(23, 17, GenImageGradient(23, 17), format);
			TestDecompressOnImage(21, 18, GenImageGradient(21, 18), format);
			TestDecompressOnImage(22, 18, GenImageGradient(22, 18), format);
			TestDecompressOnImage(16, 16, GenImageRandom(16, 16, rnd), format);
			TestDecompressOnImage(231, 177, GenImageRandom(231, 177, rnd), format);
		}
	}
}

TEST_CASE("DecompressDxt:Correctness") {
	TestDecompressDxt(false);
}

TEST_CASE("DecompressDxt:Performance"
	* doctest::skip()
) {
	TestDecompressDxt(true);
}
