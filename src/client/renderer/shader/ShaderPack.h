#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPack_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPack_H__

#include <string>
#include <vector>
#include <unordered_set>

enum ShaderPackType {
	PACK_TYPE_NONE = 0,
	PACK_TYPE_BUILTIN,
	PACK_TYPE_DIRECTORY,
	PACK_TYPE_ZIP
};

class ShaderPack {
public:
	ShaderPack();
	~ShaderPack();

	bool open(const std::string& packNameOrPath);
	void close();

	bool hasFile(const std::string& path) const;
	std::string readTextFile(const std::string& path) const;

	ShaderPackType getType() const { return m_type; }
	const std::string& getName() const { return m_name; }
	const std::string& getShaderPrefix() const { return m_shaderPrefix; }

	static std::vector<std::string> scanAvailablePacks(const std::string& directory = "shaderpacks");

private:
	bool openDirectory(const std::string& dirPath);
	bool openZip(const std::string& zipPath);
	void findShaderPrefix();
	std::string resolvePath(const std::string& path) const;

	ShaderPackType m_type;
	std::string m_name;
	std::string m_basePath;
	std::string m_shaderPrefix;

	// For ZIP archives:
	void* m_zipHandle; // unzFile
	std::unordered_set<std::string> m_zipFiles;
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderPack_H__ */
