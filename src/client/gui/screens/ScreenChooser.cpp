#include "ScreenChooser.h"
#include "StartMenuScreen.h"
#include "SelectWorldScreen.h"
#include "JoinGameScreen.h"
#include "PauseScreen.h"
#include "RenameMPLevelScreen.h"
#include "ConsoleScreen.h"
#include "IngameBlockSelectionScreen.h"
#include "Beta18CreativeScreen.h"
#include "JoinByIPScreen.h"
#include "touch/TouchStartMenuScreen.h"
#include "touch/TouchSelectWorldScreen.h"
#include "touch/TouchJoinGameScreen.h"
#include "touch/TouchIngameBlockSelectionScreen.h"

#include "../../Minecraft.h"

#include "UsernameScreen.h"

Screen* ScreenChooser::createScreen( ScreenId id )
{
	Screen* screen = NULL;

	// :sob:
	if (_mc->options.getIntValue(OPTIONS_MENU_STYLE) == 0) {
		switch (id) {
		case SCREEN_STARTMENU:	     screen = new Touch::StartMenuScreen();	break;
		case SCREEN_SELECTWORLD:     screen = new Touch::SelectWorldScreen();break;
		case SCREEN_JOINGAME:	     screen = new Touch::JoinGameScreen();	break;
		case SCREEN_PAUSE:	         screen = new PauseScreen(false); break;
		case SCREEN_PAUSEPREV:	     screen = new PauseScreen(true);	 break;
		case SCREEN_BLOCKSELECTION:	 screen = new Touch::IngameBlockSelectionScreen();	break;
		case SCREEN_JOINBYIP:        screen = new JoinByIPScreen(); break;
		case SCREEN_CONSOLE:		 screen = new ConsoleScreen(); break;
		case SCREEN_NONE:
		default:
			// Do nothing
			break;
		}
	} else {
		switch (id) {
		case SCREEN_STARTMENU:	     screen = new StartMenuScreen();  break;
		case SCREEN_SELECTWORLD:     screen = new SelectWorldScreen();break;
		case SCREEN_JOINGAME:	     screen = new JoinGameScreen();   break;
		case SCREEN_PAUSE:	         screen = new PauseScreen(false); break;
		case SCREEN_PAUSEPREV:	     screen = new PauseScreen(true);	 break;
		// Desktop creative uses the Beta 1.8 item-selection screen.
		case SCREEN_BLOCKSELECTION:	 screen = new Beta18CreativeScreen();	break;
		case SCREEN_JOINBYIP:        screen = new JoinByIPScreen(); break;
		case SCREEN_CONSOLE:		 screen = new ConsoleScreen(); break;
		case SCREEN_NONE:
		default:
			// Do nothing
			break;
		}
	}
	return screen;
}

Screen* ScreenChooser::setScreen(ScreenId id)
{
	Screen* screen = createScreen(id);
	_mc->setScreen(screen);
	return screen;
}
