#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPipeline_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPipeline_H__

#include <string>
#include <vector>
#include "ShaderPass.h"
#include "ShaderPack.h"
#include "ShaderFramebuffers.h"
#include "ShaderUniforms.h"

class Minecraft;

enum ShaderPipelineMode {
	SHADER_MODE_OFF = 0,
	SHADER_MODE_BUILTIN,
	SHADER_MODE_CUSTOM
};

enum ShaderStage {
	STAGE_NONE = 0,
	STAGE_GBUFFERS_BASIC,
	STAGE_GBUFFERS_TEXTURED,
	STAGE_GBUFFERS_TERRAIN,
	STAGE_GBUFFERS_WATER,
	STAGE_GBUFFERS_ENTITIES,
	STAGE_GBUFFERS_HAND,
	STAGE_GBUFFERS_SKYBASIC,
	STAGE_GBUFFERS_SKYTEXTURED,
	STAGE_GBUFFERS_CLOUDS,
	STAGE_GBUFFERS_WEATHER
};

class ShaderPipeline {
public:
	ShaderPipeline();
	~ShaderPipeline();

	bool init(int width, int height);
	void resize(int width, int height);

	void beginFrame(Minecraft* mc, double partialTicks);
	void beginScene();
	void bindPass(ShaderStage stage, int layer = 0);
	void unbindPass();
	void endScene();

	bool isEnabled() const;
	void setMode(ShaderPipelineMode mode);
	ShaderPipelineMode getMode() const { return m_mode; }

	bool loadShaderPack(const std::string& packNameOrPath);
	void reload();

	std::vector<std::string> getAvailablePacks() const;
	const std::string& getCurrentPack() const { return m_currentPack; }

private:
	void setupBuiltInShaders();
	void updateLightmap(float timeOfDay);
	void destroyPasses();

	bool m_initialized;
	bool m_sceneActive;
	ShaderPipelineMode m_mode;
	std::string m_currentPack;

	ShaderFramebuffers m_fbo;
	ShaderUniformValues m_uniforms;
	unsigned int m_lightmapTexture;

	// Shader Passes
	ShaderPass m_passTerrain;
	ShaderPass m_passWater;
	ShaderPass m_passEntities;
	ShaderPass m_passHand;
	ShaderPass m_passSky;
	ShaderPass m_passSkyTextured;
	ShaderPass m_passClouds;
	ShaderPass m_passWeather;
	ShaderPass m_passComposite;
	ShaderPass m_passFinal;

	ShaderStage m_activeStage;
};

extern ShaderPipeline g_shaderPipeline;

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPipeline_H__ */
