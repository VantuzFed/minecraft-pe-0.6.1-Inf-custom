#version 120
#include "/lib/settings.glsl"

uniform sampler2D colortex0;
varying vec2 v_texCoord;

void main() {
    vec4 col = texture2D(colortex0, v_texCoord);
    col.rgb *= CONTRAST;
    gl_FragData[0] = col;
}
