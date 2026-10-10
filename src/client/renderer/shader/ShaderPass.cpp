#include "ShaderPass.h"
#include "../gles.h"
#include <cstdio>
#include <fstream>
#include <sstream>

ShaderPass::ShaderPass()
	: m_program(0),
	  m_valid(false)
{}

ShaderPass::~ShaderPass() {
	destroy();
}

void ShaderPass::destroy() {
	if (m_program != 0 && glDeleteProgram) {
		glDeleteProgram(m_program);
		m_program = 0;
	}
	m_valid = false;
}

unsigned int ShaderPass::compileShader(unsigned int type, const char* source) {
	if (!glCreateShader || !glShaderSource || !glCompileShader || !source) return 0;
	GLuint shader = glCreateShader(type);
	if (!shader) return 0;

	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char log[1024];
		GLsizei len = 0;
		glGetShaderInfoLog(shader, sizeof(log), &len, log);
		printf("[ShaderPass %s] Compilation failed (%s): %s\n",
		       m_name.c_str(),
		       (type == GL_VERTEX_SHADER ? "vertex" : "fragment"),
		       log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

bool ShaderPass::loadFromSource(const char* vsSrc, const char* fsSrc, const std::string& passName) {
	destroy();
	m_name = passName;

	if (!glCreateProgram || !glCreateShader) return false;

	GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
	if (!vs) return false;

	GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
	if (!fs) {
		glDeleteShader(vs);
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
		char log[1024];
		GLsizei len = 0;
		glGetProgramInfoLog(m_program, sizeof(log), &len, log);
		printf("[ShaderPass %s] Link failed: %s\n", m_name.c_str(), log);
		destroy();
		return false;
	}

	m_uniforms.findLocations(m_program);
	m_valid = true;
	return true;
}

static std::string readFileContent(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool ShaderPass::loadFromFiles(const std::string& vsPath, const std::string& fsPath) {
	std::string vsSrc = readFileContent(vsPath);
	std::string fsSrc = readFileContent(fsPath);
	if (vsSrc.empty() || fsSrc.empty()) return false;
	return loadFromSource(vsSrc.c_str(), fsSrc.c_str(), vsPath);
}

void ShaderPass::bind(const ShaderUniformValues& vals, bool isCompositeOrFinal) {
	if (!m_valid || m_program == 0 || !glUseProgram) return;
	glUseProgram(m_program);
	m_uniforms.apply(m_program, vals, isCompositeOrFinal);
}

void ShaderPass::unbind() {
	if (glUseProgram) {
		glUseProgram(0);
	}
}
