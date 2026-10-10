#include "PauseScreen.h"
#include "StartMenuScreen.h"
#include "../components/ImageButton.h"
#include "../../Minecraft.h"
#include "../../../util/Mth.h"
#include "../../../network/RakNetInstance.h"
#include "../../../network/ServerSideNetworkHandler.h"
#include "client/Options.h"
#include "client/gui/components/Button.h"
#include "client/gui/screens/OptionsScreen.h"
#include "../../../world/level/Level.h"
#include "../../../world/level/chunk/ChunkSource.h"

PauseScreen::PauseScreen(bool wasBackPaused)
	:	saveStep(0),
	visibleTime(0),
	bContinue(0),
	bQuit(0),
	bOptions(0),
	bQuitAndSaveLocally(0),
	bServerVisibility(0),
	wasBackPaused(wasBackPaused)
{
}

PauseScreen::~PauseScreen() {
	delete bContinue;
	delete bQuit;
	delete bQuitAndSaveLocally;
	delete bServerVisibility;
	delete bOptions;
}

void PauseScreen::init() {
	bContinue = new Button(1, 0, 0, 200, 20, "Back to game");
	bServerVisibility = new Button(4, 0, 0, 200, 20, "");
	bOptions = new Button(5, 0, 0, 200, 20, "Options...");
	bQuit = new Button(2, 0, 0, 200, 20, "Save and quit to title");
	bQuitAndSaveLocally = new Button(3, 0, 0, 200, 20, "Copy and quit map");

	buttons.clear();
	tabButtons.clear();

	buttons.push_back(bContinue);

#if !defined(APPLE_DEMO_PROMOTION) && !defined(RPI)
	if (minecraft->raknetInstance) {
		if (minecraft->raknetInstance->isServer()) {
			updateServerVisibilityText();
			buttons.push_back(bServerVisibility);
		}
		else {
#if !defined(DEMO_MODE)
			buttons.push_back(bQuitAndSaveLocally);
#endif
		}
	}
#endif

	buttons.push_back(bOptions);
	buttons.push_back(bQuit);

	for (size_t i = 0; i < buttons.size(); ++i) {
		tabButtons.push_back(buttons[i]);
	}

	if (minecraft && minecraft->level && minecraft->level->getChunkSource()) {
		minecraft->level->saveGame();
		minecraft->level->getChunkSource()->saveAll(true);
	}
}

void PauseScreen::setupPositions() {
	saveStep = 0;
	int btnW = Mth::Min(200, width - 20);

	bContinue->width = bOptions->width = bQuit->width = btnW;
	bQuitAndSaveLocally->width = bServerVisibility->width = btnW;

	bool hasServerVis = (minecraft->raknetInstance && minecraft->raknetInstance->isServer());
	bool hasQuitSave = (minecraft->raknetInstance && !minecraft->raknetInstance->isServer());

	int totalButtons = 3 + ((hasServerVis || hasQuitSave) ? 1 : 0);
	int startY = height / 4 + 8;
	if (startY + totalButtons * 24 > height - 10) {
		startY = (height - totalButtons * 24) / 2;
	}

	int row = 0;
	bContinue->x = (width - btnW) / 2;
	bContinue->y = startY + (row++) * 24;

	if (hasServerVis) {
		bServerVisibility->x = (width - btnW) / 2;
		bServerVisibility->y = startY + (row++) * 24;
	} else if (hasQuitSave) {
		bQuitAndSaveLocally->x = (width - btnW) / 2;
		bQuitAndSaveLocally->y = startY + (row++) * 24;
	}

	bOptions->x = (width - btnW) / 2;
	bOptions->y = startY + (row++) * 24;

	bQuit->x = (width - btnW) / 2;
	bQuit->y = startY + (row++) * 24;
}

void PauseScreen::tick() {
	super::tick();
	visibleTime++;
}

void PauseScreen::render(int xm, int ym, float a) {
	renderBackground();

	//bool isSaving = !minecraft->level.pauseSave(saveStep++);
	//if (isSaving || visibleTime < 20) {
	//	float col = ((visibleTime % 10) + a) / 10.0f;
	//	col = Mth::sin(col * Mth::PI * 2) * 0.2f + 0.8f;
	//	int br = (int) (255 * col);

	//	drawString(font, "Saving level..", 8, height - 16, br << 16 | br << 8 | br);
	//}

	drawCenteredString(font, "Game menu", width / 2, 24, 0xffffff);

	super::render(xm, ym, a);
}

void PauseScreen::buttonClicked(Button* button) {
	if (button->id == bContinue->id) {
		minecraft->setScreen(NULL);
		//minecraft->grabMouse();
	}
	if (button->id == bQuit->id) {
		minecraft->leaveGame();
	}
	if (button->id == bQuitAndSaveLocally->id) {
		minecraft->leaveGame(true);
	}
	if (button->id == bOptions->id) {
		minecraft->setScreen(new OptionsScreen());
	}
	if (button->id == bServerVisibility->id) {
		if (minecraft->raknetInstance && minecraft->netCallback && minecraft->raknetInstance->isServer()) {
			ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) minecraft->netCallback;
			bool allows = !ss->allowsIncomingConnections();
			ss->allowIncomingConnections(allows);

			updateServerVisibilityText();
		}
	}


}

void PauseScreen::updateServerVisibilityText()
{
	if (!minecraft->raknetInstance || !minecraft->raknetInstance->isServer())
		return;

	ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) minecraft->netCallback;
	bServerVisibility->msg = ss->allowsIncomingConnections()?
		"Server is visible"
		:   "Server is invisible";
}
