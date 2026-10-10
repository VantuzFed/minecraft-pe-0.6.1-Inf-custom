#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderUniforms_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderUniforms_H__

#include "Matrix4.h"

struct ShaderUniformValues {
	float cameraPosition[3];
	float previousCameraPosition[3];
	Matrix4 gbufferModelView;
	Matrix4 gbufferModelViewInverse;
	Matrix4 gbufferProjection;
	Matrix4 gbufferProjectionInverse;
	float sunPosition[3];
	float moonPosition[3];
	float upPosition[3];
	float sunDir[3];
	float moonDir[3];
	float lightDir[3];
	float upDir[3];
	float sunDirView[3];
	float moonDirView[3];
	float upDirView[3];
	float daytime[4];
	float frameTimeCounter;
	int frameCounter;
	float viewWidth;
	float viewHeight;
	float aspectRatio;
	float nearPlane;
	float farPlane;
	float rainStrength;
	int worldTime;
	int isEyeInWater;
	float timeOfDay;
	int currentLayer;

	ShaderUniformValues();
};

class ShaderUniformLocations {
public:
	int u_cameraPosition;
	int u_previousCameraPosition;
	int u_gbufferModelView;
	int u_gbufferModelViewInverse;
	int u_gbufferProjection;
	int u_gbufferProjectionInverse;
	int u_gbufferPreviousModelView;
	int u_gbufferPreviousProjection;
	int u_sunPosition;
	int u_moonPosition;
	int u_upPosition;
	int u_frameTimeCounter;
	int u_frameCounter;
	int u_frameTime;
	int u_viewWidth;
	int u_viewHeight;
	int u_aspectRatio;
	int u_near;
	int u_far;
	int u_rainStrength;
	int u_wetness;
	int u_worldTime;
	int u_isEyeInWater;

	// Texture samplers
	int u_gtexture;
	int u_texture;
	int u_gcolor;
	int u_lightmap;
	int u_colortex0;
	int u_colortex1;
	int u_colortex2;
	int u_colortex3;
	int u_colortex4;
	int u_depthtex0;
	int u_depthtex1;
	int u_depthtex2;
	int u_gdepthtex;
	int u_gnormal;
	int u_normals;
	int u_noisetex;

	// View size & pixel size
	int u_viewSize;
	int u_pixelSize;

	// Sky & lighting
	int u_sunDirView;
	int u_moonDirView;
	int u_upDirView;
	int u_sunDir;
	int u_moonDir;
	int u_lightDir;
	int u_upDir;
	int u_daytime;
	int u_eyeBrightness;
	int u_eyeBrightnessSmooth;
	int u_exposureLevel;
	int u_centerDepthSmooth;
	int u_entityColor;
	int u_entityId;

	// Backward-compatible built-ins
	int u_layer;
	int u_time;
	int u_camPos;
	int u_timeOfDay;

	ShaderUniformLocations();
	void findLocations(unsigned int program);
	void apply(unsigned int program, const ShaderUniformValues& vals, bool isCompositeOrFinal) const;
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderUniforms_H__ */
