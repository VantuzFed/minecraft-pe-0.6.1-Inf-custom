#include "ShaderPack.h"
#include "../../compat/minizip/unzip.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>

static std::string normalizeSeparators(const std::string& path) {
	std::string s = path;
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '\\') s[i] = '/';
	}
	return s;
}

static bool endsWith(const std::string& str, const std::string& suffix) {
	if (str.length() < suffix.length()) return false;
	return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

static void normalizeNewlines(std::string& content) {
	size_t pos = 0;
	while ((pos = content.find("\r\n", pos)) != std::string::npos) {
		content.replace(pos, 2, "\n");
		pos += 1;
	}
	std::replace(content.begin(), content.end(), '\r', '\n');
}

ShaderPack::ShaderPack()
	: m_type(PACK_TYPE_NONE),
	  m_name(""),
	  m_basePath(""),
	  m_shaderPrefix(""),
	  m_zipHandle(NULL)
{}

ShaderPack::~ShaderPack() {
	close();
}

void ShaderPack::close() {
	if (m_zipHandle != NULL) {
		unzClose((unzFile)m_zipHandle);
		m_zipHandle = NULL;
	}
	m_zipFiles.clear();
	m_type = PACK_TYPE_NONE;
	m_name = "";
	m_basePath = "";
	m_shaderPrefix = "";
}

bool ShaderPack::open(const std::string& packNameOrPath) {
	close();

	if (packNameOrPath.empty() || packNameOrPath == "(OFF)" || packNameOrPath == "OFF") {
		m_name = "(OFF)";
		m_type = PACK_TYPE_NONE;
		return true;
	}

	if (packNameOrPath == "Built-in" || packNameOrPath == "(internal)" || packNameOrPath == "internal") {
		m_name = "Built-in";
		m_type = PACK_TYPE_BUILTIN;
		return true;
	}

	std::string fullPath = packNameOrPath;
	struct stat st;
	if (stat(fullPath.c_str(), &st) != 0) {
		std::string testPath = "shaderpacks/" + packNameOrPath;
		if (stat(testPath.c_str(), &st) == 0) {
			fullPath = testPath;
		} else {
			std::string buildPath = "build/shaderpacks/" + packNameOrPath;
			if (stat(buildPath.c_str(), &st) == 0) {
				fullPath = buildPath;
			}
		}
	}

	if (stat(fullPath.c_str(), &st) != 0) {
		printf("[ShaderPack] Could not find pack at '%s'\n", packNameOrPath.c_str());
		return false;
	}

	m_name = packNameOrPath;
	// Extract simple name if full path was given
	size_t slash = m_name.find_last_of("/\\");
	if (slash != std::string::npos) {
		m_name = m_name.substr(slash + 1);
	}

	if (S_ISDIR(st.st_mode)) {
		return openDirectory(fullPath);
	} else if (endsWith(fullPath, ".zip")) {
		return openZip(fullPath);
	}

	return false;
}

bool ShaderPack::openDirectory(const std::string& dirPath) {
	m_basePath = normalizeSeparators(dirPath);
	if (!m_basePath.empty() && m_basePath.back() != '/') {
		m_basePath += '/';
	}
	m_type = PACK_TYPE_DIRECTORY;
	findShaderPrefix();
	printf("[ShaderPack] Opened directory pack '%s', prefix='%s'\n", m_name.c_str(), m_shaderPrefix.c_str());
	return true;
}

bool ShaderPack::openZip(const std::string& zipPath) {
	unzFile uf = unzOpen(zipPath.c_str());
	if (!uf) {
		printf("[ShaderPack] Failed to open zip file '%s'\n", zipPath.c_str());
		return false;
	}

	m_zipHandle = uf;
	m_basePath = zipPath;
	m_type = PACK_TYPE_ZIP;

	// Index all files in ZIP
	if (unzGoToFirstFile(uf) == UNZ_OK) {
		do {
			char fn[512];
			unz_file_info file_info;
			if (unzGetCurrentFileInfo(uf, &file_info, fn, sizeof(fn), NULL, 0, NULL, 0) == UNZ_OK) {
				std::string normalized = normalizeSeparators(fn);
				while (!normalized.empty() && (normalized[0] == '/' || normalized[0] == '\\')) {
					normalized.erase(0, 1);
				}
				m_zipFiles.insert(normalized);
			}
		} while (unzGoToNextFile(uf) == UNZ_OK);
	}

	findShaderPrefix();
	printf("[ShaderPack] Opened zip pack '%s' (%zu entries), prefix='%s'\n",
	       m_name.c_str(), m_zipFiles.size(), m_shaderPrefix.c_str());
	return true;
}

void ShaderPack::findShaderPrefix() {
	m_shaderPrefix = "";

	if (m_type == PACK_TYPE_DIRECTORY) {
		struct stat st;
		std::string test = m_basePath + "shaders/";
		if (stat(test.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
			m_shaderPrefix = "shaders/";
			return;
		}

		// Check subfolder with shaders/
		DIR* dir = opendir(m_basePath.c_str());
		if (dir) {
			struct dirent* entry;
			while ((entry = readdir(dir)) != NULL) {
				if (entry->d_name[0] == '.') continue;
				std::string sub = m_basePath + entry->d_name + "/shaders/";
				if (stat(sub.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
					m_shaderPrefix = std::string(entry->d_name) + "/shaders/";
					break;
				}
			}
			closedir(dir);
			if (!m_shaderPrefix.empty()) return;
		}

		// Flat root in directory
		m_shaderPrefix = "";
	} else if (m_type == PACK_TYPE_ZIP) {
		// Look for any file in shaders/
		for (const auto& f : m_zipFiles) {
			size_t idx = f.find("shaders/");
			if (idx != std::string::npos) {
				m_shaderPrefix = f.substr(0, idx + 8);
				return;
			}
		}
		// Look for world0/
		for (const auto& f : m_zipFiles) {
			size_t idx = f.find("world0/");
			if (idx != std::string::npos) {
				m_shaderPrefix = f.substr(0, idx);
				return;
			}
		}
		// Flat root in zip
		m_shaderPrefix = "";
	}
}

std::string ShaderPack::resolvePath(const std::string& path) const {
	std::string clean = normalizeSeparators(path);
	while (!clean.empty() && (clean[0] == '/' || clean[0] == '\\')) {
		clean.erase(0, 1);
	}
	// If path explicitly starts with shaders/, don't double prefix
	if (clean.rfind("shaders/", 0) == 0 && m_shaderPrefix.rfind("shaders/", 0) == 0) {
		return clean;
	}
	return m_shaderPrefix + clean;
}

bool ShaderPack::hasFile(const std::string& path) const {
	std::string fullRel = resolvePath(path);

	if (m_type == PACK_TYPE_DIRECTORY) {
		std::string absPath = m_basePath + fullRel;
		struct stat st;
		return (stat(absPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode));
	} else if (m_type == PACK_TYPE_ZIP) {
		return (m_zipFiles.find(fullRel) != m_zipFiles.end());
	}
	return false;
}

std::string ShaderPack::readTextFile(const std::string& path) const {
	std::string fullRel = resolvePath(path);

	if (m_type == PACK_TYPE_DIRECTORY) {
		std::string absPath = m_basePath + fullRel;
		std::ifstream file(absPath, std::ios::in | std::ios::binary);
		if (!file.is_open()) return "";
		std::stringstream ss;
		ss << file.rdbuf();
		std::string content = ss.str();
		normalizeNewlines(content);
		return content;
	} else if (m_type == PACK_TYPE_ZIP && m_zipHandle != NULL) {
		unzFile uf = (unzFile)m_zipHandle;
		if (unzLocateFile(uf, fullRel.c_str(), 1) != UNZ_OK) {
			return "";
		}
		if (unzOpenCurrentFile(uf) != UNZ_OK) {
			return "";
		}

		unz_file_info file_info;
		unzGetCurrentFileInfo(uf, &file_info, NULL, 0, NULL, 0, NULL, 0);
		std::string buffer;
		buffer.resize(file_info.uncompressed_size);

		int readBytes = unzReadCurrentFile(uf, &buffer[0], (unsigned int)file_info.uncompressed_size);
		unzCloseCurrentFile(uf);

		if (readBytes > 0) {
			buffer.resize(readBytes);
			normalizeNewlines(buffer);
			return buffer;
		}
	}

	return "";
}

std::vector<std::string> ShaderPack::scanAvailablePacks(const std::string& directory) {
	std::vector<std::string> packs;
	auto scanDir = [&](const std::string& dirPath) {
		DIR* dir = opendir(dirPath.c_str());
		if (!dir) return;

		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_name[0] == '.') continue;
			std::string name = entry->d_name;
			std::string fullPath = dirPath + "/" + name;
			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0) {
				if (S_ISDIR(st.st_mode) || endsWith(name, ".zip")) {
					if (std::find(packs.begin(), packs.end(), name) == packs.end()) {
						packs.push_back(name);
					}
				}
			}
		}
		closedir(dir);
	};

	scanDir(directory);
	if (directory == "shaderpacks") {
		scanDir("build/shaderpacks");
	}
	std::sort(packs.begin(), packs.end());
	return packs;
}
