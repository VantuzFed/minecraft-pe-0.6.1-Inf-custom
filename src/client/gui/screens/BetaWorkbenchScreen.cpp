#include "BetaWorkbenchScreen.h"

#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/inventory/CraftingContainer.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ArmorItem.h"
#include "../../../world/item/crafting/Recipes.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../locale/I18n.h"
#include <cstdio>

class BetaWorkGrid : public CraftingContainer {
public:
	BetaWorkGrid() : CraftingContainer(3, 3) {}
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

BetaWorkbenchScreen::BetaWorkbenchScreen()
	: hasCraftResult(false), hasCarried(false),
	  pressed(false), pressSlot(-1), pressButton(0), pressPickedUp(false) {
	for (int i = 0; i < 9; i++)
		craftMatrix[i].setNull();
	craftResult.setNull();
	carried.setNull();
}

void BetaWorkbenchScreen::init() {
	updateCraftResult();
}

void BetaWorkbenchScreen::setupPositions() {
}

bool BetaWorkbenchScreen::slotPos(int betaIdx, int& sx, int& sy) {
	if (betaIdx == 0) { sx = 124; sy = 35; return true; } // result
	if (betaIdx >= 1 && betaIdx <= 9) { // 3x3 matrix
		int k = betaIdx - 1;
		sx = 30 + (k % 3) * 18;
		sy = 17 + (k / 3) * 18;
		return true;
	}
	if (betaIdx >= 10 && betaIdx <= 36) { // main, 3 rows
		int k = betaIdx - 10;
		sx = 8 + (k % 9) * 18;
		sy = 84 + (k / 9) * 18;
		return true;
	}
	if (betaIdx >= 37 && betaIdx <= 45) { // hotbar
		sx = 8 + (betaIdx - 37) * 18;
		sy = 142;
		return true;
	}
	return false;
}

int BetaWorkbenchScreen::slotAt(int x, int y) const {
	int px = panelX(), py = panelY();
	for (int i = 0; i <= 45; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		if (x >= px + sx && x < px + sx + 18 && y >= py + sy && y < py + sy + 18)
			return i;
	}
	return -1;
}

Inventory* BetaWorkbenchScreen::inv() {
	Player* player = minecraft ? minecraft->player : NULL;
	if (!player)
		return NULL;
	return player->inventory;
}

int BetaWorkbenchScreen::betaToPe(int betaIdx) {
	if (betaIdx >= 10 && betaIdx <= 45)
		return betaIdx - 1;
	return -1;
}

ItemInstance* BetaWorkbenchScreen::getSlotItem(int betaIdx) {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return NULL;
	if (betaIdx == 0)
		return hasCraftResult ? &craftResult : NULL;
	if (betaIdx >= 1 && betaIdx <= 9) {
		ItemInstance& it = craftMatrix[betaIdx - 1];
		return it.isNull() ? NULL : &it;
	}
	if (betaIdx >= 10 && betaIdx <= 45) {
		ItemInstance* it = in->getItem(betaIdx - 1);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	return NULL;
}

void BetaWorkbenchScreen::setSlotItem(int betaIdx, const ItemInstance* item) {
	Inventory* in = inv();
	if (!in)
		return;
	bool empty = !item || item->isNull();
	if (betaIdx >= 1 && betaIdx <= 9) {
		if (empty) craftMatrix[betaIdx - 1].setNull();
		else craftMatrix[betaIdx - 1] = *item;
		updateCraftResult();
		return;
	}
	if (betaIdx >= 10 && betaIdx <= 45) {
		if (empty) in->clearSlot(betaIdx - 1);
		else in->setItem(betaIdx - 1, const_cast<ItemInstance*>(item));
	}
}

void BetaWorkbenchScreen::updateCraftResult() {
	craftResult.setNull();
	hasCraftResult = false;
	bool any = false;
	for (int i = 0; i < 9; i++)
		if (!craftMatrix[i].isNull()) { any = true; break; }
	if (!any)
		return;
	BetaWorkGrid cc;
	for (int i = 0; i < 9; i++)
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

void BetaWorkbenchScreen::consumeMatrix() {
	for (int i = 0; i < 9; i++) {
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

bool BetaWorkbenchScreen::mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse, int skipPe) {
	if (stack.isNull())
		return false;
	bool moved = false;
	if (stack.isStackable()) {
		for (int pass = 0; pass < 2 && !stack.isNull(); pass++) {
			for (int i = from; i < to; i++) {
				int idx = reverse ? (to - 1 - (i - from)) : i;
				if (skipPe >= 0 && betaToPe(idx) == skipPe)
					continue;
				ItemInstance* dst = getSlotItem(idx);
				if (pass == 0) {
					if (!dst || !sameStack(&stack, dst))
						continue;
					int space = stack.getMaxStackSize() - dst->count;
					if (space <= 0)
						continue;
					int take = stack.count < space ? stack.count : space;
					dst->count += take;
					stack.count -= take;
					moved = true;
				} else if (!dst || dst->isNull()) {
					ItemInstance v = stack;
					setSlotItem(idx, &v);
					ItemInstance* check = getSlotItem(idx);
					if (!check || check->isNull())
						continue;
					stack.setNull();
					return true;
				}
			}
		}
	} else {
		for (int i = from; i < to; i++) {
			int idx = reverse ? (to - 1 - (i - from)) : i;
			if (skipPe >= 0 && betaToPe(idx) == skipPe)
				continue;
			if (!getSlotItem(idx)) {
				ItemInstance v = stack;
				setSlotItem(idx, &v);
				ItemInstance* check = getSlotItem(idx);
				if (!check || check->isNull())
					continue;
				stack.setNull();
				return true;
			}
		}
	}
	return moved;
}

int BetaWorkbenchScreen::spaceFor(const ItemInstance& stack, int from, int to) {
	int space = 0;
	int max = stack.getMaxStackSize();
	for (int i = from; i < to; i++) {
		ItemInstance* dst = getSlotItem(i);
		if (!dst || dst->isNull())
			space += max;
		else if (sameStack(&stack, dst))
			space += max - dst->count;
	}
	return space;
}

bool BetaWorkbenchScreen::takeResultToCursor() {
	if (!hasCraftResult)
		return false;
	if (!hasCarried) {
		carried = craftResult;
		hasCarried = true;
		consumeMatrix();
		return true;
	}
	if (sameStack(&carried, &craftResult)
		&& carried.count + craftResult.count <= carried.getMaxStackSize()) {
		carried.count += craftResult.count;
		consumeMatrix();
		return true;
	}
	return false;
}

bool BetaWorkbenchScreen::takeResultToInventory() {
	bool moved = false;
	while (hasCraftResult && (spaceFor(craftResult, 37, 46) + spaceFor(craftResult, 10, 37)) >= craftResult.count) {
		ItemInstance one = craftResult;
		mergeIntoRange(one, 37, 46, false);
		if (!one.isNull())
			mergeIntoRange(one, 10, 37, false);
		if (!one.isNull())
			break;
		consumeMatrix();
		moved = true;
	}
	return moved;
}

bool BetaWorkbenchScreen::quickTransfer(int betaIdx) {
	ItemInstance* src = getSlotItem(betaIdx);
	if (!src || src->isNull())
		return false;
	if (betaIdx == 0)
		return takeResultToInventory();
	ItemInstance stack = *src;
	int srcPe = betaToPe(betaIdx);
	bool moved = false;
	if (betaIdx >= 1 && betaIdx <= 9) {
		moved = mergeIntoRange(stack, 37, 46, false, srcPe);
		if (!moved || !stack.isNull())
			moved = mergeIntoRange(stack, 10, 37, false, srcPe) || moved;
	} else if (betaIdx >= 10 && betaIdx <= 36) {
		moved = mergeIntoRange(stack, 37, 46, false, srcPe);
	} else {
		moved = mergeIntoRange(stack, 10, 37, false, srcPe);
	}
	if (moved) {
		if (stack.isNull()) {
			ItemInstance empty;
			empty.setNull();
			setSlotItem(betaIdx, &empty);
		} else {
			setSlotItem(betaIdx, &stack);
		}
		if (betaIdx >= 1 && betaIdx <= 9)
			updateCraftResult();
	}
	return moved;
}

void BetaWorkbenchScreen::spillCarried() {
	if (!hasCarried || carried.isNull())
		return;
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (player && in) {
		in->add(&carried);
		if (carried.isNull() || carried.count <= 0) {
			carried.setNull();
			hasCarried = false;
			return;
		}
		in->doDrop(&carried, true);
		carried.setNull();
		hasCarried = false;
	}
}

void BetaWorkbenchScreen::render(int xm, int ym, float a) {
	renderBackground();
	// render() coords are already GUI units (see BetaInventoryScreen).
	int mx = xm;
	int my = ym - 1;
	int px = panelX(), py = panelY();

	TextureId bg = minecraft->textures->loadTexture("gui/crafting.png");
	if (Textures::isTextureIdValid(bg)) {
		minecraft->textures->bind(bg);
		glColor4f2(1, 1, 1, 1);
		blit(px, py, 0, 0, 176, 166);
	}

	drawString(minecraft->font, I18n::get("container.crafting"), px + 28, py + 6, 0xffffffff);
	drawString(minecraft->font, I18n::get("container.inventory"), px + 8, py + 72, 0xffffffff);

	for (int i = 0; i <= 45; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		int x0 = px + sx, y0 = py + sy;
		ItemInstance* it = getSlotItem(i);
		if (it && !it->isNull()) {
			ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, it, (float)x0, (float)y0, true);
			ItemRenderer::renderGuiItemDecorations(minecraft->font, minecraft->textures, it, x0, y0);
		}
	}

	int hover = slotAt(mx, my);
	if (hover >= 0) {
		int sx, sy;
		if (slotPos(hover, sx, sy))
			fill(px + sx, py + sy, px + sx + 16, py + sy + 16, 0x80ffffff);
	}

	if (hasCarried && !carried.isNull()) {
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &carried, (float)(mx - 8), (float)(my - 8), true);
		ItemRenderer::renderGuiItemDecorations(minecraft->font, minecraft->textures, &carried, mx - 8, my - 8);
	}

	super::render(xm, ym, a);
}

void BetaWorkbenchScreen::tick() {
	super::tick();
}

void BetaWorkbenchScreen::placeInto(int slot) {
	if (!hasCarried || carried.isNull() || slot == 0)
		return;
	ItemInstance* dst = getSlotItem(slot);
	if (!dst || dst->isNull()) {
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
}

void BetaWorkbenchScreen::placeOneInto(int slot) {
	if (!hasCarried || carried.isNull() || slot == 0)
		return;
	ItemInstance* dst = getSlotItem(slot);
	if (!dst || dst->isNull()) {
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

// Same press-based model as BetaInventoryScreen: beta acts on press,
// release on another slot finishes a drag.
void BetaWorkbenchScreen::mouseClicked(int x, int y, int buttonNum) {
	if (buttonNum != MouseAction::ACTION_LEFT && buttonNum != MouseAction::ACTION_RIGHT)
		return;
	pressed = true;
	pressSlot = slotAt(x, y);
	pressButton = buttonNum;
	pressPickedUp = false;
	bool shift = Keyboard::isKeyDown(Keyboard::KEY_LSHIFT);

	if (pressButton == MouseAction::ACTION_LEFT) {
		if (pressSlot < 0) {
			spillCarried();
			return;
		}
		if (shift) {
			if (hasCarried) spillCarried();
			quickTransfer(pressSlot);
			return;
		}
		if (pressSlot == 0) {
			takeResultToCursor();
			return;
		}
		if (!hasCarried) {
			ItemInstance* dst = getSlotItem(pressSlot);
			if (dst && !dst->isNull()) {
				carried = *dst;
				hasCarried = true;
				ItemInstance empty;
				empty.setNull();
				setSlotItem(pressSlot, &empty);
				pressPickedUp = true;
			}
			return;
		}
		placeInto(pressSlot);
	} else {
		if (pressSlot < 0 || pressSlot == 0 || shift)
			return;
		if (!hasCarried) {
			ItemInstance* dst = getSlotItem(pressSlot);
			if (dst && !dst->isNull()) {
				if (dst->count > 1) {
					int half = (dst->count + 1) / 2;
					carried = *dst;
					carried.count = half;
					hasCarried = true;
					dst->count -= half;
					if (dst->count <= 0) {
						ItemInstance empty;
						empty.setNull();
						setSlotItem(pressSlot, &empty);
					}
				} else {
					carried = *dst;
					hasCarried = true;
					ItemInstance empty;
					empty.setNull();
					setSlotItem(pressSlot, &empty);
				}
				pressPickedUp = true;
			}
			return;
		}
		placeOneInto(pressSlot);
	}
}

void BetaWorkbenchScreen::mouseReleased(int x, int y, int buttonNum) {
	if (!pressed || buttonNum != pressButton) {
		pressed = false;
		return;
	}
	pressed = false;
	if (!pressPickedUp || !hasCarried || carried.isNull())
		return;
	pressPickedUp = false;
	int slot = slotAt(x, y);
	if (slot < 0) {
		spillCarried();
		return;
	}
	if (slot == pressSlot || slot == 0)
		return;
	placeInto(slot);
}

void BetaWorkbenchScreen::keyPressed(int eventKey) {
	super::keyPressed(eventKey);
	if (eventKey == Keyboard::KEY_E && minecraft && !minecraft->isCreativeMode())
		minecraft->setScreen(NULL);
}

void BetaWorkbenchScreen::removed() {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return;
	for (int i = 0; i < 9; i++) {
		if (craftMatrix[i].isNull())
			continue;
		in->add(&craftMatrix[i]);
		if (!craftMatrix[i].isNull() && craftMatrix[i].count > 0)
			in->doDrop(&craftMatrix[i], true);
		craftMatrix[i].setNull();
	}
	if (hasCarried && !carried.isNull()) {
		in->add(&carried);
		if (!carried.isNull() && carried.count > 0)
			in->doDrop(&carried, true);
		carried.setNull();
		hasCarried = false;
	}
}
