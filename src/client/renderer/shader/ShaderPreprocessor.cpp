#include "ShaderPreprocessor.h"
#include <sstream>
#include <algorithm>

static std::string trim(const std::string& str) {
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

std::string ShaderPreprocessor::getDirectoryOfFile(const std::string& filePath) {
	size_t lastSlash = filePath.find_last_of("/\\");
	if (lastSlash != std::string::npos) {
		return filePath.substr(0, lastSlash + 1);
	}
	return "";
}

bool ShaderPreprocessor::process(const ShaderPack& pack,
                                 const std::string& entryFile,
                                 std::string& outGlsl,
                                 std::string& outError) {
	std::unordered_set<std::string> activeIncludes;
	std::string body;
	std::string detectedVersion;

	if (!processFile(pack, entryFile, body, detectedVersion, activeIncludes, 0, outError)) {
		return false;
	}

	if (detectedVersion.empty()) {
		detectedVersion = "#version 120";
	}

	std::ostringstream ss;
	ss << detectedVersion << "\n";
	// OptiFine / Iris compatibility defines
	ss << "#define MC_VERSION 10710\n"
	   << "#define MC_GL_VERSION 330\n"
	   << "#define MC_GLSL_VERSION 330\n"
	   << "#define MC_OS_LINUX\n"
	   << "#define MC_GL_VENDOR_GENERIC\n"
	   << "#define MC_GL_RENDERER_GENERIC\n"
	   << "#define MC_NORMAL_MAP\n"
	   << "#define MC_SPECULAR_MAP\n"
	   << "#line 1 0\n";
	ss << body;

	outGlsl = ss.str();
	return true;
}

bool ShaderPreprocessor::processFile(const ShaderPack& pack,
                                     const std::string& filePath,
                                     std::string& outGlsl,
                                     std::string& outVersion,
                                     std::unordered_set<std::string>& activeIncludes,
                                     int depth,
                                     std::string& outError) {
	if (depth > 32) {
		outError = "Include depth limit exceeded (> 32) in " + filePath;
		return false;
	}

	if (activeIncludes.find(filePath) != activeIncludes.end()) {
		outError = "Cyclic include detected: " + filePath;
		return false;
	}

	std::string rawContent = pack.readTextFile(filePath);
	if (rawContent.empty() && !pack.hasFile(filePath)) {
		outError = "File not found in shaderpack: " + filePath;
		return false;
	}

	activeIncludes.insert(filePath);
	std::string currentDir = getDirectoryOfFile(filePath);

	std::istringstream stream(rawContent);
	std::string line;
	std::ostringstream processedStream;
	bool inBlockComment = false;

	while (std::getline(stream, line)) {
		std::string trimmed = trim(line);

		// Handle block comments
		if (!inBlockComment && trimmed.find("/*") != std::string::npos) {
			size_t start = trimmed.find("/*");
			size_t end = trimmed.find("*/", start + 2);
			if (end == std::string::npos) {
				inBlockComment = true;
			}
		} else if (inBlockComment) {
			if (trimmed.find("*/") != std::string::npos) {
				inBlockComment = false;
			}
			processedStream << line << "\n";
			continue;
		}

		if (inBlockComment) {
			processedStream << line << "\n";
			continue;
		}

		// Check for #version
		if (trimmed.rfind("#version", 0) == 0) {
			if (depth == 0 && outVersion.empty()) {
				outVersion = trimmed;
			}
			// Strip version from output; main version is injected at the top
			processedStream << "// " << line << "\n";
			continue;
		}

		// Check for #include
		if (trimmed.rfind("#include", 0) == 0) {
			std::string incDirective = trim(trimmed.substr(8));
			if (incDirective.size() >= 2 &&
			    ((incDirective.front() == '"' && incDirective.back() == '"') ||
			     (incDirective.front() == '<' && incDirective.back() == '>'))) {
				std::string incPath = incDirective.substr(1, incDirective.size() - 2);
				std::string resolvedTarget;

				if (!incPath.empty() && (incPath[0] == '/' || incPath[0] == '\\')) {
					// Absolute inside pack shaders/
					resolvedTarget = incPath.substr(1);
				} else {
					// Relative to current file
					resolvedTarget = currentDir + incPath;
				}

				std::string subGlsl;
				std::string dummyVersion;
				if (!processFile(pack, resolvedTarget, subGlsl, dummyVersion, activeIncludes, depth + 1, outError)) {
					// If failed, try resolving from root as fallback
					if (!processFile(pack, incPath, subGlsl, dummyVersion, activeIncludes, depth + 1, outError)) {
						return false;
					}
				}
				processedStream << "// Begin include: " << incPath << "\n";
				processedStream << subGlsl << "\n";
				processedStream << "// End include: " << incPath << "\n";
				continue;
			}
		}

		processedStream << line << "\n";
	}

	activeIncludes.erase(filePath);
	outGlsl = processedStream.str();
	return true;
}
