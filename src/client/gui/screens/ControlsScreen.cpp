#include "ControlsScreen.h"
#include "../../Minecraft.h"
#include "../../../locale/I18n.h"
#include "platform/input/Keyboard.h"

ControlsScreen::ControlsScreen(Screen* parent)
:	m_parent(parent),
	m_btnDone(200, ""),
	m_btnAutoJump(201, ""),
	m_btnInvertMouse(202, ""),
	m_selectedKeyOpt(-1)
{
}

ControlsScreen::~ControlsScreen() {
	for (size_t i = 0; i < m_bindings.size(); i++) {
		if (m_bindings[i].button)
			delete m_bindings[i].button;
	}
	m_bindings.clear();
}

void ControlsScreen::init() {
	buttons.clear();
	for (size_t i = 0; i < m_bindings.size(); i++) {
		if (m_bindings[i].button)
			delete m_bindings[i].button;
	}
	m_bindings.clear();

	m_btnDone.msg = I18n::get("gui.done");

	struct Def { int id; const char* key; };
	Def defs[] = {
		{ OPTIONS_KEY_FORWARD,   "options.key.forward" },
		{ OPTIONS_KEY_LEFT,      "options.key.left" },
		{ OPTIONS_KEY_BACK,      "options.key.back" },
		{ OPTIONS_KEY_RIGHT,     "options.key.right" },
		{ OPTIONS_KEY_JUMP,      "options.key.jump" },
		{ OPTIONS_KEY_SNEAK,     "options.key.sneak" },
		{ OPTIONS_KEY_DROP,      "options.key.drop" },
		{ OPTIONS_KEY_INVENTORY, "options.key.inventory" },
		{ OPTIONS_KEY_CHAT,      "options.key.chat" },
		{ OPTIONS_KEY_FOG,       "options.key.fog" },
		{ OPTIONS_KEY_SPRINT,    "options.key.sprint" },
		{ OPTIONS_KEY_USE,       "options.key.use" }
	};

	for (size_t i = 0; i < sizeof(defs)/sizeof(defs[0]); i++) {
		KeyBindingEntry e;
		e.optId = defs[i].id;
		e.labelKey = defs[i].key;
		e.button = new Button((int)(100 + i), "");
		m_bindings.push_back(e);
		buttons.push_back(e.button);
	}

	buttons.push_back(&m_btnAutoJump);
	buttons.push_back(&m_btnInvertMouse);
	buttons.push_back(&m_btnDone);
	updateButtonTexts();
}

void ControlsScreen::updateButtonTexts() {
	for (size_t i = 0; i < m_bindings.size(); i++) {
		if (!m_bindings[i].button) continue;
		std::string label = I18n::get(m_bindings[i].labelKey);
		if (m_selectedKeyOpt == m_bindings[i].optId) {
			m_bindings[i].button->msg = "> " + label + ": ??? <";
		} else {
			int code = minecraft->options.getIntValue((OptionId)m_bindings[i].optId);
			std::string keyName = Keyboard::getKeyName(code);
			m_bindings[i].button->msg = label + ": " + keyName;
		}
	}

	bool aj = minecraft->options.getBooleanValue(OPTIONS_AUTOJUMP);
	m_btnAutoJump.msg = I18n::get("options.autoJump") + ": " + (aj ? I18n::get("options.on") : I18n::get("options.off"));

	bool inv = minecraft->options.getBooleanValue(OPTIONS_INVERT_Y_MOUSE);
	m_btnInvertMouse.msg = I18n::get("options.invertMouse") + ": " + (inv ? I18n::get("options.on") : I18n::get("options.off"));
}

void ControlsScreen::setupPositions() {
	int btnW = 150;
	int btnH = 20;
	int leftX = width / 2 - 155;
	int rightX = width / 2 + 5;
	if (width < 330) {
		btnW = (width - 24) / 2;
		leftX = width / 2 - btnW - 2;
		rightX = width / 2 + 2;
	}

	int startY = 32;
	for (size_t i = 0; i < m_bindings.size(); i++) {
		int row = (int)(i / 2);
		int col = (int)(i % 2);
		Button* b = m_bindings[i].button;
		if (b) {
			b->x = (col == 0) ? leftX : rightX;
			b->y = startY + row * 24;
			b->width = btnW;
			b->height = btnH;
		}
	}

	int extraRow = (int)(m_bindings.size() / 2);
	m_btnAutoJump.x = leftX;
	m_btnAutoJump.y = startY + extraRow * 24;
	m_btnAutoJump.width = btnW;
	m_btnAutoJump.height = btnH;

	m_btnInvertMouse.x = rightX;
	m_btnInvertMouse.y = startY + extraRow * 24;
	m_btnInvertMouse.width = btnW;
	m_btnInvertMouse.height = btnH;

	int doneW = 200;
	if (doneW > width - 20) doneW = width - 20;
	m_btnDone.width = doneW;
	m_btnDone.height = 20;
	m_btnDone.x = (width - doneW) / 2;
	int doneY = height - 28;
	if (doneY < startY + (extraRow + 1) * 24 + 4) doneY = startY + (extraRow + 1) * 24 + 4;
	m_btnDone.y = doneY;
}

void ControlsScreen::render(int xm, int ym, float a) {
	renderDirtBackground(0);
	drawCenteredString(minecraft->font, I18n::get("options.controls"), width / 2, 14, 0xffffffff);
	Screen::render(xm, ym, a);
}

void ControlsScreen::buttonClicked(Button* button) {
	if (button->id == m_btnDone.id) {
		minecraft->options.save();
		minecraft->setScreen(m_parent);
		return;
	}

	if (button->id == m_btnAutoJump.id) {
		bool val = minecraft->options.getBooleanValue(OPTIONS_AUTOJUMP);
		minecraft->options.set(OPTIONS_AUTOJUMP, !val);
		minecraft->options.save();
		updateButtonTexts();
		return;
	}

	if (button->id == m_btnInvertMouse.id) {
		bool val = minecraft->options.getBooleanValue(OPTIONS_INVERT_Y_MOUSE);
		minecraft->options.set(OPTIONS_INVERT_Y_MOUSE, !val);
		minecraft->options.save();
		updateButtonTexts();
		return;
	}

	for (size_t i = 0; i < m_bindings.size(); i++) {
		if (button == m_bindings[i].button) {
			m_selectedKeyOpt = m_bindings[i].optId;
			updateButtonTexts();
			return;
		}
	}
}

void ControlsScreen::keyPressed(int eventKey) {
	if (m_selectedKeyOpt != -1) {
		if (eventKey != Keyboard::KEY_ESCAPE) {
			minecraft->options.set((OptionId)m_selectedKeyOpt, eventKey);
			minecraft->options.save();
		}
		m_selectedKeyOpt = -1;
		updateButtonTexts();
		return;
	}

	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->options.save();
		minecraft->setScreen(m_parent);
		return;
	}

	Screen::keyPressed(eventKey);
}
