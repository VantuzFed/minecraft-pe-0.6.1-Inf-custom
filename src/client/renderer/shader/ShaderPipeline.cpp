#include "ShaderPipeline.h"
#include "../gles.h"
#include "../../Minecraft.h"
#include "../../../world/level/Level.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/level/material/Material.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

ShaderPipeline g_shaderPipeline;

// Built-in G-buffers Terrain Vertex Shader (GLSL 1.20)
static const char* kBuiltinTerrainVS =
	"#version 120\n"
	"uniform int u_layer;\n"
	"uniform float frameTimeCounter;\n"
	"uniform vec3 cameraPosition;\n"
	"varying vec4 v_color;\n"
	"varying vec2 v_texCoord;\n"
	"varying vec3 v_worldPos;\n"
	"varying vec3 v_viewDir;\n"
	"varying vec3 v_normal;\n"
	"void main() {\n"
	"    v_color = gl_Color;\n"
	"    v_texCoord = gl_MultiTexCoord0.xy;\n"
	"    v_normal = gl_NormalMatrix * gl_Normal;\n"
	"    vec4 pos = gl_Vertex;\n"
	"    vec4 viewPos = gl_ModelViewMatrix * pos;\n"
	"    vec3 wpos = cameraPosition + viewPos.xyz;\n"
	"    v_worldPos = wpos;\n"
	"    v_viewDir = -viewPos.xyz;\n"
	"    if (u_layer == 1) {\n"
	"        float wave1 = sin(frameTimeCounter * 2.2 + wpos.x * 1.3 + wpos.z * 1.3) * 0.05;\n"
	"        float wave2 = cos(frameTimeCounter * 1.5 + wpos.x * 0.8 - wpos.z * 1.1) * 0.03;\n"
	"        pos.x += wave1;\n"
	"        pos.z += wave2;\n"
	"    } else if (u_layer == 2) {\n"
	"        float waveY = sin(frameTimeCounter * 2.8 + wpos.x * 1.8 + wpos.z * 1.8) * 0.04;\n"
	"        pos.y += waveY;\n"
	"    }\n"
	"    gl_Position = gl_ModelViewProjectionMatrix * pos;\n"
	"    gl_FogFragCoord = length(viewPos.xyz);\n"
	"}\n";

// Built-in G-buffers Terrain Fragment Shader (GLSL 1.20, MRT output)
static const char* kBuiltinTerrainFS =
	"#version 120\n"
	"uniform sampler2D texture;\n"
	"uniform sampler2D lightmap;\n"
	"uniform int u_layer;\n"
	"uniform float frameTimeCounter;\n"
	"uniform vec3 cameraPosition;\n"
	"uniform float timeOfDay;\n"
	"varying vec4 v_color;\n"
	"varying vec2 v_texCoord;\n"
	"varying vec3 v_worldPos;\n"
	"varying vec3 v_viewDir;\n"
	"varying vec3 v_normal;\n"
	"void main() {\n"
	"    vec4 texColor = texture2D(texture, v_texCoord);\n"
	"    vec4 color = texColor * v_color;\n"
	"    if (color.a < 0.1) discard;\n"
	"\n"
	"    // Dynamic Lighting & Color Grading\n"
	"    float light = max(v_color.r, max(v_color.g, v_color.b));\n"
	"    float sunHeight = sin(timeOfDay * 6.2831853);\n"
	"    float dayFactor = clamp(sunHeight * 2.0 + 0.5, 0.0, 1.0);\n"
	"    float sunsetFactor = clamp(1.0 - abs(sunHeight) * 3.0, 0.0, 1.0);\n"
	"\n"
	"    vec3 daySunColor = vec3(1.10, 1.04, 0.92);\n"
	"    vec3 sunsetColor = vec3(1.30, 0.85, 0.55);\n"
	"    vec3 sunLightColor = mix(daySunColor, sunsetColor, sunsetFactor);\n"
	"\n"
	"    vec3 dayAmbient = vec3(0.78, 0.84, 0.98);\n"
	"    vec3 nightAmbient = vec3(0.55, 0.62, 0.88);\n"
	"    vec3 ambientColor = mix(nightAmbient, dayAmbient, dayFactor);\n"
	"\n"
	"    vec3 lightTint = mix(ambientColor, sunLightColor, smoothstep(0.15, 0.85, light));\n"
	"    color.rgb *= lightTint;\n"
	"\n"
	"    // Torchlight warm glow\n"
	"    float torchLight = smoothstep(0.40, 0.95, light);\n"
	"    color.rgb += vec3(0.06, 0.03, 0.00) * torchLight;\n"
	"\n"
	"    // Water Shader (Layer 2)\n"
	"    if (u_layer == 2) {\n"
	"        float wave1 = sin(frameTimeCounter * 3.2 + v_worldPos.x * 2.5 + v_worldPos.z * 2.5);\n"
	"        float wave2 = cos(frameTimeCounter * 2.4 - v_worldPos.x * 1.8 + v_worldPos.z * 2.0);\n"
	"        float caustic = wave1 * wave2;\n"
	"        vec3 vdir = normalize(v_viewDir);\n"
	"        vec3 sdir = normalize(vec3(0.35, 0.75, 0.45));\n"
	"        vec3 norm = normalize(vec3(caustic * 0.12, 1.0, caustic * 0.12));\n"
	"        vec3 hdir = normalize(vdir + sdir);\n"
	"        float spec = pow(max(dot(norm, hdir), 0.0), 32.0);\n"
	"        color.rgb = mix(color.rgb, vec3(0.12, 0.50, 0.72), 0.28);\n"
	"        color.rgb += vec3(1.0, 0.96, 0.85) * spec * 0.55;\n"
	"        color.rgb += vec3(caustic * 0.03, caustic * 0.045, caustic * 0.06);\n"
	"    }\n"
	"\n"
	"    // Output MRT: colortex0 = diffuse, colortex1 = normal, colortex2 = extra\n"
	"    gl_FragData[0] = color;\n"
	"    gl_FragData[1] = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);\n"
	"    gl_FragData[2] = vec4(light, float(u_layer) / 4.0, 0.0, 1.0);\n"
	"}\n";

// Built-in Final Postprocessing Vertex Shader
static const char* kBuiltinFinalVS =
	"#version 120\n"
	"varying vec2 v_texCoord;\n"
	"void main() {\n"
	"    v_texCoord = gl_MultiTexCoord0.xy;\n"
	"    gl_Position = vec4(gl_Vertex.xy, 0.0, 1.0);\n"
	"}\n";

// Built-in Final Postprocessing Fragment Shader (Vibrance & Tone Mapping)
static const char* kBuiltinFinalFS =
	"#version 120\n"
	"uniform sampler2D colortex0;\n"
	"varying vec2 v_texCoord;\n"
	"void main() {\n"
	"    vec4 color = texture2D(colortex0, v_texCoord);\n"
	"    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));\n"
	"    color.rgb = mix(vec3(luma), color.rgb, 1.15);\n"
	"    color.rgb = pow(color.rgb, vec3(0.95));\n"
	"    color.rgb = color.rgb * color.rgb * (3.0 - 2.0 * color.rgb);\n"
	"    gl_FragColor = color;\n"
	"}\n";

ShaderPipeline::ShaderPipeline()
	: m_initialized(false),
	  m_sceneActive(false),
	  m_mode(SHADER_MODE_BUILTIN),
	  m_currentPack("Built-in"),
	  m_lightmapTexture(0),
	  m_activeStage(STAGE_NONE)
{}

ShaderPipeline::~ShaderPipeline() {
	// Do not delete GL resources in destructor; context is already destroyed at process exit.
}

bool ShaderPipeline::init(int width, int height) {
	if (m_initialized) return true;

	if (!m_fbo.init(width, height)) {
		printf("[ShaderPipeline] Failed to create framebuffers\n");
		return false;
	}

	if (m_lightmapTexture == 0 && glGenTextures) {
		glGenTextures(1, &m_lightmapTexture);
		glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		updateLightmap(0.0f);
	}

	setupBuiltInShaders();
	m_initialized = true;
	printf("[ShaderPipeline] Pipeline successfully initialized (%dx%d)\n", width, height);
	return true;
}

void ShaderPipeline::resize(int width, int height) {
	if (!m_initialized) {
		init(width, height);
		return;
	}
	m_fbo.resize(width, height);
	m_uniforms.viewWidth = (float)width;
	m_uniforms.viewHeight = (float)height;
	m_uniforms.aspectRatio = (height > 0) ? ((float)width / (float)height) : 1.0f;
}

void ShaderPipeline::destroyPasses() {
	m_passTerrain.destroy();
	m_passWater.destroy();
	m_passEntities.destroy();
	m_passHand.destroy();
	m_passSky.destroy();
	m_passSkyTextured.destroy();
	m_passClouds.destroy();
	m_passWeather.destroy();
	m_passComposite.destroy();
	m_passFinal.destroy();
}

void ShaderPipeline::setupBuiltInShaders() {
	destroyPasses();
	m_passTerrain.loadFromSource(kBuiltinTerrainVS, kBuiltinTerrainFS, "gbuffers_terrain (built-in)");
	m_passFinal.loadFromSource(kBuiltinFinalVS, kBuiltinFinalFS, "final (built-in)");
	m_mode = SHADER_MODE_BUILTIN;
	m_currentPack = "Built-in";
}

void ShaderPipeline::updateLightmap(float timeOfDay) {
	if (m_lightmapTexture == 0) return;

	unsigned char pixels[16 * 16 * 4];
	float sunAngle = timeOfDay * 3.14159265f * 2.0f;
	float sunHeight = std::sin(sunAngle);
	float dayFactor = std::max(0.0f, std::min(1.0f, sunHeight * 2.0f + 0.5f));

	for (int y = 0; y < 16; ++y) {
		float skyFactor = std::pow((float)y / 15.0f, 1.2f) * dayFactor;
		float nightFactor = std::pow((float)y / 15.0f, 1.2f) * (1.0f - dayFactor) * 0.15f;

		for (int x = 0; x < 16; ++x) {
			float blockFactor = std::pow((float)x / 15.0f, 1.5f);

			float r = blockFactor * 1.00f + skyFactor * 1.00f + nightFactor * 0.35f;
			float g = blockFactor * 0.82f + skyFactor * 0.98f + nightFactor * 0.45f;
			float b = blockFactor * 0.60f + skyFactor * 0.90f + nightFactor * 0.75f;

			int idx = (y * 16 + x) * 4;
			pixels[idx + 0] = (unsigned char)(std::min(1.0f, r) * 255.0f);
			pixels[idx + 1] = (unsigned char)(std::min(1.0f, g) * 255.0f);
			pixels[idx + 2] = (unsigned char)(std::min(1.0f, b) * 255.0f);
			pixels[idx + 3] = 255;
		}
	}

	glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

bool ShaderPipeline::isEnabled() const {
	return m_initialized && (m_mode != SHADER_MODE_OFF);
}

void ShaderPipeline::setMode(ShaderPipelineMode mode) {
	m_mode = mode;
	if (m_mode == SHADER_MODE_OFF) {
		m_currentPack = "(OFF)";
	} else if (m_mode == SHADER_MODE_BUILTIN) {
		setupBuiltInShaders();
	}
}

void ShaderPipeline::beginFrame(Minecraft* mc, double partialTicks) {
	if (!m_initialized || !mc || m_mode == SHADER_MODE_OFF) return;

	resize(mc->width, mc->height);

	m_uniforms.frameCounter++;
	m_uniforms.frameTimeCounter += 0.05f;

	if (mc->cameraTargetPlayer) {
		Mob* p = mc->cameraTargetPlayer;
		m_uniforms.previousCameraPosition[0] = m_uniforms.cameraPosition[0];
		m_uniforms.previousCameraPosition[1] = m_uniforms.cameraPosition[1];
		m_uniforms.previousCameraPosition[2] = m_uniforms.cameraPosition[2];

		m_uniforms.cameraPosition[0] = (float)(p->xOld + (p->x - p->xOld) * partialTicks);
		m_uniforms.cameraPosition[1] = (float)(p->yOld + (p->y - p->yOld) * partialTicks);
		m_uniforms.cameraPosition[2] = (float)(p->zOld + (p->z - p->zOld) * partialTicks);

		m_uniforms.isEyeInWater = p->isUnderLiquid(Material::water) ? 1 : 0;
	}

	if (mc->level) {
		m_uniforms.timeOfDay = mc->level->getTimeOfDay(partialTicks);
		m_uniforms.worldTime = (int)mc->level->getTime();
		m_uniforms.rainStrength = 0.0f;

		float angle = m_uniforms.timeOfDay * 3.14159265f * 2.0f;
		m_uniforms.sunPosition[0] = std::sin(angle) * 100.0f;
		m_uniforms.sunPosition[1] = std::cos(angle) * 100.0f;
		m_uniforms.sunPosition[2] = 0.0f;

		m_uniforms.moonPosition[0] = -m_uniforms.sunPosition[0];
		m_uniforms.moonPosition[1] = -m_uniforms.sunPosition[1];
		m_uniforms.moonPosition[2] = 0.0f;

		updateLightmap(m_uniforms.timeOfDay);
	}

	// Read ModelView and Projection matrices directly from current OpenGL state
	float mv[16];
	float pr[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);
	glGetFloatv(GL_PROJECTION_MATRIX, pr);
	m_uniforms.gbufferModelView = Matrix4(mv);
	m_uniforms.gbufferModelViewInverse = m_uniforms.gbufferModelView.inverse();
	m_uniforms.gbufferProjection = Matrix4(pr);
	m_uniforms.gbufferProjectionInverse = m_uniforms.gbufferProjection.inverse();
}

void ShaderPipeline::beginScene() {
	if (!m_initialized || m_mode == SHADER_MODE_OFF) return;
	m_fbo.bindScene();
	m_sceneActive = true;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ShaderPipeline::bindPass(ShaderStage stage, int layer) {
	if (!m_sceneActive || m_mode == SHADER_MODE_OFF) return;

	m_uniforms.currentLayer = layer;
	m_activeStage = stage;

	// Bind lightmap texture to texture unit 1 for all gbuffers passes
	if (glActiveTexture && m_lightmapTexture != 0) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
		glActiveTexture(GL_TEXTURE0);
	}

	switch (stage) {
	case STAGE_GBUFFERS_TERRAIN:
		if (layer == 1 && m_passWater.isValid()) {
			m_passWater.bind(m_uniforms, false);
		} else if (m_passTerrain.isValid()) {
			m_passTerrain.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_WATER:
		if (m_passWater.isValid()) {
			m_passWater.bind(m_uniforms, false);
		} else if (m_passTerrain.isValid()) {
			m_passTerrain.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_ENTITIES:
		if (m_passEntities.isValid()) {
			m_passEntities.bind(m_uniforms, false);
		} else if (m_passTerrain.isValid()) {
			m_passTerrain.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_HAND:
		if (m_passHand.isValid()) {
			m_passHand.bind(m_uniforms, false);
		} else if (m_passEntities.isValid()) {
			m_passEntities.bind(m_uniforms, false);
		} else if (m_passTerrain.isValid()) {
			m_passTerrain.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_SKYBASIC:
		if (m_passSky.isValid()) {
			m_passSky.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_SKYTEXTURED:
		if (m_passSkyTextured.isValid()) {
			m_passSkyTextured.bind(m_uniforms, false);
		} else if (m_passSky.isValid()) {
			m_passSky.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_CLOUDS:
		if (m_passClouds.isValid()) {
			m_passClouds.bind(m_uniforms, false);
		} else if (m_passSky.isValid()) {
			m_passSky.bind(m_uniforms, false);
		}
		break;
	case STAGE_GBUFFERS_WEATHER:
		if (m_passWeather.isValid()) {
			m_passWeather.bind(m_uniforms, false);
		}
		break;
	default:
		break;
	}
}

void ShaderPipeline::unbindPass() {
	if (glUseProgram) {
		glUseProgram(0);
	}
	m_activeStage = STAGE_NONE;
}

void ShaderPipeline::endScene() {
	if (!m_sceneActive || m_mode == SHADER_MODE_OFF) return;
	unbindPass();
	m_fbo.unbind();
	m_sceneActive = false;

	bool hadComposite = false;

	// 1. Composite Pass (if present, renders to alt FBO)
	if (m_passComposite.isValid()) {
		m_fbo.bindComposite();
		m_passComposite.bind(m_uniforms, true);
		m_fbo.drawFullscreenQuad();
		m_passComposite.unbind();
		hadComposite = true;
	}

	// 2. Final Pass (renders to screen, sampling either alt FBO or main FBO)
	if (m_passFinal.isValid()) {
		m_fbo.bindFinal(hadComposite);
		m_passFinal.bind(m_uniforms, true);
		m_fbo.drawFullscreenQuad();
		m_passFinal.unbind();
	} else {
		m_fbo.blitToScreen(hadComposite);
	}

	if (glActiveTexture) {
		glActiveTexture(GL_TEXTURE0);
	}
}

bool ShaderPipeline::loadShaderPack(const std::string& packNameOrPath) {
	if (packNameOrPath.empty() || packNameOrPath == "(OFF)" || packNameOrPath == "OFF") {
		setMode(SHADER_MODE_OFF);
		return true;
	}

	if (packNameOrPath == "Built-in" || packNameOrPath == "(internal)" || packNameOrPath == "internal") {
		setupBuiltInShaders();
		return true;
	}

	ShaderPack pack;
	if (!pack.open(packNameOrPath)) {
		printf("[ShaderPipeline] Failed to open shaderpack '%s'\n", packNameOrPath.c_str());
		setupBuiltInShaders();
		return false;
	}

	printf("[ShaderPipeline] Loading shaderpack '%s'...\n", pack.getName().c_str());
	destroyPasses();

	bool loadedAny = false;

	// Load stages with standard OptiFine/Iris fallbacks
	if (m_passTerrain.loadWithFallback(pack, { "gbuffers_terrain", "gbuffers_textured", "gbuffers_basic" })) {
		loadedAny = true;
	}
	if (m_passWater.loadWithFallback(pack, { "gbuffers_water", "gbuffers_terrain" })) {
		loadedAny = true;
	}
	if (m_passEntities.loadWithFallback(pack, { "gbuffers_entities", "gbuffers_textured", "gbuffers_terrain" })) {
		loadedAny = true;
	}
	if (m_passHand.loadWithFallback(pack, { "gbuffers_hand", "gbuffers_entities", "gbuffers_textured" })) {
		loadedAny = true;
	}
	if (m_passSky.loadWithFallback(pack, { "gbuffers_skybasic", "gbuffers_basic" })) {
		loadedAny = true;
	}
	if (m_passSkyTextured.loadWithFallback(pack, { "gbuffers_skytextured", "gbuffers_skybasic", "gbuffers_textured" })) {
		loadedAny = true;
	}
	if (m_passClouds.loadWithFallback(pack, { "gbuffers_clouds", "gbuffers_skybasic", "gbuffers_textured" })) {
		loadedAny = true;
	}
	if (m_passWeather.loadWithFallback(pack, { "gbuffers_weather", "gbuffers_textured" })) {
		loadedAny = true;
	}
	if (m_passComposite.loadWithFallback(pack, { "composite", "composite0" })) {
		loadedAny = true;
	}
	if (m_passFinal.loadWithFallback(pack, { "final" })) {
		loadedAny = true;
	}

	if (loadedAny) {
		m_mode = SHADER_MODE_CUSTOM;
		m_currentPack = pack.getName();
		printf("[ShaderPipeline] Shaderpack '%s' successfully loaded!\n", m_currentPack.c_str());
		return true;
	} else {
		printf("[ShaderPipeline] No valid shader passes in '%s', reverting to built-in\n", packNameOrPath.c_str());
		setupBuiltInShaders();
		return false;
	}
}

void ShaderPipeline::reload() {
	if (m_mode == SHADER_MODE_CUSTOM && !m_currentPack.empty()) {
		loadShaderPack(m_currentPack);
	} else if (m_mode == SHADER_MODE_BUILTIN) {
		setupBuiltInShaders();
	}
}

std::vector<std::string> ShaderPipeline::getAvailablePacks() const {
	std::vector<std::string> packs;
	packs.push_back("(OFF)");
	packs.push_back("(internal)");

	std::vector<std::string> customPacks = ShaderPack::scanAvailablePacks("shaderpacks");
	packs.insert(packs.end(), customPacks.begin(), customPacks.end());
	return packs;
}
