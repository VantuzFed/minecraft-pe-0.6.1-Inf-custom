#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ControlsScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ControlsScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include <vector>
#include <string>

class ControlsScreen : public Screen {
public:
	ControlsScreen(Screen* parent);
	virtual ~ControlsScreen();

	virtual void init() override;
	virtual void setupPositions() override;
	virtual void render(int xm, int ym, float a) override;
	virtual void buttonClicked(Button* button) override;
	virtual void keyPressed(int eventKey) override;

private:
	Screen* m_parent;
	Button m_btnDone;
	Button m_btnAutoJump;
	Button m_btnInvertMouse;
	int m_selectedKeyOpt;

	struct KeyBindingEntry {
		int optId;
		std::string labelKey;
		Button* button;
	};
	std::vector<KeyBindingEntry> m_bindings;

	void updateButtonTexts();
};

#endif /* NET_MINECRAFT_CLIENT_GUI_SCREENS__ControlsScreen_H__ */
