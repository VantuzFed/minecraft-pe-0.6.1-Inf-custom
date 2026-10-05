#include "PngLoader.h"

#include <cstring>
#include <vector>

#if defined(PLATFORM_ANDROID) || defined(ANDROID)

#include <zlib.h>
#include <arpa/inet.h>

TextureData loadPngFromMemory(const unsigned char* data, size_t size) {
    return TextureData();
}

static void writeChunk(FILE* fp, const char* type, const unsigned char* data, uint32_t len) {
    uint32_t netLen = htonl(len);
    fwrite(&netLen, 1, 4, fp);
    fwrite(type, 1, 4, fp);
    uint32_t crc = crc32(0, (const Bytef*)type, 4);
    if (len > 0 && data) {
        fwrite(data, 1, len, fp);
        crc = crc32(crc, data, len);
    }
    uint32_t netCrc = htonl(crc);
    fwrite(&netCrc, 1, 4, fp);
}

bool savePngToFile(const std::string& filename, int width, int height, const unsigned char* rgbaPixels, bool flipY) {
#ifndef STANDALONE_SERVER
    if (!rgbaPixels || width <= 0 || height <= 0) return false;
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) return false;

    const unsigned char sig[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
    fwrite(sig, 1, 8, fp);

    unsigned char ihdr[13];
    uint32_t nw = htonl(width);
    uint32_t nh = htonl(height);
    memcpy(&ihdr[0], &nw, 4);
    memcpy(&ihdr[4], &nh, 4);
    ihdr[8] = 8;
    ihdr[9] = 6; // RGBA
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;
    writeChunk(fp, "IHDR", ihdr, 13);

    int stride = width * 4;
    size_t rawSize = (stride + 1) * height;
    std::vector<unsigned char> rawData(rawSize);
    for (int y = 0; y < height; ++y) {
        int srcY = flipY ? (height - 1 - y) : y;
        unsigned char* row = &rawData[y * (stride + 1)];
        row[0] = 0;
        memcpy(row + 1, rgbaPixels + srcY * stride, stride);
    }

    uLongf destLen = compressBound(rawSize);
    std::vector<unsigned char> compressedData(destLen);
    if (compress(compressedData.data(), &destLen, rawData.data(), rawSize) != Z_OK) {
        fclose(fp);
        return false;
    }

    writeChunk(fp, "IDAT", compressedData.data(), destLen);
    writeChunk(fp, "IEND", nullptr, 0);

    fclose(fp);
    return true;
#else
    return false;
#endif
}

#else

#include <png.h>

struct MemoryReader {
    const unsigned char* data;
    size_t size;
    size_t pos;
};

static void pngMemoryRead(png_structp pngPtr, png_bytep outBytes, png_size_t byteCountToRead) {
#ifndef STANDALONE_SERVER
    MemoryReader* reader = (MemoryReader*)png_get_io_ptr(pngPtr);
    if (!reader)
        return;

    if (reader->pos + byteCountToRead > reader->size) {
        png_error(pngPtr, "Read past end of buffer");
        return;
    }

    memcpy(outBytes, reader->data + reader->pos, byteCountToRead);
    reader->pos += byteCountToRead;
#endif
}

TextureData loadPngFromMemory(const unsigned char* data, size_t size) {
    TextureData out;
#ifndef STANDALONE_SERVER
    if (!data || size == 0) return out;

    png_structp pngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngPtr) return out;

    png_infop infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_read_struct(&pngPtr, NULL, NULL);
        return out;
    }

    if (setjmp(png_jmpbuf(pngPtr))) {
        png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
        return out;
    }

    MemoryReader reader;
    reader.data = data;
    reader.size = size;
    reader.pos = 0;

    png_set_read_fn(pngPtr, &reader, pngMemoryRead);
    png_read_info(pngPtr, infoPtr);

    // Convert any color type to 8-bit RGBA
    if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(pngPtr);
    if (png_get_color_type(pngPtr, infoPtr) == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(pngPtr, infoPtr) < 8)
        png_set_expand_gray_1_2_4_to_8(pngPtr);
    if (png_get_valid(pngPtr, infoPtr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(pngPtr);
    if (png_get_bit_depth(pngPtr, infoPtr) == 16)
        png_set_strip_16(pngPtr);

    // Ensure we always have RGBA (4 bytes per pixel)
    // Only add alpha if the image lacks it (e.g., RGB skin files).
    png_set_gray_to_rgb(pngPtr);

    // Handle interlaced PNGs properly
    int number_passes = png_set_interlace_handling(pngPtr);

    png_read_update_info(pngPtr, infoPtr);

    int colorType = png_get_color_type(pngPtr, infoPtr);
    if (colorType == PNG_COLOR_TYPE_RGB) {
        png_set_filler(pngPtr, 0xFF, PNG_FILLER_AFTER);
    }

    out.w = png_get_image_width(pngPtr, infoPtr);
    out.h = png_get_image_height(pngPtr, infoPtr);

    png_bytep* rowPtrs = new png_bytep[out.h];
    out.data = new unsigned char[4 * out.w * out.h];
    out.memoryHandledExternally = false;

    int rowStrideBytes = 4 * out.w;
    for (int i = 0; i < out.h; i++) {
        rowPtrs[i] = (png_bytep)&out.data[i*rowStrideBytes];
    }

    png_read_image(pngPtr, rowPtrs);

    png_destroy_read_struct(&pngPtr, &infoPtr, NULL);
    delete[] rowPtrs;
#endif

    return out;
}

bool savePngToFile(const std::string& filename, int width, int height, const unsigned char* rgbaPixels, bool flipY) {
#ifndef STANDALONE_SERVER
    if (!rgbaPixels || width <= 0 || height <= 0) return false;

    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) return false;

    png_structp pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngPtr) {
        fclose(fp);
        return false;
    }

    png_infop infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_write_struct(&pngPtr, NULL);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(pngPtr))) {
        png_destroy_write_struct(&pngPtr, &infoPtr);
        fclose(fp);
        return false;
    }

    png_init_io(pngPtr, fp);
    png_set_IHDR(pngPtr, infoPtr, width, height, 8,
                 PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(pngPtr, infoPtr);

    png_bytep* rowPtrs = new png_bytep[height];
    int stride = width * 4;
    for (int y = 0; y < height; ++y) {
        int srcY = flipY ? (height - 1 - y) : y;
        rowPtrs[y] = (png_bytep)(rgbaPixels + srcY * stride);
    }

    png_write_image(pngPtr, rowPtrs);
    png_write_end(pngPtr, NULL);

    png_destroy_write_struct(&pngPtr, &infoPtr);
    delete[] rowPtrs;
    fclose(fp);
    return true;
#else
    return false;
#endif
}

#endif
