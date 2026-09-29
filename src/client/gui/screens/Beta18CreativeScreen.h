#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__Beta18CreativeScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__Beta18CreativeScreen_H__

#include "../Screen.h"
#include "../../../world/item/ItemInstance.h"
#include <vector>

class Inventory;

// Creative "Item selection" after Beta 1.8 (gui/allitems.png, 176x208):
// 72-slot scrolling grid (9 rows x 8 cols) of infinite creative stacks
// plus the survival hotbar row. Ported from beta_18/on.class behavior:
// same-ID clicks grow/shrink the cursor stack, shift fills to max,
// outside clicks drop to the world, hotbar uses normal survival logic.
class Beta18CreativeScreen : public Screen {
	typedef Screen super;
public:
	Beta18CreativeScreen();
	virtual ~Beta18CreativeScreen() {}

	void init();
	void setupPositions();
	void render(int xm, int ym, float a);
	void tick();

	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);
	virtual void mouseWheel(int dx, int dy, int xm, int ym);
	virtual void keyPressed(int eventKey);
	virtual void removed();
	virtual bool isPauseScreen() { return true; }

private:
	static const int PANEL_W = 176;
	static const int PANEL_H = 208;
	static const int GRID_COLS = 8;
	static const int GRID_ROWS = 9;
	static const int VIEW_SIZE = 72;

	// betaIdx: 0-71 creative viewport, 72-80 hotbar links.
	static bool slotPos(int betaIdx, int& sx, int& sy);
	int slotAt(int x, int y) const;

	Inventory* inv();
	ItemInstance* getSlotItem(int betaIdx);
	void setSlotItem(int betaIdx, const ItemInstance* item);
	int resolveHotbar(int betaIdx);
	int ensureHotbarLink(int betaIdx);
	int betaToPe(int betaIdx);
	bool isLinkedMain(int peSlot);
	// Hotbar place/merge/swap (normal survival semantics).
	void placeIntoHotbar(int slot);
	bool mergeIntoHotbar(ItemInstance& stack);
	// Refresh the 72 viewport cells from fullList at scroll g.
	void refreshViewport();
	// Rebuild the full creative list (blocks, items, dyes).
	void buildFullList();

	float scrollG() const { return scroll; }
	void setScroll(float g);

	int panelX() const { return (width - PANEL_W) / 2; }
	int panelY() const { return (height - PANEL_H) / 2; }

	std::vector<ItemInstance> fullList;
	ItemInstance viewport[VIEW_SIZE];
	ItemInstance carried;
	bool hasCarried;

	float scroll;
	bool pressed;
	int pressSlot;
	int pressButton;
	bool pressPickedUp;
};

#endif
