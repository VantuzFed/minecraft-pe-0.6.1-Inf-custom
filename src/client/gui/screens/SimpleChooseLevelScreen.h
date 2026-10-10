#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__

#include "ChooseLevelScreen.h"
#include "../components/TextBox.h"
#include "../components/Button.h"

class SimpleChooseLevelScreen: public ChooseLevelScreen
{
public:
	SimpleChooseLevelScreen(const std::string& levelName);

	virtual ~SimpleChooseLevelScreen();

	void init();
	void setupPositions();
	void tick();

	void render(int xm, int ym, float a);

	void buttonClicked(Button* button);
	bool handleBackEvent(bool isDown);
	virtual void keyPressed(int eventKey);
	virtual void mouseClicked(int x, int y, int buttonNum);

private:
	Button* bGamemode;
	Button* bCheats;
	Button* bWorldType;
	Button* bCreate;
	Button* bCancel;
	bool hasChosen;

	std::string levelName;
	int gamemode;
	bool cheatsEnabled;
	int worldType;

	TextBox tLevelName;
	TextBox tSeed;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__DemoChooseLevelScreen_H__*/
