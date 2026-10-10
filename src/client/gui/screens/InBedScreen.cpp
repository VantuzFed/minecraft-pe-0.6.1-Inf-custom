#include "InBedScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../platform/time.h"

static const int WAIT_TICKS = 30;

InBedScreen::InBedScreen()
	: bWakeUp(0)
{
}

InBedScreen::~InBedScreen() {
	delete bWakeUp;
}

void InBedScreen::init() {
	bWakeUp = new Button(1, "Leave Bed");
	buttons.push_back(bWakeUp);
	tabButtons.push_back(bWakeUp);
}

void InBedScreen::setupPositions() {
	bWakeUp->width = Mth::Min(200, width - 20);
	bWakeUp->height = 20;
	bWakeUp->x = (width - bWakeUp->width) / 2;
	bWakeUp->y = height / 4 + 100;
	if (bWakeUp->y > height - 30) bWakeUp->y = height - 30;
}

void InBedScreen::render( int xm, int ym, float a ) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Screen::render(xm, ym, a);
	glDisable(GL_BLEND);
}

void InBedScreen::buttonClicked( Button* button ) {
	if (button == bWakeUp) {
		minecraft->player->stopSleepInBed(true, true, true);
		minecraft->setScreen(NULL);
	}
}
