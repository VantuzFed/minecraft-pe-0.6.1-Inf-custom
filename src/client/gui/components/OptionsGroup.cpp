#include "OptionsGroup.h"
#include "../../Minecraft.h"
#include "ImageButton.h"
#include "OptionsItem.h"
#include "Slider.h"
#include "../../../locale/I18n.h"
#include "TextOption.h"
#include "KeyOption.h"

OptionsGroup::OptionsGroup( std::string labelID )
	: m_scrollY(0), m_viewHeight(10000), m_contentHeight(0) {
	label = I18n::get(labelID);
}

void OptionsGroup::setViewHeight(int h) {
	if (h < 1) h = 1;
	if (h != m_viewHeight) {
		m_viewHeight = h;
		setupPositions();
	}
}

void OptionsGroup::resetScroll() {
	if (m_scrollY != 0) {
		m_scrollY = 0;
		setupPositions();
	}
}

int OptionsGroup::getMaxScroll() const {
	int max = m_contentHeight - m_viewHeight;
	if (max < 0) max = 0;
	return max;
}

void OptionsGroup::clampScroll() {
	int max = getMaxScroll();
	if (m_scrollY < 0) m_scrollY = 0;
	if (m_scrollY > max) m_scrollY = max;
}

void OptionsGroup::scrollBy(int dy) {
	if (dy == 0) return;
	m_scrollY += dy;
	clampScroll();
	setupPositions();
}

void OptionsGroup::setScrollY(int y) {
	if (y == m_scrollY) return;
	m_scrollY = y;
	clampScroll();
	setupPositions();
}

bool OptionsGroup::isInsideView(int x, int y) const {
	return x >= this->x && x < this->x + this->width
		&& y >= this->y && y < this->y + m_viewHeight;
}

bool OptionsGroup::hitsControl(int x, int y) const {
	for (std::vector<GuiElement*>::const_iterator it = children.begin(); it != children.end(); ++it) {
		OptionsItem* item = dynamic_cast<OptionsItem*>(*it);
		if (!item) continue;
		// only consider visible rows
		if (item->y + item->height < this->y) continue;
		if (item->y >= this->y + m_viewHeight) continue;
		if (item->isControlAt(x, y)) return true;
	}
	return false;
}

void OptionsGroup::mouseClicked(Minecraft* minecraft, int x, int y, int buttonNum) {
	if (!isInsideView(x, y)) return;
	for (std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		GuiElement* child = *it;
		if (child->y + child->height < this->y) continue;
		if (child->y >= this->y + m_viewHeight) continue;
		child->mouseClicked(minecraft, x, y, buttonNum);
	}
}

void OptionsGroup::mouseReleased(Minecraft* minecraft, int x, int y, int buttonNum) {
	for (std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		GuiElement* child = *it;
		if (child->y + child->height < this->y) continue;
		if (child->y >= this->y + m_viewHeight) continue;
		child->mouseReleased(minecraft, x, y, buttonNum);
	}
}

void OptionsGroup::setupPositions() {
	// First we write the header and then we add the items.
	// Content is shifted up by m_scrollY so the user can see items below the fold.
	int curY = y + 18 - m_scrollY;
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->width = width - 5;
		
		(*it)->y = curY;
		(*it)->x = x + 10;
		(*it)->setupPositions();
		curY += (*it)->height + 3;
	}
	m_contentHeight = curY - y + m_scrollY;
	if (m_contentHeight < 0) m_contentHeight = 0;
	height = m_contentHeight;

	int oldScroll = m_scrollY;
	clampScroll();
	if (m_scrollY != oldScroll) {
		// scroll was out of range (e.g. view resized) -> relayout with clamped value
		curY = y + 18 - m_scrollY;
		for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
			(*it)->width = width - 5;
			(*it)->y = curY;
			(*it)->x = x + 10;
			(*it)->setupPositions();
			curY += (*it)->height + 3;
		}
		m_contentHeight = curY - y + m_scrollY;
		if (m_contentHeight < 0) m_contentHeight = 0;
		height = m_contentHeight;
	}
}

void OptionsGroup::render( Minecraft* minecraft, int xm, int ym ) {
	float padX = 10.0f;
	float padY = 5.0f;
	
	minecraft->font->draw(label, (float)x + padX, (float)y + padY, 0xffffffff, false);

	int viewBottom = y + m_viewHeight;
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		GuiElement* child = *it;
		if (child->y + child->height < y) continue;
		if (child->y >= viewBottom) continue;
		child->render(minecraft, xm, ym);
	}

	int maxScroll = getMaxScroll();
	if (maxScroll > 0 && m_viewHeight > 0) {
		int sbX0 = x + width - 4;
		int sbX1 = x + width - 1;
		int sbY0 = y + 18;
		int sbY1 = y + m_viewHeight - 2;
		if (sbY1 > sbY0) {
			fill(sbX0, sbY0, sbX1, sbY1, 0xFF000000);
			int barH = (m_viewHeight * (sbY1 - sbY0)) / m_contentHeight;
			if (barH < 8) barH = 8;
			if (barH > (sbY1 - sbY0)) barH = sbY1 - sbY0;
			int travel = (sbY1 - sbY0) - barH;
			int barY = sbY0;
			if (travel > 0)
				barY = sbY0 + (m_scrollY * travel) / maxScroll;
			fill(sbX0, barY, sbX1, barY + barH, 0xFF808080);
		}
	}
}

OptionsGroup& OptionsGroup::addOptionItem(OptionId optId, Minecraft* minecraft ) {
	auto option = minecraft->options.getOpt(optId);

	if (option == nullptr) return *this;

	// TODO: do a options key class to check it faster via dynamic_cast
	if (option->getStringId().find("options.key") != std::string::npos) createKey(optId, minecraft);
	else if (dynamic_cast<OptionBool*>(option)) createToggle(optId, minecraft);
	else if (dynamic_cast<OptionFloat*>(option)) createProgressSlider(optId, minecraft);
	else if (dynamic_cast<OptionInt*>(option)) createStepSlider(optId, minecraft);
	else if (dynamic_cast<OptionString*>(option)) createTextbox(optId, minecraft);

	return *this;
}

// TODO: wrap this copypaste shit into templates

void OptionsGroup::createToggle(OptionId optId, Minecraft* minecraft ) {
	ImageDef def;

	def.setSrc(IntRectangle(160, 206, 39, 20));
	def.name = "gui/touchgui.png";
	def.width = 39 * 0.7f;
	def.height = 20 * 0.7f;
	
	OptionButton* element = new OptionButton(optId);
	element->setImageDef(def, true);
	element->updateImage(&minecraft->options);
	
	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	
	addChild(item);
	setupPositions();
}

void OptionsGroup::createProgressSlider(OptionId optId, Minecraft* minecraft ) {
	Slider* element = new SliderFloat(minecraft, optId);
	element->width = 100;
	element->height = 20;

	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createStepSlider(OptionId optId, Minecraft* minecraft ) {
	Slider* element = new SliderInt(minecraft, optId);
	element->width = 100;
	element->height = 20;
	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createTextbox(OptionId optId, Minecraft* minecraft) {
	TextBox* element = new TextOption(minecraft, optId);
	element->width = 100;
	element->height = 20;

	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createKey(OptionId optId, Minecraft* minecraft) {
	KeyOption* element = new KeyOption(minecraft, optId);
	element->width = 50;
	element->height = 20;

	std::string itemLabel = I18n::get(minecraft->options.getOpt(optId)->getStringId());
	OptionsItem* item = new OptionsItem(optId, itemLabel, element);
	addChild(item);
	setupPositions();
}