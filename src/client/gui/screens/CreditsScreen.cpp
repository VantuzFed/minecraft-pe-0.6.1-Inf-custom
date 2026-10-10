#include "CreditsScreen.h"
#include "StartMenuScreen.h"
#include "OptionsScreen.h"
#include "../../Minecraft.h"
#include "../components/Button.h"
#include "../components/ImageButton.h"
#include "platform/input/Mouse.h"

#include "../../../util/Mth.h"
#include "../../../platform/input/Keyboard.h"

CreditsScreen::CreditsScreen()
: btnDone(NULL)
{}
CreditsScreen::~CreditsScreen() {
    if (btnDone) delete btnDone;
}

void CreditsScreen::init() {
    btnDone = new Button(1, "Done");
    buttons.push_back(btnDone);
    tabButtons.push_back(btnDone);

    // prepare text lines
    _lines.clear();
    _lines.push_back("Minecraft: Pocket Edition");
    _lines.push_back("Original game by Mojang");
    _lines.push_back("");
    _lines.push_back("Programmers:");
    _lines.push_back("mschiller890");
    _lines.push_back("InviseDivine");
    _lines.push_back("Kolyah35");
    _lines.push_back("karson");
    _lines.push_back("deepfriedwaffles");
    _lines.push_back("EpikIzCool");
	_lines.push_back("fileshredder");
    _lines.push_back("");
    // avoid color tags around the URL so it isn't mangled by the parser please
    _lines.push_back("Join our Discord server: https://discord.gg/c58YesBxve");
    _scrollSpeed = 0.5f;
    _scrollY = height; // start below screen
}

void CreditsScreen::setupPositions() {
    int doneW = Mth::Min(200, width - 20);
    btnDone->width = doneW;
    btnDone->height = 20;
    btnDone->x = (width - doneW) / 2;
    btnDone->y = height - 28;

    // reset scroll starting position when screen size changes
    _scrollY = height;
}

void CreditsScreen::keyPressed(int eventKey) {
    if (eventKey == Keyboard::KEY_ESCAPE) {
        minecraft->setScreen(new OptionsScreen());
        return;
    }
    Screen::keyPressed(eventKey);
}

void CreditsScreen::tick() {
    // move text upward
    _scrollY -= _scrollSpeed;
    // if text has scrolled off the top, restart
    float totalHeight = _lines.size() * (minecraft->font->lineHeight + 8);
    if (_scrollY + totalHeight < 0) {
        _scrollY = height;
    }

    if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
        _scrollSpeed = 1.5f;
    } else {
        _scrollSpeed = 0.5f;
    }
}

void CreditsScreen::render(int xm, int ym, float a) {
    renderBackground();
    int w = width;
    Font* font = minecraft->font;
    float y = _scrollY;
    const float lineHeight = font->lineHeight + 8;
    for (size_t i = 0; i < _lines.size(); ++i) {
        const std::string& line = _lines[i];
        // use color-tag-aware drawing, centre by total width
        float lineWidth = Gui::getColoredWidth(font, line);
        Gui::drawColoredString(font, line, w/2 - lineWidth/2, (int)y, 255);
        // underline hyperlink lines manually
        if (line.find("http") != std::string::npos || line.find("discord.gg") != std::string::npos) {
            float x0 = w/2 - lineWidth/2;
            float y0 = y + font->lineHeight - 1;
            this->fill(x0, y0, x0 + lineWidth, y0 + 1, 0xffffffff);
        }
        y += lineHeight;
    }
    
    super::render(xm, ym, a);
}

void CreditsScreen::buttonClicked(Button* button) {
    if (button->id == 1) {
        minecraft->setScreen(new OptionsScreen());
    }
}

void CreditsScreen::mouseClicked(int x, int y, int buttonNum) {
    // map click to a line in the scrolling text
    const float lineHeight = minecraft->font->lineHeight + 8;
    for (size_t i = 0; i < _lines.size(); ++i) {
        float lineY = _scrollY + i * lineHeight;
        if (y >= lineY && y < lineY + lineHeight) {
            const std::string& line = _lines[i];
            size_t start = line.find("http");
            if (start == std::string::npos)
                start = line.find("discord.gg");
            if (start != std::string::npos) {
                // extract until space
                size_t end = line.find(' ', start);
                std::string url = line.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
                minecraft->platform()->openURL(url);
                return;
            }
        }
    }
    super::mouseClicked(x, y, buttonNum);
}
