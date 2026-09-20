#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsGroup_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsGroup_H__

//package net.minecraft.client.gui;

#include <string>
#include "GuiElementContainer.h"
#include "ScrollingPane.h"
#include "../../Options.h"

class Font;
class Minecraft;

class OptionsGroup: public GuiElementContainer {
	typedef GuiElementContainer super;
public:
	OptionsGroup(std::string labelID);
	// Adds a custom row (e.g. preset buttons) at the top of the group.
	void addHeaderRow(GuiElement* element);
	virtual void setupPositions();
	virtual void render(Minecraft* minecraft, int xm, int ym);
	OptionsGroup& addOptionItem(OptionId optId, Minecraft* minecraft);

	void setViewHeight(int h);
	void resetScroll();
	void scrollBy(int dy);
	void setScrollY(int y);
	int getScrollY() const { return m_scrollY; }
	int getMaxScroll() const;
	bool isInsideView(int x, int y) const;
	bool hitsControl(int x, int y) const;

	virtual void mouseClicked(Minecraft* minecraft, int x, int y, int buttonNum) override;
	virtual void mouseReleased(Minecraft* minecraft, int x, int y, int buttonNum) override;
protected:

	void createToggle(OptionId optId, Minecraft* minecraft);
	void createProgressSlider(OptionId optId, Minecraft* minecraft);
	void createStepSlider(OptionId optId, Minecraft* minecraft);
	void createTextbox(OptionId optId, Minecraft* minecraft);
	void createKey(OptionId optId, Minecraft* minecraft);

	std::string label;

	int m_scrollY;
	int m_viewHeight;
	int m_contentHeight;
	void clampScroll();
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsGroup_H__*/
