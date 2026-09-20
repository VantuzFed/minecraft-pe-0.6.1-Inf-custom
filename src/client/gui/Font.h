#ifndef NET_MINECRAFT_CLIENT_GUI__Font_H__
#define NET_MINECRAFT_CLIENT_GUI__Font_H__

//package net.minecraft.client.gui;

#include <string>
#include <cctype>

#include "../renderer/gles.h"

class Textures;
class Options;

class Font
{
public:
    Font(Options* options, const std::string& name, Textures* textures);
	//Font(Options* options, const std::string& name, Textures* textures, int imgW, int imgH, int x, int y, int cols, int rows, unsigned char charOffset);
	
	void init(Options* options);
	void onGraphicsReset();

	void draw(const char* str, float x, float y, int color);
    void draw(const std::string& str, float x, float y, int color);
	void draw(const char* str, float x, float y, int color, bool darken);
    void draw(const std::string& str, float x, float y, int color, bool darken);
	void drawShadow(const std::string& str, float x, float y, int color);
	void drawShadow(const char* str, float x, float y, int color);
	void drawWordWrap(const std::string& str, float x, float y, float w, int col);

	int width(const std::string& str);
	int height(const std::string& str);

	static std::string sanitize(const std::string& str);

	// Decode one character at str[i] into a font cell 0..255.
	// Returns -1 for unsupported scripts (bytes are still consumed so
	// they never render as garbage). Advances i past consumed bytes.
	// Latin bytes pass through, D0/D1 lead bytes map Cyrillic into the
	// patched cells 0x80..0xC1, everything else is forbidden.
	static int utf8Cell(const std::string& str, unsigned int& i);
private:
	void buildChar(unsigned char i, float x = 0, float y = 0);
	void drawSlow(const std::string& str, float x, float y, int color, bool darken = false);
	void drawSlow(const char* str, float x, float y, int color, bool darken = false);
public:
	int fontTexture;
	int lineHeight;
	static const int DefaultLineHeight = 10;
private:
	int charWidths[256];
	float fcharWidths[256];
	int listPos;

	int index;
	int count;
	GLuint lists[1024];

	std::string fontName;
	Textures* _textures;

	Options* options;

	int _x, _y;
	int _cols;
	int _rows;
	unsigned char _charOffset;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__Font_H__*/
