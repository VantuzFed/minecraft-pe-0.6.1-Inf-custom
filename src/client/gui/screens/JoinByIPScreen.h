#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__JoinByIPScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__JoinByIPScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/TextBox.h"

class JoinByIPScreen: public Screen
{
public:
	JoinByIPScreen();
	virtual ~JoinByIPScreen();

	void init();
	void setupPositions();

	virtual void tick();
	void render(int xm, int ym, float a);

	virtual void keyPressed(int eventKey);
	void buttonClicked(Button* button);
	virtual bool handleBackEvent(bool isDown);
private:
	TextBox tIP;
	Button bJoin;
	Button bBack;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__JoinByIPScreen_H__*/
