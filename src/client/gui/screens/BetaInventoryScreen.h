#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaInventoryScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__BetaInventoryScreen_H__

#include "../Screen.h"
#include "../../../world/item/ItemInstance.h"

// Survival inventory in the style of Minecraft Beta 1.7.3: classic
// container panel with a 2x2 crafting grid + result, armor slots and
// the player inventory. Slot layout (panel-relative, 18px pitch)
// mirrors ContainerPlayer from the original jar:
//
//   result 0 at (144,36); craft 1-4 at (88,26)+(18px grid);
//   armor 5-8 at (8,8..62); main 9-35 in rows y=84..138;
//   hotbar 36-44 in row y=160.
//
// Deviation: PE inventories hold 36 main slots, not 27, so the main
// block has four rows (panel 176x184 instead of 176x166); otherwise
// nine slots would be unreachable.
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
	static const int PANEL_H = 184;

	// Beta container slot index -> panel coords. Returns false for bad idx.
	static bool slotPos(int betaIdx, int& sx, int& sy);
	// Beta slot index under the point, or -1.
	int slotAt(int x, int y) const;
	// Live item in a beta slot (NULL when empty). Never keep across calls.
	ItemInstance* getSlotItem(int betaIdx);
	// Write an item into a beta slot (copies).
	void setSlotItem(int betaIdx, const ItemInstance* item);
	// Recompute the crafting result from the matrix.
	void updateCraftResult();
	// Consume one unit from every non-empty matrix cell.
	void consumeMatrix();
	// Shift-click quick transfer. Returns true if anything moved.
	bool quickTransfer(int betaIdx);
	// Merge helpers over beta slot ranges [from, to).
	bool mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse);
	// Drop the carried stack back into the inventory, or into the world.
	void spillCarried();

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
};

#endif
