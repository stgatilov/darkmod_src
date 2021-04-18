#version 330

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

out float var_Intensity;
uniform float u_extrusion;

vec2 xyPos(int i) {
    return gl_in[i].gl_Position.xy / gl_in[i].gl_Position.w;
}

void emit(int i, vec2 xy, float intensity) {
    vec4 incoming = gl_in[i].gl_Position;
    gl_Position = vec4(xy * incoming.w, incoming.z, incoming.w);
    var_Intensity = intensity;
    EmitVertex();
}

void emitEdge(int i, int j, int k) {
    vec2 a = xyPos(i);
    vec2 b = xyPos(j);
    vec2 c = xyPos(k);

    vec2 ab = normalize(b - a);
    vec2 cb = normalize(b - c);
    vec2 ac = normalize(c - a);
    vec2 nab = vec2(-ab.y, ab.x);
    vec2 bout = normalize(ab + cb);
    vec2 aout = -normalize(ab + ac);
    
    float ext = u_extrusion * 0.001;
    emit(i, a + ext*aout, 0);
    emit(i, a + ext*nab, 0);
    emit(i, a, 1);
    emit(j, b + ext*nab, 0);
    emit(j, b, 1);
    emit(j, b + ext*bout, 0);
    EndPrimitive();
}

void main() {
    emitEdge(0, 1, 2);
    emitEdge(1, 2, 0);
    emitEdge(2, 0, 1);
}
