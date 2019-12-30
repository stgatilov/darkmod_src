#version 330

 #pragma tdm_include "tests/nested_include.glsl"
#pragma  tdm_include "tests/shared_common.glsl"  // ignore this comment
#pragma tdm_include "tests/advanced_includes.glsl"
void main() {
  float myVar = myFunc();
}
#pragma tdm_include "tests/basic_shader.glsl"
