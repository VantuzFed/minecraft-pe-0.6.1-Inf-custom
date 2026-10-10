#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPipeline_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPipeline_H__

#include <string>
#include <vector>
#include "ShaderPass.h"
#include "ShaderFramebuffers.h"
#include "ShaderUniforms.h"

class Minecraft;

enum ShaderStage {
	STAGE_NONE = 0,
	STAGE_GBUFFERS_BASIC,
	STAGE_GBUFFERS_TEXTURED,
	STAGE_GBUFFERS_TERRAIN,
	STAGE_GBUFFERS_WATER,
	STAGE_GBUFFERS_ENTITIES,
	STAGE_GBUFFERS_HAND,
	STAGE_GBUFFERS_SKYBASIC,
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
	bool loadShaderPack(const std::string& packPath);
	void reload();

	std::vector<std::string> getAvailablePacks() const;
	const std::string& getCurrentPack() const { return m_currentPack; }

private:
	void setupBuiltInShaders();

	bool m_initialized;
	bool m_sceneActive;
	std::string m_currentPack;

	ShaderFramebuffers m_fbo;
	ShaderUniformValues m_uniforms;

	// Passes
	ShaderPass m_passTerrain;
	ShaderPass m_passWater;
	ShaderPass m_passEntities;
	ShaderPass m_passHand;
	ShaderPass m_passSky;
	ShaderPass m_passComposite;
	ShaderPass m_passFinal;

	ShaderStage m_activeStage;
};

extern ShaderPipeline g_shaderPipeline;

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPipeline_H__ */
