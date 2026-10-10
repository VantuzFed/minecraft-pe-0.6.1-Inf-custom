#include "ShadersScreen.h"
#include "OptionsScreen.h"
#include "../../renderer/shader/ShaderPipeline.h"
#include "../../renderer/LevelRenderer.h"
#include "../../Minecraft.h"
#include "../../../locale/I18n.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include <sys/stat.h>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#endif

//
// ShaderPackList implementation
//
ShaderPackList::ShaderPackList(ShadersScreen* screen, Minecraft* minecraft, int width, int height)
	: RolledSelectionListV(minecraft, width, height, 0, width, 32, height - 56, 24),
	  m_screen(screen),
	  m_selectedIndex(-1)
{
	setRenderSelection(true);
	refreshList();
}

void ShaderPackList::refreshList() {
	m_packs = g_shaderPipeline.getAvailablePacks();
	std::string current = g_shaderPipeline.getCurrentPack();

	m_selectedIndex = -1;
	for (size_t i = 0; i < m_packs.size(); ++i) {
		if (m_packs[i] == current) {
			m_selectedIndex = (int)i;
			break;
		}
		if (current == "Built-in" && m_packs[i] == "(internal)") {
			m_selectedIndex = (int)i;
			break;
		}
	}
	if (m_selectedIndex == -1 && !m_packs.empty()) {
		if (g_shaderPipeline.getMode() == SHADER_MODE_OFF) {
			m_selectedIndex = 0; // (OFF)
		} else {
			m_selectedIndex = 1; // (internal)
		}
	}
}

const std::string& ShaderPackList::getSelectedPackName() const {
	static const std::string empty;
	if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_packs.size()) {
		return m_packs[m_selectedIndex];
	}
	return empty;
}

int ShaderPackList::getNumberOfItems() {
	return (int)m_packs.size();
}

void ShaderPackList::selectItem(int item, bool doubleClick) {
	if (item < 0 || item >= (int)m_packs.size()) return;
	m_selectedIndex = item;
	if (m_screen) {
		m_screen->applySelectedPack(m_packs[item]);
	}
}

bool ShaderPackList::isSelectedItem(int item) {
	return (item == m_selectedIndex);
}

void ShaderPackList::renderItem(int i, int x, int y, int h, Tesselator& t) {
	if (i < 0 || i >= (int)m_packs.size() || !minecraft) return;

	std::string name = m_packs[i];
	int boxX = width / 2 - 120;
	int color = (i == m_selectedIndex) ? 0xFFFFA0 : 0xFFFFFF;

	if (name == "(OFF)") {
		color = (i == m_selectedIndex) ? 0xFF8080 : 0xAAAAAA;
	} else if (name == "(internal)") {
		color = (i == m_selectedIndex) ? 0x80FFA0 : 0xAAAAAA;
	}

	minecraft->font->draw(name, boxX + 6, y + 6, color);
}

//
// ShadersScreen implementation
//
static void openShaderpacksFolder() {
	struct stat st;
	if (stat("shaderpacks", &st) != 0) {
#if defined(_WIN32)
		_mkdir("shaderpacks");
#else
		mkdir("shaderpacks", 0755);
#endif
	}

#if defined(_WIN32)
	ShellExecuteA(NULL, "open", "shaderpacks", NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
	system("open shaderpacks &");
#else
	system("xdg-open shaderpacks &");
#endif
}

ShadersScreen::ShadersScreen()
	: m_packList(nullptr),
	  m_btnOpenFolder(101, "Open shaderpacks folder"),
	  m_btnDone(102, I18n::get("gui.done"))
{}

ShadersScreen::~ShadersScreen() {
	if (m_packList) {
		delete m_packList;
		m_packList = nullptr;
	}
}

void ShadersScreen::init() {
	buttons.clear();

	if (m_packList) {
		delete m_packList;
	}
	m_packList = new ShaderPackList(this, minecraft, width, height);

	m_btnOpenFolder.msg = I18n::get("options.shaders.folder");
	if (m_btnOpenFolder.msg.empty() || m_btnOpenFolder.msg == "options.shaders.folder") {
		m_btnOpenFolder.msg = "Shaders Folder";
	}
	m_btnDone.msg = I18n::get("gui.done");

	buttons.push_back(&m_btnOpenFolder);
	buttons.push_back(&m_btnDone);

	setupPositions();
}

void ShadersScreen::setupPositions() {
	int btnW = 150;
	int btnH = 20;
	int gap = 10;
	int totalW = btnW * 2 + gap;
	int startX = (width - totalW) / 2;
	int y = height - 36;

	m_btnOpenFolder.x = startX;
	m_btnOpenFolder.y = y;
	m_btnOpenFolder.width = btnW;
	m_btnOpenFolder.height = btnH;

	m_btnDone.x = startX + btnW + gap;
	m_btnDone.y = y;
	m_btnDone.width = btnW;
	m_btnDone.height = btnH;

	if (m_packList) {
		m_packList->setSize(width, height, 0, width, 32, height - 48);
	}
}

void ShadersScreen::applySelectedPack(const std::string& packName) {
	if (!minecraft) return;

	if (packName == "(OFF)") {
		minecraft->options.set(OPTIONS_SHADERPACK, "(OFF)");
		minecraft->options.set(OPTIONS_SHADERS, false);
		g_shaderPipeline.setMode(SHADER_MODE_OFF);
	} else if (packName == "(internal)" || packName == "Built-in") {
		minecraft->options.set(OPTIONS_SHADERPACK, "Built-in");
		minecraft->options.set(OPTIONS_SHADERS, true);
		g_shaderPipeline.loadShaderPack("Built-in");
	} else {
		minecraft->options.set(OPTIONS_SHADERPACK, packName);
		minecraft->options.set(OPTIONS_SHADERS, true);
		if (!g_shaderPipeline.loadShaderPack(packName)) {
			minecraft->options.set(OPTIONS_SHADERPACK, "Built-in");
			g_shaderPipeline.loadShaderPack("Built-in");
		}
	}
	minecraft->options.save();
	if (minecraft->levelRenderer) {
		minecraft->levelRenderer->allChanged();
	}
}

void ShadersScreen::buttonClicked(Button* button) {
	if (!button || !minecraft) return;

	if (button->id == m_btnOpenFolder.id) {
		openShaderpacksFolder();
		if (m_packList) {
			m_packList->refreshList();
		}
	} else if (button->id == m_btnDone.id) {
		minecraft->options.save();
		minecraft->setScreen(new OptionsScreen());
	}
}

void ShadersScreen::keyPressed(int eventKey) {
	if (eventKey == Keyboard::KEY_ESCAPE) {
		if (minecraft) {
			minecraft->options.save();
			minecraft->setScreen(new OptionsScreen());
		}
		return;
	}
	Screen::keyPressed(eventKey);
}

void ShadersScreen::mouseWheel(int dx, int dy, int xm, int ym) {
	if (m_packList && dy != 0) {
		m_packList->yo -= dy * 12;
		m_packList->capYPosition();
	}
}

void ShadersScreen::render(int xm, int ym, float a) {
	renderDirtBackground(0);

	if (m_packList) {
		m_packList->render(xm, ym, a);
	}

	Screen::render(xm, ym, a);

	std::string title = I18n::get("options.shaders");
	if (title.empty() || title == "options.shaders") {
		title = "Shaders";
	}
	drawCenteredString(font, title, width / 2, 14, 0xFFFFFFFF);
}
