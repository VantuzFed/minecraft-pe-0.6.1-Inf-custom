#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaChestScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaChestScreen_H__

#include "../Screen.h"
#include "../../../world/item/ItemInstance.h"

class Container;
class Inventory;

// Classic chest UI matching Desktop Beta 1.7.3: gui/container.png
// Supports single chest (27 slots, 176x168) and double chest (54 slots, 176x222).
class BetaChestScreen : public Screen {
	typedef Screen super;
public:
	BetaChestScreen(Container* container);
	virtual ~BetaChestScreen();

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

	int numChestRows() const;
	int panelH() const;
	int panelX() const { return (width - PANEL_W) / 2; }
	int panelY() const { return (height - panelH()) / 2; }

	// Slot indexing:
	// 0 .. (numRows * 9 - 1) = Chest slots
	// (numRows * 9) .. (numRows * 9 + 26) = Player main inventory (PE slots 9-35)
	// (numRows * 9 + 27) .. (numRows * 9 + 35) = Player hotbar (PE slots 0-8)
	bool slotPos(int idx, int& sx, int& sy) const;
	int slotAt(int x, int y) const;

	Inventory* inv();
	ItemInstance* getSlotItem(int idx);
	void setSlotItem(int idx, const ItemInstance* item);

	bool quickTransfer(int idx);
	bool mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse);

	void spillCarried();
	void placeInto(int slot);
	void placeOneInto(int slot);

	Container* container;
	ItemInstance carried;
	bool hasCarried;
	bool pressed;
	int pressSlot;
	int pressButton;
	bool pressPickedUp;
};

#endif /* NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaChestScreen_H__ */
