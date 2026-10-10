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
	int u_sunPosition;
	int u_moonPosition;
	int u_upPosition;
	int u_frameTimeCounter;
	int u_frameCounter;
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
	int u_lightmap;
	int u_colortex0;
	int u_colortex1;
	int u_colortex2;
	int u_depthtex0;

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
