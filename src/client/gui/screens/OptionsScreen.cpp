#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "UsernameScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "CreditsScreen.h"

#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
#include "../../../locale/I18n.h"
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"

// Self-contained preset button living inside the Tweaks option group.
// Applies the preset on click; the group rebuild is deferred to the next
// tick (see m_pendingOptionsRefresh) because the firing button itself
// belongs to the group being rebuilt.
class PresetButton : public Touch::TButton {
	typedef Touch::TButton super;
public:
	PresetButton(int id, const std::string& msg, bool beta)
	:	super(id, msg),
		m_beta(beta),
		m_pressed(false),
		m_screen(NULL) {
	}

	void setScreen(OptionsScreen* s) { m_screen = s; }

	virtual void mouseClicked(Minecraft* mc, int x, int y, int buttonNum) {
		if (buttonNum == MouseAction::ACTION_LEFT && clicked(mc, x, y)) {
			setPressed();
			m_pressed = true;
		}
	}

	virtual void mouseReleased(Minecraft* mc, int x, int y, int buttonNum) {
		if (buttonNum == MouseAction::ACTION_LEFT && m_pressed) {
			m_pressed = false;
			released(x, y);
			if (clicked(mc, x, y) && m_screen != NULL)
				m_screen->applyVisualPreset(m_beta);
		}
	}

private:
	bool m_beta;
	bool m_pressed;
	OptionsScreen* m_screen;
};

OptionsScreen::OptionsScreen()
	: btnClose(NULL),
	bHeader(NULL),
	btnCredits(NULL),
	currentOptionsGroup(NULL),
	selectedCategory(0),
	m_pendingOptionsRefresh(false),
	m_dragActive(false),
	m_dragScrolling(false),
	m_dragStartY(0),
	m_dragStartScroll(0) {
}

OptionsScreen::~OptionsScreen() {
	if (btnClose != NULL) {
		delete btnClose;
		btnClose = NULL;
	}

	if (bHeader != NULL) {
		delete bHeader;
		bHeader = NULL;
	}

	if (btnCredits != NULL) {
		delete btnCredits;
		btnCredits = NULL;
	}

	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if (*it != NULL) {
			delete* it;
			*it = NULL;
		}
	}

	for (std::vector<OptionsGroup*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if (*it != NULL) {
			delete* it;
			*it = NULL;
		}
	}

	categoryButtons.clear();
}

void OptionsScreen::init() {
	bHeader = new Touch::THeader(0, "Options");

	btnClose = new ImageButton(1, "");

	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	categoryButtons.push_back(new Touch::TButton(2, "General"));
	categoryButtons.push_back(new Touch::TButton(3, "Game"));
	categoryButtons.push_back(new Touch::TButton(4, "Controls"));
	categoryButtons.push_back(new Touch::TButton(5, "Graphics"));
	categoryButtons.push_back(new Touch::TButton(6, "Tweaks"));

	btnCredits = new Touch::TButton(11, "Credits");

	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	buttons.push_back(btnCredits);

	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}

	generateOptionScreens();
	// start with first category selected
	selectCategory(0);
}

void OptionsScreen::setupPositions() {
	int buttonHeight = btnClose->height;

	btnClose->x = width - btnClose->width;
	btnClose->y = 0;

	int offsetNum = 1;

	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {

		(*it)->x = 0;
		(*it)->y = offsetNum * buttonHeight;
		(*it)->selected = false;

		offsetNum++;
	}

	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - btnClose->width;
	bHeader->height = btnClose->height;

	// Credits button: same left column as General/Game/Controls/etc.,
	// right below the categories, so it never overlaps the option list.
	if (btnCredits != NULL) {
		btnCredits->x = 0;
		btnCredits->y = offsetNum * buttonHeight;
		offsetNum++;
	}

	for (std::vector<OptionsGroup*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {

		if (categoryButtons.size() > 0 && categoryButtons[0] != NULL) {

			(*it)->x = categoryButtons[0]->width;
			(*it)->y = bHeader->height;
			(*it)->width = width - categoryButtons[0]->width;

			(*it)->setViewHeight(height - bHeader->height);
		}
	}

	// don't override user selection on resize
}


void OptionsScreen::render(int xm, int ym, float a) {
	renderBackground();

	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;

	if (currentOptionsGroup != NULL)
		currentOptionsGroup->render(minecraft, xmm, ymm);

	super::render(xm, ym, a);
}

void OptionsScreen::removed() {
}

void OptionsScreen::buttonClicked(Button* button) {
	if (button == btnClose) {
		minecraft->options.save();
		if (minecraft->screen != NULL) {
			minecraft->setScreen(NULL);
		} else {
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
	}
	else if (button->id > 1 && button->id < 7) {
		int categoryButton = button->id - categoryButtons[0]->id;
		selectCategory(categoryButton);
	}
	else if (button == btnCredits) {
		minecraft->setScreen(new CreditsScreen());
	}
}

void OptionsScreen::applyVisualPreset(bool beta) {
	Options& o = minecraft->options;
	// One click bundles the scattered beta/PE-look settings.
	o.set(OPTIONS_FOLIAGE_TINT, true);
	o.set(OPTIONS_TINTED_SIDE, beta);
	o.set(OPTIONS_JAVA_HUD, beta);
	o.set(OPTIONS_FOG_TYPE, beta ? 1 : 0);
	o.set(OPTIONS_BETA_SKY, beta);
	o.set(OPTIONS_BEAUTIFUL_SKY, !beta);
	o.set(OPTIONS_BLOCK_OUTLINE, beta ? 1 : 0);
	o.set(OPTIONS_VIGNETTE, !beta);
	o.set(OPTIONS_RESTORED_ANIMS, beta);
	o.set(OPTIONS_MENU_STYLE, beta ? 2 : 0);
	o.save();
	// The option rows show the new values only after a rebuild; do it on
	// the next tick, never from inside event dispatch (the firing button
	// lives in the group being rebuilt).
	m_pendingOptionsRefresh = true;
}

void OptionsScreen::refreshOptions() {
	m_pendingOptionsRefresh = false;
	int category = selectedCategory;
	for (std::vector<OptionsGroup*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if (*it != NULL) {
			delete* it;
			*it = NULL;
		}
	}
	optionPanes.clear();
	currentOptionsGroup = NULL;
	generateOptionScreens();
	selectCategory(category);
	setupPositions();
}

void OptionsScreen::selectCategory(int index) {
	int currentIndex = 0;

	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {

		if (index == currentIndex)
			(*it)->selected = true;
		else
			(*it)->selected = false;

		currentIndex++;
	}

	selectedCategory = index;
	m_dragActive = false;
	m_dragScrolling = false;

	if (index >= 0 && index < (int)optionPanes.size()) {
		currentOptionsGroup = optionPanes[index];
		currentOptionsGroup->resetScroll();
	}

	// Refresh preset button visibility (and clickability) on category switch.
	setupPositions();
}

void OptionsScreen::generateOptionScreens() {
	// how the fuck it works

	optionPanes.push_back(new OptionsGroup("options.group.general"));
	optionPanes.push_back(new OptionsGroup("options.group.game"));
	optionPanes.push_back(new OptionsGroup("options.group.controls"));
	optionPanes.push_back(new OptionsGroup("options.group.graphics"));
	optionPanes.push_back(new OptionsGroup("options.group.tweaks"));

	// General Pane
	optionPanes[0]->addOptionItem(OPTIONS_USERNAME, minecraft)
		.addOptionItem(OPTIONS_SENSITIVITY, minecraft);

	// Game Pane
	optionPanes[1]->addOptionItem(OPTIONS_DIFFICULTY, minecraft)
		.addOptionItem(OPTIONS_SERVER_VISIBLE, minecraft)
		.addOptionItem(OPTIONS_THIRD_PERSON_VIEW, minecraft)
		.addOptionItem(OPTIONS_WINDOW_SCALE, minecraft)
		.addOptionItem(OPTIONS_GUI_SCALE, minecraft)
		.addOptionItem(OPTIONS_SENSITIVITY, minecraft)
		.addOptionItem(OPTIONS_MUSIC_VOLUME, minecraft)
		.addOptionItem(OPTIONS_SOUND_VOLUME, minecraft)
		.addOptionItem(OPTIONS_SMOOTH_CAMERA, minecraft)
		.addOptionItem(OPTIONS_DESTROY_VIBRATION, minecraft)
		.addOptionItem(OPTIONS_IS_LEFT_HANDED, minecraft);

	// // Controls Pane
	optionPanes[2]->addOptionItem(OPTIONS_INVERT_Y_MOUSE, minecraft)
		.addOptionItem(OPTIONS_USE_TOUCHSCREEN, minecraft)
		.addOptionItem(OPTIONS_AUTOJUMP, minecraft)
		.addOptionItem(OPTIONS_IS_JOY_TOUCH_AREA, minecraft);

	for (int i = OPTIONS_KEY_FORWARD; i <= OPTIONS_KEY_SPRINT; i++) {
		optionPanes[2]->addOptionItem((OptionId)i, minecraft);
	}

	// // Graphics Pane
	optionPanes[3]->addOptionItem(OPTIONS_FANCY_GRAPHICS, minecraft)
		.addOptionItem(OPTIONS_BLOCK_OUTLINE, minecraft)
		// .addOptionItem(&Option::VIEW_BOBBING, minecraft)
		// .addOptionItem(&Option::AMBIENT_OCCLUSION, minecraft)
		// .addOptionItem(&Option::ANAGLYPH, minecraft)
		.addOptionItem(OPTIONS_LIMIT_FRAMERATE, minecraft)
		.addOptionItem(OPTIONS_VSYNC, minecraft)
		.addOptionItem(OPTIONS_VIEW_DISTANCE, minecraft)
		.addOptionItem(OPTIONS_RENDER_DEBUG, minecraft)
		.addOptionItem(OPTIONS_ANAGLYPH_3D, minecraft)
		.addOptionItem(OPTIONS_VIEW_BOBBING, minecraft)
		.addOptionItem(OPTIONS_AMBIENT_OCCLUSION, minecraft)
		.addOptionItem(OPTIONS_NORMAL_LIGHTING, minecraft)
		.addOptionItem(OPTIONS_BEAUTIFUL_SKY, minecraft)
		.addOptionItem(OPTIONS_VIGNETTE, minecraft);

	// Visual preset buttons at the top of the Tweaks section.
	{
		PresetButton* beta = new PresetButton(12, I18n::get("options.visualPreset.beta"), true);
		beta->setScreen(this);
		PresetButton* pe = new PresetButton(13, I18n::get("options.visualPreset.pe"), false);
		pe->setScreen(this);
		optionPanes[4]->addHeaderRow(pe);
		optionPanes[4]->addHeaderRow(beta);
	}

	optionPanes[4]->addOptionItem(OPTIONS_ALLOW_SPRINT, minecraft)
		.addOptionItem(OPTIONS_BAR_ON_TOP, minecraft)
		.addOptionItem(OPTIONS_MENU_STYLE, minecraft)
		.addOptionItem(OPTIONS_RPI_CURSOR, minecraft)
		.addOptionItem(OPTIONS_FOLIAGE_TINT, minecraft)
		.addOptionItem(OPTIONS_TINTED_SIDE, minecraft)
		.addOptionItem(OPTIONS_JAVA_HUD, minecraft)
		.addOptionItem(OPTIONS_FOG_TYPE, minecraft)
		.addOptionItem(OPTIONS_BETA_SKY, minecraft)
		.addOptionItem(OPTIONS_RESTORED_ANIMS, minecraft)
		.addOptionItem(OPTIONS_DEBUG_STYLE, minecraft)
		.addOptionItem(OPTIONS_LOG_LEVEL, minecraft);
		
}

void OptionsScreen::mouseClicked(int x, int y, int buttonNum) {
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->mouseClicked(minecraft, x, y, buttonNum);

	super::mouseClicked(x, y, buttonNum);

	// Begin potential drag-scroll only when the press is inside the list
	// but NOT on an interactive control (slider/toggle/textbox/key).
	// This way dragging a slider still adjusts it, while dragging the
	// label/empty area scrolls the list (works for mouse + touch).
	m_dragActive = false;
	m_dragScrolling = false;
	if (buttonNum == MouseAction::ACTION_LEFT && currentOptionsGroup != NULL) {
		if (currentOptionsGroup->isInsideView(x, y)
			&& !currentOptionsGroup->hitsControl(x, y)) {
			m_dragActive = true;
			m_dragStartY = y;
			m_dragStartScroll = currentOptionsGroup->getScrollY();
		}
	}
}

void OptionsScreen::mouseReleased(int x, int y, int buttonNum) {
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->mouseReleased(minecraft, x, y, buttonNum);

	super::mouseReleased(x, y, buttonNum);

	m_dragActive = false;
	m_dragScrolling = false;
}

void OptionsScreen::mouseWheel(int dx, int dy, int xm, int ym) {
	if (currentOptionsGroup == NULL) return;
	if (dy == 0 && dx == 0) return;
	// GLFW: dy > 0 = wheel up. Scrolling up shows earlier items (scrollY down).
	int delta = -dy * 20;
	if (delta == 0) delta = -dx * 20;
	currentOptionsGroup->scrollBy(delta);
}

void OptionsScreen::keyPressed(int eventKey) {
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->keyPressed(minecraft, eventKey);
	if (eventKey == Keyboard::KEY_ESCAPE) 
		minecraft->options.save();

	super::keyPressed(eventKey);
}

void OptionsScreen::charPressed(char inputChar) {
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->charPressed(minecraft, inputChar);

	super::keyPressed(inputChar);
}

void OptionsScreen::tick() {

	if (m_pendingOptionsRefresh)
		refreshOptions();

	if (currentOptionsGroup != NULL)
		currentOptionsGroup->tick(minecraft);

	// Continue drag-scroll while the button is held.
	if (m_dragActive && currentOptionsGroup != NULL
		&& Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
		int mx = Mouse::getX();
		int my = Mouse::getY();
		toGUICoordinate(mx, my);
		int dy = my - m_dragStartY;
		if (!m_dragScrolling && (dy > 4 || dy < -4))
			m_dragScrolling = true;
		if (m_dragScrolling)
			currentOptionsGroup->setScrollY(m_dragStartScroll - dy);
	}

	super::tick();
}
