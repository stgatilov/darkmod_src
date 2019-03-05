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

#ifndef __SHADER_H__
#define __SHADER_H__
#include <unordered_map>

class Shader {
public:
	~Shader();

private:
	GLuint program;
	idHashTable<int> activeUniformLocations;
	idHashIndex virtualLocationMap;
};

struct sourceBlock_t {
	idStr sourceFile;
	unsigned int startingLine;
	unsigned int lineCount;
};

struct originalLine_t {
	idStr file;
	unsigned int line;
};

class ShaderSource {
public:
	ShaderSource( const idStr &sourceFile );
	void EnableFeature( const std::string &feature );
	void DisableFeature( const std::string &feature );
	std::string GetSource();
	originalLine_t MapExpandedLineToOriginalSource( unsigned int expandedLineNo ) const;

private:
	idStr sourceFile;
	std::unordered_map<std::string, bool> features;
	std::vector<sourceBlock_t> sourceBlocks;

	std::string ReadFile( const idStr &file );
	std::string ResolveIncludes( std::string source );
};
#endif
