#include "precompiled.h"
#include "Test.h"
#include "../renderer/Shader.h"

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
		"#pragma tdm_include \"tests/shared_common.glsl\"\n"
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
		"void main() {\n"
		"  float myVar = myFunc();\n"
		"}\n";

	const std::string EXPANDED_INCLUDE_SHADER =
		"#version 140\n"
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n"
		"\n"
		"void main() {}\n";
	const std::string EXPANDED_ADVANCED_INCLUDES =
		"#version 330\n"
		"\n"
		"uniform vec4 someParam;\n"
		"\n"
		"vec4 doSomething {\n"
		"  return someParam * 2;\n"
		"}\n"
		"\n"
		"float myFunc() {\n"
		"  return 0.3;\n"
		"}\n"
		"// already included tests/shared_common.glsl\n"
		"void main() {\n"
		"  float myVar = myFunc();\n"
		"}\n";

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

bool operator==(const originalLine_t &lhs, const originalLine_t &rhs) {
	return lhs.file == rhs.file && lhs.line == rhs.line;
}

std::ostream &operator<<(std::ostream &stream, const originalLine_t l) {
	return stream << l.file.c_str() << " (" << l.line << ")";
}

TEST_CASE("Shader source includes", "[shaders]") {
	PrepareTestShaders();

	SECTION( "Basic shader without includes remains unaltered" ) {
		ShaderSource shaderSource = ShaderSource( "tests/basic_shader.glsl" );
		REQUIRE( shaderSource.GetSource() == BASIC_SHADER );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 1 ) == originalLine_t{ "tests/basic_shader.glsl", 1 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 2 ) == originalLine_t{ "tests/basic_shader.glsl", 2 } );
		// next case is "beyond" the end of the file, but should still return something sensible
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 3 ) == originalLine_t{ "tests/basic_shader.glsl", 3 } );
	}

	SECTION( "Simple include works" ) {
		ShaderSource shaderSource = ShaderSource( "tests/include_shader.glsl" );
		REQUIRE( shaderSource.GetSource() == EXPANDED_INCLUDE_SHADER );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 1 ) == originalLine_t{ "tests/include_shader.glsl", 1 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 2 ) == originalLine_t{ "tests/shared_common.glsl", 1 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 3 ) == originalLine_t{ "tests/shared_common.glsl", 2 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 4 ) == originalLine_t{ "tests/shared_common.glsl", 3 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 5 ) == originalLine_t{ "tests/shared_common.glsl", 4 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 6 ) == originalLine_t{ "tests/shared_common.glsl", 5 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 7 ) == originalLine_t{ "tests/shared_common.glsl", 6 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 8 ) == originalLine_t{ "tests/include_shader.glsl", 3 } );
	}

	SECTION( "Multiple and nested includes" ) {
		ShaderSource shaderSource = ShaderSource( "tests/advanced_includes.glsl" );
		REQUIRE( shaderSource.GetSource() == EXPANDED_ADVANCED_INCLUDES );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 1 ) == originalLine_t{ "tests/advanced_includes.glsl", 1 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 2 ) == originalLine_t{ "tests/advanced_includes.glsl", 2 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 3 ) == originalLine_t{ "tests/shared_common.glsl", 1 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 4 ) == originalLine_t{ "tests/shared_common.glsl", 2 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 5 ) == originalLine_t{ "tests/shared_common.glsl", 3 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 6 ) == originalLine_t{ "tests/shared_common.glsl", 4 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 7 ) == originalLine_t{ "tests/shared_common.glsl", 5 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 8 ) == originalLine_t{ "tests/shared_common.glsl", 6 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 9 ) == originalLine_t{ "tests/nested_include.glsl", 2 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 10 ) == originalLine_t{ "tests/nested_include.glsl", 3 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 11 ) == originalLine_t{ "tests/nested_include.glsl", 4 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 12 ) == originalLine_t{ "tests/advanced_includes.glsl", 4 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 13 ) == originalLine_t{ "tests/advanced_includes.glsl", 5 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 14 ) == originalLine_t{ "tests/advanced_includes.glsl", 6 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 15 ) == originalLine_t{ "tests/advanced_includes.glsl", 7 } );
		REQUIRE( shaderSource.MapExpandedLineToOriginalSource( 16 ) == originalLine_t{ "tests/advanced_includes.glsl", 8 } );
	}

	CleanupTestShaders();
}
