#include "ConsoleScreen.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../world/level/Level.h"
#include "../../../network/RakNetInstance.h"
#include "../../../network/ServerSideNetworkHandler.h"
#include "../../../network/packet/ChatPacket.h"
#include "../../../platform/log.h"

#include <sstream>
#include <cstdlib>
#include <cctype>

ConsoleScreen::ConsoleScreen(const std::string& initialInput /*= ""*/)
:   _input(initialInput),
    _cursorBlink(0)
{
}

void ConsoleScreen::init()
{
}

void ConsoleScreen::tick()
{
    _cursorBlink++;
}

bool ConsoleScreen::handleBackEvent(bool /*isDown*/)
{
    minecraft->setScreen(NULL);
    return true;
}

void ConsoleScreen::keyPressed(int eventKey)
{
    if (eventKey == Keyboard::KEY_ESCAPE) {
        minecraft->setScreen(NULL);
    } else if (eventKey == Keyboard::KEY_RETURN) {
        execute();
    } else if (eventKey == Keyboard::KEY_BACKSPACE) {
        if (!_input.empty())
            TextBox::popInputChar(_input);
        _pendingLead = 0;
    } else {
        super::keyPressed(eventKey);
    }
}

void ConsoleScreen::mouseClicked(int x, int y, int buttonNum)
{
    // Clicking a clickable chat line (e.g. the /seed output) copies it.
    // Console has no buttons, so a consumed click ends here.
    if (buttonNum == MouseAction::ACTION_LEFT && minecraft->gui.chatClickCopy(x, y))
        return;
    super::mouseClicked(x, y, buttonNum);
}

void ConsoleScreen::charPressed(char inputChar)
{
    // Chat/commands: ASCII plus renderable Cyrillic, capped at the
    // protocol chat length (in characters).
    TextBox::appendInputByte(_input, _pendingLead, 100, (unsigned char)inputChar);
}

// ---------------------------------------------------------------------------
// execute: run _input as a command, print result, close screen
// ---------------------------------------------------------------------------
void ConsoleScreen::execute()
{
    if (_input.empty()) {
        minecraft->setScreen(NULL);
        return;
    }

    try {
        if (_input[0] == '/') {
            // Command
            std::string result = processCommand(_input);
            if (!result.empty())
                minecraft->gui.addMessage(result);
        } else {
            // Chat message: <name> message
            std::string msg = std::string("<") + minecraft->player->name + "> " + _input;
            if (minecraft->netCallback && minecraft->raknetInstance->isServer()) {
                // Hosting a LAN game: displayGameMessage shows locally + broadcasts MessagePacket to clients
                static_cast<ServerSideNetworkHandler*>(minecraft->netCallback)->displayGameMessage(msg);
            } else if (minecraft->netCallback) {
                // Connected client: send ChatPacket to server; server echoes it back as MessagePacket
                ChatPacket chatPkt(msg);
                minecraft->raknetInstance->send(chatPkt);
            } else {
                // Singleplayer: show locally only
                minecraft->gui.addMessage(msg);
            }
        }
    } catch (const std::exception& e) {
        LOGE("Error in ConsoleScreen::execute: %s", e.what());
        minecraft->gui.addMessage(std::string("Error: ") + e.what());
    } catch (...) {
        LOGE("Unknown error in ConsoleScreen::execute");
        minecraft->gui.addMessage("An unknown error occurred while executing command.");
    }

    minecraft->setScreen(NULL);
}

// ---------------------------------------------------------------------------
// processCommand
// ---------------------------------------------------------------------------
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

std::string ConsoleScreen::processCommand(const std::string& raw)
{
    // strip leading '/'
    std::string line = raw;
    if (!line.empty() && line[0] == '/') line = line.substr(1);
    line = trim(line);

    // tokenise
    std::vector<std::string> args;
    {
        std::istringstream ss(line);
        std::string tok;
        while (ss >> tok) args.push_back(tok);
    }

    if (args.empty()) return "";

    Level* level = minecraft->level;
    if (!level) return "No level loaded.";

    // -----------------------------------------------------------------------
    // /time ...
    // -----------------------------------------------------------------------
    if (args[0] == "time") {
        if (args.size() < 2)
            return "Usage: /time <add|set|query> ...";

        const std::string& sub = args[1];

        // -- time add <value> -----------------------------------------------
        if (sub == "add") {
            if (args.size() < 3) return "Usage: /time add <value>";
            long delta = std::atol(args[2].c_str());
            long newTime = level->getTime() + delta;
            level->setTime(newTime);
            std::ostringstream out;
            out << "Set the time to " << (newTime % Level::TICKS_PER_DAY);
            return out.str();
        }

        // -- time set <value|day|night|noon|midnight> -----------------------
        if (sub == "set") {
            if (args.size() < 3) return "Usage: /time set <value|day|night|noon|midnight>";
            const std::string& val = args[2];

            long t = -1;
            if      (val == "day")      t = 1000;
            else if (val == "noon")     t = 6000;
            else if (val == "night")    t = 13000;
            else if (val == "midnight") t = 18000;
            else {
                // numeric — accept positive integers only
                bool numeric = true;
                for (char c : val)
                    if (!std::isdigit((unsigned char)c)) { numeric = false; break; }
                if (!numeric) return std::string("Unknown value: ") + val;
                t = std::atol(val.c_str());
            }

            // Preserve the total day count so only the time-of-day changes
            long dayCount = level->getTime() / Level::TICKS_PER_DAY;
            long newTime  = dayCount * Level::TICKS_PER_DAY + (t % Level::TICKS_PER_DAY);
            level->setTime(newTime);
            std::ostringstream out;
            out << "Set the time to " << t;
            return out.str();
        }

        // -- time query <daytime|gametime|day> ------------------------------
        if (sub == "query") {
            if (args.size() < 3) return "Usage: /time query <daytime|gametime|day>";
            const std::string& what = args[2];

            long total   = level->getTime();
            long daytime = total % Level::TICKS_PER_DAY;
            long day     = total / Level::TICKS_PER_DAY;

            std::ostringstream out;
            if      (what == "daytime")  { out << "The time of day is " << daytime; }
            else if (what == "gametime") { out << "The game time is "   << total;   }
            else if (what == "day")      { out << "The day is "         << day;     }
            else return std::string("Unknown query: ") + what;
            return out.str();
        }

        return "Unknown sub-command. Usage: /time <add|set|query> ...";
    }

    // -----------------------------------------------------------------------
    // /tp <x> <y> <z>
    // -----------------------------------------------------------------------
    if (args[0] == "tp") {
        if (args.size() != 4)
            return "Usage: /tp <x> <y> <z>";
    
        if (!minecraft->player)
            return "No player loaded.";
    
        char* endX = NULL;
        char* endY = NULL;
        char* endZ = NULL;
    
        double x = std::strtod(args[1].c_str(), &endX);
        double y = std::strtod(args[2].c_str(), &endY);
        double z = std::strtod(args[3].c_str(), &endZ);
    
        if (*endX != '\0' || *endY != '\0' || *endZ != '\0')
            return "Usage: /tp <x> <y> <z>";
    
        minecraft->player->moveTo(
            x,
            y,
            z,
            minecraft->player->yRot,
            minecraft->player->xRot
        );
    
        std::ostringstream out;
        out << "Teleported player to "
            << x << " " << y << " " << z;
        return out.str();
    }

    // -----------------------------------------------------------------------
    // /gamemode <survival|creative|0|1|s|c>
    // -----------------------------------------------------------------------
    if (args[0] == "gamemode" || args[0] == "defaultgamemode") {
        if (args.size() < 2) {
            return "Usage: /gamemode <survival|creative|0|1|s|c>";
        }

        std::string mode = args[1];
        std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

        bool isCreative = false;
        std::string modeName;

        if (mode == "0" || mode == "s" || mode == "survival") {
            isCreative = false;
            modeName = "Survival";
        } else if (mode == "1" || mode == "c" || mode == "creative") {
            isCreative = true;
            modeName = "Creative";
        } else {
            return "Unknown game mode '" + args[1] + "'. Usage: /gamemode <survival|creative|0|1|s|c>";
        }

        minecraft->setIsCreativeMode(isCreative);
        if (level->getLevelData()) {
            level->getLevelData()->setGameType(isCreative ? GameType::Creative : GameType::Survival);
        }

        return "Your game mode has been updated to " + modeName + " Mode";
    }

    // -----------------------------------------------------------------------
    // /seed — print the world seed; clicking the line copies it.
    // -----------------------------------------------------------------------
    if (args[0] == "seed") {
        std::ostringstream out;
        out << level->getSeed();
        std::string seedText = out.str();
        minecraft->platform()->setClipboardText(seedText);
        minecraft->gui.addClickableMessage("Seed: " + seedText + " (copied to clipboard)", seedText);
        return "";
    }

    return std::string("Unknown command: /") + args[0];
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------
void ConsoleScreen::render(int /*xm*/, int /*ym*/, float /*a*/)
{
    // Dim the game world slightly
    fillGradient(0, 0, width, height, 0x00000000, 0x40000000);

    const int boxH  = 12;
    const int boxY  = height - boxH - 2;
    const int boxX0 = 2;
    const int boxX1 = width - 2;

    // Input box background
    fill(boxX0, boxY, boxX1, boxY + boxH, 0xc0000000);
    // Border
    fill(boxX0,     boxY,          boxX1, boxY + 1,   0xff808080);
    fill(boxX0,     boxY + boxH - 1, boxX1, boxY + boxH, 0xff808080);
    fill(boxX0,     boxY,          boxX0 + 1, boxY + boxH, 0xff808080);
    fill(boxX1 - 1, boxY,          boxX1,     boxY + boxH, 0xff808080);

    // Input text + blinking cursor
    std::string displayed = _input;
    if ((_cursorBlink / 10) % 2 == 0)
        displayed += '_';

    // Placeholder hint when empty
    font->drawShadow(displayed, (float)(boxX0 + 2), (float)(boxY + 2), 0xffffffff);
}
