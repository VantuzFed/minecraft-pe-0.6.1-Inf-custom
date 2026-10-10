#include "SimpleChooseLevelScreen.h"
#include "ProgressScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../../world/level/LevelSettings.h"
#include "../../../SharedConstants.h"
#include "../../../platform/time.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/log.h"
#include "../../../util/Mth.h"
#include <cstdlib>

SimpleChooseLevelScreen::SimpleChooseLevelScreen(const std::string& levelName)
:   bGamemode(0),
    bCheats(0),
    bWorldType(0),
    bCreate(0),
    bCancel(0),
    levelName(levelName),
    hasChosen(false),
    gamemode(GameType::Survival),
    cheatsEnabled(false),
    worldType(LGV_ORIGINAL),
    tLevelName(0, "World name"),
    tSeed(1, "World seed")
{
}

SimpleChooseLevelScreen::~SimpleChooseLevelScreen()
{
    delete bGamemode;
    delete bCheats;
    delete bWorldType;
    delete bCreate;
    delete bCancel;
}

void SimpleChooseLevelScreen::init()
{
    ChooseLevelScreen::init();

    tLevelName.text = "New World";
    tLevelName.setMaxChars(32);
    tSeed.setMaxChars(64);

    bGamemode = new Button(1, "Survival mode");
    bCheats  = new Button(4, "Cheats: Off");
    bWorldType = new Button(5, "World: PE 0.6.1");
    bCreate  = new Button(3, "Create New World");
    bCancel  = new Button(2, "Cancel");

    buttons.clear();
    tabButtons.clear();
    textBoxes.clear();

    buttons.push_back(bGamemode);
    buttons.push_back(bCheats);
    buttons.push_back(bWorldType);
    buttons.push_back(bCreate);
    buttons.push_back(bCancel);

    tabButtons.push_back(bGamemode);
    tabButtons.push_back(bCheats);
    tabButtons.push_back(bWorldType);
    tabButtons.push_back(bCreate);
    tabButtons.push_back(bCancel);

    textBoxes.push_back(&tLevelName);
    textBoxes.push_back(&tSeed);

    tLevelName.setFocus(minecraft);
}

void SimpleChooseLevelScreen::setupPositions()
{
    int centerX = width / 2;

    tLevelName.width = tSeed.width = 200;
    tLevelName.height = tSeed.height = 20;
    tLevelName.x = tSeed.x = centerX - 100;

    tLevelName.y = 48;
    tSeed.y = 88;

    int optBtnW = 150;
    bGamemode->width = optBtnW;
    bGamemode->height = 20;
    bGamemode->x = centerX - optBtnW / 2;
    bGamemode->y = 126;

    int halfBtnW = 98;
    bCheats->width = bWorldType->width = halfBtnW;
    bCheats->height = bWorldType->height = 20;
    bCheats->x = centerX - halfBtnW - 4;
    bWorldType->x = centerX + 4;
    bCheats->y = bWorldType->y = 162;

    int bottomBtnW = 150;
    if (bottomBtnW * 2 + 10 > width) {
        bottomBtnW = (width - 24) / 2;
    }
    bCreate->width = bCancel->width = bottomBtnW;
    bCreate->height = bCancel->height = 20;
    bCreate->x = centerX - bottomBtnW - 5;
    bCancel->x = centerX + 5;
    bCreate->y = bCancel->y = height - 28;
}

void SimpleChooseLevelScreen::tick()
{
    for (auto* tb : textBoxes)
        tb->tick(minecraft);
}

void SimpleChooseLevelScreen::render( int xm, int ym, float a )
{
    renderDirtBackground(0);
    glEnable2(GL_BLEND);

    drawCenteredString(minecraft->font, "Create New World", width / 2, 14, 0xffffffff);

    drawString(minecraft->font, "World Name:", tLevelName.x, tLevelName.y - 11, 0xffa0a0a0);
    drawString(minecraft->font, "Seed for World Generator:", tSeed.x, tSeed.y - 11, 0xffa0a0a0);

    const char* modeDesc = (gamemode == GameType::Survival)
        ? "Survival mode: Mobs, health and gather resources"
        : "Creative mode: Unlimited resources and flying";
    drawCenteredString(minecraft->font, modeDesc, width / 2, bGamemode->y + 22, 0xff808080);

    Screen::render(xm, ym, a);
    glDisable2(GL_BLEND);
}

void SimpleChooseLevelScreen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum == MouseAction::ACTION_LEFT) {
        int lvlTop = tLevelName.y - 12;
        int lvlBottom = tLevelName.y + tLevelName.height;
        int lvlLeft = tLevelName.x;
        int lvlRight = tLevelName.x + tLevelName.width;
        bool clickedLevel = x >= lvlLeft && x < lvlRight && y >= lvlTop && y < lvlBottom;

        int seedTop = tSeed.y - 12;
        int seedBottom = tSeed.y + tSeed.height;
        int seedLeft = tSeed.x;
        int seedRight = tSeed.x + tSeed.width;
        bool clickedSeed = x >= seedLeft && x < seedRight && y >= seedTop && y < seedBottom;

        if (clickedLevel) {
            tLevelName.setFocus(minecraft);
            tSeed.loseFocus(minecraft);
        } else if (clickedSeed) {
            tSeed.setFocus(minecraft);
            tLevelName.loseFocus(minecraft);
        } else {
            tLevelName.loseFocus(minecraft);
            tSeed.loseFocus(minecraft);
        }
    }

    Screen::mouseClicked(x, y, buttonNum);
}

void SimpleChooseLevelScreen::buttonClicked( Button* button )
{
    if (hasChosen)
        return;

    if (button == bGamemode) {
        gamemode ^= 1;
        bGamemode->msg = (gamemode == GameType::Survival) ? "Survival mode" : "Creative mode";
        return;
    }

    if (button == bCheats) {
        cheatsEnabled = !cheatsEnabled;
        bCheats->msg = cheatsEnabled ? "Cheats: On" : "Cheats: Off";
        return;
    }

    if (button == bWorldType) {
        if (worldType == LGV_ORIGINAL)
            worldType = LGV_BETA173;
        else if (worldType == LGV_BETA173)
            worldType = LGV_ALPHA112;
        else
            worldType = LGV_ORIGINAL;
        bWorldType->msg = (worldType == LGV_BETA173) ? "World: Beta 1.7.3"
            : ((worldType == LGV_ALPHA112) ? "World: Alpha 1.1.2" : "World: PE 0.6.1");
        return;
    }

    if (button == bCreate && !tLevelName.text.empty()) {
        int64_t seed = getEpochTimeS();
        if (!tSeed.text.empty()) {
            std::string seedString = Util::stringTrim(tSeed.text);
            char* end = NULL;
            long long parsed = strtoll(seedString.c_str(), &end, 10);
            if (end != seedString.c_str() && *end == '\0') {
                seed = (int64_t)parsed;
            } else {
                seed = (int64_t)Util::hashCode(seedString);
            }
        }
        std::string levelId = getUniqueLevelName(tLevelName.text);
        LevelSettings settings((long)seed, gamemode, cheatsEnabled);
        minecraft->selectLevel(levelId, levelId, settings, worldType);
        minecraft->hostMultiplayer();
        minecraft->setScreen(new ProgressScreen());
        hasChosen = true;
        return;
    }

    if (button == bCancel) {
        minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
    }
}

void SimpleChooseLevelScreen::keyPressed(int eventKey)
{
    if (eventKey == Keyboard::KEY_ESCAPE) {
        minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
        return;
    }
    if (eventKey == Keyboard::KEY_RETURN && !tLevelName.text.empty()) {
        buttonClicked(bCreate);
        return;
    }
    Screen::keyPressed(eventKey);
}

bool SimpleChooseLevelScreen::handleBackEvent(bool isDown) {
	if (!isDown)
		minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
	return true; 
}
