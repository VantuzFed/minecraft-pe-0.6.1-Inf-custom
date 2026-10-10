#version 120
#include "/lib/settings.glsl"

varying vec4 v_color;
varying vec2 v_texCoord;
varying vec3 v_normal;

void main() {
    v_color = gl_Color;
    v_texCoord = gl_MultiTexCoord0.xy;
    v_normal = gl_NormalMatrix * gl_Normal;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
