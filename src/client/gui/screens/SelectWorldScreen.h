#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__SelectWorldScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__SelectWorldScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/RolledSelectionListV.h"
#include "../../Minecraft.h"
#include "../../../world/level/storage/LevelStorageSource.h"
#include "ConfirmScreen.h"

class SelectWorldScreen;

//
// Desktop Beta 1.7.3 vertical World selection list
//
class WorldSelectionList : public RolledSelectionListV
{
public:
	WorldSelectionList(Minecraft* _minecraft, int _width, int _height);
	virtual ~WorldSelectionList() {}

	void commit();

protected:
	virtual int getNumberOfItems();
	virtual void selectItem(int item, bool doubleClick);
	virtual bool isSelectedItem(int item);

	virtual void renderBackground() {}
	virtual void renderItem(int i, int x, int y, int h, Tesselator& t);

private:
	int selectedItem;
	int lastClickedItem;
	long long lastClickTime;

	LevelSummaryList levels;

	bool hasPickedLevel;
	LevelSummary pickedLevel;

	friend class SelectWorldScreen;
};

//
// Delete World screen
//
class DeleteWorldScreen: public ConfirmScreen
{
public:
	DeleteWorldScreen(const LevelSummary& levelId);
protected:
	virtual void postResult(bool isOk);
private:
	LevelSummary _level;
};

//
// Select world screen (Desktop Beta 1.7.3 style)
//
class SelectWorldScreen: public Screen
{
public:
	SelectWorldScreen();
	virtual ~SelectWorldScreen();

	virtual void init();
	virtual void setupPositions();
	virtual void tick();

	virtual bool isIndexValid(int index);
	virtual bool handleBackEvent(bool isDown);
	virtual void buttonClicked(Button* button);
	virtual void keyPressed(int eventKey);

	void render(int xm, int ym, float a);
	virtual void mouseWheel(int dx, int dy, int xm, int ym);

	bool isInGameScreen();
private:
	void loadLevelSource();
	std::string getUniqueLevelName(const std::string& level);

	Button bSelect;
	Button bCreate;
	Button bRename;
	Button bDelete;
	Button bCancel;

	WorldSelectionList* worldsList;
	LevelSummaryList levels;

	bool _mouseHasBeenUp;
	bool _hasStartedLevel;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__SelectWorldScreen_H__*/
