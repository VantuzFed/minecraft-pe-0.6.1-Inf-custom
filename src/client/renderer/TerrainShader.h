#ifndef NET_MINECRAFT_CLIENT_RENDERER__TerrainShader_H__
#define NET_MINECRAFT_CLIENT_RENDERER__TerrainShader_H__

class TerrainShader {
public:
	TerrainShader();
	~TerrainShader();

	bool init();
	void bind(int layer, float time, float camX, float camY, float camZ, float timeOfDay = 0.0f);
	void unbind();
	bool isAvailable() const;

private:
#if defined(PLATFORM_DESKTOP)
	unsigned int compileShader(unsigned int type, const char* source);

	bool m_initialized;
	bool m_failed;
	unsigned int m_program;
	int m_uLayer;
	int m_uTime;
	int m_uCamPos;
	int m_uTexture;
	int m_uTimeOfDay;
#endif
};

extern TerrainShader g_terrainShader;

#endif /* NET_MINECRAFT_CLIENT_RENDERER__TerrainShader_H__ */
