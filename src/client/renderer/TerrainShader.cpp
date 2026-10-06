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
	"void main() {\n"
	"    v_color = gl_Color;\n"
	"    v_texCoord = gl_MultiTexCoord0.xy;\n"
	"    vec4 pos = gl_Vertex;\n"
	"    vec4 viewPos = gl_ModelViewMatrix * pos;\n"
	"    vec3 wpos = u_camPos + viewPos.xyz;\n"
	"    v_worldPos = wpos;\n"
	"    if (u_layer == 1) {\n"
	"        float wave1 = sin(u_time * 2.0 + wpos.x * 1.2 + wpos.z * 1.2) * 0.05;\n"
	"        float wave2 = cos(u_time * 1.3 + wpos.x * 0.7 - wpos.z * 0.9) * 0.03;\n"
	"        pos.x += wave1;\n"
	"        pos.z += wave2;\n"
	"    } else if (u_layer == 2) {\n"
	"        float waveY = sin(u_time * 2.5 + wpos.x * 1.5 + wpos.z * 1.5) * 0.04;\n"
	"        pos.y += waveY;\n"
	"    }\n"
	"    gl_Position = gl_ModelViewProjectionMatrix * pos;\n"
	"    gl_FogFragCoord = length((gl_ModelViewMatrix * pos).xyz);\n"
	"}\n";

static const char* kFragmentShaderSource =
	"#version 120\n"
	"uniform sampler2D u_texture;\n"
	"uniform int u_layer;\n"
	"uniform float u_time;\n"
	"varying vec4 v_color;\n"
	"varying vec2 v_texCoord;\n"
	"varying vec3 v_worldPos;\n"
	"void main() {\n"
	"    vec4 texColor = texture2D(u_texture, v_texCoord);\n"
	"    vec4 color = texColor * v_color;\n"
	"    if (color.a < 0.1) {\n"
	"        discard;\n"
	"    }\n"
	"    if (u_layer == 2) {\n"
	"        float shimmer = sin(u_time * 3.0 + v_worldPos.x * 2.0 + v_worldPos.z * 2.0) * 0.04;\n"
	"        color.rgb += vec3(shimmer, shimmer * 1.1, shimmer * 1.2);\n"
	"    }\n"
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
	  m_uLayer(-1), m_uTime(-1), m_uCamPos(-1), m_uTexture(-1) {}

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

	m_initialized = true;
	printf("[TerrainShader] Initialized terrain shader successfully!\n");
	return true;
}

void TerrainShader::bind(int layer, float time, float camX, float camY, float camZ) {
	if (!m_initialized) {
		if (!init()) return;
	}
	if (!m_program) return;

	glUseProgram(m_program);
	if (m_uLayer >= 0) glUniform1i(m_uLayer, layer);
	if (m_uTime >= 0) glUniform1f(m_uTime, time);
	if (m_uCamPos >= 0) glUniform3f(m_uCamPos, camX, camY, camZ);
	if (m_uTexture >= 0) glUniform1i(m_uTexture, 0);
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
void TerrainShader::bind(int, float, float, float, float) {}
void TerrainShader::unbind() {}
bool TerrainShader::isAvailable() const { return false; }

#endif
