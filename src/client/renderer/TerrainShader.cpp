#include "TerrainShader.h"

TerrainShader g_terrainShader;

#if defined(PLATFORM_DESKTOP)
#include "gles.h"
#include <cstdio>

static const char* kVertexShaderSource =
	"#version 120\n"
	"uniform int u_layer;\n"
	"uniform float u_time;\n"
	"uniform vec3 u_camPos;\n"
	"varying vec4 v_color;\n"
	"varying vec2 v_texCoord;\n"
	"varying vec3 v_worldPos;\n"
	"varying vec3 v_viewDir;\n"
	"void main() {\n"
	"    v_color = gl_Color;\n"
	"    v_texCoord = gl_MultiTexCoord0.xy;\n"
	"    vec4 pos = gl_Vertex;\n"
	"    vec4 viewPos = gl_ModelViewMatrix * pos;\n"
	"    vec3 wpos = u_camPos + viewPos.xyz;\n"
	"    v_worldPos = wpos;\n"
	"    v_viewDir = -viewPos.xyz;\n"
	"    if (u_layer == 1) {\n"
	"        float wave1 = sin(u_time * 2.2 + wpos.x * 1.3 + wpos.z * 1.3) * 0.055;\n"
	"        float wave2 = cos(u_time * 1.5 + wpos.x * 0.8 - wpos.z * 1.1) * 0.035;\n"
	"        pos.x += wave1;\n"
	"        pos.z += wave2;\n"
	"    } else if (u_layer == 2) {\n"
	"        float waveY = sin(u_time * 2.8 + wpos.x * 1.8 + wpos.z * 1.8) * 0.045;\n"
	"        pos.y += waveY;\n"
	"    }\n"
	"    gl_Position = gl_ModelViewProjectionMatrix * pos;\n"
	"    gl_FogFragCoord = length(viewPos.xyz);\n"
	"}\n";

static const char* kFragmentShaderSource =
	"#version 120\n"
	"uniform sampler2D u_texture;\n"
	"uniform int u_layer;\n"
	"uniform float u_time;\n"
	"uniform vec3 u_camPos;\n"
	"uniform float u_timeOfDay;\n"
	"varying vec4 v_color;\n"
	"varying vec2 v_texCoord;\n"
	"varying vec3 v_worldPos;\n"
	"varying vec3 v_viewDir;\n"
	"void main() {\n"
	"    vec4 texColor = texture2D(u_texture, v_texCoord);\n"
	"    vec4 color = texColor * v_color;\n"
	"    if (color.a < 0.1) discard;\n"
	"\n"
	"    // Dynamic Lighting & Color Grading\n"
	"    float light = max(v_color.r, max(v_color.g, v_color.b));\n"
	"    float sunHeight = sin(u_timeOfDay * 6.2831853);\n"
	"    float dayFactor = clamp(sunHeight * 2.0 + 0.5, 0.0, 1.0);\n"
	"    float sunsetFactor = clamp(1.0 - abs(sunHeight) * 3.0, 0.0, 1.0);\n"
	"\n"
	"    // Warm sunlight vs sunset vs ambient sky shadow\n"
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
	"    // Cozy torchlight warm glow\n"
	"    float torchLight = smoothstep(0.40, 0.95, light);\n"
	"    color.rgb += vec3(0.06, 0.03, 0.00) * torchLight;\n"
	"\n"
	"    // Water Shader (Layer 2)\n"
	"    if (u_layer == 2) {\n"
	"        float wave1 = sin(u_time * 3.2 + v_worldPos.x * 2.5 + v_worldPos.z * 2.5);\n"
	"        float wave2 = cos(u_time * 2.4 - v_worldPos.x * 1.8 + v_worldPos.z * 2.0);\n"
	"        float caustic = wave1 * wave2;\n"
	"\n"
	"        vec3 vdir = normalize(v_viewDir);\n"
	"        vec3 sdir = normalize(vec3(0.35, 0.75, 0.45));\n"
	"        vec3 norm = normalize(vec3(caustic * 0.12, 1.0, caustic * 0.12));\n"
	"        vec3 hdir = normalize(vdir + sdir);\n"
	"        float spec = pow(max(dot(norm, hdir), 0.0), 32.0);\n"
	"\n"
	"        color.rgb = mix(color.rgb, vec3(0.12, 0.50, 0.72), 0.28);\n"
	"        color.rgb += vec3(1.0, 0.96, 0.85) * spec * 0.55;\n"
	"        color.rgb += vec3(caustic * 0.03, caustic * 0.045, caustic * 0.06);\n"
	"    }\n"
	"\n"
	"    // Vibrance & Filmic Tone Curve\n"
	"    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));\n"
	"    color.rgb = mix(vec3(luma), color.rgb, 1.20);\n"
	"    color.rgb = pow(color.rgb, vec3(0.92));\n"
	"    color.rgb = color.rgb * color.rgb * (3.0 - 2.0 * color.rgb);\n"
	"\n"
	"    // Atmospheric Fog\n"
	"    float fogFactor = 1.0;\n"
	"    if (gl_Fog.scale > 0.0) {\n"
	"        fogFactor = clamp((gl_Fog.end - gl_FogFragCoord) * gl_Fog.scale, 0.0, 1.0);\n"
	"    } else if (gl_Fog.density > 0.0) {\n"
	"        fogFactor = clamp(exp(-gl_Fog.density * gl_FogFragCoord), 0.0, 1.0);\n"
	"    }\n"
	"    color.rgb = mix(gl_Fog.color.rgb, color.rgb, fogFactor);\n"
	"    gl_FragColor = color;\n"
	"}\n";

TerrainShader::TerrainShader()
	: m_initialized(false), m_failed(false), m_program(0),
	  m_uLayer(-1), m_uTime(-1), m_uCamPos(-1), m_uTexture(-1), m_uTimeOfDay(-1) {}

TerrainShader::~TerrainShader() {
	if (m_program != 0 && glDeleteProgram) {
		glDeleteProgram(m_program);
		m_program = 0;
	}
}

bool TerrainShader::isAvailable() const {
	return m_initialized;
}

unsigned int TerrainShader::compileShader(unsigned int type, const char* source) {
	if (!glCreateShader || !glShaderSource || !glCompileShader) return 0;
	GLuint shader = glCreateShader(type);
	if (!shader) return 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[512];
		GLsizei len = 0;
		glGetShaderInfoLog(shader, sizeof(log), &len, log);
		printf("[TerrainShader] Shader compile failed: %s\n", log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

bool TerrainShader::init() {
	if (m_initialized) return true;
	if (m_failed) return false;

	if (!glCreateProgram || !glCreateShader) {
		m_failed = true;
		return false;
	}

	GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
	if (!vs) {
		m_failed = true;
		return false;
	}

	GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
	if (!fs) {
		glDeleteShader(vs);
		m_failed = true;
		return false;
	}

	m_program = glCreateProgram();
	glAttachShader(m_program, vs);
	glAttachShader(m_program, fs);
	glLinkProgram(m_program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	GLint linkStatus = 0;
	glGetProgramiv(m_program, GL_LINK_STATUS, &linkStatus);
	if (!linkStatus) {
		char log[512];
		GLsizei len = 0;
		glGetProgramInfoLog(m_program, sizeof(log), &len, log);
		printf("[TerrainShader] Program link failed: %s\n", log);
		glDeleteProgram(m_program);
		m_program = 0;
		m_failed = true;
		return false;
	}

	m_uLayer = glGetUniformLocation(m_program, "u_layer");
	m_uTime = glGetUniformLocation(m_program, "u_time");
	m_uCamPos = glGetUniformLocation(m_program, "u_camPos");
	m_uTexture = glGetUniformLocation(m_program, "u_texture");
	m_uTimeOfDay = glGetUniformLocation(m_program, "u_timeOfDay");

	m_initialized = true;
	printf("[TerrainShader] Initialized enhanced terrain shader successfully!\n");
	return true;
}

void TerrainShader::bind(int layer, float time, float camX, float camY, float camZ, float timeOfDay) {
	if (!m_initialized) {
		if (!init()) return;
	}
	if (!m_program) return;

	glUseProgram(m_program);
	if (m_uLayer >= 0) glUniform1i(m_uLayer, layer);
	if (m_uTime >= 0) glUniform1f(m_uTime, time);
	if (m_uCamPos >= 0) glUniform3f(m_uCamPos, camX, camY, camZ);
	if (m_uTexture >= 0) glUniform1i(m_uTexture, 0);
	if (m_uTimeOfDay >= 0) glUniform1f(m_uTimeOfDay, timeOfDay);
}

void TerrainShader::unbind() {
	if (glUseProgram) {
		glUseProgram(0);
	}
}

#else

TerrainShader::TerrainShader() {}
TerrainShader::~TerrainShader() {}
bool TerrainShader::init() { return false; }
void TerrainShader::bind(int, float, float, float, float, float) {}
void TerrainShader::unbind() {}
bool TerrainShader::isAvailable() const { return false; }

#endif
