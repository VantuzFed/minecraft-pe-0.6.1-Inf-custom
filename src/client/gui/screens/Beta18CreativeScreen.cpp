#include "Beta18CreativeScreen.h"

#include "../../Minecraft.h"
#include "../../renderer/Textures.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../renderer/entity/EntityRenderDispatcher.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ArmorItem.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/time.h"
#include <cstdio>

Beta18CreativeScreen::Beta18CreativeScreen()
	: hasCarried(false), scroll(0.0f),
	  pressed(false), pressSlot(-1), pressButton(0), pressPickedUp(false) {
	for (int i = 0; i < VIEW_SIZE; i++)
		viewport[i].setNull();
	carried.setNull();
}

void Beta18CreativeScreen::init() {
	buildFullList();
	setScroll(0.0f);
}

void Beta18CreativeScreen::setupPositions() {
}

bool Beta18CreativeScreen::slotPos(int betaIdx, int& sx, int& sy) {
	if (betaIdx >= 0 && betaIdx < VIEW_SIZE) {
		sx = 8 + (betaIdx % GRID_COLS) * 18;
		sy = 18 + (betaIdx / GRID_COLS) * 18;
		return true;
	}
	if (betaIdx >= 72 && betaIdx <= 80) {
		sx = 8 + (betaIdx - 72) * 18;
		sy = 184;
		return true;
	}
	return false;
}

int Beta18CreativeScreen::slotAt(int x, int y) const {
	int px = panelX(), py = panelY();
	for (int i = 0; i <= 80; i++) {
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		if (x >= px + sx && x < px + sx + 18 && y >= py + sy && y < py + sy + 18)
			return i;
	}
	return -1;
}

Inventory* Beta18CreativeScreen::inv() {
	Player* player = minecraft ? minecraft->player : NULL;
	if (!player)
		return NULL;
	return player->inventory;
}

int Beta18CreativeScreen::resolveHotbar(int betaIdx) {
	Inventory* in = inv();
	if (!in || betaIdx < 72 || betaIdx > 80)
		return -1;
	int link = betaIdx - 72;
	int real = in->linkedSlots[link].inventorySlot;
	if (real >= 9 && real < in->getContainerSize())
		return real;
	return -1;
}

int Beta18CreativeScreen::ensureHotbarLink(int betaIdx) {
	Inventory* in = inv();
	if (!in || betaIdx < 72 || betaIdx > 80)
		return -1;
	int real = resolveHotbar(betaIdx);
	if (real >= 0)
		return real;
	int link = betaIdx - 72;
	for (int s = 9; s < in->getContainerSize(); s++) {
		ItemInstance* it = in->getItem(s);
		if (!it || it->isNull()) {
			for (int l = 0; l < in->numLinkedSlots; l++) {
				if (l != link && in->linkedSlots[l].inventorySlot == s)
					in->linkedSlots[l].inventorySlot = -1;
			}
			if (in->linkSlot(link, s, false))
				return s;
			return -1;
		}
	}
	return -1;
}

int Beta18CreativeScreen::betaToPe(int betaIdx) {
	if (betaIdx >= 72 && betaIdx <= 80)
		return resolveHotbar(betaIdx);
	return -1;
}

bool Beta18CreativeScreen::isLinkedMain(int peSlot) {
	Inventory* in = inv();
	if (!in || peSlot < 9 || peSlot >= in->getContainerSize())
		return false;
	for (int l = 0; l < in->numLinkedSlots; l++) {
		if (in->linkedSlots[l].inventorySlot == peSlot)
			return true;
	}
	return false;
}

ItemInstance* Beta18CreativeScreen::getSlotItem(int betaIdx) {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return NULL;
	if (betaIdx >= 0 && betaIdx < VIEW_SIZE) {
		ItemInstance& it = viewport[betaIdx];
		return it.isNull() ? NULL : &it;
	}
	if (betaIdx >= 72 && betaIdx <= 80) {
		ItemInstance* it = in->getItem(betaIdx - 72);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	return NULL;
}

void Beta18CreativeScreen::setSlotItem(int betaIdx, const ItemInstance* item) {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return;
	bool empty = !item || item->isNull();
	if (betaIdx >= 72 && betaIdx <= 80) {
		int real = resolveHotbar(betaIdx);
		if (real < 0 && !empty)
			real = ensureHotbarLink(betaIdx);
		if (real < 0)
			return;
		if (empty) {
			in->clearSlot(real);
			in->linkedSlots[betaIdx - 72].inventorySlot = -1;
		} else in->setItem(real, const_cast<ItemInstance*>(item));
		for (int l = 0; l < in->numLinkedSlots; l++) {
			if (l != betaIdx - 72 && in->linkedSlots[l].inventorySlot == real)
				in->linkedSlots[l].inventorySlot = -1;
		}
	}
}

// Blocks never shown in creative (fluids, fire, portal, technical).
static bool isHiddenCreativeBlock(int id) {
	return id == 0 || id == 8 || id == 9 || id == 10 || id == 11
		|| id == 34 || id == 36 || id == 51 || id == 90;
}

// Damage variants per Beta 1.8 creative (only when stacked by data).
static int creativeVariants(int id) {
	if (id == 35) return 16; // wool
	if (id == 44) return 6;  // stone slab
	if (id == 6 || id == 17 || id == 18 || id == 31) return 3;
	return 1;
}

void Beta18CreativeScreen::buildFullList() {
	fullList.clear();
	for (int id = 1; id < Tile::NUM_BLOCK_TYPES; id++) {
		if (!Tile::tiles[id] || isHiddenCreativeBlock(id))
			continue;
		int variants = 1;
		if (Item::items[id] && Item::items[id]->isStackedByData())
			variants = creativeVariants(id);
		for (int aux = 0; aux < variants; aux++)
			fullList.push_back(ItemInstance(id, 1, aux));
	}
	for (int id = 256; id < Item::MAX_ITEMS; id++) {
		if (id == 351) // dyes appended with variants below
			continue;
		if (!Item::items[id])
			continue;
		fullList.push_back(ItemInstance(id, 1, 0));
	}
	if (Item::MAX_ITEMS > 351 && Item::items[351]) {
		for (int aux = 1; aux <= 15; aux++)
			fullList.push_back(ItemInstance(351, 1, aux));
	}
}

void Beta18CreativeScreen::refreshViewport() {
	for (int i = 0; i < VIEW_SIZE; i++)
		viewport[i].setNull();
	int rows = (int)fullList.size() / GRID_COLS - GRID_ROWS + 1;
	if (rows < 1) rows = 1;
	int start = (int)(scroll * rows + 0.5f);
	if (start < 0) start = 0;
	for (int row = 0; row < GRID_ROWS; row++) {
		for (int col = 0; col < GRID_COLS; col++) {
			int idx = col + (row + start) * GRID_COLS;
			if (idx >= 0 && idx < (int)fullList.size())
				viewport[col + row * GRID_COLS] = fullList[(size_t)idx];
		}
	}
}

void Beta18CreativeScreen::setScroll(float g) {
	if (g < 0.0f) g = 0.0f;
	if (g > 1.0f) g = 1.0f;
	scroll = g;
	refreshViewport();
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

// Normal survival place/merge/swap into a hotbar cell.
void Beta18CreativeScreen::placeIntoHotbar(int slot) {
	if (!hasCarried || carried.isNull() || slot < 72 || slot > 80)
		return;
	ItemInstance* dst = getSlotItem(slot);
	if (!dst || dst->isNull()) {
		setSlotItem(slot, &carried);
		ItemInstance* check = getSlotItem(slot);
		if (!check || check->isNull())
			return;
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

// Shift-click from the hotbar: merge into hidden mains (skip linked).
bool Beta18CreativeScreen::mergeIntoHotbar(ItemInstance& stack) {
	Inventory* in = inv();
	if (!in || stack.isNull())
		return false;
	bool moved = false;
	if (stack.isStackable()) {
		for (int pass = 0; pass < 2 && !stack.isNull(); pass++) {
			for (int s = 9; s < in->getContainerSize(); s++) {
				ItemInstance* dst = in->getItem(s);
				if (pass == 0) {
					if (!dst || dst->isNull() || !sameStack(&stack, dst))
						continue;
					int space = stack.getMaxStackSize() - dst->count;
					if (space <= 0)
						continue;
					int take = stack.count < space ? stack.count : space;
					dst->count += take;
					stack.count -= take;
					moved = true;
				} else {
					if (dst && !dst->isNull())
						continue;
					if (isLinkedMain(s))
						continue;
					ItemInstance v = stack;
					in->setItem(s, &v);
					ItemInstance* check = in->getItem(s);
					if (!check || check->isNull())
						continue;
					stack.setNull();
					return true;
				}
			}
		}
	} else {
		for (int s = 9; s < in->getContainerSize(); s++) {
			if (isLinkedMain(s))
				continue;
			ItemInstance* dst = in->getItem(s);
			if (!dst || dst->isNull()) {
				ItemInstance v = stack;
				in->setItem(s, &v);
				ItemInstance* check = in->getItem(s);
				if (!check || check->isNull())
					continue;
				stack.setNull();
				return true;
			}
		}
	}
	return moved;
}

void Beta18CreativeScreen::mouseClicked(int x, int y, int buttonNum) {
	if (buttonNum != MouseAction::ACTION_LEFT && buttonNum != MouseAction::ACTION_RIGHT)
		return;
	pressed = true;
	pressSlot = slotAt(x, y);
	pressButton = buttonNum;
	pressPickedUp = false;
	Player* player = minecraft ? minecraft->player : NULL;
	bool shift = Keyboard::isKeyDown(Keyboard::KEY_LSHIFT);

	// Scrollbar track jump (no drag-motion events in this engine, so a
	// click jumps; the wheel scrolls row by row like the original).
	{
		int px = panelX(), py = panelY();
		if (x >= px + 154 && x < px + 170 && y >= py + 17 && y < py + 179) {
			float g = (float)(y - (py + 25)) / 146.0f;
			setScroll(g);
			pressSlot = -2;
			return;
		}
	}

	if (pressButton == MouseAction::ACTION_LEFT) {
		if (pressSlot < 0) {
			// Outside: drop the carried stack into the world.
			if (hasCarried && !carried.isNull() && player) {
				player->drop(new ItemInstance(carried), false);
				carried.setNull();
				hasCarried = false;
			}
			return;
		}
		if (pressSlot < VIEW_SIZE) {
			// Creative grid: infinite source, the cell never mutates.
			ItemInstance* cell = getSlotItem(pressSlot);
			if (!hasCarried) {
				if (cell && !cell->isNull()) {
					carried = *cell;
					if (shift)
						carried.count = carried.getMaxStackSize();
					hasCarried = true;
					pressPickedUp = true;
				}
				return;
			}
			if (!cell || cell->isNull()) {
				carried.setNull();
				hasCarried = false;
				return;
			}
			if (sameStack(&carried, cell)) {
				int max = carried.getMaxStackSize();
				if (shift)
					carried.count = max;
				else if (carried.count < max)
					carried.count++;
				return;
			}
			carried.setNull();
			hasCarried = false;
			return;
		}
		// Hotbar.
		if (shift) {
			ItemInstance* src = getSlotItem(pressSlot);
			if (!src || src->isNull())
				return;
			ItemInstance stack = *src;
			if (mergeIntoHotbar(stack)) {
				if (stack.isNull()) {
					ItemInstance empty;
					empty.setNull();
					setSlotItem(pressSlot, &empty);
				} else {
					setSlotItem(pressSlot, &stack);
				}
			}
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
		placeIntoHotbar(pressSlot);
	} else {
		if (pressSlot < 0 || shift)
			return;
		if (pressSlot < VIEW_SIZE) {
			// Right click on the grid: same-ID shrinks, else clears.
			ItemInstance* cell = getSlotItem(pressSlot);
			if (!hasCarried) {
				if (cell && !cell->isNull()) {
					carried = *cell;
					hasCarried = true;
					pressPickedUp = true;
				}
				return;
			}
			if (cell && !cell->isNull() && sameStack(&carried, cell)) {
				if (carried.count > 1)
					carried.count--;
				else {
					carried.setNull();
					hasCarried = false;
				}
				return;
			}
			carried.setNull();
			hasCarried = false;
			return;
		}
		// Hotbar right click: half pickup / place one.
		ItemInstance* dst = getSlotItem(pressSlot);
		if (!hasCarried) {
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
		if (!dst || dst->isNull()) {
			ItemInstance one = carried;
			one.count = 1;
			setSlotItem(pressSlot, &one);
			ItemInstance* check = getSlotItem(pressSlot);
			if (!check || check->isNull())
				return;
			carried.count--;
			if (carried.count <= 0) { carried.setNull(); hasCarried = false; }
		} else if (sameStack(&carried, dst) && dst->count < dst->getMaxStackSize()) {
			dst->count++;
			carried.count--;
			if (carried.count <= 0) { carried.setNull(); hasCarried = false; }
		}
	}
}

void Beta18CreativeScreen::mouseReleased(int x, int y, int buttonNum) {
	if (!pressed || buttonNum != pressButton) {
		pressed = false;
		return;
	}
	pressed = false;
	if (pressSlot == -2)
		return;
	if (!pressPickedUp || !hasCarried || carried.isNull())
		return;
	pressPickedUp = false;
	int slot = slotAt(x, y);
	if (slot < 0) {
		// Released outside: drop like a press outside does.
		Player* player = minecraft ? minecraft->player : NULL;
		if (player) {
			player->drop(new ItemInstance(carried), false);
			carried.setNull();
			hasCarried = false;
		}
		return;
	}
	if (slot == pressSlot || slot < VIEW_SIZE)
		return;
	placeIntoHotbar(slot);
}

void Beta18CreativeScreen::mouseWheel(int dx, int dy, int xm, int ym) {
	(void)dx; (void)xm; (void)ym;
	int rows = (int)fullList.size() / GRID_COLS - GRID_ROWS + 1;
	if (rows < 1) rows = 1;
	int step = dy > 0 ? -1 : 1;
	setScroll(scroll + (float)step / (float)rows);
}

void Beta18CreativeScreen::keyPressed(int eventKey) {
	super::keyPressed(eventKey);
	if (eventKey == Keyboard::KEY_E && minecraft && minecraft->isCreativeMode())
		minecraft->setScreen(NULL);
}

void Beta18CreativeScreen::removed() {
	// Carried goes back to the player inventory (or drops to the world).
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return;
	if (hasCarried && !carried.isNull()) {
		in->add(&carried);
		if (!carried.isNull() && carried.count > 0)
			player->drop(new ItemInstance(carried), false);
		carried.setNull();
		hasCarried = false;
	}
}

void Beta18CreativeScreen::render(int xm, int ym, float a) {
	renderBackground();
	int mx = xm;
	int my = ym - 1;
	int px = panelX(), py = panelY();

	TextureId bg = minecraft->textures->loadTexture("gui/allitems.png");
	if (Textures::isTextureIdValid(bg)) {
		minecraft->textures->bind(bg);
		glColor4f2(1, 1, 1, 1);
		blit(px, py, 0, 0, PANEL_W, PANEL_H);
	}

	drawString(minecraft->font, "Item selection", px + 8, py + 6, 0xff404040);

	for (int i = 0; i <= 80; i++) {
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

	// Scrollbar thumb, like the original (16x16 at v=208).
	{
		TextureId bg2 = minecraft->textures->loadTexture("gui/allitems.png");
		if (Textures::isTextureIdValid(bg2)) {
			minecraft->textures->bind(bg2);
			glColor4f2(1, 1, 1, 1);
			int ty = py + 17 + (int)(145.0f * scroll);
			blit(px + 154, ty, 0, 208, 16, 16);
		}
	}

	super::render(xm, ym, a);
}

void Beta18CreativeScreen::tick() {
	super::tick();
}
