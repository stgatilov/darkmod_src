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
#include "Shader.h"
#include <regex>

GLSLProgramLoader::GLSLProgramLoader(): program(0) {
	program = qglCreateProgram();
}

void GLSLProgramLoader::AddVertexShader( const char *sourceFile, const ShaderDefines &defines ) {
	LoadAndAttachShader( GL_VERTEX_SHADER, sourceFile, defines );
}

void GLSLProgramLoader::AddFragmentShader( const char *sourceFile, const ShaderDefines &defines ) {
	LoadAndAttachShader( GL_FRAGMENT_SHADER, sourceFile, defines );
}

void GLSLProgramLoader::AddGeometryShader( const char *sourceFile, const ShaderDefines &defines ) {
	LoadAndAttachShader( GL_GEOMETRY_SHADER, sourceFile, defines );
}

void GLSLProgramLoader::LoadAndAttachShader( GLint shaderType, const char *sourceFile, const ShaderDefines &defines ) {
	GLuint shader = CompileShader( shaderType, sourceFile, defines );
	if( shader != 0) {
		qglAttachShader( program, shader );
		// won't actually be deleted until the program it's attached to is deleted
		qglDeleteShader( shader );
	}
}

std::string ReadFile( const char *sourceFile ) {
	void *buf = nullptr;
	int len = fileSystem->ReadFile( sourceFile, &buf );
	if( buf == nullptr ) {
		common->Warning( "Could not open shader file %s", sourceFile );
		return "";
	}
	std::string contents( static_cast< char* >( buf ), len );
	fileSystem->FreeFile( buf );

	return contents;
}

/**
 * Resolves include statements in GLSL source files.
 * Note that the parsing is primitive and not context-sensitive. It will not respect multi-line comments
 * or conditional preprocessor blocks, so keep includes simple in the source files!
 * 
 * Include directives should look like this:
 * 
 * #pragma tdm_include "somefile.glsl" // optional comment
 */
void ResolveIncludes( std::string &source, std::vector<std::string> &includedFiles ) {
	static const std::regex includeRegex( R"regex(^[ \t]*#[ \t]*pragma[ \t]+tdm_include[ \t]+"(.*)"[ \t]*(?:\/\/.*)?$)regex" );

	unsigned int currentFileNo = includedFiles.size() - 1;
	unsigned int totalIncludedLines = 0;

	std::smatch match;
	while( std::regex_search( source, match, includeRegex ) ) {
		std::string fileToInclude( match[ 1 ].first, match[ 1 ].second );
		if( std::find( includedFiles.begin(), includedFiles.end(), fileToInclude ) == includedFiles.end() ) {
			int nextFileNo = includedFiles.size();
			std::string includeContents = ReadFile( fileToInclude.c_str() );
			includedFiles.push_back( fileToInclude );
			ResolveIncludes( includeContents, includedFiles );

			// also add a #line instruction at beginning and end of include so that
			// compile errors are mapped to the correct file and line
			// unfortunately, #line does not take an actual filename, but only an integral reference to a file :(
			unsigned int currentLine = std::count( source.cbegin(), match[ 0 ].first, '\n' ) + 1 - totalIncludedLines;
			std::string includeBeginMarker = "#line 1 " + std::to_string( nextFileNo ) + '\n';
			std::string includeEndMarker = "\n#line " + std::to_string( currentLine + 1 ) + ' ' + std::to_string( currentFileNo );
			totalIncludedLines += std::count( includeContents.begin(), includeContents.end(), '\n' ) + 2;

			// replace include statement with content of included file
			source.replace( match[ 0 ].first, match[ 0 ].second, includeBeginMarker + includeContents + includeEndMarker );
		} else {
			source.replace( match[ 0 ].first, match[ 0 ].second, "// already included " + fileToInclude );
		}
	}
}

GLuint GLSLProgramLoader::CompileShader( GLint shaderType, const char *sourceFile, const ShaderDefines &defines ) {
	std::string source = ReadFile( sourceFile );
	if( source.empty() ) {
		return 0;
	}

	std::vector<std::string> sourceFiles { sourceFile };
	ResolveIncludes(source, sourceFiles);

	GLuint shader = qglCreateShader( shaderType );
	GLint length = source.size();
	const char *sourcePtr = source.c_str();
	qglShaderSource( shader, 1, &sourcePtr, &length );
	qglCompileShader( shader );

	// check if compilation was successful
	GLint result;
	qglGetShaderiv( shader, GL_COMPILE_STATUS, &result );
	if( result == GL_FALSE ) {
		// display the shader info log, which contains compile errors
		int length;
		qglGetShaderiv( shader, GL_INFO_LOG_LENGTH, &length );
		auto log = std::make_unique<char[]>( length );
		qglGetShaderInfoLog( shader, length, &result, log.get() );
		std::stringstream ss;
		ss << "Compiling shader file " << sourceFile << " failed:\n" << log.get() << "\n\n";
		// unfortunately, GLSL compilers don't reference any actual source files in their errors, but only
		// file index numbers. So we'll display a short legend which index corresponds to which file.
		ss << "File indexes:\n";
		for( int i = 0; i < sourceFiles.size(); ++i ) {
			ss << "  " << i << " - " << sourceFiles[i] << "\n";
		}
		common->Warning( ss.str().c_str() );

		qglDeleteShader( shader );
		return 0;
	}

	return shader;
}
