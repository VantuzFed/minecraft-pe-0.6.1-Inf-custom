#include "DeathScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../platform/time.h"

static const int WAIT_TICKS = 30;

DeathScreen::DeathScreen()
:	bRespawn(0),
	bTitle(0),
	_hasChosen(false),
	_tick(0)
{
}

DeathScreen::~DeathScreen()
{
	delete bRespawn;
	delete bTitle;
}

void DeathScreen::init()
{
	bRespawn = new Button(1, 0, 0, 200, 20, "Respawn");
	bTitle = new Button(2, 0, 0, 200, 20, "Title menu");

	buttons.push_back(bRespawn);
	buttons.push_back(bTitle);

	tabButtons.push_back(bRespawn);
	tabButtons.push_back(bTitle);
}

void DeathScreen::setupPositions()
{
    int btnW = Mth::Min(200, width - 20);
    bRespawn->width = btnW;
    bTitle->width = btnW;

    int centerX = (width - btnW) / 2;
    bRespawn->x = centerX;
    bTitle->x = centerX;

    bRespawn->y = (height / 2);
    bTitle->y = bRespawn->y + 24;
}

void DeathScreen::tick() {
	++_tick;
}

void DeathScreen::render( int xm, int ym, float a )
{
	fillGradient(0, 0, width, height, 0x60500000, 0xa0803030);

	glPushMatrix2();
	glScalef2(2, 2, 2);
	drawCenteredString(font, "Game over!", width / 2 / 2, height / 8, 0xffffff);
	glPopMatrix2();
	std::stringstream ss;
	ss << "Score: &e" << minecraft->player->getScore();
	drawCenteredString(font, ss.str(), width / 2, (height / 4) + 32, 0xffffff);

	if (_tick >= WAIT_TICKS)
		Screen::render(xm, ym, a);
}

void DeathScreen::buttonClicked( Button* button )
{
	if (_tick < WAIT_TICKS) return;

	if (button == bRespawn) {
		//RespawnPacket packet();
		//minecraft->raknetInstance->send(packet);

		minecraft->player->respawn();
		//minecraft->raknetInstance->send();
		minecraft->setScreen(NULL);
	}

	if (button == bTitle)
		minecraft->leaveGame();
}
