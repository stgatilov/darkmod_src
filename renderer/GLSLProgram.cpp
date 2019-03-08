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
#include "GLSLProgram.h"
#include <memory>
#include <regex>

GLuint GLSLProgram::currentProgram = 0;

GLSLProgram * GLSLProgram::Load( const char *vertexSourceFile, const char *fragmentSourceFile, const char *geometrySourceFile ) {
	return Load( idDict(), vertexSourceFile, fragmentSourceFile, geometrySourceFile );	
}

GLSLProgram * GLSLProgram::Load( const idDict &defines, const char *vertexSourceFile, const char *fragmentSourceFile, const char *geometrySourceFile ) {
	if( !geometrySourceFile )
		geometrySourceFile = "";
	GLSLProgramLoader loader;
	loader.AddVertexShader( vertexSourceFile, defines );
	loader.AddFragmentShader( fragmentSourceFile, defines );
	if( geometrySourceFile[0] ) {
		loader.AddGeometryShader( geometrySourceFile, defines );
	}
	GLSLProgram *prog = loader.LinkProgram();
	prog->filenames[0] = vertexSourceFile;
	prog->filenames[1] = fragmentSourceFile;
	prog->filenames[2] = geometrySourceFile;
	prog->defines = defines;
	return prog;
}

GLSLProgram::GLSLProgram( GLuint program ) : program( program ) {}

GLSLProgram::~GLSLProgram() {
	qglDeleteProgram( program );
}

const char* GLSLProgram::GetFileName(GLint shaderType) const {
	if (shaderType == GL_VERTEX_SHADER)
		return filenames[0];
	if (shaderType == GL_FRAGMENT_SHADER)
		return filenames[1];
	if (shaderType == GL_GEOMETRY_SHADER)
		return filenames[2];
	assert(0);
	return nullptr;
}

void GLSLProgram::Activate() {
	//TODO: uncomment this thing when everything uses GLSLProgram
	//right now there are too many places where qglUseProgram is called
	//if( currentProgram != program ) {
		qglUseProgram( program );
		currentProgram = program;
	//}
}

void GLSLProgram::Deactivate() {
	qglUseProgram( 0 );
	currentProgram = 0;
}

void GLSLProgram::BindAttribLocation( GLuint index, const char *name ) {
	qglBindAttribLocation( program, index, name );
	for (int i = 0; i < boundAttributes.Num(); i++) {
		if (boundAttributes[i].name == name) {
			boundAttributes[i].index = i;
			return;
		}
	}
	bindAttribute_t add = { index, name };
	boundAttributes.Append(add);
}

void GLSLProgram::AddUniformAlias( int alias, const char *uniformName ) {
	int location = qglGetUniformLocation( program, uniformName );
	if( location == -1 ) {
		//note: avoiding such warnings is hardly compatible with reusing packs of uniforms
		//common->Warning( "Did not find active uniform: %s\n", uniformName );
	}
	aliasLocationMap.Append( aliasLocation_t { alias, location } );
	aliasNames.Append(uniformName);
}

void GLSLProgram::Uniform1fL( int location, GLfloat value ) {
	Activate();
	qglUniform1f( location, value );
}

void GLSLProgram::Uniform2fL( int location, GLfloat v1, GLfloat v2 ) {
	Activate();
	qglUniform2f( location, v1, v2 );
}

void GLSLProgram::Uniform3fL( int location, GLfloat v1, GLfloat v2, GLfloat v3 ) {
	Activate();
	qglUniform3f( location, v1, v2, v3 );
}

void GLSLProgram::Uniform4fL( int location, GLfloat v1, GLfloat v2, GLfloat v3, GLfloat v4 ) {
	Activate();
	qglUniform4f( location, v1, v2, v3, v4 );
}

void GLSLProgram::Uniform1iL( int location, GLint value ) {
	Activate();
	qglUniform1i( location, value );
}

void GLSLProgram::Uniform2iL( int location, GLint v1, GLint v2 ) {
	Activate();
	qglUniform2i( location, v1, v2 );
}

void GLSLProgram::Uniform3iL( int location, GLint v1, GLint v2, GLint v3 ) {
	Activate();
	qglUniform3i( location, v1, v2, v3 );
}

void GLSLProgram::Uniform4iL( int location, GLint v1, GLint v2, GLint v3, GLint v4 ) {
	Activate();
	qglUniform4i( location, v1, v2, v3, v4 );
}

void GLSLProgram::Uniform2fL( int location, const idVec2 &value ) {
	Activate();
	qglUniform2fv( location, 1, value.ToFloatPtr() );
}

void GLSLProgram::Uniform3fL( int location, const idVec3 &value ) {
	Activate();
	qglUniform3fv( location, 1, value.ToFloatPtr() );
}

void GLSLProgram::Uniform4fL( int location, const idVec4 &value ) {
	Activate();
	qglUniform4fv( location, 1, value.ToFloatPtr() );
}

void GLSLProgram::Uniform4fvL( int location, GLfloat *value ) {
	Activate();
	qglUniform4fv(location, 1, value);
}

void GLSLProgram::UniformMatrix4L( int location, const GLfloat *matrix ) {
	Activate();
	qglUniformMatrix4fv( location, 1, GL_FALSE, matrix );
}

GLSLProgramLoader::GLSLProgramLoader(): program(0) {
	program = qglCreateProgram();
}

GLSLProgramLoader::~GLSLProgramLoader() {
	if( program != 0) {
		qglDeleteProgram( program );
	}
}

void GLSLProgramLoader::AddVertexShader( const char *sourceFile, const idDict &defines ) {
	LoadAndAttachShader( GL_VERTEX_SHADER, sourceFile, defines );
}

void GLSLProgramLoader::AddFragmentShader( const char *sourceFile, const idDict &defines ) {
	LoadAndAttachShader( GL_FRAGMENT_SHADER, sourceFile, defines );
}

void GLSLProgramLoader::AddGeometryShader( const char *sourceFile, const idDict &defines ) {
	LoadAndAttachShader( GL_GEOMETRY_SHADER, sourceFile, defines );
}

GLSLProgram * GLSLProgramLoader::LinkProgram() {
	GLint result = GL_FALSE;

	qglLinkProgram( program );
	qglGetProgramiv( program, GL_LINK_STATUS, &result );
	if( result != GL_TRUE ) {
		// display program info log, which may contain clues to the linking error
		GLint length;
		qglGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
		auto log = std::make_unique<char[]>( length );
		qglGetProgramInfoLog( program, length, &result, log.get() );
		common->Warning( "Program linking failed:\n%s\n", log.get() );
		return nullptr;
	}

	qglValidateProgram( program );
	qglGetProgramiv( program, GL_VALIDATE_STATUS, &result );
	if( result != GL_TRUE ) {
		// display program info log, which may contain clues to the linking error
		GLint length;
		qglGetProgramiv( program, GL_INFO_LOG_LENGTH, &length );
		auto log = std::make_unique<char[]>( length );
		qglGetProgramInfoLog( program, length, &result, log.get() );
		common->Warning( "Program validation failed:\n%s\n", log.get() );
		return nullptr;
	}

	GLSLProgram *glslProgram = new GLSLProgram( program );
	program = 0;  // ownership passed to glslProgram
	return glslProgram;
}

void GLSLProgramLoader::LoadAndAttachShader( GLint shaderType, const char *sourceFile, const idDict &defines ) {
	if( program == 0 ) {
		common->Warning( "Tried to attach shader to an already linked program\n" );
		return;
	}

	GLuint shader = CompileShader( shaderType, sourceFile, defines );
	if( shader != 0) {
		qglAttachShader( program, shader );
		// won't actually be deleted until the program it's attached to is deleted
		qglDeleteShader( shader );
	}
}


namespace {

	std::string ReadFile( const char *sourceFile, bool silent = false ) {
		void *buf = nullptr;
		int len = fileSystem->ReadFile( idStr("glprogs/") + sourceFile, &buf );
		if( buf == nullptr ) {
			if ( !silent ) {
				common->Warning( "Could not open shader file %s", sourceFile );
			}
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
		static const std::regex includeRegex( R"regex(^[ \t]*#[ \t]*pragma[ \t]+tdm_include[ \t]+"(.*)"[ \t]*(?:\/\/.*)?\r?$)regex" );

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
				std::string includeBeginMarker = "#line 0 " + std::to_string( nextFileNo ) + '\n';
				std::string includeEndMarker = "\n#line " + std::to_string( currentLine ) + ' ' + std::to_string( currentFileNo );
				totalIncludedLines += std::count( includeContents.begin(), includeContents.end(), '\n' ) + 2;

				// replace include statement with content of included file
				std::string replacement = includeBeginMarker + includeContents + includeEndMarker;
				source.replace( match.position( 0 ), match.length( 0 ), replacement );
			} else {
				std::string replacement = "// already included " + fileToInclude;
				source.replace( match.position( 0 ), match.length( 0 ), replacement );
			}
		}
	}

	/**
	 * Resolves dynamic defines statements in GLSL source files.
	 * Note that the parsing is primitive and not context-sensitive. It will not respect multi-line comments
	 * or conditional preprocessor blocks!
	 * 
	 * Define directives should look like this:
	 * 
	 * #pragma tdm_define "DEF_NAME" // optional comment
	 * 
	 * If DEF_NAME is contained in defines, the line will be replaced by
	 * #define DEF_NAME <value>
	 * 
	 * Otherwise, it will be commented out.
	 */
	void ResolveDefines( std::string &source, const idDict &defines ) {
		static const std::regex defineRegex( R"regex(^[ \t]*#[ \t]*pragma[ \t]+tdm_define[ \t]+"(.*)"[ \t]*(?:\/\/.*)?\r?$)regex" );
		
		std::smatch match;
		while( std::regex_search( source, match, defineRegex ) ) {
			std::string define( match[ 1 ].first, match[ 1 ].second );
			auto defIt = defines.FindKey( define.c_str() );
			if( defIt != nullptr ) {
				std::string replacement = "#define " + define + " " + defIt->GetValue().c_str();
				source.replace( match.position( 0 ), match.length( 0 ), replacement );
			} else {
				std::string replacement = "// #undef " + define;
				source.replace( match.position( 0 ), match.length( 0 ), replacement );
			}
		}
	}

}

GLuint GLSLProgramLoader::CompileShader( GLint shaderType, const char *sourceFile, const idDict &defines ) {
	std::string source = ReadFile( sourceFile );
	if( source.empty() ) {
		return 0;
	}

	std::vector<std::string> sourceFiles { sourceFile };
	ResolveIncludes( source, sourceFiles );
	ResolveDefines( source, defines );

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
		for( size_t i = 0; i < sourceFiles.size(); ++i ) {
			ss << "  " << i << " - " << sourceFiles[i] << "\n";
		}
		common->Warning( ss.str().c_str() );

		qglDeleteShader( shader );
		return 0;
	}

	return shader;
}

GLSLProgram * GLSLProgram::Load( const char *programFileName, const idDict *defines ) {
	idDict empty;
	if (!defines) {
		defines = &empty;
	}

	idStr vsName = idStr(programFileName) + ".vs";
	idStr fsName = idStr(programFileName) + ".fs";
	idStr gsName = idStr(programFileName) + ".gs";
	if (ReadFile(programFileName, true).empty())
		gsName.Clear();

	return GLSLProgram::Load(*defines, vsName, fsName, gsName);
}

globalPrograms_t globalPrograms { nullptr };

namespace {
	void BindDefaultAttribLocations( GLSLProgram *program ) {
		program->BindAttribLocation( 0, "attr_Position" );
		program->BindAttribLocation( 2, "attr_Normal" );
		program->BindAttribLocation( 3, "attr_Color" );
		program->BindAttribLocation( 8, "attr_TexCoord" );
		program->BindAttribLocation( 9, "attr_Tangent" );
		program->BindAttribLocation( 10, "attr_Bitangent" );
	}

	void LoadInteractionShader() {
		// stub, change/expand as needed
		idDict interactionDefines;
		interactionDefines.Set( "SHADOW_TYPE", "1" );
		globalPrograms.interactionShader = GLSLProgram::Load( interactionDefines, "interaction.vs", "interaction.fs" );
		if( !globalPrograms.interactionShader ) {
			common->Error( "Failed to load interaction shader" );
		}
		BindDefaultAttribLocations( globalPrograms.interactionShader );
		globalPrograms.interactionShader->AddUniformAlias( MVP_MATRIX, "u_mvpMatrix" );
	}
}

void GLSL_InitPrograms() {
	LoadInteractionShader();
}

void GLSL_DestroyPrograms() {
	GLSLProgram::Deactivate();

	delete globalPrograms.interactionShader;
	globalPrograms.interactionShader = nullptr;
}


/// UNIT TESTS FOR SHADER INCLUDES AND DEFINES

#include "../tests/testing.h"

namespace {
	const std::string BASIC_SHADER =
		"#version 150\n"
		"void main() {}";
	const std::string SHARED_COMMON =
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n";
	const std::string INCLUDE_SHADER =
		"#version 140\n"
		"#pragma tdm_include \"tests/shared_common.glsl\"\r\n"
		"void main() {}\n";

	const std::string NESTED_INCLUDE =
		"#pragma tdm_include \"tests/shared_common.glsl\"\n"
		"float myFunc() {\n"
		"  return 0.3;\n"
		"}";

	const std::string ADVANCED_INCLUDES =
		"#version 330\n"
		"\n"
		" #  pragma tdm_include \"tests/nested_include.glsl\"\n"
		"#pragma  tdm_include \"tests/shared_common.glsl\"  // ignore this comment\n"
		"#pragma tdm_include \"tests/advanced_includes.glsl\"\n"
		"void main() {\n"
		"  float myVar = myFunc();\n"
		"}\n"
		"#pragma tdm_include \"tests/basic_shader.glsl\"\n";

	const std::string EXPANDED_INCLUDE_SHADER =
		"#version 140\n"
		"#line 0 1\n"
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n"
		"\n#line 2 0\n"
		"void main() {}\n";
	const std::string EXPANDED_ADVANCED_INCLUDES =
		"#version 330\n"
		"\n"
		"#line 0 1\n"
		"#line 0 2\n"
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n"
		"\n#line 1 1\n"
		"float myFunc() {\n"
		"  return 0.3;\n"
		"}"
		"\n#line 3 0\n"
		"// already included tests/shared_common.glsl\n"
		"// already included tests/advanced_includes.glsl\n"
		"void main() {\n"
		"  float myVar = myFunc();\n"
		"}\n"
		"#line 0 3\n"
		"#version 150\n"
		"void main() {}"
		"\n#line 9 0\n";

	std::string LoadSource( const std::string &sourceFile ) {
		std::vector<std::string> includedFiles { sourceFile };
		std::string source = ReadFile( sourceFile.c_str() );
		ResolveIncludes( source, includedFiles );
		return source;
	}

	TEST_CASE("Shader include handling") {
		INFO( "Preparing test shaders" );
		fileSystem->WriteFile( "glprogs/tests/basic_shader.glsl", BASIC_SHADER.c_str(), BASIC_SHADER.size(), "fs_savepath", "" );
		fileSystem->WriteFile( "glprogs/tests/shared_common.glsl", SHARED_COMMON.c_str(), SHARED_COMMON.size(), "fs_savepath", "" );
		fileSystem->WriteFile( "glprogs/tests/include_shader.glsl", INCLUDE_SHADER.c_str(), INCLUDE_SHADER.size(), "fs_savepath", "" );
		fileSystem->WriteFile( "glprogs/tests/nested_include.glsl", NESTED_INCLUDE.c_str(), NESTED_INCLUDE.size(), "fs_savepath", "" );
		fileSystem->WriteFile( "glprogs/tests/advanced_includes.glsl", ADVANCED_INCLUDES.c_str(), ADVANCED_INCLUDES.size(), "fs_savepath", "" );

		SUBCASE( "Basic shader without includes remains unaltered" ) {
			REQUIRE( LoadSource( "tests/basic_shader.glsl" ) == BASIC_SHADER );
		}

		SUBCASE( "Simple include works" ) {
			REQUIRE( LoadSource( "tests/include_shader.glsl" ) == EXPANDED_INCLUDE_SHADER );
		}

		SUBCASE( "Multiple and nested includes" ) {
			REQUIRE( LoadSource( "tests/advanced_includes.glsl" ) == EXPANDED_ADVANCED_INCLUDES );
		}

		INFO( "Cleaning up" );
		fileSystem->RemoveFile( "tests/basic_shader.glsl", "" );
		fileSystem->RemoveFile( "tests/shared_common.glsl", "" );
		fileSystem->RemoveFile( "tests/include_shader.glsl", "" );
		fileSystem->RemoveFile( "tests/nested_include.glsl", "" );
		fileSystem->RemoveFile( "tests/advanced_includes.glsl", "" );
	}

	TEST_CASE("Shader defines handling") {
		const std::string shaderWithDynamicDefines =
			"#version 140\n"
			"#pragma tdm_define \"FIRST_DEFINE\"\n"
			"\n"
			"  # pragma   tdm_define   \"SECOND_DEFINE\"\n"
			"void main() {\n"
			"#ifdef FIRST_DEFINE\n"
			"  return;\n"
			"#endif\n"
			"}\n" ;

		const std::string expectedResult =
			"#version 140\n"
			"#define FIRST_DEFINE 1\n"
			"\n"
			"// #undef SECOND_DEFINE\n"
			"void main() {\n"
			"#ifdef FIRST_DEFINE\n"
			"  return;\n"
			"#endif\n"
			"}\n" ;

		std::string source = shaderWithDynamicDefines;
		idDict defines;
		defines.Set( "FIRST_DEFINE", "1" );
		ResolveDefines( source, defines );
		REQUIRE( source == expectedResult );
	}
}
