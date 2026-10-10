#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/OptionsGroup.h"
#include "../components/BetaSlider.h"

class ImageButton;
class OptionsPane;

class OptionsScreen: public Screen
{
	typedef Screen super;

	void init();
	void generateOptionScreens();

public:
	OptionsScreen();
	~OptionsScreen();

	void setupPositions();
	void buttonClicked(Button* button);
	void applyVisualPreset(bool beta);
	void refreshOptions();
	void render(int xm, int ym, float a);
	void removed();
	void selectCategory(int index);

	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);
	virtual void mouseWheel(int dx, int dy, int xm, int ym) override;
	virtual void keyPressed(int eventKey);
	virtual void charPressed(char inputChar);
	
	virtual void tick();

private:
	Touch::THeader* bHeader;
	ImageButton* btnClose;

	Button* btnCredits;   // <-- ADD THIS

	std::vector<Touch::TButton*> categoryButtons;
	std::vector<OptionsGroup*> optionPanes;

	OptionsGroup* currentOptionsGroup;

	int selectedCategory;

	// Set when a preset button fires: the option groups are rebuilt on
	// the next tick, never from inside event dispatch (the firing button
	// itself lives in the group being rebuilt).
	bool m_pendingOptionsRefresh;

	bool isBetaStyle() const;
	void updateBetaButtonTexts();

	// Beta style buttons
	BetaSlider sMusic;
	BetaSlider sSound;
	Button bInvertMouse;
	BetaSlider sSensitivity;
	BetaSlider sFOV;
	Button bRenderDistance;
	Button bViewBobbing;
	Button bFramerate;
	Button b3DAnaglyph;
	Button bDifficulty;
	Button bGraphics;
	Button bSmoothLighting;
	Button bShaders;
	Button bGuiScale;
	Button bControls;
	Button bAutoJump;
	Button bCreditsBeta;
	Button bDone;

	// drag-to-scroll state (mouse + touch, touch emulates left button)
	bool m_dragActive;
	bool m_dragScrolling;
	int m_dragStartY;
	int m_dragStartScroll;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__*/
