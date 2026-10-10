#include "ShaderUniforms.h"
#include "../gles.h"

ShaderUniformValues::ShaderUniformValues()
	: frameTimeCounter(0.0f),
	  frameCounter(0),
	  viewWidth(854.0f),
	  viewHeight(480.0f),
	  aspectRatio(854.0f / 480.0f),
	  nearPlane(0.05f),
	  farPlane(256.0f),
	  rainStrength(0.0f),
	  worldTime(6000),
	  isEyeInWater(0),
	  timeOfDay(0.0f),
	  currentLayer(0)
{
	cameraPosition[0] = cameraPosition[1] = cameraPosition[2] = 0.0f;
	previousCameraPosition[0] = previousCameraPosition[1] = previousCameraPosition[2] = 0.0f;
	sunPosition[0] = 0.0f; sunPosition[1] = 100.0f; sunPosition[2] = 0.0f;
	moonPosition[0] = 0.0f; moonPosition[1] = -100.0f; moonPosition[2] = 0.0f;
	upPosition[0] = 0.0f; upPosition[1] = 1.0f; upPosition[2] = 0.0f;
}

ShaderUniformLocations::ShaderUniformLocations()
	: u_cameraPosition(-1),
	  u_previousCameraPosition(-1),
	  u_gbufferModelView(-1),
	  u_gbufferModelViewInverse(-1),
	  u_gbufferProjection(-1),
	  u_gbufferProjectionInverse(-1),
	  u_sunPosition(-1),
	  u_moonPosition(-1),
	  u_upPosition(-1),
	  u_frameTimeCounter(-1),
	  u_frameCounter(-1),
	  u_viewWidth(-1),
	  u_viewHeight(-1),
	  u_aspectRatio(-1),
	  u_near(-1),
	  u_far(-1),
	  u_rainStrength(-1),
	  u_wetness(-1),
	  u_worldTime(-1),
	  u_isEyeInWater(-1),
	  u_gtexture(-1),
	  u_texture(-1),
	  u_lightmap(-1),
	  u_colortex0(-1),
	  u_colortex1(-1),
	  u_colortex2(-1),
	  u_depthtex0(-1),
	  u_layer(-1),
	  u_time(-1),
	  u_camPos(-1),
	  u_timeOfDay(-1)
{}

void ShaderUniformLocations::findLocations(unsigned int program) {
	if (!glGetUniformLocation || program == 0) return;

	u_cameraPosition = glGetUniformLocation(program, "cameraPosition");
	u_previousCameraPosition = glGetUniformLocation(program, "previousCameraPosition");
	u_gbufferModelView = glGetUniformLocation(program, "gbufferModelView");
	u_gbufferModelViewInverse = glGetUniformLocation(program, "gbufferModelViewInverse");
	u_gbufferProjection = glGetUniformLocation(program, "gbufferProjection");
	u_gbufferProjectionInverse = glGetUniformLocation(program, "gbufferProjectionInverse");
	u_sunPosition = glGetUniformLocation(program, "sunPosition");
	u_moonPosition = glGetUniformLocation(program, "moonPosition");
	u_upPosition = glGetUniformLocation(program, "upPosition");
	u_frameTimeCounter = glGetUniformLocation(program, "frameTimeCounter");
	u_frameCounter = glGetUniformLocation(program, "frameCounter");
	u_viewWidth = glGetUniformLocation(program, "viewWidth");
	u_viewHeight = glGetUniformLocation(program, "viewHeight");
	u_aspectRatio = glGetUniformLocation(program, "aspectRatio");
	u_near = glGetUniformLocation(program, "near");
	u_far = glGetUniformLocation(program, "far");
	u_rainStrength = glGetUniformLocation(program, "rainStrength");
	u_wetness = glGetUniformLocation(program, "wetness");
	u_worldTime = glGetUniformLocation(program, "worldTime");
	u_isEyeInWater = glGetUniformLocation(program, "isEyeInWater");

	u_gtexture = glGetUniformLocation(program, "gtexture");
	u_texture = glGetUniformLocation(program, "texture");
	u_lightmap = glGetUniformLocation(program, "lightmap");
	u_colortex0 = glGetUniformLocation(program, "colortex0");
	u_colortex1 = glGetUniformLocation(program, "colortex1");
	u_colortex2 = glGetUniformLocation(program, "colortex2");
	u_depthtex0 = glGetUniformLocation(program, "depthtex0");

	u_layer = glGetUniformLocation(program, "u_layer");
	u_time = glGetUniformLocation(program, "u_time");
	u_camPos = glGetUniformLocation(program, "u_camPos");
	u_timeOfDay = glGetUniformLocation(program, "u_timeOfDay");
}

void ShaderUniformLocations::apply(unsigned int program, const ShaderUniformValues& vals, bool isCompositeOrFinal) const {
	if (!glUniform1f || program == 0) return;

	if (u_cameraPosition >= 0) glUniform3fv(u_cameraPosition, 1, vals.cameraPosition);
	if (u_previousCameraPosition >= 0) glUniform3fv(u_previousCameraPosition, 1, vals.previousCameraPosition);
	if (u_sunPosition >= 0) glUniform3fv(u_sunPosition, 1, vals.sunPosition);
	if (u_moonPosition >= 0) glUniform3fv(u_moonPosition, 1, vals.moonPosition);
	if (u_upPosition >= 0) glUniform3fv(u_upPosition, 1, vals.upPosition);
	if (u_frameTimeCounter >= 0) glUniform1f(u_frameTimeCounter, vals.frameTimeCounter);
	if (u_frameCounter >= 0) glUniform1i(u_frameCounter, vals.frameCounter);
	if (u_viewWidth >= 0) glUniform1f(u_viewWidth, vals.viewWidth);
	if (u_viewHeight >= 0) glUniform1f(u_viewHeight, vals.viewHeight);
	if (u_aspectRatio >= 0) glUniform1f(u_aspectRatio, vals.aspectRatio);
	if (u_near >= 0) glUniform1f(u_near, vals.nearPlane);
	if (u_far >= 0) glUniform1f(u_far, vals.farPlane);
	if (u_rainStrength >= 0) glUniform1f(u_rainStrength, vals.rainStrength);
	if (u_wetness >= 0) glUniform1f(u_wetness, vals.rainStrength);
	if (u_worldTime >= 0) glUniform1i(u_worldTime, vals.worldTime);
	if (u_isEyeInWater >= 0) glUniform1i(u_isEyeInWater, vals.isEyeInWater);

	if (u_gbufferModelView >= 0) glUniformMatrix4fv(u_gbufferModelView, 1, GL_FALSE, vals.gbufferModelView.m);
	if (u_gbufferModelViewInverse >= 0) glUniformMatrix4fv(u_gbufferModelViewInverse, 1, GL_FALSE, vals.gbufferModelViewInverse.m);
	if (u_gbufferProjection >= 0) glUniformMatrix4fv(u_gbufferProjection, 1, GL_FALSE, vals.gbufferProjection.m);
	if (u_gbufferProjectionInverse >= 0) glUniformMatrix4fv(u_gbufferProjectionInverse, 1, GL_FALSE, vals.gbufferProjectionInverse.m);

	// Legacy & compatibility uniforms
	if (u_layer >= 0) glUniform1i(u_layer, vals.currentLayer);
	if (u_time >= 0) glUniform1f(u_time, vals.frameTimeCounter);
	if (u_camPos >= 0) glUniform3fv(u_camPos, 1, vals.cameraPosition);
	if (u_timeOfDay >= 0) glUniform1f(u_timeOfDay, vals.timeOfDay);

	// Texture samplers
	if (!isCompositeOrFinal) {
		if (u_gtexture >= 0) glUniform1i(u_gtexture, 0);
		if (u_texture >= 0) glUniform1i(u_texture, 0);
		if (u_lightmap >= 0) glUniform1i(u_lightmap, 1);
	} else {
		if (u_colortex0 >= 0) glUniform1i(u_colortex0, 0);
		if (u_colortex1 >= 0) glUniform1i(u_colortex1, 1);
		if (u_colortex2 >= 0) glUniform1i(u_colortex2, 2);
		if (u_depthtex0 >= 0) glUniform1i(u_depthtex0, 3);
	}
}
