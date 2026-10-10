#version 120
#include "/lib/settings.glsl"

uniform sampler2D texture;
uniform sampler2D lightmap;
varying vec4 v_color;
varying vec2 v_texCoord;
varying vec3 v_normal;

void main() {
    vec4 texColor = texture2D(texture, v_texCoord);
    vec4 color = texColor * v_color;
    if (color.a < 0.1) discard;

    gl_FragData[0] = color;
    gl_FragData[1] = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);
    gl_FragData[2] = vec4(1.0);
}
