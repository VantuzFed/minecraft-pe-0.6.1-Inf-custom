#include "BetaInventoryScreen.h"

#include "../../Minecraft.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/inventory/CraftingContainer.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ArmorItem.h"
#include "../../../world/item/crafting/Recipes.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../locale/I18n.h"
#include "../../player/LocalPlayer.h"
#include <cstdio>

// CraftingContainer leaves the pointer-based Container API pure;
// bridge it for recipe matching.
class BetaCraftGrid : public CraftingContainer {
public:
	BetaCraftGrid() : CraftingContainer(2, 2) {}
	virtual void setItem(int slot, ItemInstance* item) {
		if (item) {
			CraftingContainer::setItem(slot, *item);
		} else {
			ItemInstance e;
			e.setNull();
			CraftingContainer::setItem(slot, e);
		}
	}
	virtual ItemInstance removeItem(int slot, int count) {
		return CraftingContainer::removeItem(slot, count);
	}
};

BetaInventoryScreen::BetaInventoryScreen()
	: hasCraftResult(false), hasCarried(false),
	  pressed(false), pressSlot(-1), pressButton(0) {
	for (int i = 0; i < 4; i++)
		craftMatrix[i].setNull();
	craftResult.setNull();
	carried.setNull();
}

void BetaInventoryScreen::init() {
	updateCraftResult();
}

void BetaInventoryScreen::setupPositions() {
}

bool BetaInventoryScreen::slotPos(int betaIdx, int& sx, int& sy) {
	if (betaIdx == 0) { sx = 144; sy = 36; return true; } // result
	if (betaIdx >= 1 && betaIdx <= 4) { // 2x2 matrix
		int k = betaIdx - 1;
		sx = 88 + (k % 2) * 18;
		sy = 26 + (k / 2) * 18;
		return true;
	}
	if (betaIdx >= 5 && betaIdx <= 8) { // armor, helmet on top
		sx = 8;
		sy = 8 + (betaIdx - 5) * 18;
		return true;
	}
	if (betaIdx >= 9 && betaIdx <= 35) { // main, 3 classic rows
		int k = betaIdx - 9;
		sx = 8 + (k % 9) * 18;
		sy = 84 + (k / 9) * 18;
		return true;
	}
	if (betaIdx >= 36 && betaIdx <= 44) { // hotbar
		sx = 8 + (betaIdx - 36) * 18;
		sy = 160;
		return true;
	}
	if (betaIdx >= 45 && betaIdx <= 53) { // 4th main row (PE extra slots)
		sx = 8 + (betaIdx - 45) * 18;
		sy = 138;
		return true;
	}
	return false;
}

int BetaInventoryScreen::slotAt(int x, int y) const {
	int px = panelX(), py = panelY();
	for (int i = 0; i <= 53; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		if (x >= px + sx && x < px + sx + 18 && y >= py + sy && y < py + sy + 18)
			return i;
	}
	return -1;
}

// PE storage behind beta slots: main 9-35 -> inv 9-35, hotbar 36-44 ->
// inv 0-8, extra row 45-53 -> inv 36-44, armor 5-8 -> player armor 0-3.
static int betaToPeInv(int betaIdx) {
	if (betaIdx >= 9 && betaIdx <= 35) return betaIdx;
	if (betaIdx >= 36 && betaIdx <= 44) return betaIdx - 36;
	if (betaIdx >= 45 && betaIdx <= 53) return betaIdx - 9;
	return -1;
}

ItemInstance* BetaInventoryScreen::getSlotItem(int betaIdx) {
	Player* player = minecraft->player;
	if (!player || !player->inventory)
		return NULL;
	if (betaIdx == 0)
		return hasCraftResult ? &craftResult : NULL;
	if (betaIdx >= 1 && betaIdx <= 4) {
		ItemInstance& it = craftMatrix[betaIdx - 1];
		return it.isNull() ? NULL : &it;
	}
	if (betaIdx >= 5 && betaIdx <= 8)
		return player->getArmor(betaIdx - 5);
	int pe = betaToPeInv(betaIdx);
	if (pe < 0)
		return NULL;
	ItemInstance* it = player->inventory->getItem(pe);
	if (!it || it->isNull())
		return NULL;
	return it;
}

void BetaInventoryScreen::setSlotItem(int betaIdx, const ItemInstance* item) {
	Player* player = minecraft->player;
	if (!player || !player->inventory)
		return;
	ItemInstance v;
	if (item && !item->isNull())
		v = *item;
	if (betaIdx == 0) {
		if (item && !item->isNull()) { craftResult = *item; hasCraftResult = true; }
		else { craftResult.setNull(); hasCraftResult = false; }
		return;
	}
	if (betaIdx >= 1 && betaIdx <= 4) {
		craftMatrix[betaIdx - 1] = v;
		updateCraftResult();
		return;
	}
	if (betaIdx >= 5 && betaIdx <= 8) {
		player->setArmor(betaIdx - 5, (item && !item->isNull()) ? item : NULL);
		return;
	}
	int pe = betaToPeInv(betaIdx);
	if (pe >= 0)
		player->inventory->setItem(pe, (item && !item->isNull()) ? const_cast<ItemInstance*>(item) : NULL);
}

void BetaInventoryScreen::updateCraftResult() {
	craftResult.setNull();
	hasCraftResult = false;
	bool any = false;
	for (int i = 0; i < 4; i++)
		if (!craftMatrix[i].isNull()) { any = true; break; }
	if (!any)
		return;
	BetaCraftGrid cc;
	for (int i = 0; i < 4; i++)
		if (!craftMatrix[i].isNull())
			cc.CraftingContainer::setItem(i, craftMatrix[i]);
	Recipes* recipes = Recipes::getInstance();
	if (!recipes)
		return;
	const RecipeList& list = recipes->getRecipes();
	for (size_t i = 0; i < list.size(); i++) {
		Recipe* r = list[i];
		if (!r)
			continue;
		if (r->matches(&cc)) {
			craftResult = r->assemble(&cc);
			if (!craftResult.isNull() && craftResult.count > 0)
				hasCraftResult = true;
			else {
				craftResult.setNull();
				hasCraftResult = false;
			}
			return;
		}
	}
}

void BetaInventoryScreen::consumeMatrix() {
	for (int i = 0; i < 4; i++) {
		if (craftMatrix[i].isNull())
			continue;
		craftMatrix[i].count--;
		if (craftMatrix[i].count <= 0)
			craftMatrix[i].setNull();
	}
	updateCraftResult();
}

static bool sameStack(const ItemInstance* a, const ItemInstance* b) {
	if (!a || !b || a->isNull() || b->isNull())
		return false;
	if (a->id != b->id)
		return false;
	if (a->isStackedByData() && a->getAuxValue() != b->getAuxValue())
		return false;
	return true;
}

bool BetaInventoryScreen::mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse) {
	if (stack.isNull())
		return false;
	bool moved = false;
	// First pass: top up existing stacks.
	if (stack.isStackable()) {
		for (int pass = 0; pass < 2; pass++) {
			for (int i = from; i < to; i++) {
				int idx = reverse ? (to - 1 - (i - from)) : i;
				ItemInstance* dst = getSlotItem(idx);
				if (pass == 0 && (!dst || !sameStack(&stack, dst)))
					continue;
				if (pass == 1 && dst && !dst->isNull())
					continue;
				int max = stack.getMaxStackSize();
				if (pass == 0) {
					int space = max - dst->count;
					if (space <= 0)
						continue;
					int take = stack.count < space ? stack.count : space;
					dst->count += take;
					stack.count -= take;
					moved = true;
					if (stack.count <= 0) { stack.setNull(); return true; }
				} else {
					ItemInstance v = stack;
					setSlotItem(idx, &v);
					stack.setNull();
					return true;
				}
			}
			if (stack.isNull())
				return moved;
		}
	} else {
		for (int i = from; i < to; i++) {
			int idx = reverse ? (to - 1 - (i - from)) : i;
			if (!getSlotItem(idx)) {
				ItemInstance v = stack;
				setSlotItem(idx, &v);
				stack.setNull();
				return true;
			}
		}
	}
	return moved;
}

bool BetaInventoryScreen::quickTransfer(int betaIdx) {
	ItemInstance* src = getSlotItem(betaIdx);
	if (!src || src->isNull())
		return false;
	ItemInstance stack = *src;
	bool moved = false;
	if (betaIdx == 0) {
		// Crafting result goes to inventory/hotbar.
		moved = mergeIntoRange(stack, 9, 54, false);
		if (moved)
			consumeMatrix();
		return moved;
	}
	if (betaIdx >= 1 && betaIdx <= 8) {
		moved = mergeIntoRange(stack, 9, 54, false);
	} else if (betaIdx >= 9 && betaIdx <= 35) {
		moved = mergeIntoRange(stack, 36, 45, false);
		if (!moved)
			moved = mergeIntoRange(stack, 45, 54, false);
	} else {
		// Armor pieces prefer their armor slot, like the original.
		if (ItemInstance::isArmorItem(&stack)) {
			const ArmorItem* ar = (const ArmorItem*)stack.getItem();
			if (ar && ar->slot >= 0 && ar->slot < 4 && !getSlotItem(5 + ar->slot)) {
				ItemInstance v = stack;
				setSlotItem(5 + ar->slot, &v);
				stack.setNull();
				moved = true;
			}
		}
		if (!moved)
			moved = mergeIntoRange(stack, 9, 36, false);
	}
	if (moved) {
		if (stack.isNull()) {
			ItemInstance empty;
			empty.setNull();
			setSlotItem(betaIdx, &empty);
		} else {
			setSlotItem(betaIdx, &stack);
		}
		if (betaIdx >= 1 && betaIdx <= 4)
			updateCraftResult();
	}
	return moved;
}

void BetaInventoryScreen::spillCarried() {
	if (!hasCarried || carried.isNull())
		return;
	Player* player = minecraft->player;
	if (player && player->inventory) {
		// add() merges what fits and leaves the rest in count.
		player->inventory->add(&carried);
		if (carried.isNull() || carried.count <= 0) {
			carried.setNull();
			hasCarried = false;
			return;
		}
		player->inventory->doDrop(&carried, true);
		carried.setNull();
		hasCarried = false;
	}
}

void BetaInventoryScreen::render(int xm, int ym, float a) {
	renderBackground();
	int px = panelX(), py = panelY();

	// Classic container panel: gray body, light top/left, dark bottom/right.
	fill(px - 2, py - 2, px + PANEL_W + 2, py + PANEL_H + 2, 0xff000000);
	fill(px, py, px + PANEL_W, py + PANEL_H, 0xffc6c6c6);
	fill(px + 1, py + 1, px + PANEL_W - 1, py + 2, 0xffffffff);
	fill(px + 1, py + 1, px + 2, py + PANEL_H - 1, 0xffffffff);
	fill(px + 1, py + PANEL_H - 2, px + PANEL_W - 1, py + PANEL_H - 1, 0xff555555);
	fill(px + PANEL_W - 2, py + 1, px + PANEL_W - 1, py + PANEL_H - 1, 0xff555555);

	drawString(minecraft->font, "Crafting", px + 86, py + 16, 0xff404040);

	for (int i = 0; i <= 53; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		int x0 = px + sx, y0 = py + sy;
		// Slot recess.
		fill(x0, y0, x0 + 18, y0 + 18, 0xff373737);
		fill(x0 + 1, y0 + 1, x0 + 17, y0 + 17, 0xff8b8b8b);
		ItemInstance* it = getSlotItem(i);
		if (it && !it->isNull()) {
			ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, it, (float)(x0 + 1), (float)(y0 + 1), true);
			if (it->count > 1) {
				char buf[16];
				sprintf(buf, "%d", it->count);
				minecraft->font->drawShadow(buf, (float)(x0 + 17 - minecraft->font->width(buf)), (float)(y0 + 9), 0xffffffff);
			}
		}
	}

	if (hasCarried && !carried.isNull()) {
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &carried, (float)(xm - 8), (float)(ym - 8), true);
		if (carried.count > 1) {
			char buf[16];
			sprintf(buf, "%d", carried.count);
			minecraft->font->drawShadow(buf, (float)(xm + 8 - minecraft->font->width(buf)), (float)(ym + 1), 0xffffffff);
		}
	}

	super::render(xm, ym, a);
}

void BetaInventoryScreen::tick() {
	super::tick();
}

void BetaInventoryScreen::mouseClicked(int x, int y, int buttonNum) {
	if (buttonNum != MouseAction::ACTION_LEFT && buttonNum != MouseAction::ACTION_RIGHT)
		return;
	pressed = true;
	pressSlot = slotAt(x, y);
	pressButton = buttonNum;
}

static bool canWearIn(int betaIdx, const ItemInstance* item) {
	if (betaIdx < 5 || betaIdx > 8)
		return true;
	if (!item || item->isNull())
		return true;
	if (!ItemInstance::isArmorItem(item))
		return false;
	const ArmorItem* armor = (const ArmorItem*)item->getItem();
	if (!armor)
		return false;
	return armor->slot == (betaIdx - 5);
}

void BetaInventoryScreen::mouseReleased(int x, int y, int buttonNum) {
	if (!pressed || buttonNum != pressButton) {
		pressed = false;
		return;
	}
	pressed = false;
	int slot = slotAt(x, y);
	bool shift = Keyboard::isKeyDown(Keyboard::KEY_LSHIFT);

	if (pressButton == MouseAction::ACTION_LEFT) {
		if (slot < 0) {
			spillCarried();
			return;
		}
		if (shift) {
			if (hasCarried) spillCarried();
			quickTransfer(slot);
			return;
		}
		if (slot == 0) {
			// Crafting result: take only with an accepting cursor.
			if (!hasCraftResult)
				return;
			if (!hasCarried) {
				carried = craftResult;
				hasCarried = true;
				consumeMatrix();
			} else if (sameStack(&carried, &craftResult)) {
				int space = carried.getMaxStackSize() - carried.count;
				int take = craftResult.count < space ? craftResult.count : space;
				if (take > 0) {
					carried.count += take;
					for (int t = 0; t < take; t++)
						consumeMatrix();
				}
			}
			return;
		}
		ItemInstance* dst = getSlotItem(slot);
		if (!hasCarried) {
			if (dst && !dst->isNull()) {
				carried = *dst;
				hasCarried = true;
				ItemInstance empty;
				empty.setNull();
				setSlotItem(slot, &empty);
			}
		} else if (!canWearIn(slot, &carried)) {
			return;
		} else if (!dst || dst->isNull()) {
			setSlotItem(slot, &carried);
			carried.setNull();
			hasCarried = false;
		} else if (sameStack(&carried, dst)) {
			int space = dst->getMaxStackSize() - dst->count;
			int take = carried.count < space ? carried.count : space;
			if (take > 0) {
				dst->count += take;
				carried.count -= take;
				if (carried.count <= 0) { carried.setNull(); hasCarried = false; }
			} else {
				ItemInstance tmp = *dst;
				setSlotItem(slot, &carried);
				carried = tmp;
			}
		} else {
			ItemInstance tmp = *dst;
			setSlotItem(slot, &carried);
			carried = tmp;
		}
	} else {
		// Right click: place one / pick up half.
		if (slot < 0 || slot == 0)
			return;
		if (shift)
			return;
		ItemInstance* dst = getSlotItem(slot);
		if (!hasCarried) {
			if (dst && !dst->isNull() && dst->count > 1) {
				int half = (dst->count + 1) / 2;
				carried = *dst;
				carried.count = half;
				hasCarried = true;
				dst->count -= half;
				if (dst->count <= 0) {
					ItemInstance empty;
					empty.setNull();
					setSlotItem(slot, &empty);
				}
			} else if (dst && !dst->isNull()) {
				carried = *dst;
				hasCarried = true;
				ItemInstance empty;
				empty.setNull();
				setSlotItem(slot, &empty);
			}
		} else if (!canWearIn(slot, &carried)) {
			return;
		} else if (!dst || dst->isNull()) {
			ItemInstance one = carried;
			one.count = 1;
			setSlotItem(slot, &one);
			carried.count--;
			if (carried.count <= 0) { carried.setNull(); hasCarried = false; }
		} else if (sameStack(&carried, dst) && dst->count < dst->getMaxStackSize()) {
			dst->count++;
			carried.count--;
			if (carried.count <= 0) { carried.setNull(); hasCarried = false; }
		}
	}
}

void BetaInventoryScreen::keyPressed(int eventKey) {
	super::keyPressed(eventKey);
	if (eventKey == Keyboard::KEY_E && minecraft && !minecraft->isCreativeMode())
		minecraft->setScreen(NULL);
}

void BetaInventoryScreen::removed() {
	// Grid contents go back to the player inventory (or drop).
	Player* player = minecraft ? minecraft->player : NULL;
	if (!player || !player->inventory)
		return;
	for (int i = 0; i < 4; i++) {
		if (craftMatrix[i].isNull())
			continue;
		player->inventory->add(&craftMatrix[i]);
		if (!craftMatrix[i].isNull() && craftMatrix[i].count > 0)
			player->inventory->doDrop(&craftMatrix[i], true);
		craftMatrix[i].setNull();
	}
	if (hasCarried && !carried.isNull()) {
		player->inventory->add(&carried);
		if (!carried.isNull() && carried.count > 0)
			player->inventory->doDrop(&carried, true);
		carried.setNull();
		hasCarried = false;
	}
}
