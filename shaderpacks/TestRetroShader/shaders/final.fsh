#version 120
#include "/lib/settings.glsl"

uniform sampler2D colortex0;
varying vec2 v_texCoord;

void main() {
    vec4 col = texture2D(colortex0, v_texCoord);
    float luma = dot(col.rgb, vec3(0.299, 0.587, 0.114));
    col.rgb = mix(vec3(luma), col.rgb, SATURATION);
    gl_FragColor = col;
}
