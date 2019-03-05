#include "precompiled.h"
#include "Test.h"
#include "../renderer/GLSLProgram.h"

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

	void PrepareTestShaders() {
		INFO( "Preparing test shaders" );
		REQUIRE( fileSystem->WriteFile( "tests/basic_shader.glsl", BASIC_SHADER.c_str(), BASIC_SHADER.size(), "fs_savepath", "" ) >= 0 );
		REQUIRE( fileSystem->WriteFile( "tests/shared_common.glsl", SHARED_COMMON.c_str(), SHARED_COMMON.size(), "fs_savepath", "" ) >= 0 );
		REQUIRE( fileSystem->WriteFile( "tests/include_shader.glsl", INCLUDE_SHADER.c_str(), INCLUDE_SHADER.size(), "fs_savepath", "" ) >= 0 );
		REQUIRE( fileSystem->WriteFile( "tests/nested_include.glsl", NESTED_INCLUDE.c_str(), NESTED_INCLUDE.size(), "fs_savepath", "" ) >= 0 );
		REQUIRE( fileSystem->WriteFile( "tests/advanced_includes.glsl", ADVANCED_INCLUDES.c_str(), ADVANCED_INCLUDES.size(), "fs_savepath", "" ) >= 0 );
	}

	void CleanupTestShaders() {
		INFO( "Cleaning up" );
		fileSystem->RemoveFile( "tests/basic_shader.glsl", "" );
		fileSystem->RemoveFile( "tests/shared_common.glsl", "" );
		fileSystem->RemoveFile( "tests/include_shader.glsl", "" );
		fileSystem->RemoveFile( "tests/nested_include.glsl", "" );
		fileSystem->RemoveFile( "tests/advanced_includes.glsl", "" );
	}
}

std::string ReadFile( const char *sourceFile );
void ResolveIncludes( std::string &source, std::vector<std::string> &includedFiles );
void ResolveDefines( std::string &source, const idDict &defines );

TEST_CASE("Shader include handling", "[shaders]") {
	PrepareTestShaders();

	SECTION( "Basic shader without includes remains unaltered" ) {
		std::vector<std::string> includedFiles {"tests/basic_shader.glsl"};
		std::string source = ReadFile( "tests/basic_shader.glsl" );
		ResolveIncludes( source, includedFiles );
		REQUIRE( source == BASIC_SHADER );
	}

	SECTION( "Simple include works" ) {
		std::vector<std::string> includedFiles {"tests/include_shader.glsl"};
		std::string source = ReadFile( "tests/include_shader.glsl" );
		ResolveIncludes( source, includedFiles );
		REQUIRE( source == EXPANDED_INCLUDE_SHADER );
	}

	SECTION( "Multiple and nested includes" ) {
		std::vector<std::string> includedFiles {"tests/advanced_includes.glsl"};
		std::string source = ReadFile( "tests/advanced_includes.glsl" );
		ResolveIncludes( source, includedFiles );
		REQUIRE( source == EXPANDED_ADVANCED_INCLUDES );
	}

	CleanupTestShaders();
}

TEST_CASE("Shader defines handling", "[shaders]") {
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
