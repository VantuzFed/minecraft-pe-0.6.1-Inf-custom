#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderFramebuffers_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderFramebuffers_H__

class ShaderFramebuffers {
public:
	ShaderFramebuffers();
	~ShaderFramebuffers();

	bool init(int width, int height);
	void destroy();
	void resize(int width, int height);

	void bindScene();
	void unbind();
	void bindTexturesForComposite();
	void drawFullscreenQuad();
	void blitToScreen();

	bool isReady() const { return m_ready; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }

	unsigned int getColorTex0() const { return m_colortex0; }
	unsigned int getColorTex1() const { return m_colortex1; }
	unsigned int getColorTex2() const { return m_colortex2; }
	unsigned int getDepthTex0() const { return m_depthtex0; }

private:
	int m_width;
	int m_height;
	bool m_ready;

	unsigned int m_mainFbo;
	unsigned int m_colortex0;
	unsigned int m_colortex1;
	unsigned int m_colortex2;
	unsigned int m_depthtex0;
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_ShaderFramebuffers_H__ */
