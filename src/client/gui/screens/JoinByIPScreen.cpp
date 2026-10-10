#include "JoinByIPScreen.h"

#include "JoinGameScreen.h"
#include "StartMenuScreen.h"
#include "ProgressScreen.h"
#include "../Font.h"
#include "../../../network/RakNetInstance.h"
#include "client/Options.h"
#include "client/gui/Screen.h"
#include "client/gui/components/TextBox.h"
#include "network/ClientSideNetworkHandler.h"
#include "../../../platform/input/Keyboard.h"

JoinByIPScreen::JoinByIPScreen() :
	tIP(0, "Server IP"),
	bJoin(1, "Join Server"),
	bBack(2, "Cancel")
{
	bJoin.active = false;
}

JoinByIPScreen::~JoinByIPScreen()
{
}

void JoinByIPScreen::init()
{
	tIP.setMaxChars(64);

	buttons.clear();
	tabButtons.clear();
	textBoxes.clear();

	buttons.push_back(&bJoin);
	buttons.push_back(&bBack);

	tabButtons.push_back(&bJoin);
	tabButtons.push_back(&bBack);

	textBoxes.push_back(&tIP);

	tIP.text = minecraft->options.getStringValue(OPTIONS_LAST_IP);
	tIP.setFocus(minecraft);
}

void JoinByIPScreen::setupPositions() {
	tIP.width = 200;
	tIP.height = 20;
	tIP.x = (width - 200) / 2;
	tIP.y = height / 4 + 36;

	int btnW = 100;
	bJoin.width = bBack.width = btnW;
	bJoin.height = bBack.height = 20;

	bJoin.x = width / 2 - 104;
	bBack.x = width / 2 + 4;
	bJoin.y = bBack.y = height / 4 + 96 + 12;
}

void JoinByIPScreen::tick()
{
	Screen::tick();
	bJoin.active = !tIP.text.empty();
	for (auto* tb : textBoxes) {
		tb->tick(minecraft);
	}
}

void JoinByIPScreen::render( int xm, int ym, float a )
{
	renderDirtBackground(0);

	drawCenteredString(font, "Direct Connect", width / 2, height / 4 - 60 + 20, 0xffffffff);
	drawString(font, "Server Address", width / 2 - 100, height / 4 + 20, 0xffa0a0a0);

	Screen::render(xm, ym, a);
}

void JoinByIPScreen::buttonClicked(Button* button)
{
	if (button->id == bJoin.id)
	{
		minecraft->isLookingForMultiplayer = true;
		minecraft->netCallback = new ClientSideNetworkHandler(minecraft, minecraft->raknetInstance);

		minecraft->joinMultiplayerFromString(tIP.text);
		{
			minecraft->options.set(OPTIONS_LAST_IP, tIP.text);
			bJoin.active = false;
			bBack.active = false;
			minecraft->setScreen(new ProgressScreen());
		}
	}
	if (button->id == bBack.id)
	{
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
	}
}

bool JoinByIPScreen::handleBackEvent(bool isDown)
{
	if (!isDown)
	{
		minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
	}
	return true;
}

void JoinByIPScreen::keyPressed(int eventKey)
{
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
		return;
	}
	if (eventKey == Keyboard::KEY_RETURN && !tIP.text.empty()) {
		buttonClicked(&bJoin);
		return;
	}
	Screen::keyPressed(eventKey);
}