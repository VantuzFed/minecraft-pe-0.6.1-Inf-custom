#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "UsernameScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "CreditsScreen.h"
#include "ControlsScreen.h"

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

#include <sstream>

OptionsScreen::OptionsScreen()
	: btnClose(NULL),
	bHeader(NULL),
	btnCredits(NULL),
	currentOptionsGroup(NULL),
	selectedCategory(0),
	m_pendingOptionsRefresh(false),
	bMusic(101, ""),
	bSound(102, ""),
	bInvertMouse(103, ""),
	bSensitivity(104, ""),
	bFOV(115, ""),
	bRenderDistance(105, ""),
	bViewBobbing(106, ""),
	bFramerate(107, ""),
	b3DAnaglyph(108, ""),
	bDifficulty(109, ""),
	bGraphics(110, ""),
	bSmoothLighting(111, ""),
	bMenuStyle(112, ""),
	bControls(113, ""),
	bCreditsBeta(114, ""),
	bDone(200, ""),
	m_dragActive(false),
	m_dragScrolling(false),
	m_dragStartY(0),
	m_dragStartScroll(0) {
}

bool OptionsScreen::isBetaStyle() const {
	return minecraft && minecraft->options.getIntValue(OPTIONS_MENU_STYLE) == 2;
}

void OptionsScreen::updateBetaButtonTexts() {
	Options& o = minecraft->options;

	float musicVol = o.getProgressValue(OPTIONS_MUSIC_VOLUME);
	int musicPct = (int)(musicVol * 100.0f + 0.5f);
	bMusic.msg = I18n::get("options.music") + ": " + (musicPct > 0 ? std::to_string(musicPct) + "%" : I18n::get("options.off"));

	float soundVol = o.getProgressValue(OPTIONS_SOUND_VOLUME);
	int soundPct = (int)(soundVol * 100.0f + 0.5f);
	bSound.msg = I18n::get("options.sound") + ": " + (soundPct > 0 ? std::to_string(soundPct) + "%" : I18n::get("options.off"));

	bInvertMouse.msg = I18n::get("options.invertMouse") + ": " + (o.getIntValue(OPTIONS_INVERT_Y_MOUSE) ? I18n::get("options.on") : I18n::get("options.off"));

	int sens = o.getIntValue(OPTIONS_SENSITIVITY);
	{
		std::stringstream ss;
		ss << I18n::get("options.sensitivity") << ": " << (sens * 20) << "%";
		bSensitivity.msg = ss.str();
	}

	int fovVal = o.getIntValue(OPTIONS_FOV);
	if (fovVal < 30 || fovVal > 130) fovVal = 70;
	if (fovVal == 70) {
		bFOV.msg = I18n::get("options.fov") + ": " + I18n::get("options.fov.min");
	} else if (fovVal >= 110) {
		bFOV.msg = I18n::get("options.fov") + ": " + I18n::get("options.fov.max");
	} else {
		bFOV.msg = I18n::get("options.fov") + ": " + std::to_string(fovVal);
	}

	int rd = o.getIntValue(OPTIONS_VIEW_DISTANCE);
	std::string rdStr;
	switch (rd) {
		case 0: rdStr = I18n::get("options.renderDistance.far"); break;
		case 1: rdStr = I18n::get("options.renderDistance.normal"); break;
		case 2: rdStr = I18n::get("options.renderDistance.short"); break;
		default: rdStr = I18n::get("options.renderDistance.tiny"); break;
	}
	bRenderDistance.msg = I18n::get("options.renderDistance") + ": " + rdStr;

	bViewBobbing.msg = I18n::get("options.viewBobbing") + ": " + (o.getIntValue(OPTIONS_VIEW_BOBBING) ? I18n::get("options.on") : I18n::get("options.off"));

	int perf = o.getIntValue(OPTIONS_LIMIT_FRAMERATE);
	std::string perfStr;
	switch (perf) {
		case 1: perfStr = I18n::get("performance.balanced"); break;
		case 2: perfStr = I18n::get("performance.powersaver"); break;
		default: perfStr = I18n::get("performance.max"); break;
	}
	bFramerate.msg = I18n::get("options.framerateLimit") + ": " + perfStr;

	b3DAnaglyph.msg = I18n::get("options.anaglyph") + ": " + (o.getIntValue(OPTIONS_ANAGLYPH_3D) ? I18n::get("options.on") : I18n::get("options.off"));

	int diff = o.getIntValue(OPTIONS_DIFFICULTY);
	std::string diffStr;
	switch (diff) {
		case 0: diffStr = I18n::get("options.difficulty.peaceful"); break;
		case 1: diffStr = I18n::get("options.difficulty.easy"); break;
		case 2: diffStr = I18n::get("options.difficulty.normal"); break;
		default: diffStr = I18n::get("options.difficulty.hard"); break;
	}
	bDifficulty.msg = I18n::get("options.difficulty") + ": " + diffStr;

	bGraphics.msg = I18n::get("options.graphics") + ": " + (o.getIntValue(OPTIONS_FANCY_GRAPHICS) ? I18n::get("options.graphics.fancy") : I18n::get("options.graphics.fast"));
	bSmoothLighting.msg = I18n::get("options.ao") + ": " + (o.getIntValue(OPTIONS_AMBIENT_OCCLUSION) ? I18n::get("options.on") : I18n::get("options.off"));

	int style = o.getIntValue(OPTIONS_MENU_STYLE);
	bMenuStyle.msg = I18n::get("options.menuStyle") + ": " + (style == 2 ? I18n::get("options.menuStyle.java") : I18n::get("options.menuStyle.pocket"));

	bControls.msg = I18n::get("options.controls") + "...";
	bCreditsBeta.msg = "Credits";
	bDone.msg = I18n::get("gui.done");
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
	buttons.clear();
	tabButtons.clear();

	if (isBetaStyle()) {
		updateBetaButtonTexts();

		buttons.push_back(&bMusic);
		buttons.push_back(&bSound);
		buttons.push_back(&bInvertMouse);
		buttons.push_back(&bSensitivity);
		buttons.push_back(&bFOV);
		buttons.push_back(&bRenderDistance);
		buttons.push_back(&bViewBobbing);
		buttons.push_back(&bFramerate);
		buttons.push_back(&bDifficulty);
		buttons.push_back(&bGraphics);
		buttons.push_back(&bSmoothLighting);
		buttons.push_back(&b3DAnaglyph);
		buttons.push_back(&bControls);
		buttons.push_back(&bMenuStyle);
		buttons.push_back(&bDone);

		for (size_t i = 0; i < buttons.size(); i++) {
			tabButtons.push_back(buttons[i]);
		}
		return;
	}

	bHeader = new Touch::THeader(0, "Options");

	btnClose = new ImageButton(1, "");

	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	for (size_t i = 0; i < categoryButtons.size(); i++) {
		if (categoryButtons[i]) delete categoryButtons[i];
	}
	categoryButtons.clear();

	categoryButtons.push_back(new Touch::TButton(2, "General"));
	categoryButtons.push_back(new Touch::TButton(3, "Game"));
	categoryButtons.push_back(new Touch::TButton(4, "Controls"));
	categoryButtons.push_back(new Touch::TButton(5, "Graphics"));
	categoryButtons.push_back(new Touch::TButton(6, "Tweaks"));

	if (btnCredits) delete btnCredits;
	btnCredits = new Touch::TButton(11, "Credits");

	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	buttons.push_back(btnCredits);

	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}

	for (size_t i = 0; i < optionPanes.size(); i++) {
		if (optionPanes[i]) delete optionPanes[i];
	}
	optionPanes.clear();
	currentOptionsGroup = NULL;

	generateOptionScreens();
	// start with first category selected
	selectCategory(0);
}

void OptionsScreen::setupPositions() {
	if (isBetaStyle()) {
		int totalRows = 7;
		int totalH = totalRows * 24 + 32;
		int startY = (height - totalH) / 2;
		if (startY < 24) startY = 24;

		int btnW = 150;
		int btnH = 20;
		int leftX = width / 2 - 155;
		int rightX = width / 2 + 5;
		if (width < 330) {
			btnW = Mth::Max(80, (width - 24) / 2);
			leftX = width / 2 - btnW - 2;
			rightX = width / 2 + 2;
		}

		// Row 0: Music / Sound
		bMusic.x = leftX; bMusic.y = startY + 0 * 24; bMusic.width = btnW; bMusic.height = btnH;
		bSound.x = rightX; bSound.y = startY + 0 * 24; bSound.width = btnW; bSound.height = btnH;

		// Row 1: Invert Mouse / Sensitivity
		bInvertMouse.x = leftX; bInvertMouse.y = startY + 1 * 24; bInvertMouse.width = btnW; bInvertMouse.height = btnH;
		bSensitivity.x = rightX; bSensitivity.y = startY + 1 * 24; bSensitivity.width = btnW; bSensitivity.height = btnH;

		// Row 2: FOV / Render Distance
		bFOV.x = leftX; bFOV.y = startY + 2 * 24; bFOV.width = btnW; bFOV.height = btnH;
		bRenderDistance.x = rightX; bRenderDistance.y = startY + 2 * 24; bRenderDistance.width = btnW; bRenderDistance.height = btnH;

		// Row 3: View Bobbing / Performance
		bViewBobbing.x = leftX; bViewBobbing.y = startY + 3 * 24; bViewBobbing.width = btnW; bViewBobbing.height = btnH;
		bFramerate.x = rightX; bFramerate.y = startY + 3 * 24; bFramerate.width = btnW; bFramerate.height = btnH;

		// Row 4: Difficulty / Graphics
		bDifficulty.x = leftX; bDifficulty.y = startY + 4 * 24; bDifficulty.width = btnW; bDifficulty.height = btnH;
		bGraphics.x = rightX; bGraphics.y = startY + 4 * 24; bGraphics.width = btnW; bGraphics.height = btnH;

		// Row 5: Smooth Lighting / 3D Anaglyph
		bSmoothLighting.x = leftX; bSmoothLighting.y = startY + 5 * 24; bSmoothLighting.width = btnW; bSmoothLighting.height = btnH;
		b3DAnaglyph.x = rightX; b3DAnaglyph.y = startY + 5 * 24; b3DAnaglyph.width = btnW; b3DAnaglyph.height = btnH;

		// Row 6: Controls / UI Style
		bControls.x = leftX; bControls.y = startY + 6 * 24; bControls.width = btnW; bControls.height = btnH;
		bMenuStyle.x = rightX; bMenuStyle.y = startY + 6 * 24; bMenuStyle.width = btnW; bMenuStyle.height = btnH;

		// Done button
		int doneW = Mth::Min(200, width - 20);
		bDone.width = doneW;
		bDone.height = 20;
		bDone.x = (width - doneW) / 2;
		int doneY = height - 26;
		int minDoneY = startY + totalRows * 24 + 4;
		if (doneY < minDoneY) doneY = minDoneY;
		bDone.y = doneY;

		return;
	}

	if (!btnClose) return;
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
			(*it)->setupPositions();
		}
	}

	// don't override user selection on resize
}


void OptionsScreen::render(int xm, int ym, float a) {
	if (isBetaStyle()) {
		renderDirtBackground(0);

		int totalRows = 6;
		int totalH = totalRows * 24 + 32;
		int startY = (height - totalH) / 2;
		if (startY < 26) startY = 26;
		int titleY = startY > 20 ? (startY - 16) : 6;

		drawCenteredString(minecraft->font, I18n::get("options.title"), width / 2, titleY, 0xffffffff);

		Screen::render(xm, ym, a);
		return;
	}

	renderBackground();

	if (currentOptionsGroup != NULL)
		currentOptionsGroup->render(minecraft, xm, ym);

	super::render(xm, ym, a);
}

void OptionsScreen::removed() {
}

void OptionsScreen::buttonClicked(Button* button) {
	if (isBetaStyle()) {
		if (button->id == bDone.id) {
			minecraft->options.save();
			minecraft->reloadOptions();
			if (minecraft->screen != NULL) {
				minecraft->setScreen(NULL);
			} else {
				minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
			}
			return;
		}
		if (button->id == bMusic.id) {
			float vol = minecraft->options.getProgressValue(OPTIONS_MUSIC_VOLUME);
			int step = (int)(vol * 5.0f + 0.5f);
			step = (step + 1) % 6;
			float nextVol = step * 0.2f;
			minecraft->options.set(OPTIONS_MUSIC_VOLUME, nextVol);
			minecraft->options.music = nextVol;
			minecraft->options.save();
		}
		else if (button->id == bSound.id) {
			float vol = minecraft->options.getProgressValue(OPTIONS_SOUND_VOLUME);
			int step = (int)(vol * 5.0f + 0.5f);
			step = (step + 1) % 6;
			float nextVol = step * 0.2f;
			minecraft->options.set(OPTIONS_SOUND_VOLUME, nextVol);
			minecraft->options.sound = nextVol;
			minecraft->options.save();
		}
		else if (button->id == bInvertMouse.id) {
			bool val = minecraft->options.getIntValue(OPTIONS_INVERT_Y_MOUSE) != 0;
			minecraft->options.set(OPTIONS_INVERT_Y_MOUSE, !val);
			minecraft->options.save();
		}
		else if (button->id == bSensitivity.id) {
			int sens = minecraft->options.getIntValue(OPTIONS_SENSITIVITY);
			sens = (sens + 1) % 6;
			if (sens == 0) sens = 1;
			minecraft->options.set(OPTIONS_SENSITIVITY, sens);
			minecraft->options.save();
		}
		else if (button->id == bFOV.id) {
			int fovVal = minecraft->options.getIntValue(OPTIONS_FOV);
			if (fovVal < 70) fovVal = 70;
			fovVal += 10;
			if (fovVal > 110) fovVal = 70;
			minecraft->options.set(OPTIONS_FOV, fovVal);
			minecraft->options.save();
		}
		else if (button->id == bControls.id) {
			minecraft->setScreen(new ControlsScreen(this));
			return;
		}
		else if (button->id == bRenderDistance.id) {
			int rd = (minecraft->options.getIntValue(OPTIONS_VIEW_DISTANCE) + 1) % 4;
			minecraft->options.set(OPTIONS_VIEW_DISTANCE, rd);
			minecraft->options.save();
		}
		else if (button->id == bViewBobbing.id) {
			bool val = minecraft->options.getIntValue(OPTIONS_VIEW_BOBBING) != 0;
			minecraft->options.set(OPTIONS_VIEW_BOBBING, !val);
			minecraft->options.save();
		}
		else if (button->id == bFramerate.id) {
			int perf = (minecraft->options.getIntValue(OPTIONS_LIMIT_FRAMERATE) + 1) % 3;
			minecraft->options.set(OPTIONS_LIMIT_FRAMERATE, perf);
			minecraft->options.save();
		}
		else if (button->id == b3DAnaglyph.id) {
			bool val = minecraft->options.getIntValue(OPTIONS_ANAGLYPH_3D) != 0;
			minecraft->options.set(OPTIONS_ANAGLYPH_3D, !val);
			minecraft->options.save();
		}
		else if (button->id == bDifficulty.id) {
			int diff = (minecraft->options.getIntValue(OPTIONS_DIFFICULTY) + 1) % 4;
			minecraft->options.set(OPTIONS_DIFFICULTY, diff);
			minecraft->options.save();
		}
		else if (button->id == bGraphics.id) {
			bool val = minecraft->options.getIntValue(OPTIONS_FANCY_GRAPHICS) != 0;
			minecraft->options.set(OPTIONS_FANCY_GRAPHICS, !val);
			minecraft->options.save();
		}
		else if (button->id == bSmoothLighting.id) {
			bool val = minecraft->options.getIntValue(OPTIONS_AMBIENT_OCCLUSION) != 0;
			minecraft->options.set(OPTIONS_AMBIENT_OCCLUSION, !val);
			minecraft->options.save();
		}
		else if (button->id == bMenuStyle.id) {
			int cur = minecraft->options.getIntValue(OPTIONS_MENU_STYLE);
			int next = (cur == 2) ? 0 : 2;
			minecraft->options.set(OPTIONS_MENU_STYLE, next);
			minecraft->options.save();
			refreshOptions();
			return;
		}
		updateBetaButtonTexts();
		return;
	}

	if (button == btnClose) {
		minecraft->options.save();
		minecraft->reloadOptions();
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
	m_pendingOptionsRefresh = true;
}

void OptionsScreen::refreshOptions() {
	m_pendingOptionsRefresh = false;
	init();
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
	optionPanes[3]->addOptionItem(OPTIONS_FOV, minecraft)
		.addOptionItem(OPTIONS_FANCY_GRAPHICS, minecraft)
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
	if (isBetaStyle()) {
		super::mouseClicked(x, y, buttonNum);
		return;
	}

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
	if (isBetaStyle()) {
		super::mouseReleased(x, y, buttonNum);
		return;
	}

	if (currentOptionsGroup != NULL)
		currentOptionsGroup->mouseReleased(minecraft, x, y, buttonNum);

	super::mouseReleased(x, y, buttonNum);

	m_dragActive = false;
	m_dragScrolling = false;
}

void OptionsScreen::mouseWheel(int dx, int dy, int xm, int ym) {
	if (isBetaStyle()) return;
	if (currentOptionsGroup == NULL) return;
	if (dy == 0 && dx == 0) return;
	// GLFW: dy > 0 = wheel up. Scrolling up shows earlier items (scrollY down).
	int delta = -dy * 20;
	if (delta == 0) delta = -dx * 20;
	currentOptionsGroup->scrollBy(delta);
}

void OptionsScreen::keyPressed(int eventKey) {
	if (isBetaStyle()) {
		if (eventKey == Keyboard::KEY_ESCAPE) {
			minecraft->options.save();
			minecraft->reloadOptions();
			if (minecraft->screen != NULL) {
				minecraft->setScreen(NULL);
			} else {
				minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
			}
			return;
		}
		super::keyPressed(eventKey);
		return;
	}

	if (currentOptionsGroup != NULL)
		currentOptionsGroup->keyPressed(minecraft, eventKey);
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->options.save();
		minecraft->reloadOptions();
	}

	super::keyPressed(eventKey);
}

void OptionsScreen::charPressed(char inputChar) {
	if (isBetaStyle()) return;
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->charPressed(minecraft, inputChar);

	super::keyPressed(inputChar);
}

void OptionsScreen::tick() {
	if (m_pendingOptionsRefresh)
		refreshOptions();

	if (isBetaStyle()) {
		super::tick();
		return;
	}

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
