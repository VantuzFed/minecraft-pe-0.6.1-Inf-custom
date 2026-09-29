#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaInventoryScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaInventoryScreen_H__

#include "../Screen.h"
#include "../../../world/item/ItemInstance.h"

class Inventory;

// Survival inventory in the style of Minecraft Beta 1.7.3: the original
// gui/inventory.png panel (176x166) plus an 18px strip, ContainerPlayer
// slot map (result, 2x2 matrix, armor, 3 main rows, hotbar), hover
// highlight and the rotating player preview.
//
// PE stores 36 main slots while beta shows 27, so a fourth row below
// the hotbar exposes main 36-44; without it items parked there were
// unreachable ("lost"). PE hotbar slots are links (views) into main
// storage, not storage, so every hotbar mutation resolves (or creates)
// the underlying link; the orphan link cells are never written directly.
class BetaInventoryScreen : public Screen {
	typedef Screen super;
public:
	BetaInventoryScreen();
	virtual ~BetaInventoryScreen() {}

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

	// Beta container slot index -> panel coords. Returns false for bad idx.
	static bool slotPos(int betaIdx, int& sx, int& sy);
	// Beta slot index under the point, or -1.
	int slotAt(int x, int y) const;

	Inventory* inv() const;
	// Live item in a beta slot (NULL when empty). Never keep across calls.
	ItemInstance* getSlotItem(int betaIdx);
	// Write an item into a beta slot (copies).
	void setSlotItem(int betaIdx, const ItemInstance* item);
	// Beta slot -> underlying PE slot (9-44), or -1 for crafting/result/armor.
	int betaToPe(int betaIdx);
	// Recompute the crafting result from the matrix.
	void updateCraftResult();
	// Consume one unit from every non-empty matrix cell.
	void consumeMatrix();
	// Shift-click quick transfer. Returns true if anything moved.
	bool quickTransfer(int betaIdx);
	// Result-slot takes, one atomic craft at a time (no dupes).
	bool takeResultToCursor();
	bool takeResultToInventory();
	// Merge helpers over beta slot ranges [from, to). skipPe excludes
	// one underlying PE slot (the shift-click source's own cell).
	bool mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse, int skipPe = -1);
	// Read-only free space for a stack across a beta range.
	int spaceFor(const ItemInstance& stack, int from, int to);
	// Drop the carried stack back into the inventory, or into the world.
	void spillCarried();
	// Place the whole carried stack into a slot (merge or swap).
	void placeInto(int slot);
	// Place a single unit from the carried stack into a slot.
	void placeOneInto(int slot);
	// Rotating player preview, like the original inventory.
	void renderPlayerModel(float xo, float yo);

	int panelX() const { return (width - PANEL_W) / 2; }
	int panelY() const { return (height - PANEL_H) / 2; }

	ItemInstance craftMatrix[4];
	ItemInstance craftResult;
	bool hasCraftResult;
	ItemInstance carried;
	bool hasCarried;

	bool pressed;
	int pressSlot;
	int pressButton;
	// True when the press picked something up: releasing on another
	// slot drops it there (drag and drop). Click-click keeps working
	// because releasing on the press slot does nothing.
	bool pressPickedUp;
};

#endif
