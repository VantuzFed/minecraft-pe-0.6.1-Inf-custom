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
	sunDir[0] = 0.0f; sunDir[1] = 1.0f; sunDir[2] = 0.0f;
	moonDir[0] = 0.0f; moonDir[1] = -1.0f; moonDir[2] = 0.0f;
	lightDir[0] = 0.0f; lightDir[1] = 1.0f; lightDir[2] = 0.0f;
	upDir[0] = 0.0f; upDir[1] = 1.0f; upDir[2] = 0.0f;
	sunDirView[0] = 0.0f; sunDirView[1] = 1.0f; sunDirView[2] = 0.0f;
	moonDirView[0] = 0.0f; moonDirView[1] = -1.0f; moonDirView[2] = 0.0f;
	upDirView[0] = 0.0f; upDirView[1] = 1.0f; upDirView[2] = 0.0f;
	daytime[0] = 0.0f; daytime[1] = 1.0f; daytime[2] = 0.0f; daytime[3] = 0.0f;
}

ShaderUniformLocations::ShaderUniformLocations()
	: u_cameraPosition(-1),
	  u_previousCameraPosition(-1),
	  u_gbufferModelView(-1),
	  u_gbufferModelViewInverse(-1),
	  u_gbufferProjection(-1),
	  u_gbufferProjectionInverse(-1),
	  u_gbufferPreviousModelView(-1),
	  u_gbufferPreviousProjection(-1),
	  u_sunPosition(-1),
	  u_moonPosition(-1),
	  u_upPosition(-1),
	  u_frameTimeCounter(-1),
	  u_frameCounter(-1),
	  u_frameTime(-1),
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
	  u_gcolor(-1),
	  u_lightmap(-1),
	  u_colortex0(-1),
	  u_colortex1(-1),
	  u_colortex2(-1),
	  u_colortex3(-1),
	  u_colortex4(-1),
	  u_depthtex0(-1),
	  u_depthtex1(-1),
	  u_depthtex2(-1),
	  u_gdepthtex(-1),
	  u_gnormal(-1),
	  u_normals(-1),
	  u_noisetex(-1),
	  u_viewSize(-1),
	  u_pixelSize(-1),
	  u_sunDirView(-1),
	  u_moonDirView(-1),
	  u_upDirView(-1),
	  u_sunDir(-1),
	  u_moonDir(-1),
	  u_lightDir(-1),
	  u_upDir(-1),
	  u_daytime(-1),
	  u_eyeBrightness(-1),
	  u_eyeBrightnessSmooth(-1),
	  u_exposureLevel(-1),
	  u_centerDepthSmooth(-1),
	  u_entityColor(-1),
	  u_entityId(-1),
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
	u_gbufferPreviousModelView = glGetUniformLocation(program, "gbufferPreviousModelView");
	u_gbufferPreviousProjection = glGetUniformLocation(program, "gbufferPreviousProjection");
	u_sunPosition = glGetUniformLocation(program, "sunPosition");
	u_moonPosition = glGetUniformLocation(program, "moonPosition");
	u_upPosition = glGetUniformLocation(program, "upPosition");
	u_frameTimeCounter = glGetUniformLocation(program, "frameTimeCounter");
	u_frameCounter = glGetUniformLocation(program, "frameCounter");
	u_frameTime = glGetUniformLocation(program, "frameTime");
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
	u_gcolor = glGetUniformLocation(program, "gcolor");
	u_lightmap = glGetUniformLocation(program, "lightmap");
	u_colortex0 = glGetUniformLocation(program, "colortex0");
	u_colortex1 = glGetUniformLocation(program, "colortex1");
	u_colortex2 = glGetUniformLocation(program, "colortex2");
	u_colortex3 = glGetUniformLocation(program, "colortex3");
	u_colortex4 = glGetUniformLocation(program, "colortex4");
	u_depthtex0 = glGetUniformLocation(program, "depthtex0");
	u_depthtex1 = glGetUniformLocation(program, "depthtex1");
	u_depthtex2 = glGetUniformLocation(program, "depthtex2");
	u_gdepthtex = glGetUniformLocation(program, "gdepthtex");
	u_gnormal = glGetUniformLocation(program, "gnormal");
	u_normals = glGetUniformLocation(program, "normals");
	u_noisetex = glGetUniformLocation(program, "noisetex");

	u_viewSize = glGetUniformLocation(program, "viewSize");
	u_pixelSize = glGetUniformLocation(program, "pixelSize");

	u_sunDirView = glGetUniformLocation(program, "sunDirView");
	u_moonDirView = glGetUniformLocation(program, "moonDirView");
	u_upDirView = glGetUniformLocation(program, "upDirView");
	u_sunDir = glGetUniformLocation(program, "sunDir");
	u_moonDir = glGetUniformLocation(program, "moonDir");
	u_lightDir = glGetUniformLocation(program, "lightDir");
	u_upDir = glGetUniformLocation(program, "upDir");
	u_daytime = glGetUniformLocation(program, "daytime");

	u_eyeBrightness = glGetUniformLocation(program, "eyeBrightness");
	u_eyeBrightnessSmooth = glGetUniformLocation(program, "eyeBrightnessSmooth");
	u_exposureLevel = glGetUniformLocation(program, "exposureLevel");
	u_centerDepthSmooth = glGetUniformLocation(program, "centerDepthSmooth");
	u_entityColor = glGetUniformLocation(program, "entityColor");
	u_entityId = glGetUniformLocation(program, "entityId");

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
	if (u_frameTime >= 0) glUniform1f(u_frameTime, 0.05f);
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
	if (u_gbufferPreviousModelView >= 0) glUniformMatrix4fv(u_gbufferPreviousModelView, 1, GL_FALSE, vals.gbufferModelView.m);
	if (u_gbufferPreviousProjection >= 0) glUniformMatrix4fv(u_gbufferPreviousProjection, 1, GL_FALSE, vals.gbufferProjection.m);

	if (u_viewSize >= 0) glUniform2f(u_viewSize, vals.viewWidth, vals.viewHeight);
	if (u_pixelSize >= 0) glUniform2f(u_pixelSize, (vals.viewWidth > 0 ? 1.0f / vals.viewWidth : 0.0f), (vals.viewHeight > 0 ? 1.0f / vals.viewHeight : 0.0f));

	if (u_eyeBrightness >= 0) glUniform2i(u_eyeBrightness, 240, 240);
	if (u_eyeBrightnessSmooth >= 0) glUniform2i(u_eyeBrightnessSmooth, 240, 240);

	if (u_sunDir >= 0) glUniform3f(u_sunDir, vals.sunDir[0], vals.sunDir[1], vals.sunDir[2]);
	if (u_moonDir >= 0) glUniform3f(u_moonDir, vals.moonDir[0], vals.moonDir[1], vals.moonDir[2]);
	if (u_lightDir >= 0) glUniform3f(u_lightDir, vals.lightDir[0], vals.lightDir[1], vals.lightDir[2]);
	if (u_upDir >= 0) glUniform3f(u_upDir, vals.upDir[0], vals.upDir[1], vals.upDir[2]);

	if (u_sunDirView >= 0) glUniform3f(u_sunDirView, vals.sunDirView[0], vals.sunDirView[1], vals.sunDirView[2]);
	if (u_moonDirView >= 0) glUniform3f(u_moonDirView, vals.moonDirView[0], vals.moonDirView[1], vals.moonDirView[2]);
	if (u_upDirView >= 0) glUniform3f(u_upDirView, vals.upDirView[0], vals.upDirView[1], vals.upDirView[2]);

	if (u_daytime >= 0) glUniform4f(u_daytime, vals.daytime[0], vals.daytime[1], vals.daytime[2], vals.daytime[3]);

	if (u_exposureLevel >= 0) glUniform1f(u_exposureLevel, 1.0f);
	if (u_centerDepthSmooth >= 0) glUniform1f(u_centerDepthSmooth, 0.5f);
	if (u_entityColor >= 0) glUniform4f(u_entityColor, 1.0f, 1.0f, 1.0f, 1.0f);
	if (u_entityId >= 0) glUniform1i(u_entityId, 0);

	// Legacy & compatibility uniforms
	if (u_layer >= 0) glUniform1i(u_layer, vals.currentLayer);
	if (u_time >= 0) glUniform1f(u_time, vals.frameTimeCounter);
	if (u_camPos >= 0) glUniform3fv(u_camPos, 1, vals.cameraPosition);
	if (u_timeOfDay >= 0) glUniform1f(u_timeOfDay, vals.timeOfDay);

	// Texture samplers
	if (!isCompositeOrFinal) {
		if (u_gtexture >= 0) glUniform1i(u_gtexture, 0);
		if (u_texture >= 0) glUniform1i(u_texture, 0);
		if (u_gcolor >= 0) glUniform1i(u_gcolor, 0);
		if (u_normals >= 0) glUniform1i(u_normals, 1);
		if (u_lightmap >= 0) glUniform1i(u_lightmap, 1);
		if (u_noisetex >= 0) glUniform1i(u_noisetex, 2);
	} else {
		if (u_colortex0 >= 0) glUniform1i(u_colortex0, 0);
		if (u_colortex1 >= 0) glUniform1i(u_colortex1, 1);
		if (u_colortex2 >= 0) glUniform1i(u_colortex2, 2);
		if (u_colortex3 >= 0) glUniform1i(u_colortex3, 0);
		if (u_colortex4 >= 0) glUniform1i(u_colortex4, 1);
		if (u_depthtex0 >= 0) glUniform1i(u_depthtex0, 3);
		if (u_depthtex1 >= 0) glUniform1i(u_depthtex1, 3);
		if (u_depthtex2 >= 0) glUniform1i(u_depthtex2, 3);
		if (u_gdepthtex >= 0) glUniform1i(u_gdepthtex, 3);
		if (u_gnormal >= 0) glUniform1i(u_gnormal, 1);
		if (u_noisetex >= 0) glUniform1i(u_noisetex, 2);
	}
}
