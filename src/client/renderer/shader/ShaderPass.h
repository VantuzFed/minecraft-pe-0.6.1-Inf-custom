#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPass_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPass_H__

#include <string>
#include "ShaderUniforms.h"

class ShaderPass {
public:
	ShaderPass();
	~ShaderPass();

	bool loadFromSource(const char* vsSrc, const char* fsSrc, const std::string& passName = "");
	bool loadFromFiles(const std::string& vsPath, const std::string& fsPath);
	void destroy();

	void bind(const ShaderUniformValues& vals, bool isCompositeOrFinal);
	void unbind();

	bool isValid() const { return m_valid; }
	unsigned int getProgram() const { return m_program; }
	const std::string& getName() const { return m_name; }

private:
	unsigned int compileShader(unsigned int type, const char* source);

	std::string m_name;
	unsigned int m_program;
	bool m_valid;
	ShaderUniformLocations m_uniforms;
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPass_H__ */
