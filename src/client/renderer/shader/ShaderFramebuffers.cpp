#include "ShaderFramebuffers.h"
#include "../gles.h"
#include <cstdio>

ShaderFramebuffers::ShaderFramebuffers()
	: m_width(0),
	  m_height(0),
	  m_ready(false),
	  m_mainFbo(0),
	  m_colortex0(0),
	  m_colortex1(0),
	  m_colortex2(0),
	  m_depthtex0(0)
{}

ShaderFramebuffers::~ShaderFramebuffers() {
	destroy();
}

void ShaderFramebuffers::destroy() {
	if (m_mainFbo != 0 && glDeleteFramebuffers) {
		glDeleteFramebuffers(1, &m_mainFbo);
		m_mainFbo = 0;
	}
	if (m_colortex0 != 0) { glDeleteTextures(1, &m_colortex0); m_colortex0 = 0; }
	if (m_colortex1 != 0) { glDeleteTextures(1, &m_colortex1); m_colortex1 = 0; }
	if (m_colortex2 != 0) { glDeleteTextures(1, &m_colortex2); m_colortex2 = 0; }
	if (m_depthtex0 != 0) { glDeleteTextures(1, &m_depthtex0); m_depthtex0 = 0; }
	m_ready = false;
}

bool ShaderFramebuffers::init(int width, int height) {
	if (width <= 0 || height <= 0) return false;
	if (!glGenFramebuffers || !glBindFramebuffer || !glFramebufferTexture2D || !glDrawBuffers) {
		printf("[ShaderFBO] Framebuffer extensions not available on this context\n");
		return false;
	}

	destroy();
	m_width = width;
	m_height = height;

	glGenFramebuffers(1, &m_mainFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_mainFbo);

	// colortex0 (Main diffuse / color)
	glGenTextures(1, &m_colortex0);
	glBindTexture(GL_TEXTURE_2D, m_colortex0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colortex0, 0);

	// colortex1 (Normals / specular / light levels)
	glGenTextures(1, &m_colortex1);
	glBindTexture(GL_TEXTURE_2D, m_colortex1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_colortex1, 0);

	// colortex2 (Extra data / material IDs)
	glGenTextures(1, &m_colortex2);
	glBindTexture(GL_TEXTURE_2D, m_colortex2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_colortex2, 0);

	// depthtex0 (Scene Depth Buffer)
	glGenTextures(1, &m_depthtex0);
	glBindTexture(GL_TEXTURE_2D, m_depthtex0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthtex0, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		printf("[ShaderFBO] Framebuffer incomplete! Status = 0x%x\n", status);
		destroy();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	m_ready = true;
	printf("[ShaderFBO] Initialized MRT Framebuffer (%dx%d) successfully\n", m_width, m_height);
	return true;
}

void ShaderFramebuffers::resize(int width, int height) {
	if (width <= 0 || height <= 0) return;
	if (width == m_width && height == m_height && m_ready) return;
	init(width, height);
}

void ShaderFramebuffers::bindScene() {
	if (!m_ready || !glBindFramebuffer) return;
	glBindFramebuffer(GL_FRAMEBUFFER, m_mainFbo);
	glViewport(0, 0, m_width, m_height);

	GLenum bufs[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, bufs);
}

void ShaderFramebuffers::unbind() {
	if (glBindFramebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void ShaderFramebuffers::bindTexturesForComposite() {
	if (!m_ready) return;

	if (glActiveTexture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_colortex0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, m_colortex1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_colortex2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, m_depthtex0);

		glActiveTexture(GL_TEXTURE0);
	}
}

void ShaderFramebuffers::drawFullscreenQuad() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
	glEnd();

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void ShaderFramebuffers::blitToScreen() {
	if (!m_ready || !glBindFramebuffer) return;

	if (glBlitFramebuffer) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mainFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_colortex0);
		drawFullscreenQuad();
	}
}
