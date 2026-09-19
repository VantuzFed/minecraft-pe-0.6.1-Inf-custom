#ifndef NET_MINECRAFT_CLIENT_RENDERER__Textures_H__
#define NET_MINECRAFT_CLIENT_RENDERER__Textures_H__

//package net.minecraft.client.renderer;

#include <string>
#include <map>
#include <utility>
#include <vector>
#include "gles.h"
#include "TextureData.h"

class DynamicTexture;
class Options;
class AppPlatform;

typedef GLuint TextureId;
typedef std::map<std::string, TextureId> TextureMap;
typedef std::map<TextureId, TextureData> TextureImageMap;

//@todo: Should probably delete the data buffers with image data
//       after we've created an OpenGL-texture, and rewrite the
//       getTemporaryTextureData() to actually load from file IF
//       it's only read ~once anyway.
class Textures
{
public:
    Textures(Options* options_,  AppPlatform* platform_);
	~Textures();

	void addDynamicTexture(DynamicTexture* dynamicTexture);

	__inline void bind(TextureId id) {
		if (id != Textures::InvalidId && lastBoundTexture != id) {
			glBindTexture2(GL_TEXTURE_2D, id);
			lastBoundTexture = id;
			++textureChanges;
		} else if (id == Textures::InvalidId){
            LOGI("invalidId!\n");
        }
	}
	TextureId loadTexture(const std::string& resourceName, bool inTextureFolder = true);
	TextureId loadAndBindTexture(const std::string& resourceName);

	// Beta worlds use Beta 1.7.3 terrain sprites: red rose, grayscale
	// leaves and the beta grass-side overlay (PE keeps its own).
	// Applied lazily in tick() so it always runs with GL context current.
	void setBetaTerrain(bool beta);

    TextureId assignTexture(const std::string& resourceName, const TextureData& img);
	const TextureData* getTemporaryTextureData(TextureId id);

	int* loadTexturePixels(TextureId texId, const std::string& resourceName);

	void tick(bool uploadToGraphicsCard);

	void clear();
	void reloadAll();

	__inline static bool isTextureIdValid(TextureId t) { return t != Textures::InvalidId; }

private:
	int smoothBlend(int c0, int c1);
	int crispBlend(int c0, int c1);

public:
	static bool MIPMAP;
	static int textureChanges;
	static const TextureId InvalidId;

private:
	TextureMap idMap;
	TextureImageMap loadedImages;

	Options* options;
	AppPlatform* platform;

	bool clamp;
	bool blur;

	int lastBoundTexture;
	std::vector<DynamicTexture*> dynamicTextures;

	bool betaTerrain;
	bool pendingTerrainPatch;
	// PE originals of the patched tiles, saved on first patch.
	std::vector<unsigned char> origTile52;
	std::vector<unsigned char> origTile53;
	std::vector<unsigned char> origTile3;
	void applyBetaTerrainPatch();
	TextureData* getEditableTextureData(TextureId id);
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__Textures_H__*/
