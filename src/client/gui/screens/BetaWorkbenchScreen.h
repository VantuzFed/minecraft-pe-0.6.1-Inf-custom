#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaWorkbenchScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaWorkbenchScreen_H__

#include "../Screen.h"
#include "../../../world/item/ItemInstance.h"

class Inventory;

// Classic workbench UI after the Beta 1.7.3 jar: gui/crafting.png
// background (176x166), 3x3 matrix at (30,17), result at (124,35),
// player main rows + hotbar. Same click model as BetaInventoryScreen.
class BetaWorkbenchScreen : public Screen {
	typedef Screen super;
public:
	BetaWorkbenchScreen();
	virtual ~BetaWorkbenchScreen() {}

	void init();
	void setupPositions();
	void render(int xm, int ym, float a);
	void tick();

	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);
	virtual void keyPressed(int eventKey);
	virtual void removed();
	virtual bool isPauseScreen() { return true; }

private:
	static const int PANEL_W = 176;
	static const int PANEL_H = 166;

	// 0 = result, 1-9 = 3x3 matrix, 10-36 = main (PE inv 9-35),
	// 37-45 = hotbar (PE hotbar links).
	static bool slotPos(int betaIdx, int& sx, int& sy);
	int slotAt(int x, int y) const;

	Inventory* inv();
	ItemInstance* getSlotItem(int betaIdx);
	void setSlotItem(int betaIdx, const ItemInstance* item);
	int betaToPe(int betaIdx);
	void updateCraftResult();
	void consumeMatrix();
	bool quickTransfer(int betaIdx);
	bool takeResultToCursor();
	bool takeResultToInventory();
	bool mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse, int skipPe = -1);
	int spaceFor(const ItemInstance& stack, int from, int to);
	void spillCarried();
	void placeInto(int slot);
	void placeOneInto(int slot);

	int panelX() const { return (width - PANEL_W) / 2; }
	int panelY() const { return (height - PANEL_H) / 2; }

	ItemInstance craftMatrix[9];
	ItemInstance craftResult;
	bool hasCraftResult;
	ItemInstance carried;
	bool hasCarried;

	bool pressed;
	int pressSlot;
	int pressButton;
	bool pressPickedUp;
};

#endif
