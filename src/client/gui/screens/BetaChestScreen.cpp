#include "BetaChestScreen.h"

#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/Container.h"
#include "../../../world/item/Item.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../locale/I18n.h"
#include <cstdio>
#include "../../../world/CompoundContainer.h"
#include <algorithm>

BetaChestScreen::BetaChestScreen(Container* container)
	: container(container), hasCarried(false), pressed(false),
	  pressSlot(-1), pressButton(0), pressPickedUp(false) {
	carried.setNull();
}

BetaChestScreen::~BetaChestScreen() {
	if (dynamic_cast<CompoundContainer*>(container)) {
		delete container;
		container = NULL;
	}
}

void BetaChestScreen::init() {
	if (container) {
		container->startOpen();
	}
}

void BetaChestScreen::setupPositions() {
}

int BetaChestScreen::numChestRows() const {
	int rows = container ? (container->getContainerSize() / 9) : 3;
	if (rows < 3) rows = 3;
	if (rows > 6) rows = 6;
	return rows;
}

int BetaChestScreen::panelH() const {
	return 114 + numChestRows() * 18;
}

bool BetaChestScreen::slotPos(int idx, int& sx, int& sy) const {
	int rows = numChestRows();
	int chestSlots = rows * 9;
	if (idx >= 0 && idx < chestSlots) {
		sx = 8 + (idx % 9) * 18;
		sy = 18 + (idx / 9) * 18;
		return true;
	}
	int invIdx = idx - chestSlots;
	if (invIdx >= 0 && invIdx < 27) { // Player main inventory (3 rows)
		sx = 8 + (invIdx % 9) * 18;
		sy = 18 + rows * 18 + 12 + (invIdx / 9) * 18;
		return true;
	}
	if (invIdx >= 27 && invIdx < 36) { // Player hotbar (1 row)
		sx = 8 + (invIdx - 27) * 18;
		sy = 18 + rows * 18 + 12 + 3 * 18 + 4;
		return true;
	}
	return false;
}

int BetaChestScreen::slotAt(int x, int y) const {
	int px = panelX(), py = panelY();
	int totalSlots = numChestRows() * 9 + 36;
	for (int i = 0; i < totalSlots; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		if (x >= px + sx && x < px + sx + 18 && y >= py + sy && y < py + sy + 18)
			return i;
	}
	return -1;
}

Inventory* BetaChestScreen::inv() {
	return (minecraft && minecraft->player) ? minecraft->player->inventory : NULL;
}

ItemInstance* BetaChestScreen::getSlotItem(int idx) {
	int rows = numChestRows();
	int chestSlots = rows * 9;
	if (idx >= 0 && idx < chestSlots) {
		return container ? container->getItem(idx) : NULL;
	}
	int invIdx = idx - chestSlots;
	Inventory* pInv = inv();
	if (!pInv) return NULL;
	if (invIdx >= 0 && invIdx < 27) {
		return pInv->getItem(invIdx + 9);
	}
	if (invIdx >= 27 && invIdx < 36) {
		return pInv->getItem(invIdx - 27);
	}
	return NULL;
}

void BetaChestScreen::setSlotItem(int idx, const ItemInstance* item) {
	int rows = numChestRows();
	int chestSlots = rows * 9;
	if (idx >= 0 && idx < chestSlots) {
		if (container) {
			ItemInstance copy = (item && !item->isNull() && item->count > 0) ? *item : ItemInstance();
			container->setItem(idx, &copy);
		}
		return;
	}
	int invIdx = idx - chestSlots;
	Inventory* pInv = inv();
	if (!pInv) return;
	int peIdx = -1;
	if (invIdx >= 0 && invIdx < 27) peIdx = invIdx + 9;
	else if (invIdx >= 27 && invIdx < 36) peIdx = invIdx - 27;

	if (peIdx >= 0) {
		if (item && !item->isNull() && item->count > 0)
			pInv->setItem(peIdx, new ItemInstance(*item));
		else
			pInv->setItem(peIdx, NULL);
	}
}

bool BetaChestScreen::mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse) {
	if (stack.isNull() || stack.count <= 0)
		return false;

	int start = reverse ? (to - 1) : from;
	int end = reverse ? (from - 1) : to;
	int step = reverse ? -1 : 1;

	// First pass: stack onto existing matching items
	for (int i = start; i != end; i += step) {
		ItemInstance* target = getSlotItem(i);
		if (target && !target->isNull() && target->id == stack.id
			&& target->getAuxValue() == stack.getAuxValue()) {
			int maxStack = target->getItem() ? target->getItem()->getMaxStackSize() : 64;
			int available = maxStack - target->count;
			if (available > 0) {
				int toAdd = std::min(available, stack.count);
				target->count += toAdd;
				stack.count -= toAdd;
				setSlotItem(i, target);
				if (stack.count <= 0) {
					stack.setNull();
					return true;
				}
			}
		}
	}

	// Second pass: empty slots
	for (int i = start; i != end; i += step) {
		ItemInstance* target = getSlotItem(i);
		if (!target || target->isNull() || target->count <= 0) {
			setSlotItem(i, &stack);
			stack.setNull();
			return true;
		}
	}

	return false;
}

bool BetaChestScreen::quickTransfer(int idx) {
	ItemInstance* it = getSlotItem(idx);
	if (!it || it->isNull() || it->count <= 0)
		return false;

	int rows = numChestRows();
	int chestSlots = rows * 9;
	ItemInstance moving = *it;

	if (idx < chestSlots) {
		// From chest -> into player inventory (hotbar first, then main)
		mergeIntoRange(moving, chestSlots + 27, chestSlots + 36, false);
		if (moving.count > 0) {
			mergeIntoRange(moving, chestSlots, chestSlots + 27, false);
		}
	} else {
		// From player inventory -> into chest
		mergeIntoRange(moving, 0, chestSlots, false);
	}

	if (moving.count <= 0) {
		setSlotItem(idx, NULL);
	} else {
		setSlotItem(idx, &moving);
	}
	return true;
}

void BetaChestScreen::placeInto(int slot) {
	ItemInstance* existing = getSlotItem(slot);
	if (!existing || existing->isNull() || existing->count <= 0) {
		setSlotItem(slot, &carried);
		carried.setNull();
		hasCarried = false;
		return;
	}
	if (existing->id == carried.id && existing->getAuxValue() == carried.getAuxValue()) {
		int maxStack = existing->getItem() ? existing->getItem()->getMaxStackSize() : 64;
		int space = maxStack - existing->count;
		if (space > 0) {
			int add = std::min(space, carried.count);
			existing->count += add;
			carried.count -= add;
			setSlotItem(slot, existing);
			if (carried.count <= 0) {
				carried.setNull();
				hasCarried = false;
			}
		}
		return;
	}
	// Swap
	ItemInstance tmp = *existing;
	setSlotItem(slot, &carried);
	carried = tmp;
	hasCarried = true;
}

void BetaChestScreen::placeOneInto(int slot) {
	ItemInstance* existing = getSlotItem(slot);
	if (!existing || existing->isNull() || existing->count <= 0) {
		ItemInstance one = carried;
		one.count = 1;
		setSlotItem(slot, &one);
		carried.count--;
		if (carried.count <= 0) {
			carried.setNull();
			hasCarried = false;
		}
		return;
	}
	if (existing->id == carried.id && existing->getAuxValue() == carried.getAuxValue()) {
		int maxStack = existing->getItem() ? existing->getItem()->getMaxStackSize() : 64;
		if (existing->count < maxStack) {
			existing->count++;
			setSlotItem(slot, existing);
			carried.count--;
			if (carried.count <= 0) {
				carried.setNull();
				hasCarried = false;
			}
		}
	}
}

void BetaChestScreen::spillCarried() {
	if (!hasCarried || carried.isNull() || carried.count <= 0)
		return;

	Inventory* pInv = inv();
	if (pInv) {
		// Try placing into hotbar first, then main inventory
		for (int i = 0; i < 36 && carried.count > 0; i++) {
			ItemInstance* cur = pInv->getItem(i);
			if (cur && !cur->isNull() && cur->id == carried.id && cur->getAuxValue() == carried.getAuxValue()) {
				int maxStack = cur->getItem() ? cur->getItem()->getMaxStackSize() : 64;
				int space = maxStack - cur->count;
				if (space > 0) {
					int add = std::min(space, carried.count);
					cur->count += add;
					carried.count -= add;
				}
			}
		}
		for (int i = 0; i < 36 && carried.count > 0; i++) {
			ItemInstance* cur = pInv->getItem(i);
			if (!cur || cur->isNull() || cur->count <= 0) {
				pInv->setItem(i, new ItemInstance(carried));
				carried.setNull();
				hasCarried = false;
				return;
			}
		}
	}

	if (carried.count > 0 && minecraft && minecraft->player) {
		minecraft->player->drop(&carried, false);
	}
	carried.setNull();
	hasCarried = false;
}

void BetaChestScreen::removed() {
	spillCarried();
	if (container) {
		container->stopOpen();
	}
}

void BetaChestScreen::tick() {
	super::tick();
	if (container && minecraft && minecraft->player) {
		if (!container->stillValid(minecraft->player)) {
			minecraft->setScreen(NULL);
		}
	}
}

void BetaChestScreen::mouseClicked(int x, int y, int buttonNum) {
	super::mouseClicked(x, y, buttonNum);
	int slot = slotAt(x, y);

	// Shift-click quick transfer
	bool shift = Keyboard::isKeyDown(Keyboard::KEY_LSHIFT);
	if (shift && slot >= 0 && buttonNum == MouseAction::ACTION_LEFT && !hasCarried) {
		quickTransfer(slot);
		return;
	}

	if (slot >= 0) {
		pressed = true;
		pressSlot = slot;
		pressButton = buttonNum;
		pressPickedUp = false;

		ItemInstance* target = getSlotItem(slot);
		if (!hasCarried) {
			if (target && !target->isNull() && target->count > 0) {
				if (buttonNum == MouseAction::ACTION_LEFT) {
					carried = *target;
					hasCarried = true;
					setSlotItem(slot, NULL);
					pressPickedUp = true;
				} else if (buttonNum == MouseAction::ACTION_RIGHT) {
					int half = (target->count + 1) / 2;
					carried = *target;
					carried.count = half;
					hasCarried = true;
					target->count -= half;
					if (target->count <= 0) setSlotItem(slot, NULL);
					else setSlotItem(slot, target);
					pressPickedUp = true;
				}
			}
		} else {
			if (buttonNum == MouseAction::ACTION_LEFT) {
				placeInto(slot);
			} else if (buttonNum == MouseAction::ACTION_RIGHT) {
				placeOneInto(slot);
			}
		}
	} else if (hasCarried) {
		// Click outside the panel drops carried item
		int px = panelX(), py = panelY();
		int ph = panelH();
		if (x < px || x > px + PANEL_W || y < py || y > py + ph) {
			if (buttonNum == MouseAction::ACTION_LEFT) {
				if (minecraft && minecraft->player) {
					minecraft->player->drop(&carried, false);
				}
				carried.setNull();
				hasCarried = false;
			} else if (buttonNum == MouseAction::ACTION_RIGHT) {
				ItemInstance one = carried;
				one.count = 1;
				if (minecraft && minecraft->player) {
					minecraft->player->drop(&one, false);
				}
				carried.count--;
				if (carried.count <= 0) {
					carried.setNull();
					hasCarried = false;
				}
			}
		}
	}
}

void BetaChestScreen::mouseReleased(int x, int y, int buttonNum) {
	super::mouseReleased(x, y, buttonNum);
	pressed = false;
	pressSlot = -1;
}

void BetaChestScreen::keyPressed(int eventKey) {
	if (eventKey == Keyboard::KEY_ESCAPE || eventKey == Keyboard::KEY_E
		|| eventKey == minecraft->options.getIntValue(OPTIONS_KEY_INVENTORY)) {
		minecraft->setScreen(NULL);
		return;
	}
	super::keyPressed(eventKey);
}

void BetaChestScreen::render(int xm, int ym, float a) {
	renderBackground();

	int px = panelX();
	int py = panelY();
	int rows = numChestRows();

	TextureId bg = minecraft->textures->loadTexture("gui/container.png");
	if (Textures::isTextureIdValid(bg)) {
		minecraft->textures->bind(bg);
		glColor4f2(1, 1, 1, 1);
		// Top part: chest rows
		blit(px, py, 0, 0, PANEL_W, rows * 18 + 17);
		// Bottom part: player inventory
		blit(px, py + rows * 18 + 17, 0, 126, PANEL_W, 96);
	}

	std::string title = (container && !container->getName().empty()) ? container->getName() : "Chest";
	if (title == "Large chest") title = I18n::get("container.chestDouble");
	else if (title == "Chest") title = I18n::get("container.chest");
	drawString(minecraft->font, title, px + 8, py + 6, 0xffffffff);
	drawString(minecraft->font, I18n::get("container.inventory"), px + 8, py + (panelH() - 96 + 2), 0xffffffff);

	int totalSlots = rows * 9 + 36;
	int hoveredSlot = slotAt(xm, ym);

	for (int i = 0; i < totalSlots; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		int x0 = px + sx, y0 = py + sy;
		ItemInstance* it = getSlotItem(i);
		if (it && !it->isNull() && it->count > 0) {
			ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, it, x0 + 1, y0 + 1, true);
			ItemRenderer::renderGuiItemDecorations(minecraft->font, minecraft->textures, it, x0 + 1, y0 + 1);
		}
		if (i == hoveredSlot) {
			glDisable2(GL_TEXTURE_2D);
			fill(x0 + 1, y0 + 1, x0 + 17, y0 + 17, 0x80ffffff);
			glEnable2(GL_TEXTURE_2D);
		}
	}

	// Tooltip for hovered slot
	if (!hasCarried && hoveredSlot >= 0) {
		ItemInstance* hoverItem = getSlotItem(hoveredSlot);
		if (hoverItem && !hoverItem->isNull() && hoverItem->count > 0) {
			std::string name = hoverItem->getName();
			if (!name.empty()) {
				int tw = minecraft->font->width(name);
				int tx = xm + 12;
				int ty = ym - 12;
				if (tx + tw + 6 > width) tx = width - tw - 6;
				if (ty < 6) ty = 6;
				fill(tx - 3, ty - 3, tx + tw + 3, ty + 11, 0xf0100010);
				drawString(minecraft->font, name, tx, ty, 0xffffffff);
			}
		}
	}

	// Floating carried item following cursor
	if (hasCarried && !carried.isNull() && carried.count > 0) {
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &carried, xm - 8, ym - 8, true);
		ItemRenderer::renderGuiItemDecorations(minecraft->font, minecraft->textures, &carried, xm - 8, ym - 8);
	}
}
