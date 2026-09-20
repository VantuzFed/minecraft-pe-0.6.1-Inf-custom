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
	  hasCarried(false), pressed(false), pressSlot(-1), pressButton(0) {
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

int BetaFurnaceScreen::resolveHotbar(int betaIdx) {
	Inventory* in = inv();
	if (!in || betaIdx < 30 || betaIdx > 38)
		return -1;
	int real = in->linkedSlots[betaIdx - 30].inventorySlot;
	if (real >= 9 && real < in->getContainerSize())
		return real;
	return -1;
}

int BetaFurnaceScreen::ensureHotbarLink(int betaIdx) {
	Inventory* in = inv();
	if (!in || betaIdx < 30 || betaIdx > 38)
		return -1;
	int real = resolveHotbar(betaIdx);
	if (real >= 0)
		return real;
	int link = betaIdx - 30;
	for (int s = 9; s < in->getContainerSize(); s++) {
		ItemInstance* it = in->getItem(s);
		if (!it || it->isNull()) {
			if (in->linkSlot(link, s, false))
				return s;
			return -1;
		}
	}
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
	if (betaIdx >= 3 && betaIdx <= 29) {
		ItemInstance* it = in->getItem(betaIdx + 6);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	if (betaIdx >= 30 && betaIdx <= 38) {
		ItemInstance* it = in->getItem(betaIdx - 30);
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
	if (betaIdx >= 30 && betaIdx <= 38) {
		int real = resolveHotbar(betaIdx);
		if (real < 0 && !empty)
			real = ensureHotbarLink(betaIdx);
		if (real < 0)
			return;
		if (empty) in->clearSlot(real);
		else in->setItem(real, const_cast<ItemInstance*>(item));
		return;
	}
	int pe = -1;
	if (betaIdx >= 3 && betaIdx <= 29)
		pe = betaIdx + 6;
	if (pe >= 0) {
		if (empty) in->clearSlot(pe);
		else in->setItem(pe, const_cast<ItemInstance*>(item));
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

bool BetaFurnaceScreen::mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse) {
	if (stack.isNull())
		return false;
	bool moved = false;
	if (stack.isStackable()) {
		for (int pass = 0; pass < 2 && !stack.isNull(); pass++) {
			for (int i = from; i < to; i++) {
				int idx = reverse ? (to - 1 - (i - from)) : i;
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
					stack.setNull();
					return true;
				}
			}
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

bool BetaFurnaceScreen::quickTransfer(int betaIdx) {
	ItemInstance* src = getSlotItem(betaIdx);
	if (!src || src->isNull())
		return false;
	ItemInstance stack = *src;
	bool moved = false;
	if (betaIdx == 2) {
		moved = mergeIntoRange(stack, 3, 39, false);
	} else if (betaIdx == 0 || betaIdx == 1) {
		moved = mergeIntoRange(stack, 3, 39, false);
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
			moved = mergeIntoRange(stack, 30, 39, false);
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
			moved = mergeIntoRange(stack, 3, 30, false);
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
	int mx = xm * width / minecraft->width;
	int my = ym * height / minecraft->height - 1;
	int px = panelX(), py = panelY();

	TextureId bg = minecraft->textures->loadTexture("gui/furnace.png");
	if (Textures::isTextureIdValid(bg)) {
		minecraft->textures->bind(bg);
		glColor4f2(1, 1, 1, 1);
		blit(px, py, 0, 0, 176, 166, 256, 256);
	}

	drawString(minecraft->font, I18n::get("container.furnace"), px + 56, py + 6, 0xff404040);
	drawString(minecraft->font, I18n::get("container.inventory"), px + 8, py + 72, 0xff404040);

	// Flame: 14px tall, burns bottom-up. Arrow: 24px wide, fills left-right.
	if (furnace && !furnaceGone()) {
		int lit = furnace->getLitProgress(14);
		if (lit > 0)
			blit(px + 56, py + 36 + 14 - lit, 176, 14 - lit, 14, lit, 256, 256);
		int burn = furnace->getBurnProgress(24);
		if (burn > 0)
			blit(px + 79, py + 34, 176, 14, burn, 17, 256, 256);
	}

	for (int i = 0; i <= 38; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		int x0 = px + sx, y0 = py + sy;
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

	int hover = slotAt(mx, my);
	if (hover >= 0) {
		int sx, sy;
		if (slotPos(hover, sx, sy))
			fill(px + sx + 1, py + sy + 1, px + sx + 17, py + sy + 17, 0x80ffffff);
	}

	if (hasCarried && !carried.isNull()) {
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &carried, (float)(mx - 8), (float)(my - 8), true);
		if (carried.count > 1) {
			char buf[16];
			sprintf(buf, "%d", carried.count);
			minecraft->font->drawShadow(buf, (float)(mx + 8 - minecraft->font->width(buf)), (float)(my + 1), 0xffffffff);
		}
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

void BetaFurnaceScreen::mouseClicked(int x, int y, int buttonNum) {
	if (buttonNum != MouseAction::ACTION_LEFT && buttonNum != MouseAction::ACTION_RIGHT)
		return;
	pressed = true;
	pressSlot = slotAt(x, y);
	pressButton = buttonNum;
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
		ItemInstance* dst = getSlotItem(slot);
		if (!hasCarried) {
			if (dst && !dst->isNull()) {
				carried = *dst;
				hasCarried = true;
				ItemInstance empty;
				empty.setNull();
				setSlotItem(slot, &empty);
			}
		} else if (slot <= 2 && !acceptsFurnaceSlot(slot, &carried)) {
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
				if (slot > 2) {
					ItemInstance tmp = *dst;
					setSlotItem(slot, &carried);
					carried = tmp;
				}
			}
		} else {
			if (slot > 2) {
				ItemInstance tmp = *dst;
				setSlotItem(slot, &carried);
				carried = tmp;
			}
		}
	} else {
		if (slot < 0 || slot == 2)
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
		} else if (slot <= 1 && !acceptsFurnaceSlot(slot, &carried)) {
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
