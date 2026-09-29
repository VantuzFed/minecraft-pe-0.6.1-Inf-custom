#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaFurnaceScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaFurnaceScreen_H__

#include "../Screen.h"
#include "../../../world/item/ItemInstance.h"

class Inventory;
class FurnaceTileEntity;

// Classic furnace UI after Beta 1.7.3: gui/furnace.png background
// (176x166), ingredient (56,17), fuel (56,53), result (116,35) with
// flame/arrow progress, player main rows + hotbar below.
class BetaFurnaceScreen : public Screen {
	typedef Screen super;
public:
	BetaFurnaceScreen(FurnaceTileEntity* furnace);
	virtual ~BetaFurnaceScreen() {}

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

	// 0 = ingredient, 1 = fuel, 2 = result, 3-29 = main (PE inv 9-35),
	// 30-38 = hotbar (PE hotbar links).
	static bool slotPos(int betaIdx, int& sx, int& sy);
	int slotAt(int x, int y) const;
	bool furnaceGone() const;

	Inventory* inv();
	ItemInstance* getSlotItem(int betaIdx);
	void setSlotItem(int betaIdx, const ItemInstance* item);
	int resolveHotbar(int betaIdx);
	int ensureHotbarLink(int betaIdx);
	int betaToPe(int betaIdx);
	bool isLinkedMain(int peSlot);
	bool quickTransfer(int betaIdx);
	bool mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse, int skipPe = -1);
	int spaceFor(const ItemInstance& stack, int from, int to);
	void spillCarried();
	void placeInto(int slot);
	void placeOneInto(int slot);

	int panelX() const { return (width - PANEL_W) / 2; }
	int panelY() const { return (height - PANEL_H) / 2; }

	FurnaceTileEntity* furnace;
	int fx, fy, fz;
	ItemInstance carried;
	bool hasCarried;

	bool pressed;
	int pressSlot;
	int pressButton;
	bool pressPickedUp;
};

#endif
