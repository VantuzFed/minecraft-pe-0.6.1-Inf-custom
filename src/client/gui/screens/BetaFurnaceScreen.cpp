#include "BetaFurnaceScreen.h"

#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/crafting/FurnaceRecipes.h"
#include "../../../world/level/tile/entity/FurnaceTileEntity.h"
#include "../../../world/level/Level.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../locale/I18n.h"
#include <cstdio>

BetaFurnaceScreen::BetaFurnaceScreen(FurnaceTileEntity* furnace)
	: furnace(furnace), fx(furnace ? furnace->x : 0),
	  fy(furnace ? furnace->y : 0), fz(furnace ? furnace->z : 0),
	  hasCarried(false), pressed(false), pressSlot(-1), pressButton(0), pressPickedUp(false) {
	carried.setNull();
}

void BetaFurnaceScreen::init() {
}

void BetaFurnaceScreen::setupPositions() {
}

bool BetaFurnaceScreen::slotPos(int betaIdx, int& sx, int& sy) {
	if (betaIdx == 0) { sx = 56; sy = 17; return true; } // ingredient
	if (betaIdx == 1) { sx = 56; sy = 53; return true; } // fuel
	if (betaIdx == 2) { sx = 116; sy = 35; return true; } // result
	if (betaIdx >= 3 && betaIdx <= 29) { // main, 3 rows
		int k = betaIdx - 3;
		sx = 8 + (k % 9) * 18;
		sy = 84 + (k / 9) * 18;
		return true;
	}
	if (betaIdx >= 30 && betaIdx <= 38) { // hotbar
		sx = 8 + (betaIdx - 30) * 18;
		sy = 142;
		return true;
	}
	return false;
}

int BetaFurnaceScreen::slotAt(int x, int y) const {
	int px = panelX(), py = panelY();
	for (int i = 0; i <= 38; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		if (x >= px + sx && x < px + sx + 18 && y >= py + sy && y < py + sy + 18)
			return i;
	}
	return -1;
}

bool BetaFurnaceScreen::furnaceGone() const {
	if (!furnace || !minecraft || !minecraft->level)
		return true;
	return minecraft->level->getTileEntity(fx, fy, fz) != furnace;
}

Inventory* BetaFurnaceScreen::inv() {
	Player* player = minecraft ? minecraft->player : NULL;
	if (!player)
		return NULL;
	return player->inventory;
}

int BetaFurnaceScreen::betaToPe(int betaIdx) {
	if (betaIdx >= 3 && betaIdx <= 38)
		return betaIdx + 6;
	return -1;
}

ItemInstance* BetaFurnaceScreen::getSlotItem(int betaIdx) {
	if (furnaceGone())
		return NULL;
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (betaIdx >= 0 && betaIdx <= 2) {
		ItemInstance* it = furnace->getItem(betaIdx);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	if (!player || !in)
		return NULL;
	if (betaIdx >= 3 && betaIdx <= 38) {
		ItemInstance* it = in->getItem(betaIdx + 6);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	return NULL;
}

void BetaFurnaceScreen::setSlotItem(int betaIdx, const ItemInstance* item) {
	if (furnaceGone())
		return;
	Inventory* in = inv();
	if (!in)
		return;
	bool empty = !item || item->isNull();
	if (betaIdx >= 0 && betaIdx <= 2) {
		if (empty) {
			ItemInstance e;
			e.setNull();
			furnace->setItem(betaIdx, &e);
		} else {
			furnace->setItem(betaIdx, const_cast<ItemInstance*>(item));
		}
		return;
	}
	if (betaIdx >= 3 && betaIdx <= 38) {
		if (empty) in->clearSlot(betaIdx + 6);
		else in->setItem(betaIdx + 6, const_cast<ItemInstance*>(item));
	}
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

static bool canSmelt(const ItemInstance* item) {
	if (!item || item->isNull())
		return false;
	const FurnaceRecipes* recipes = FurnaceRecipes::getInstance();
	return recipes && recipes->isFurnaceItem(item->id);
}

static bool canBurn(const ItemInstance* item) {
	if (!item || item->isNull())
		return false;
	return FurnaceTileEntity::getBurnDuration(*item) > 0;
}

bool BetaFurnaceScreen::mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse, int skipPe) {
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

bool BetaFurnaceScreen::quickTransfer(int betaIdx) {
	ItemInstance* src = getSlotItem(betaIdx);
	if (!src || src->isNull())
		return false;
	ItemInstance stack = *src;
	int srcPe = betaToPe(betaIdx);
	bool moved = false;
	if (betaIdx == 2 || betaIdx == 0 || betaIdx == 1) {
		moved = mergeIntoRange(stack, 30, 39, false, srcPe);
		if (!moved || !stack.isNull())
			moved = mergeIntoRange(stack, 3, 30, false, srcPe) || moved;
	} else if (betaIdx >= 3 && betaIdx <= 29) {
		// Smeltables prefer the ingredient slot, fuel prefers fuel.
		if (canSmelt(&stack) && !getSlotItem(0)) {
			ItemInstance v = stack;
			setSlotItem(0, &v);
			stack.setNull();
			moved = true;
		} else if (canBurn(&stack) && !getSlotItem(1)) {
			ItemInstance v = stack;
			setSlotItem(1, &v);
			stack.setNull();
			moved = true;
		} else {
			moved = mergeIntoRange(stack, 30, 39, false, srcPe);
		}
	} else {
		if (canSmelt(&stack) && !getSlotItem(0)) {
			ItemInstance v = stack;
			setSlotItem(0, &v);
			stack.setNull();
			moved = true;
		} else if (canBurn(&stack) && !getSlotItem(1)) {
			ItemInstance v = stack;
			setSlotItem(1, &v);
			stack.setNull();
			moved = true;
		} else {
			moved = mergeIntoRange(stack, 3, 30, false, srcPe);
		}
	}
	if (moved) {
		if (stack.isNull()) {
			ItemInstance empty;
			empty.setNull();
			setSlotItem(betaIdx, &empty);
		} else {
			setSlotItem(betaIdx, &stack);
		}
	}
	return moved;
}

void BetaFurnaceScreen::spillCarried() {
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

void BetaFurnaceScreen::render(int xm, int ym, float a) {
	renderBackground();
	// render() coords are already GUI units (see BetaInventoryScreen).
	int mx = xm;
	int my = ym - 1;
	int px = panelX(), py = panelY();

	TextureId bg = minecraft->textures->loadTexture("gui/furnace.png");
	if (Textures::isTextureIdValid(bg)) {
		minecraft->textures->bind(bg);
		glColor4f2(1, 1, 1, 1);
		blit(px, py, 0, 0, 176, 166);
	}

	drawString(minecraft->font, I18n::get("container.furnace"), px + 56, py + 6, 0xffffffff);
	drawString(minecraft->font, I18n::get("container.inventory"), px + 8, py + 72, 0xffffffff);

	// Flame: 14px tall, burns bottom-up. Arrow: 24px wide, fills left-right.
	if (furnace && !furnaceGone()) {
		int lit = furnace->getLitProgress(14);
		if (lit > 0)
			blit(px + 56, py + 36 + 14 - lit, 176, 14 - lit, 14, lit);
		int burn = furnace->getBurnProgress(24);
		if (burn > 0)
			blit(px + 79, py + 34, 176, 14, burn, 17);
	}

	for (int i = 0; i <= 38; i++) {
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

void BetaFurnaceScreen::tick() {
	if (furnaceGone() && minecraft) {
		minecraft->setScreen(NULL);
		return;
	}
	super::tick();
}

static bool acceptsFurnaceSlot(int betaIdx, const ItemInstance* item);

void BetaFurnaceScreen::placeInto(int slot) {
	if (!hasCarried || carried.isNull())
		return;
	if (slot <= 2 && !acceptsFurnaceSlot(slot, &carried))
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
		} else if (slot > 2) {
			ItemInstance tmp = *dst;
			setSlotItem(slot, &carried);
			carried = tmp;
		}
	} else if (slot > 2) {
		ItemInstance tmp = *dst;
		setSlotItem(slot, &carried);
		carried = tmp;
	}
}

void BetaFurnaceScreen::placeOneInto(int slot) {
	if (!hasCarried || carried.isNull() || slot == 2)
		return;
	if (slot <= 1 && !acceptsFurnaceSlot(slot, &carried))
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

// Same press-based model as BetaInventoryScreen.
void BetaFurnaceScreen::mouseClicked(int x, int y, int buttonNum) {
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
		if (pressSlot < 0 || pressSlot == 2 || shift)
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

static bool acceptsFurnaceSlot(int betaIdx, const ItemInstance* item) {
	if (betaIdx == 2)
		return false; // result is take-only
	if (!item || item->isNull())
		return true;
	if (betaIdx == 0)
		return canSmelt(item);
	if (betaIdx == 1)
		return canBurn(item);
	return true;
}

void BetaFurnaceScreen::mouseReleased(int x, int y, int buttonNum) {
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
	if (slot == pressSlot || slot == 2)
		return;
	placeInto(slot);
}

void BetaFurnaceScreen::keyPressed(int eventKey) {
	super::keyPressed(eventKey);
	if (eventKey == Keyboard::KEY_E && minecraft && !minecraft->isCreativeMode())
		minecraft->setScreen(NULL);
}

void BetaFurnaceScreen::removed() {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (player && in && hasCarried && !carried.isNull()) {
		in->add(&carried);
		if (!carried.isNull() && carried.count > 0)
			in->doDrop(&carried, true);
		carried.setNull();
		hasCarried = false;
	}
}
