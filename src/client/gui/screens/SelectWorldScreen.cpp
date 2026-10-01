#include "SelectWorldScreen.h"
#include "StartMenuScreen.h"
#include "ProgressScreen.h"
#include "RenameMPLevelScreen.h"
#include "SimpleChooseLevelScreen.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/Textures.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/time.h"
#include "../../../locale/I18n.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../AppPlatform.h"
#include "../../../util/StringUtils.h"

#include <algorithm>
#include <set>
#include <sstream>

//
// World Selection List (Desktop Beta 1.7.3 vertical GuiSlot style)
//
WorldSelectionList::WorldSelectionList(Minecraft* minecraft, int width, int height)
:	RolledSelectionListV(minecraft, width, height, 0, width, 32, height - 64, 36),
	selectedItem(-1),
	lastClickedItem(-1),
	lastClickTime(0),
	hasPickedLevel(false)
{
	setRenderSelection(true);
}

int WorldSelectionList::getNumberOfItems() {
	return (int)levels.size();
}

void WorldSelectionList::selectItem(int item, bool doubleClick) {
	long long now = getTimeMs();
	bool isDouble = doubleClick || (selectedItem == item && (now - lastClickTime < 400));
	selectedItem = item;
	lastClickTime = now;
	lastClickedItem = item;

	if (isDouble && item >= 0 && item < (int)levels.size()) {
		if (!hasPickedLevel) {
			hasPickedLevel = true;
			pickedLevel = levels[item];
		}
	}
}

bool WorldSelectionList::isSelectedItem(int item) {
	return item == selectedItem;
}

void WorldSelectionList::renderItem(int i, int x, int y, int h, Tesselator& t) {
	if (i < 0 || i >= (int)levels.size()) return;

	int boxX = width / 2 - 110;
	int boxW = 220;

	if (isSelectedItem(i)) {
		glDisable2(GL_TEXTURE_2D);
		glColor4f2(1, 1, 1, 1);
		t.begin();
		t.color(0x80, 0x80, 0x80);
		t.vertex(boxX - 2, y + h + 2, 0);
		t.vertex(boxX + boxW + 2, y + h + 2, 0);
		t.vertex(boxX + boxW + 2, y - 2, 0);
		t.vertex(boxX - 2, y - 2, 0);

		t.color(0x00, 0x00, 0x00);
		t.vertex(boxX - 1, y + h + 1, 0);
		t.vertex(boxX + boxW + 1, y + h + 1, 0);
		t.vertex(boxX + boxW + 1, y - 1, 0);
		t.vertex(boxX - 1, y - 1, 0);
		t.draw();
		glEnable2(GL_TEXTURE_2D);
	}

	LevelSummary& level = levels[i];
	std::string name = level.name;
	if (name.empty()) {
		std::stringstream ss;
		ss << I18n::get("selectWorld.world") << " " << (i + 1);
		name = ss.str();
	}

	std::string info = level.id;
	std::string dateStr = minecraft->platform()->getDateString(level.lastPlayed);
	if (!dateStr.empty()) {
		info += " (" + dateStr + ")";
	}

	std::string mode = I18n::get("selectWorld.gameMode") + " " + LevelSettings::gameTypeToString(level.gameType);

	drawString(minecraft->font, name, boxX + 2, y + 1, 0xffffffff);
	drawString(minecraft->font, info, boxX + 2, y + 12, 0x808080ff);
	drawString(minecraft->font, mode, boxX + 2, y + 23, 0x808080ff);
}

void WorldSelectionList::commit() {
	if (!levels.empty()) {
		selectedItem = 0;
	} else {
		selectedItem = -1;
	}
}

//
// Select World Screen (Faithful Desktop Beta 1.7.3 GuiSelectWorld)
//
SelectWorldScreen::SelectWorldScreen()
:	bSelect (1, "Play Selected World"),
	bDelete (2, "Delete"),
	bCreate (3, "Create New World"),
	bRename (6, "Rename"),
	bCancel (0, "Cancel"),
	worldsList(NULL),
	_mouseHasBeenUp(false),
	_hasStartedLevel(false)
{
	bSelect.active = false;
	bRename.active = false;
	bDelete.active = false;
}

SelectWorldScreen::~SelectWorldScreen()
{
	delete worldsList;
}

void SelectWorldScreen::buttonClicked(Button* button)
{
	if (button->id == bSelect.id) {
		if (isIndexValid(worldsList->selectedItem)) {
			worldsList->hasPickedLevel = true;
			worldsList->pickedLevel = worldsList->levels[worldsList->selectedItem];
		}
	}
	else if (button->id == bCreate.id) {
		if (!_hasStartedLevel) {
			std::string name = getUniqueLevelName("World");
			minecraft->setScreen(new SimpleChooseLevelScreen(name));
		}
	}
	else if (button->id == bRename.id) {
		if (isIndexValid(worldsList->selectedItem)) {
			LevelSummary level = worldsList->levels[worldsList->selectedItem];
			minecraft->setScreen(new RenameMPLevelScreen(level.id));
		}
	}
	else if (button->id == bDelete.id) {
		if (isIndexValid(worldsList->selectedItem)) {
			LevelSummary level = worldsList->levels[worldsList->selectedItem];
			minecraft->setScreen(new DeleteWorldScreen(level));
		}
	}
	else if (button->id == bCancel.id) {
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	}
}

bool SelectWorldScreen::handleBackEvent(bool isDown)
{
	if (!isDown) {
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	}
	return true;
}

bool SelectWorldScreen::isIndexValid(int index)
{
	return worldsList && index >= 0 && index < worldsList->getNumberOfItems();
}

void SelectWorldScreen::tick()
{
	worldsList->tick();

	if (worldsList->hasPickedLevel) {
		if (!minecraft->selectLevel(worldsList->pickedLevel.id, worldsList->pickedLevel.name, LevelSettings::None())) {
			worldsList->hasPickedLevel = false;
			return;
		}
		minecraft->hostMultiplayer();
		minecraft->setScreen(new ProgressScreen());
		_hasStartedLevel = true;
		return;
	}

	bool hasSelection = isIndexValid(worldsList->selectedItem);
	bSelect.active = hasSelection;
	bRename.active = hasSelection;
	bDelete.active = hasSelection;
}

void SelectWorldScreen::init()
{
	worldsList = new WorldSelectionList(minecraft, width, height);
	loadLevelSource();
	worldsList->commit();

	bSelect.msg = I18n::get("selectWorld.select");
	bCreate.msg = I18n::get("selectWorld.create");
	bRename.msg = I18n::get("selectWorld.rename");
	bDelete.msg = I18n::get("selectWorld.delete");
	bCancel.msg = I18n::get("gui.cancel");

	buttons.clear();
	tabButtons.clear();

	buttons.push_back(&bSelect);
	buttons.push_back(&bCreate);
	buttons.push_back(&bRename);
	buttons.push_back(&bDelete);
	buttons.push_back(&bCancel);

	tabButtons.push_back(&bSelect);
	tabButtons.push_back(&bCreate);
	tabButtons.push_back(&bRename);
	tabButtons.push_back(&bDelete);
	tabButtons.push_back(&bCancel);

	_mouseHasBeenUp = !Mouse::getButtonState(MouseAction::ACTION_LEFT);
}

void SelectWorldScreen::setupPositions()
{
	int yRow1 = height - 52;
	int yRow2 = height - 28;

	bSelect.y = yRow1;
	bCreate.y = yRow1;
	bSelect.width = 150;
	bCreate.width = 150;
	bSelect.height = 20;
	bCreate.height = 20;
	bSelect.x = width / 2 - 154;
	bCreate.x = width / 2 + 4;

	bRename.y = yRow2;
	bDelete.y = yRow2;
	bCancel.y = yRow2;
	bRename.width = 98;
	bDelete.width = 98;
	bCancel.width = 98;
	bRename.height = 20;
	bDelete.height = 20;
	bCancel.height = 20;
	bRename.x = width / 2 - 154;
	bDelete.x = width / 2 - 49;
	bCancel.x = width / 2 + 56;
}

void SelectWorldScreen::render(int xm, int ym, float a)
{
	renderDirtBackground(0);

	if (_mouseHasBeenUp)
		worldsList->render(xm, ym, a);
	else {
		worldsList->render(0, 0, a);
		_mouseHasBeenUp = !Mouse::getButtonState(MouseAction::ACTION_LEFT);
	}

	Screen::render(xm, ym, a);

	drawCenteredString(minecraft->font, I18n::get("selectWorld.title"), width / 2, 16, 0xffffffff);
}

void SelectWorldScreen::loadLevelSource()
{
	LevelStorageSource* levelSource = minecraft->getLevelSource();
	levelSource->getLevelList(levels);
	std::sort(levels.begin(), levels.end());

	for (unsigned int i = 0; i < levels.size(); ++i) {
		if (levels[i].id != LevelStorageSource::TempLevelId)
			worldsList->levels.push_back(levels[i]);
	}
}

std::string SelectWorldScreen::getUniqueLevelName(const std::string& level)
{
	std::set<std::string> Set;
	for (unsigned int i = 0; i < levels.size(); ++i)
		Set.insert(levels[i].id);

	std::string s = level;
	while (Set.find(s) != Set.end())
		s += "-";
	return s;
}

bool SelectWorldScreen::isInGameScreen() {
	return true;
}

void SelectWorldScreen::mouseWheel(int dx, int dy, int xm, int ym)
{
	if (!worldsList || dy == 0)
		return;
	worldsList->yo -= dy * 12;
	worldsList->capYPosition();
}

void SelectWorldScreen::keyPressed(int eventKey)
{
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		return;
	}
	if (worldsList) {
		int count = worldsList->getNumberOfItems();
		if (eventKey == Keyboard::KEY_W) {
			if (worldsList->selectedItem > 0)
				worldsList->selectedItem--;
		} else if (eventKey == Keyboard::KEY_S) {
			if (worldsList->selectedItem < count - 1)
				worldsList->selectedItem++;
		} else if (eventKey == Keyboard::KEY_RETURN) {
			if (isIndexValid(worldsList->selectedItem)) {
				worldsList->hasPickedLevel = true;
				worldsList->pickedLevel = worldsList->levels[worldsList->selectedItem];
			}
		}
	}

	Screen::keyPressed(eventKey);
}

//
// Delete World Screen
//
DeleteWorldScreen::DeleteWorldScreen(const LevelSummary& level)
:	ConfirmScreen(NULL,
				  I18n::get("selectWorld.deleteQuestion"),
				  "'" + level.name + "' " + I18n::get("selectWorld.deleteWarning"),
				  I18n::get("selectWorld.deleteButton"),
				  I18n::get("gui.cancel"), 0),
	_level(level)
{
	tabButtonIndex = 1;
}

void DeleteWorldScreen::postResult(bool isOk)
{
	if (isOk) {
		LevelStorageSource* storageSource = minecraft->getLevelSource();
		storageSource->deleteLevel(_level.id);
	}
	minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
}
