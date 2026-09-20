#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__

//package net.minecraft.client.gui;

#include <string>
#include "GuiElement.h"
#include "../../Options.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/input/Keyboard.h"

class Font;
class Minecraft;

class TextBox: public GuiElement
{
public:
	TextBox(int id, const std::string& msg);
	TextBox(int id, int x, int y, const std::string& msg);
	TextBox(int id, int x, int y, int w, int h, const std::string& msg);

	virtual void mouseClicked(Minecraft* minecraft, int x, int y, int buttonNum);

	virtual void setFocus(Minecraft* minecraft);
	virtual bool loseFocus(Minecraft* minecraft);

	virtual void render(Minecraft* minecraft, int xm, int ym);

	virtual void keyPressed(Minecraft* minecraft, int key);
	virtual void charPressed(Minecraft* minecraft, char c);
	virtual void tick(Minecraft* minecraft);

	void setMaxChars(int n) { maxChars = n; }
	int getMaxChars() const { return maxChars; }

	// Count user-perceived characters (UTF-8 sequences count as one).
	static int charCount(const std::string& s);

	// Feed one raw input byte into text, assembling UTF-8 sequences.
	// Only printable ASCII and the renderable Cyrillic subset are kept,
	// everything else is forbidden. pending holds an incomplete lead byte
	// between calls. Respects maxChars (in characters, not bytes).
	static void appendInputByte(std::string& text, unsigned char& pending, int maxChars, unsigned char u);

	// Drop one whole character (plus any dangling lead) from the end.
	static void popInputChar(std::string& text);

public:
	std::string hint;
	std::string text;
	int id;

	int blinkTicks;

	bool focused;
	bool blink;

private:
	// Max characters (not bytes) accepted into text.
	int maxChars;
	// Pending UTF-8 lead byte between the two charPressed calls of one key.
	unsigned char pendingLead;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__TextBox_H__*/
