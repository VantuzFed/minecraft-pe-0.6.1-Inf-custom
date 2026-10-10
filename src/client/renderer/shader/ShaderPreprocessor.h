#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPreprocessor_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPreprocessor_H__

#include <string>
#include <unordered_set>
#include "ShaderPack.h"

class ShaderPreprocessor {
public:
	static bool process(const ShaderPack& pack,
	                    const std::string& entryFile,
	                    std::string& outGlsl,
	                    std::string& outError);

private:
	static bool processFile(const ShaderPack& pack,
	                        const std::string& filePath,
	                        std::string& outGlsl,
	                        std::string& outVersion,
	                        std::unordered_set<std::string>& activeIncludes,
	                        int depth,
	                        std::string& outError);

	static std::string getDirectoryOfFile(const std::string& filePath);
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPreprocessor_H__ */
