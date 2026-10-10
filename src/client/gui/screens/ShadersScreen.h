#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ShadersScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ShadersScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/RolledSelectionListV.h"
#include <string>
#include <vector>

class ShadersScreen;

class ShaderPackList : public RolledSelectionListV {
public:
	ShaderPackList(ShadersScreen* screen, Minecraft* minecraft, int width, int height);
	virtual ~ShaderPackList() {}

	void refreshList();
	const std::string& getSelectedPackName() const;

protected:
	virtual int getNumberOfItems() override;
	virtual void selectItem(int item, bool doubleClick) override;
	virtual bool isSelectedItem(int item) override;
	virtual void renderBackground() override {}
	virtual void renderItem(int i, int x, int y, int h, Tesselator& t) override;

private:
	ShadersScreen* m_screen;
	std::vector<std::string> m_packs;
	int m_selectedIndex;

	friend class ShadersScreen;
};

class ShadersScreen : public Screen {
public:
	ShadersScreen();
	virtual ~ShadersScreen();

	virtual void init() override;
	virtual void setupPositions() override;
	virtual void render(int xm, int ym, float a) override;
	virtual void buttonClicked(Button* button) override;
	virtual void keyPressed(int eventKey) override;
	virtual void mouseWheel(int dx, int dy, int xm, int ym) override;
	virtual void mouseClicked(int x, int y, int buttonNum) override;
	virtual void tick() override;

	void applySelectedPack(const std::string& packName);

private:
	ShaderPackList* m_packList;
	bool m_mouseHasBeenUp;

	Button m_btnOpenFolder;
	Button m_btnDone;
};

#endif /* NET_MINECRAFT_CLIENT_GUI_SCREENS__ShadersScreen_H__ */
