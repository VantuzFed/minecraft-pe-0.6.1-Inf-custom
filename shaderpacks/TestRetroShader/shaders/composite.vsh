#version 120
varying vec2 v_texCoord;

void main() {
    v_texCoord = gl_MultiTexCoord0.xy;
    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);
}
