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
#include <unordered_set>
#include <iostream>

ShaderSource::ShaderSource( const idStr &sourceFile ) : sourceFile( sourceFile ) { }

void ShaderSource::EnableFeature( const std::string &feature ) {
	features[ feature ] = true;
}

void ShaderSource::DisableFeature( const std::string &feature ) {
	features[ feature ] = false;
}

std::string ShaderSource::GetSource() {
	std::string contents = ResolveIncludes( ReadFile( sourceFile ) );

	return contents;
}

originalLine_t ShaderSource::MapExpandedLineToOriginalSource( unsigned expandedLineNo ) const {
	assert( expandedLineNo >= 1 );
	
	unsigned int accumulatedLines = 0;
	auto it = sourceBlocks.cbegin();
	while( it < sourceBlocks.cend() - 1 && accumulatedLines + it->lineCount < expandedLineNo ) {
		accumulatedLines += it->lineCount;
		++it;
	}

	return originalLine_t{ it->sourceFile, expandedLineNo - accumulatedLines - 1 + it->startingLine };
}

std::string ShaderSource::ReadFile( const idStr &file ) {
	void *buf = nullptr;
	int len = fileSystem->ReadFile( file, &buf );
	if( buf == nullptr ) {
		common->Warning( "Could not open shader file %s", file.c_str() );
		return "";
	}
	std::string contents( static_cast< char* >( buf ), len );
	Mem_Free( buf );
	return contents;
}

std::string ShaderSource::ResolveIncludes( std::string source ) {
	// valid include directive looks something like this:
	// #pragma tdm_include "somefile.glsl" // optional comment
	static const std::regex includeRegex( R"regex(^[ \t]*#[ \t]*pragma[ \t]*tdm_include[ \t]"(.*)"[ \t]*(?:\/\/.*)?$)regex" );

	sourceBlocks.clear();
	unsigned int totalSourceLines = std::count( source.begin(), source.end(), '\n' ) + 1;
	sourceBlocks.push_back( sourceBlock_t{ sourceFile, 1, totalSourceLines } );
	std::unordered_set<std::string> alreadyIncludedFiles;

	std::smatch match;
	while( std::regex_search( source, match, includeRegex ) ) {
		std::string fileToInclude( match[ 1 ].first, match[ 1 ].second );
		if( alreadyIncludedFiles.find( fileToInclude ) == alreadyIncludedFiles.end() ) {
			std::string includeContents = ReadFile( fileToInclude.c_str() );
			// update source blocks so that we can map line nos of the expanded source back to the original source files
			unsigned int currentLine = std::count( source.cbegin(), match[ 0 ].first, '\n' ) + 1;
			unsigned int includeLines = std::count( includeContents.begin(), includeContents.end(), '\n' ) + 1;
			auto curBlock = sourceBlocks.begin();
			unsigned int accumulatedLines = 0;
			while( accumulatedLines + curBlock->lineCount < currentLine ) {
				accumulatedLines += curBlock->lineCount;
				++curBlock;
			}
			auto continuationBlock = sourceBlock_t{ curBlock->sourceFile, currentLine - accumulatedLines + 1, curBlock->lineCount - ( currentLine - accumulatedLines ) };
			curBlock->lineCount -= continuationBlock.lineCount + 1;
			auto includedBlock = sourceBlock_t{ fileToInclude.c_str(), 1, includeLines };
			auto includedIt = sourceBlocks.insert( curBlock + 1, includedBlock );
			sourceBlocks.insert( includedIt + 1, continuationBlock );

			// replace include statement with content of included file
			source.replace( match[ 0 ].first, match[ 0 ].second, includeContents );
			alreadyIncludedFiles.insert( fileToInclude );
		} else {
			source.replace( match[ 0 ].first, match[ 0 ].second, "// already included " + fileToInclude );
		}
	}
	return source;
}
