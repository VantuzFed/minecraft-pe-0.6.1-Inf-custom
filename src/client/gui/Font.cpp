#include "Font.h"

//#include "SharedConstants.h"
#include "../Options.h"
#include "../renderer/Textures.h"
#include "../renderer/Tesselator.h"
#include "../../util/Mth.h"
#include <cstring>

Font::Font( Options* options, const std::string& name, Textures* textures )
:	options(options),
	fontTexture(0),
	fontName(name),
	index(0),
	count(0),
	_textures(textures),
	_x(0), _y(0),
	_cols(16), _rows(16),
	_charOffset(0),
	lineHeight(DefaultLineHeight)
{
	init(options);
}


//Font::Font( Options* options, const std::string& name, Textures* textures, int imgW, int imgH, int x, int y, int cols, int rows, unsigned char charOffset )
//:	options(options),
//	fontTexture(0),
//	fontName(name),
//	index(0),
//	count(0),
//	_textures(textures),
//	_x(x), _y(y),
//	_cols(cols), _rows(rows),
//	_charOffset(charOffset)
//{
//	init(options);
//}

void Font::onGraphicsReset()
{
	init(options);
}

void Font::init( Options* options )
{
	TextureId fontTexture = _textures->loadTexture(fontName);
	const TextureData* tex = _textures->getTemporaryTextureData(fontTexture);

	if (!tex)
		return;

	unsigned char* rawPixels = tex->data;

	const int numChars = _rows * _cols;
	for (int i = 0; i < numChars; i++) {
		int xt = i % _cols;
		int yt = i / _cols;

		int x = 7;
		for (; x >= 0; x--) {
			int xPixel = _x + xt * 8 + x;
			bool emptyColumn = true;
			for (int y = 0; y < 8 && emptyColumn; y++) {
				int yPixel = _y + (yt * 8 + y) * tex->w;
				unsigned char pixelalpha = rawPixels[(xPixel + yPixel) << 2];
				if (pixelalpha > 0) emptyColumn = false;
			}
			if (!emptyColumn) {
				break;
			}
		}

		if (i == ' ') x = 4 - 2;
		charWidths[i] = x + 2;
		fcharWidths[i] = (float) charWidths[i];
	}

#ifdef USE_VBO
	return; // this <1
#endif

#ifndef USE_VBO
	listPos = glGenLists(256 + 32);

	Tesselator& t = Tesselator::instance;
	for (int i = 0; i < 256; i++) {
		glNewList(listPos + i, GL_COMPILE);
		// @attn @huge @note: This is some dangerous code right here / Aron, added ^1
		t.begin();
		buildChar(i);
		t.end(false, -1);

		glTranslatef2((GLfloat)charWidths[i], 0.0f, 0.0f);
		glEndList();
	}

	for (int i = 0; i < 32; i++) {
		int br = ((i >> 3) & 1) * 0x55;
		int r = ((i >> 2) & 1) * 0xaa + br;
		int g = ((i >> 1) & 1) * 0xaa + br;
		int b = ((i >> 0) & 1) * 0xaa + br;
		if (i == 6) {
			r += 0x55;
		}
		bool darken = i >= 16;

		if (options->anaglyph3d) {
			int cr = (r * 30 + g * 59 + b * 11) / 100;
			int cg = (r * 30 + g * 70) / (100);
			int cb = (r * 30 + b * 70) / (100);

			r = cr;
			g = cg;
			b = cb;
		}

		// color = r << 16 | g << 8 | b;
		if (darken) {
			r /= 4;
			g /= 4;
			b /= 4;
		}

		glNewList(listPos + 256 + i, GL_COMPILE);
		glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
		glEndList();
	}
#endif
}

int Font::utf8Cell( const std::string& str, unsigned int& i )
{
	unsigned char c0 = (unsigned char)str[i];
	if (c0 < 0x80)
		return c0;
	if (i + 1 >= str.length())
		return -1;
	unsigned char c1 = (unsigned char)str[i + 1];
	if (c0 == 0xD0) {
		if (c1 >= 0x90 && c1 <= 0xAF) { i += 1; return 0x80 + (c1 - 0x90); } // А-Я
		if (c1 >= 0xB0 && c1 <= 0xBF) { i += 1; return 0xA0 + (c1 - 0xB0); } // а-п
		if (c1 == 0x81) { i += 1; return 0xC0; } // Ё
	} else if (c0 == 0xD1) {
		if (c1 >= 0x80 && c1 <= 0x8F) { i += 1; return 0xB0 + (c1 - 0x80); } // р-я
		if (c1 == 0x91) { i += 1; return 0xC1; } // ё
	}
	// Forbidden script or malformed sequence: consume nothing, the caller
	// loop advances past this byte and the trail byte is skipped the same
	// way (it is >= 0x80 too), so no garbage is ever rendered.
	return -1;
}

void Font::drawShadow( const std::string& str, float x, float y, int color )
{
	draw(str, x + 1, y + 1, color, true);
	draw(str, x, y, color);
}
void Font::drawShadow( const char* str, float x, float y, int color )
{
	draw(str, x + 1, y + 1, color, true);
	draw(str, x, y, color);
}

void Font::draw( const std::string& str, float x, float y, int color )
{
	draw(str, x, y, color, false);
}

void Font::draw( const char* str, float x, float y, int color )
{
	draw(str, x, y, color, false);
}

void Font::draw( const char* str, float x, float y, int color, bool darken )
{
#ifdef USE_VBO
	drawSlow(str, x, y, color, darken);
#endif
}

void Font::draw( const std::string& str, float x, float y, int color, bool darken )
{
#ifdef USE_VBO
	drawSlow(str, x, y, color, darken);
	return;
#endif

	if (str.empty()) return;

	if (darken) {
		int oldAlpha = color & 0xff000000;
		color = (color & 0xfcfcfc) >> 2;
		color += oldAlpha;
	}

	_textures->loadAndBindTexture(fontName);
	float r = ((color >> 16) & 0xff) / 255.0f;
	float g = ((color >> 8) & 0xff) / 255.0f;
	float b = ((color) & 0xff) / 255.0f;
	float a = ((color >> 24) & 0xff) / 255.0f;
	if (a == 0) a = 1;
	glColor4f2(r, g, b, a);

	static const std::string hex("0123456789abcdef");

	index = 0;
	glPushMatrix2();
	glTranslatef2((GLfloat)x, (GLfloat)y, 0.0f);
	for (unsigned int i = 0; i < str.length(); i++) {
		// Decode multibyte characters first: a UTF-8 trail byte may
		// numerically equal '\xa7' (e.g. Ч = D0 A7) and must never be
		// mistaken for a color code.
		if ((unsigned char)str[i] >= 0x80) {
			int cell = utf8Cell(str, i);
			if (cell >= 0)
				lists[index++] = listPos + cell;

			if (index == 1024) {
				count = index;
				index = 0;
#ifndef USE_VBO
				glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
				count = 1024;
			}
		} else {
			while (str.length() > i + 1 && str[i] == '\xa7') {
				int cc = hex.find((char)tolower(str[i + 1]));
				if (cc < 0 || cc > 15) cc = 15;
				lists[index++] = listPos + 256 + cc + (darken ? 16 : 0);

				if (index == 1024) {
					count = index;
					index = 0;
#ifndef USE_VBO
					glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
					count = 1024;
				}

				i += 2;
			}

			if (i < str.length()) {
				//int ch = SharedConstants.acceptableLetters.indexOf(str.charAt(i));
				char ch = str[i];
				if (ch >= 0) {
					//ib.put(listPos + ch + 32);
					lists[index++] = listPos + ch;
				}
			}
		}

		if (index == 1024) {
			count = index;
			index = 0;
#ifndef USE_VBO
			glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
			count = 1024;
		}
	}
	count = index;
	index = 0;
#ifndef USE_VBO
	glCallLists(count, GL_UNSIGNED_INT, lists);
#endif
	glPopMatrix2();
}

int Font::width( const std::string& str )
{
	int maxLen = 0;
	int len = 0;

	for (unsigned int i = 0; i < str.length(); i++) {
		if ((unsigned char)str[i] >= 0x80) {
			// Multibyte character: measure the mapped cell, never the
			// raw bytes (a trail byte like A7 in Ч must not count).
			int cell = utf8Cell(str, i);
			if (cell >= 0)
				len += charWidths[cell];
		} else if (str[i] == '\xa7') {
			i++;
		} else {
			//int ch = SharedConstants.acceptableLetters.indexOf(str.charAt(i));
			//if (ch >= 0) {
			//    len += charWidths[ch + 32];
			//}
			if (str[i] == '\n') {
				if (len > maxLen) maxLen = len;
				len = 0;
			}
			else {
				int charWidth = charWidths[ (unsigned char) str[i] ];
				len += charWidth;
			}
		}
	}
	return maxLen>len? maxLen : len;
}

int Font::height( const std::string& str ) {
	int h = 0;
	bool hasLine = false;
	for (unsigned int i = 0; i < str.length(); ++i) {
		if (str[i] == '\n') hasLine = true;
		else {
			if (hasLine) h += lineHeight;
			hasLine = false;
		}
	}
	return h;
}

std::string Font::sanitize( const std::string& str )
{
	std::string sanitized(str.length() + 1, 0);
	int j = 0;

	for (unsigned int i = 0; i < str.length(); i++) {
		if ((unsigned char)str[i] >= 0x80) {
			// Multibyte character: copy whole so a trail byte like A7
			// in Ч is never mistaken for a § color code.
			unsigned char lead = str[i];
			int cell = utf8Cell(str, i);
			if (cell >= 0) {
				// utf8Cell leaves i on the trail byte; copy both bytes.
				sanitized[j++] = (char)lead;
				sanitized[j++] = str[i];
			}
			// Unsupported script: drop the lead byte, the loop advance
			// drops the trail byte right after.
		} else if (str[i] == '\xa7') {
			i++;
			//} else if (SharedConstants.acceptableLetters.indexOf(str.charAt(i)) >= 0) {
		} else {
			sanitized[j++] = str[i];
		}
	}
	return sanitized.erase(j);
}

void Font::drawWordWrap( const std::string& str, float x, float y, float w, int col )
{
	char* cstr = new char[str.length() + 1];
	strncpy(cstr, str.c_str(), str.length());
	cstr[str.length()] = 0;

	const char* lims = " \n\t\r";
	char* ptok = strtok(cstr, lims);

	std::vector<std::string> words;
	while (ptok != NULL) {
		words.push_back( ptok );
		ptok = strtok(NULL, lims);
	}

	delete[] cstr;

	int pos = 0;
	while (pos < (int)words.size()) {
		std::string line = words[pos++] + " ";
		while (pos < (int)words.size() && width(line + words[pos]) < w) {
			line += words[pos++] + " ";
		}
		drawShadow(line, x, y, col);
		y += lineHeight;
	}
}

void Font::drawSlow( const std::string& str, float x, float y, int color, bool darken /*= false*/ ) {
	drawSlow(str.c_str(), x, y, color, darken);
}
void Font::drawSlow( const char* str, float x, float y, int color, bool darken /*= false*/ )
{
	if (!str) return;

	if (darken) {
		int oldAlpha = color & 0xff000000;
		color = (color & 0xfcfcfc) >> 2;
		color += oldAlpha;
	}

	_textures->loadAndBindTexture(fontName);

	Tesselator& t = Tesselator::instance;
	t.begin();
	int alpha = (0xff000000 & color) >> 24;
	if (!alpha) alpha = 0xff;
	t.color((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff, alpha);
	
	t.addOffset((float)x, (float)y, 0);
	float xOffset = 0;
	float yOffset = 0;

	while (*str) {
		unsigned char ch = (unsigned char)*str;
		if (ch == '\n') {
			xOffset = 0;
			yOffset += lineHeight;
			str++;
		} else if (ch >= 0x80) {
			// Same mapping as utf8Cell, on raw bytes (str is NUL
			// terminated, so str[1] is always safe to read).
			unsigned char c1 = (unsigned char)str[1];
			int cell = -1;
			int adv = 1;
			if (ch == 0xD0) {
				if (c1 >= 0x90 && c1 <= 0xAF) { cell = 0x80 + (c1 - 0x90); adv = 2; }
				else if (c1 >= 0xB0 && c1 <= 0xBF) { cell = 0xA0 + (c1 - 0xB0); adv = 2; }
				else if (c1 == 0x81) { cell = 0xC0; adv = 2; }
			} else if (ch == 0xD1) {
				if (c1 >= 0x80 && c1 <= 0x8F) { cell = 0xB0 + (c1 - 0x80); adv = 2; }
				else if (c1 == 0x91) { cell = 0xC1; adv = 2; }
			}
			if (cell >= 0) {
				buildChar((unsigned char)cell, xOffset, yOffset);
				xOffset += fcharWidths[cell];
			}
			str += adv;
		} else {
			buildChar(ch, xOffset, yOffset);
			xOffset += fcharWidths[ch];
			str++;
		}
	}
	t.draw();
	t.addOffset(-(float)x, -(float)y, 0);
}

void Font::buildChar( unsigned char i, float x /*= 0*/, float y /*=0*/ )
{
	Tesselator& t = Tesselator::instance;

	//i -= _charOffset;
	//int ix = (i % _cols) * 8 + _x;
	//int iy = (i / _cols) * 8 + _y;
	float ix = (float)((i & 15) * 8);
	float iy = (float)((i >> 4) * 8);
	float s = 7.99f;

	float uo = (0.0f) / 128.0f;
	float vo = (0.0f) / 128.0f;

	t.vertexUV(x, y + s, 0, ix / 128.0f + uo, (iy + s) / 128.0f + vo);
	t.vertexUV(x + s, y + s, 0, (ix + s) / 128.0f + uo, (iy + s) / 128.0f + vo);
	t.vertexUV(x + s, y, 0, (ix + s) / 128.0f + uo, iy / 128.0f + vo);
	t.vertexUV(x, y, 0, ix / 128.0f + uo, iy / 128.0f + vo);
}

