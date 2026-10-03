#ifndef PNGLOADER_H__
#define PNGLOADER_H__

#include "../client/renderer/TextureData.h"

#include <cstddef>
#include <string>

/// Decode a PNG (from memory) into a TextureData.
/// Returns an empty TextureData on failure.
TextureData loadPngFromMemory(const unsigned char* data, size_t size);

/// Encode an RGBA pixel buffer to a PNG file on disk.
bool savePngToFile(const std::string& filename, int width, int height, const unsigned char* rgbaPixels, bool flipY = true);

#endif // PNGLOADER_H__
