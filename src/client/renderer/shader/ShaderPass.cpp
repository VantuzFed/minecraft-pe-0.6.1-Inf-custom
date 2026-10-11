#include "ShaderPass.h"
#include "ShaderPack.h"
#include "ShaderPreprocessor.h"
#include "../gles.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_set>

ShaderPass::ShaderPass()
	: m_program(0),
	  m_valid(false)
{}

ShaderPass::~ShaderPass() {
	// Do not delete GL resources in destructor; context is already destroyed at process exit.
}

void ShaderPass::destroy() {
	if (m_program != 0 && glDeleteProgram) {
		glDeleteProgram(m_program);
		m_program = 0;
	}
	m_valid = false;
	m_name = "";
	m_drawBuffers.clear();
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
		char log[2048];
		GLsizei len = 0;
		glGetShaderInfoLog(shader, sizeof(log), &len, log);
		printf("[ShaderPass %s] Compilation failed (%s):\n%s\n",
		       m_name.c_str(),
		       (type == GL_VERTEX_SHADER ? "vertex" : "fragment"),
		       log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static std::vector<GLenum> parseDrawBuffers(const char* fsSrc) {
	std::vector<GLenum> buffers;
	if (!fsSrc) return buffers;

	std::istringstream stream(fsSrc);
	std::string line;
	std::vector<bool> conditionStack;
	std::unordered_set<std::string> defines;
	defines.insert("MC_VERSION");
	defines.insert("IS_IRIS");
	defines.insert("MC_NORMAL_MAP");
	defines.insert("MC_SPECULAR_MAP");

	std::string matchedDirective;

	while (std::getline(stream, line)) {
		size_t first = line.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) continue;
		std::string trimmed = line.substr(first);

		if (trimmed.rfind("#define", 0) == 0) {
			std::string rest = trimmed.substr(7);
			size_t symStart = rest.find_first_not_of(" \t");
			if (symStart != std::string::npos) {
				size_t symEnd = rest.find_first_of(" \t\r\n", symStart);
				std::string sym = rest.substr(symStart, (symEnd == std::string::npos ? rest.size() - symStart : symEnd - symStart));
				defines.insert(sym);
			}
			continue;
		} else if (trimmed.rfind("#undef", 0) == 0) {
			std::string rest = trimmed.substr(6);
			size_t symStart = rest.find_first_not_of(" \t");
			if (symStart != std::string::npos) {
				size_t symEnd = rest.find_first_of(" \t\r\n", symStart);
				std::string sym = rest.substr(symStart, (symEnd == std::string::npos ? rest.size() - symStart : symEnd - symStart));
				defines.erase(sym);
			}
			continue;
		} else if (trimmed.rfind("#ifdef", 0) == 0) {
			std::string rest = trimmed.substr(6);
			size_t symStart = rest.find_first_not_of(" \t");
			bool cond = false;
			if (symStart != std::string::npos) {
				size_t symEnd = rest.find_first_of(" \t\r\n", symStart);
				std::string sym = rest.substr(symStart, (symEnd == std::string::npos ? rest.size() - symStart : symEnd - symStart));
				cond = (defines.find(sym) != defines.end());
			}
			bool parentActive = conditionStack.empty() || conditionStack.back();
			conditionStack.push_back(parentActive && cond);
			continue;
		} else if (trimmed.rfind("#ifndef", 0) == 0) {
			std::string rest = trimmed.substr(7);
			size_t symStart = rest.find_first_not_of(" \t");
			bool cond = false;
			if (symStart != std::string::npos) {
				size_t symEnd = rest.find_first_of(" \t\r\n", symStart);
				std::string sym = rest.substr(symStart, (symEnd == std::string::npos ? rest.size() - symStart : symEnd - symStart));
				cond = (defines.find(sym) == defines.end());
			}
			bool parentActive = conditionStack.empty() || conditionStack.back();
			conditionStack.push_back(parentActive && cond);
			continue;
		} else if (trimmed.rfind("#if", 0) == 0 && trimmed.rfind("#ifdef", 0) != 0 && trimmed.rfind("#ifndef", 0) != 0) {
			bool cond = false;
			if (trimmed.find("MC_VERSION") != std::string::npos) {
				if (trimmed.find("<") != std::string::npos) cond = true;
				else if (trimmed.find(">") != std::string::npos) cond = false;
			}
			bool parentActive = conditionStack.empty() || conditionStack.back();
			conditionStack.push_back(parentActive && cond);
			continue;
		} else if (trimmed.rfind("#else", 0) == 0) {
			if (!conditionStack.empty()) {
				bool current = conditionStack.back();
				conditionStack.pop_back();
				bool parentActive = conditionStack.empty() || conditionStack.back();
				conditionStack.push_back(parentActive && !current);
			}
			continue;
		} else if (trimmed.rfind("#endif", 0) == 0) {
			if (!conditionStack.empty()) {
				conditionStack.pop_back();
			}
			continue;
		}

		bool isActive = conditionStack.empty() || conditionStack.back();
		if (!isActive) continue;

		size_t dbPos = trimmed.find("DRAWBUFFERS");
		if (dbPos != std::string::npos) {
			size_t colon = trimmed.find(':', dbPos);
			if (colon != std::string::npos) {
				size_t endComment = trimmed.find("*/", colon);
				if (endComment != std::string::npos) {
					matchedDirective = trimmed.substr(colon + 1, endComment - (colon + 1));
				}
			}
		}
	}

	for (char c : matchedDirective) {
		if (c >= '0' && c <= '7') {
			buffers.push_back(GL_COLOR_ATTACHMENT0 + (c - '0'));
		} else if (c == 'N' || c == 'n') {
			buffers.push_back(GL_NONE);
		}
	}

	return buffers;
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
		char log[2048];
		GLsizei len = 0;
		glGetProgramInfoLog(m_program, sizeof(log), &len, log);
		printf("[ShaderPass %s] Link failed:\n%s\n", m_name.c_str(), log);
		destroy();
		return false;
	}

	m_uniforms.findLocations(m_program);
	m_drawBuffers = parseDrawBuffers(fsSrc);
	if (!m_drawBuffers.empty()) {
		printf("[ShaderPass %s] Parsed DRAWBUFFERS (count = %zu)\n", m_name.c_str(), m_drawBuffers.size());
	}
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

bool ShaderPass::loadFromPack(const ShaderPack& pack, const std::string& baseName) {
	std::vector<std::string> candidatePrefixes;
	if (baseName.find("world") == std::string::npos) {
		candidatePrefixes.push_back("world0/" + baseName);
	}
	candidatePrefixes.push_back(baseName);

	for (const auto& target : candidatePrefixes) {
		std::string vsFile = target + ".vsh";
		std::string fsFile = target + ".fsh";

		if (!pack.hasFile(fsFile)) {
			continue;
		}

		std::string vsProcessed, fsProcessed, err;

		if (pack.hasFile(vsFile)) {
			if (!ShaderPreprocessor::process(pack, vsFile, vsProcessed, err)) {
				printf("[ShaderPass] Preprocess error in %s: %s\n", vsFile.c_str(), err.c_str());
				continue;
			}
		} else if (baseName.rfind("composite", 0) == 0 || baseName.rfind("deferred", 0) == 0 || baseName == "final") {
			// Fallback to default fullscreen quad vertex shader (OptiFine/Iris standard)
			vsProcessed =
				"#version 120\n"
				"varying vec2 v_texCoord;\n"
				"void main() {\n"
				"    v_texCoord = gl_MultiTexCoord0.xy;\n"
				"    gl_Position = vec4(gl_Vertex.xy * 2.0 - 1.0, 0.0, 1.0);\n"
				"}\n";
		} else {
			continue;
		}

		if (!ShaderPreprocessor::process(pack, fsFile, fsProcessed, err)) {
			printf("[ShaderPass] Preprocess error in %s: %s\n", fsFile.c_str(), err.c_str());
			continue;
		}

		if (loadFromSource(vsProcessed.c_str(), fsProcessed.c_str(), baseName)) {
			return true;
		}
	}

	return false;
}

bool ShaderPass::loadWithFallback(const ShaderPack& pack, const std::vector<std::string>& candidates) {
	for (const auto& candidate : candidates) {
		if (loadFromPack(pack, candidate)) {
			printf("[ShaderPass] Successfully loaded pass '%s' (for target '%s')\n",
			       candidate.c_str(), candidates.front().c_str());
			return true;
		}
	}
	return false;
}

void ShaderPass::bind(const ShaderUniformValues& vals, bool isCompositeOrFinal) {
	if (!m_valid || m_program == 0 || !glUseProgram) return;
	glUseProgram(m_program);
	m_uniforms.apply(m_program, vals, isCompositeOrFinal);

	GLint currentFbo = 0;
	if (glGetIntegerv) {
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);
	}

	if (currentFbo != 0 && glDrawBuffers) {
		if (!m_drawBuffers.empty()) {
			glDrawBuffers((GLsizei)m_drawBuffers.size(), m_drawBuffers.data());
		} else if (isCompositeOrFinal) {
			GLenum buf = GL_COLOR_ATTACHMENT0;
			glDrawBuffers(1, &buf);
		}
	}
}

void ShaderPass::unbind() {
	if (glUseProgram) {
		glUseProgram(0);
	}
}
