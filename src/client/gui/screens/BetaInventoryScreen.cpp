#include "BetaInventoryScreen.h"

#include "../../Minecraft.h"
#include "../../renderer/Lighting.h"
#include "../../renderer/Textures.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../renderer/entity/EntityRenderDispatcher.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/inventory/CraftingContainer.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ArmorItem.h"
#include "../../../world/item/crafting/Recipes.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/time.h"
#include "../../../util/Mth.h"
#include "../../../SharedConstants.h"
#include "../../../locale/I18n.h"
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
	  pressed(false), pressSlot(-1), pressButton(0), pressPickedUp(false) {
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
	if (betaIdx >= 9 && betaIdx <= 35) { // main, 3 rows like beta
		int k = betaIdx - 9;
		sx = 8 + (k % 9) * 18;
		sy = 84 + (k / 9) * 18;
		return true;
	}
	if (betaIdx >= 36 && betaIdx <= 44) { // hotbar
		sx = 8 + (betaIdx - 36) * 18;
		sy = 142;
		return true;
	}
	if (betaIdx >= 45 && betaIdx <= 53) { // PE overflow row (main 36-44)
		sx = 8 + (betaIdx - 45) * 18;
		sy = 160;
		return true;
	}
	return false;
}

bool BetaInventoryScreen::showOverflow() const {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = player ? player->inventory : NULL;
	if (!in)
		return false;
	for (int s = 36; s <= 44; s++) {
		ItemInstance* it = in->getItem(s);
		// Linked storage renders in the hotbar row, not in the strip.
		if (it && !it->isNull() && !isLinkedMain(s))
			return true;
	}
	return false;
}

bool BetaInventoryScreen::isLinkedMain(int peSlot) const {
	Inventory* in = inv();
	if (!in || peSlot < 9 || peSlot >= in->getContainerSize())
		return false;
	for (int l = 0; l < in->numLinkedSlots; l++) {
		if (in->linkedSlots[l].inventorySlot == peSlot)
			return true;
	}
	return false;
}

int BetaInventoryScreen::slotAt(int x, int y) const {
	int px = panelX(), py = panelY();
	bool overflow = showOverflow();
	for (int i = 0; i <= 53; i++) {
		if (i >= 45 && !overflow)
			continue;
		int sx, sy;
		if (!slotPos(i, sx, sy))
			continue;
		if (x >= px + sx && x < px + sx + 18 && y >= py + sy && y < py + sy + 18)
			return i;
	}
	return -1;
}

Inventory* BetaInventoryScreen::inv() const {
	Player* player = minecraft ? minecraft->player : NULL;
	if (!player)
		return NULL;
	return player->inventory;
}

// PE hotbar slots are links (views) into main storage, not storage.
// Resolve beta hotbar slot to the real main slot, or -1.
int BetaInventoryScreen::resolveHotbar(int betaIdx) {
	Inventory* in = inv();
	if (!in || betaIdx < 36 || betaIdx > 44)
		return -1;
	int link = betaIdx - 36;
	int real = in->linkedSlots[link].inventorySlot;
	if (real >= 9 && real < in->getContainerSize())
		return real;
	return -1;
}

// Link an empty hotbar position to a free main slot for placement.
int BetaInventoryScreen::ensureHotbarLink(int betaIdx) {
	Inventory* in = inv();
	if (!in || betaIdx < 36 || betaIdx > 44)
		return -1;
	int real = resolveHotbar(betaIdx);
	if (real >= 0)
		return real;
	int link = betaIdx - 36;
	for (int s = 9; s < in->getContainerSize(); s++) {
		ItemInstance* it = in->getItem(s);
		if (!it || it->isNull()) {
			// One main = one link, always: a stale (dangling) link may
			// still point at this just-emptied main from an earlier
			// placement. Without the steal-cleanup two hotbar cells
			// would render the same stack ("dupes in every cell").
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

int BetaInventoryScreen::betaToPe(int betaIdx) {
	if (betaIdx >= 9 && betaIdx <= 35)
		return betaIdx;
	if (betaIdx >= 36 && betaIdx <= 44)
		return resolveHotbar(betaIdx);
	if (betaIdx >= 45 && betaIdx <= 53)
		return betaIdx - 9;
	return -1;
}

ItemInstance* BetaInventoryScreen::getSlotItem(int betaIdx) {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return NULL;
	if (betaIdx == 0)
		return hasCraftResult ? &craftResult : NULL;
	if (betaIdx >= 1 && betaIdx <= 4) {
		ItemInstance& it = craftMatrix[betaIdx - 1];
		return it.isNull() ? NULL : &it;
	}
	if (betaIdx >= 5 && betaIdx <= 8)
		return player->getArmor(betaIdx - 5);
	if (betaIdx >= 9 && betaIdx <= 35) {
		// Linked storage renders in the hotbar row, not here.
		if (isLinkedMain(betaIdx))
			return NULL;
		ItemInstance* it = in->getItem(betaIdx);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	if (betaIdx >= 36 && betaIdx <= 44) {
		// Link-following read; never the orphan link storage.
		ItemInstance* it = in->getItem(betaIdx - 36);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	if (betaIdx >= 45 && betaIdx <= 53) {
		if (isLinkedMain(betaIdx - 9))
			return NULL;
		ItemInstance* it = in->getItem(betaIdx - 9);
		if (!it || it->isNull())
			return NULL;
		return it;
	}
	return NULL;
}

void BetaInventoryScreen::setSlotItem(int betaIdx, const ItemInstance* item) {
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return;
	bool empty = !item || item->isNull();
	if (betaIdx >= 1 && betaIdx <= 4) {
		if (empty) craftMatrix[betaIdx - 1].setNull();
		else craftMatrix[betaIdx - 1] = *item;
		updateCraftResult();
		return;
	}
	if (betaIdx >= 5 && betaIdx <= 8) {
		player->setArmor(betaIdx - 5, empty ? NULL : item);
		return;
	}
	if (betaIdx >= 36 && betaIdx <= 44) {
		// Never write the orphan link storage: resolve or link first.
		int real = resolveHotbar(betaIdx);
		if (real < 0 && !empty)
			real = ensureHotbarLink(betaIdx);
		if (real < 0)
			return;
		if (empty) {
			in->clearSlot(real);
			// Drop the link too: a dangling link to the just-emptied
			// main would alias the next placement into another cell.
			in->linkedSlots[betaIdx - 36].inventorySlot = -1;
		} else in->setItem(real, const_cast<ItemInstance*>(item));
		// One main = one link: whenever this cell owns storage, no
		// other hotbar cell may view it (kills live mirrors too).
		for (int l = 0; l < in->numLinkedSlots; l++) {
			if (l != betaIdx - 36 && in->linkedSlots[l].inventorySlot == real)
				in->linkedSlots[l].inventorySlot = -1;
		}
		return;
	}
	int pe = -1;
	if (betaIdx >= 9 && betaIdx <= 35)
		pe = betaIdx; // main 9-35 maps 1:1
	else if (betaIdx >= 45 && betaIdx <= 53)
		pe = betaIdx - 9; // overflow row -> main 36-44
	if (pe >= 0) {
		if (empty) in->clearSlot(pe);
		else in->setItem(pe, const_cast<ItemInstance*>(item));
	}
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

bool BetaInventoryScreen::mergeIntoRange(ItemInstance& stack, int from, int to, bool reverse, int skipPe) {
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
					// Never park stacks in storage that is hidden
					// behind a hotbar link (hotbar cells themselves
					// are fine: 36-44 resolve through their links).
					if ((idx < 36 || idx > 44) && isLinkedMain(betaToPe(idx)))
						continue;
					ItemInstance v = stack;
					setSlotItem(idx, &v);
					// setSlotItem can silently drop (no free main to
					// link a hotbar cell): only consume when it landed.
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
			if ((idx < 36 || idx > 44) && isLinkedMain(betaToPe(idx)))
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

// Read-only free space for a stack across a beta range.
int BetaInventoryScreen::spaceFor(const ItemInstance& stack, int from, int to) {
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

// Take one craft's output into the cursor. All or nothing, so the
// matrix consumption always matches what left the slot (no dupes).
bool BetaInventoryScreen::takeResultToCursor() {
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

// Shift-click the result: whole crafts while they fully fit.
bool BetaInventoryScreen::takeResultToInventory() {
	bool moved = false;
	while (hasCraftResult && spaceFor(craftResult, 9, 54) >= craftResult.count) {
		ItemInstance one = craftResult;
		mergeIntoRange(one, 9, 54, false);
		if (!one.isNull())
			break; // space accounting lied; keep matrix intact
		consumeMatrix();
		moved = true;
	}
	return moved;
}

bool BetaInventoryScreen::quickTransfer(int betaIdx) {
	ItemInstance* src = getSlotItem(betaIdx);
	if (!src || src->isNull())
		return false;
	if (betaIdx == 0)
		return takeResultToInventory();
	ItemInstance stack = *src;
	int srcPe = betaToPe(betaIdx);
	bool moved = false;
	if (betaIdx >= 1 && betaIdx <= 8) {
		moved = mergeIntoRange(stack, 9, 54, false, srcPe);
	} else if (betaIdx >= 9 && betaIdx <= 35) {
		moved = mergeIntoRange(stack, 36, 45, false, srcPe);
		if (!moved)
			moved = mergeIntoRange(stack, 45, 54, false, srcPe);
	} else if (betaIdx >= 45 && betaIdx <= 53) {
		moved = mergeIntoRange(stack, 36, 45, false, srcPe);
		if (!moved)
			moved = mergeIntoRange(stack, 9, 36, false, srcPe);
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
			moved = mergeIntoRange(stack, 9, 36, false, srcPe);
		if (!moved)
			moved = mergeIntoRange(stack, 45, 54, false, srcPe);
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
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (player && in) {
		// add() merges what fits and leaves the rest in count.
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

void BetaInventoryScreen::renderPlayerModel(float xo, float yo) {
	Player* player = (Player*)(minecraft ? minecraft->player : NULL);
	if (!player)
		return;

	glPushMatrix();

	// GUI leaves lighting/texture/blend/depth in 2D-party state; the
	// entity renderer needs opaque textured polys with depth, otherwise
	// the model comes out black or not at all. Everything is restored
	// afterwards: leaking e.g. depth-test-off into the next world frame
	// makes the terrain render see-through (x-ray) while open.
	GLboolean depthWas = glIsEnabled(GL_DEPTH_TEST);
	GLboolean lightWas = glIsEnabled(GL_LIGHTING);
	GLboolean blendWas = glIsEnabled(GL_BLEND);
	GLboolean cullWas = glIsEnabled(GL_CULL_FACE);
	glColor4f2(1, 1, 1, 1);
	glEnable2(GL_TEXTURE_2D);
	glDisable2(GL_BLEND);
	glEnable2(GL_DEPTH_TEST);
	// The model is mirrored (negative X scale) so its winding is
	// flipped: with culling on every face is rejected and nothing
	// draws. Beta's player preview needs culling off here.
	glDisable2(GL_CULL_FACE);
	glDepthMask(true);
	// Standard GUI lighting (same fixed rig as the block selection
	// screen): without it the model renders black-on-black here.
	Lighting::turnOn(minecraft);

	glTranslatef(xo, yo, -200);
	float ss = 30.0f;
	glScalef(-ss, ss, ss);

	glRotatef(180, 0, 0, 1);

	float oybr = player->yBodyRot;
	float oyr = player->yRot;
	float oxr = player->xRot;

	float t = getTimeS();
	float xd = 10 * Mth::sin(t);
	float yd = 10 * Mth::cos(t * 0.05f);

	const float xtan = Mth::atan(xd / 40.0f) * +20;
	const float ytan = Mth::atan(yd / 40.0f) * -20;

	glRotatef(ytan, 1, 0, 0);

	player->yBodyRot = xtan;
	player->yRot = xtan + xtan;
	player->xRot = ytan;
	glTranslatef(0, player->heightOffset, 0);

	float oldWAP = player->walkAnimPos;
	float oldWAS = player->walkAnimSpeed;
	float oldWASO = player->walkAnimSpeedO;

	player->walkAnimSpeedO = player->walkAnimSpeed = 0.25f;
	player->walkAnimPos = getTimeS() * player->walkAnimSpeed * SharedConstants::TicksPerSecond;

	EntityRenderDispatcher* rd = EntityRenderDispatcher::getInstance();
	rd->playerRotY = 180;
	rd->render(player, 0, 0, 0, 0, 1);

	player->walkAnimPos = oldWAP;
	player->walkAnimSpeed = oldWAS;
	player->walkAnimSpeedO = oldWASO;

	player->yBodyRot = oybr;
	player->yRot = oyr;
	player->xRot = oxr;

	// Restore whatever the GUI/world had: slots and labels need blend
	// and texture, the world behind needs its depth test and lighting.
	if (depthWas) glEnable2(GL_DEPTH_TEST); else glDisable2(GL_DEPTH_TEST);
	if (lightWas) Lighting::turnOn(minecraft); else Lighting::turnOff();
	if (blendWas) glEnable2(GL_BLEND); else glDisable2(GL_BLEND);
	if (cullWas) glEnable2(GL_CULL_FACE); else glDisable2(GL_CULL_FACE);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable2(GL_TEXTURE_2D);
	glColor4f2(1, 1, 1, 1);

	glPopMatrix();
}

void BetaInventoryScreen::render(int xm, int ym, float a) {
	renderBackground();
	// render() already gets GUI units (GameRenderer scales raw pixels by
	// InvGuiScale); Screen::mouseEvent maps raw click events into the
	// same space (with a -1 on y), so use the coords directly - mapping
	// them again squishes hover and the carried stack into a corner.
	int mx = xm;
	int my = ym - 1;
	int px = panelX(), py = panelY();

	// Original panel art, 176x166 at 1:1 from the jar. blit() always
	// divides UVs by 256, so sw/sh must default (0 -> w/h): passing
	// 256,256 squeezes the whole texture into the panel and every slot
	// recess lands off-grid.
	TextureId bg = minecraft->textures->loadTexture("gui/inventory.png");
	if (Textures::isTextureIdValid(bg)) {
		minecraft->textures->bind(bg);
		glColor4f2(1, 1, 1, 1);
		blit(px, py, 0, 0, 176, 166);
	}
	// Overflow row extension (PE mains 36-44): gray strip, black bottom
	// edge, slot recesses. Only while occupied, otherwise plain beta.
	bool overflow = showOverflow();
	int ph = panelH();
	if (overflow) {
		fill(px, py + 166, px + 176, py + ph, 0xffc6c6c6);
		fill(px, py + ph - 1, px + 176, py + ph, 0xff000000);
		for (int i = 45; i <= 53; i++) {
			int sx, sy;
			if (!slotPos(i, sx, sy))
				continue;
			int x0 = px + sx, y0 = py + sy;
			fill(x0, y0, x0 + 18, y0 + 18, 0xff373737);
			fill(x0 + 1, y0 + 1, x0 + 17, y0 + 17, 0xff8b8b8b);
		}
	}

	drawString(minecraft->font, "Crafting", px + 86, py + 16, 0xff404040);

	renderPlayerModel((float)(px + 51), (float)(py + 75));

	for (int i = 0; i <= 53; i++) {
		if (i >= 45 && !overflow)
			continue;
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

	// Hovered slot highlight, like the original.
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

void BetaInventoryScreen::tick() {
	super::tick();
}

static bool canWearIn(int betaIdx, const ItemInstance* item);

// Place the whole carried stack into a slot (merge or swap). The result
// slot is output-only and never accepts anything.
void BetaInventoryScreen::placeInto(int slot) {
	if (!hasCarried || carried.isNull() || slot == 0)
		return;
	if (!canWearIn(slot, &carried))
		return;
	ItemInstance* dst = getSlotItem(slot);
	if (!dst || dst->isNull()) {
		setSlotItem(slot, &carried);
		// An unlinked hotbar cell with a full inventory silently drops:
		// keep holding instead of deleting the stack.
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

// Place a single unit from the carried stack into a slot.
void BetaInventoryScreen::placeOneInto(int slot) {
	if (!hasCarried || carried.isNull() || slot == 0)
		return;
	if (!canWearIn(slot, &carried))
		return;
	ItemInstance* dst = getSlotItem(slot);
	if (!dst || dst->isNull()) {
		ItemInstance one = carried;
		one.count = 1;
		setSlotItem(slot, &one);
		ItemInstance* check = getSlotItem(slot);
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

// Beta acts on press, not on release: click-click picks up and places,
// and press-drag-release moves in one gesture. Acting on release instead
// shuffles stacks whenever the two land on different cells.
void BetaInventoryScreen::mouseClicked(int x, int y, int buttonNum) {
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
		// Right click: place one / pick up half. Never touches result.
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
	// Only a press that picked something up can drop it elsewhere.
	// Releasing on the press slot keeps holding (click-click), on the
	// result slot does nothing (output-only), outside drops the stack.
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

void BetaInventoryScreen::keyPressed(int eventKey) {
	super::keyPressed(eventKey);
	if (eventKey == Keyboard::KEY_E && minecraft && !minecraft->isCreativeMode())
		minecraft->setScreen(NULL);
}

void BetaInventoryScreen::removed() {
	// Grid contents go back to the player inventory (or drop).
	Player* player = minecraft ? minecraft->player : NULL;
	Inventory* in = inv();
	if (!player || !in)
		return;
	for (int i = 0; i < 4; i++) {
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
