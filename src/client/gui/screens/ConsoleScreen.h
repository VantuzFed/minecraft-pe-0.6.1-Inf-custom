#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ConsoleScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ConsoleScreen_H__

#include "../Screen.h"
#include "../components/TextBox.h"
#include <string>

class ConsoleScreen: public Screen
{
    typedef Screen super;
public:
    ConsoleScreen(const std::string& initialInput = "");
    virtual ~ConsoleScreen() {}

    void init();
    void render(int xm, int ym, float a);
    void tick();

    virtual bool renderGameBehind() { return true; }
    virtual bool isInGameScreen()   { return true; }
    virtual bool isPauseScreen()    { return true; }

    virtual void keyPressed(int eventKey);
    virtual void charPressed(char inputChar);
    virtual bool handleBackEvent(bool isDown);
    virtual void mouseClicked(int x, int y, int buttonNum);

private:
    void execute();
    std::string processCommand(const std::string& cmd);

    std::string _input;
    unsigned char _pendingLead = 0; // incomplete UTF-8 lead byte
    int         _cursorBlink; // tick counter for cursor blink
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ConsoleScreen_H__*/
