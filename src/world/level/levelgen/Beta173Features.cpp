#include "Beta173Features.h"

#include <cmath>

#include "../Level.h"
#include "../LightLayer.h"
#include "../tile/Tile.h"
#include "../tile/LeafTile.h"
#include "../tile/TallGrass.h"
#include "../material/Material.h"
#include "../tile/entity/ChestTileEntity.h"
#include "BetaMath.h"
#include "../../inventory/FillingContainer.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"

namespace Beta173Features {

static int sgnAbs(int v) { return v >= 0 ? v : -v; }

// Beta opaqueCubeLookup for PE tile ids: everything solid is opaque except
// the see-through set below (matches beta 1.7.3 block table).
bool isOpaque(int tileId) {
	switch (tileId) {
	case 0:   // air
	case 6:   // sapling
	case 8: case 9: case 10: case 11: // water/lava
	case 18:  // leaves
	case 20:  // glass
	case 26:  // bed
	case 27: case 28: // rails
	case 30:  // web
	case 31:  // tall grass
	case 37: case 38: case 39: case 40: // flowers/mushrooms
	case 44:  // slab
	case 50:  // torch
	case 51:  // fire
	case 52:  // spawner
	case 53: case 67: // stairs
	case 54:  // chest
	case 55:  // redstone wire
	case 59:  // crops
	case 60:  // farmland
	case 63: case 68: // signs
	case 64: case 71: // doors
	case 65:  // ladder
	case 66:  // rail
	case 69:  // lever
	case 70: case 72: // plates
	case 75: case 76: // redstone torches
	case 77:  // button
	case 78:  // snow layer
	case 81:  // cactus
	case 83:  // reeds
	case 85:  // fence
	case 90:  // portal
	case 92:  // cake
	case 93: case 94: // diodes
	case 96:  // trapdoor
	case 111: // lily pad (no beta counterpart, safe side)
		return false;
	default:
		break;
	}
	return true;
}

static int LOG_ID() { return Tile::treeTrunk ? Tile::treeTrunk->id : BB_LOG; }
static int LEAF_ID() { return Tile::leaves ? Tile::leaves->id : BB_LEAVES; }

// ---------------------------------------------------------------------------
// Normal oak (yh)
// ---------------------------------------------------------------------------
bool placeOak(Level* level, JavaRandom& rand, int x, int y, int z) {
	int l = rand.nextInt(3) + 4;
	int canGrow = 1;
	if (y < 1 || y + l + 1 > 128) return false;
	for (int j = y; j <= y + 1 + l; j++) {
		int r = 1;
		if (j == y) r = 0;
		if (j >= y + 1 + l - 2) r = 2;
		for (int xx = x - r; xx <= x + r && canGrow != 0; xx++) {
			for (int zz = z - r; zz <= z + r && canGrow != 0; zz++) {
				if (j < 0 || j >= 128) {
					canGrow = 0;
				} else {
					int id = level->getTile(xx, j, zz);
					if (id != 0 && id != LEAF_ID())
						canGrow = 0;
				}
			}
		}
	}
	if (canGrow == 0) return false;
	int soil = level->getTile(x, y - 1, z);
	if (soil != Tile::grass->id && soil != Tile::dirt->id) return false;
	if (y >= 128 - l - 1) return false;
	level->setTile(x, y - 1, z, Tile::dirt->id);
	for (int yy = y - 3 + l; yy <= y + l; yy++) {
		int off = yy - (y + l);
		int r = 1 - off / 2; // Java truncating division, off <= 0
		for (int xx = x - r; xx <= x + r; xx++) {
			int dx = xx - x;
			for (int zz = z - r; zz <= z + r; zz++) {
				int dz = zz - z;
				if (sgnAbs(dx) == r && sgnAbs(dz) == r) {
					if (rand.nextInt(2) == 0) continue;
					if (off == 0) continue;
				}
				if (isOpaque(level->getTile(xx, yy, zz))) continue;
				level->setTile(xx, yy, zz, LEAF_ID());
			}
		}
	}
	for (int j = 0; j < l; j++) {
		int id = level->getTile(x, y + j, z);
		if (id == 0 || id == LEAF_ID())
			level->setTileAndData(x, y + j, z, LOG_ID(), 0);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Big oak (ih). Field mapping: d[3]=origin, e=height, g=0.618, h=1.0,
// i=0.381, j=angle(1.0), k=len(1.0), l=1, m=12, n=4, o=branch table.
// ---------------------------------------------------------------------------
struct BigTreeState {
	Level* level;
	JavaRandom* rand;
	int ox, oy, oz;
	int height;      // e
	double g, h, i, j, k;
	int l;
	int m, n;
};

static double bigTaper(BigTreeState& t, int y) {
	// Mirrors ih.a(int)F: bell curve, -1.618 below 30% height.
	if ((double)y < (double)t.height * 0.3)
		return -1.618;
	double half = (double)t.height / 2.0;
	double d = half - (double)y;
	double r = sqrt(half * half - d * d);
	if (d == 0.0) r = half;
	else if (fabs(d) >= half) return 0.0;
	return r * 0.5;
}

static void bigLeafDisc(BigTreeState& t, int x, int y, int z, float radius, int leafId) {
	int r = (int)(radius + 0.618);
	for (int dx = -r; dx <= r; dx++) {
		for (int dz = -r; dz <= r; dz++) {
			double dd = sqrt(((double)(sgnAbs(dx)) + 0.5) * ((double)(sgnAbs(dx)) + 0.5)
				+ ((double)(sgnAbs(dz)) + 0.5) * ((double)(sgnAbs(dz)) + 0.5));
			if (dd > (double)radius) continue;
			int id = t.level->getTile(x + dx, y, z + dz);
			if (id == 0 || id == BB_LEAVES || id == LEAF_ID())
				t.level->setTile(x + dx, y, z + dz, leafId);
		}
	}
}

static void bigTrunkColumn(BigTreeState& t, int x, int y, int z, int top) {
	for (int yy = y; yy < y + top; yy++)
		bigLeafDisc(t, x, yy, z, 2.0f, LEAF_ID());
}

static void bigSegment(BigTreeState& t, int x0, int y0, int z0, int x1, int y1, int z1) {
	int dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
	int steps = sgnAbs(dx);
	if (sgnAbs(dy) > steps) steps = sgnAbs(dy);
	if (sgnAbs(dz) > steps) steps = sgnAbs(dz);
	if (steps == 0) steps = 1;
	for (int s = 0; s <= steps; s++) {
		int x = x0 + dx * s / steps;
		int y = y0 + dy * s / steps;
		int z = z0 + dz * s / steps;
		int id = t.level->getTile(x, y, z);
		if (id == 0 || id == LEAF_ID())
			t.level->setTileAndData(x, y, z, LOG_ID(), 0);
	}
}

bool placeBigTree(Level* level, JavaRandom& rand, int x, int y, int z) {
	BigTreeState t;
	t.level = level; t.rand = &rand;
	t.ox = x; t.oy = y; t.oz = z;
	t.g = 0.618; t.h = 1.0; t.i = 0.381; t.j = 1.0; t.k = 1.0;
	t.l = 1; t.m = 12; t.n = 4;
	t.height = 0;
	// e(): soil must be grass/dirt; height = 5 + nextInt(12); need 6+ clearance.
	int soil = level->getTile(x, y - 1, z);
	if (soil != Tile::grass->id && soil != Tile::dirt->id) return false;
	t.height = 5 + rand.nextInt(12);
	// clearance: trunk column of height e must be air/leaves.
	for (int yy = y; yy < y + t.height; yy++) {
		int id = level->getTile(x, yy, z);
		if (id != 0 && id != LEAF_ID() && id != BB_LEAVES) return false;
	}
	if (t.height < 6) return false;

	// Branch layout (ih.a()): levels from top.
	int top = y + t.height - t.n;
	int branchCount = (int)(1.382 + (t.k * t.height / 13.0) * (t.k * t.height / 13.0));
	if (branchCount < 1) branchCount = 1;
	struct Branch { int x0, y0, z0, x1, y1, z1, leafR; };
	Branch branches[64];
	int nBranches = 0;
	for (int lvl = top; lvl >= y && nBranches < 60; lvl--) {
		double taper = bigTaper(t, lvl - y);
		if (taper < 0.0) continue;
		for (int b = 0; b < branchCount && nBranches < 60; b++) {
			double spread = t.j * taper * (rand.nextDouble() + 0.328);
			double angled = rand.nextDouble() * 2.0 * 3.141592653589793;
			float angle = (float)angled;
			int bx = BetaMath::floor(spread * (double)BetaMath::sin(angle) + (double)x + 0.5);
			int bz = BetaMath::floor(spread * (double)BetaMath::cos(angle) + (double)z + 0.5);
			int by = lvl;
			// clearance walk from trunk top to endpoint must be free
			bool blocked = false;
			{
				int dx = bx - x, dy = by - (y + t.height), dz = bz - z;
				int steps = sgnAbs(dx);
				if (sgnAbs(dy) > steps) steps = sgnAbs(dy);
				if (sgnAbs(dz) > steps) steps = sgnAbs(dz);
				if (steps < 1) steps = 1;
				for (int s = 0; s <= steps && !blocked; s++) {
					int px = x + dx * s / steps;
					int py = (y + t.height) + dy * s / steps;
					int pz = z + dz * s / steps;
					int id = level->getTile(px, py, pz);
					if (id != 0 && id != LEAF_ID() && id != BB_LEAVES) blocked = true;
				}
			}
			if (blocked) continue;
			branches[nBranches].x0 = x; branches[nBranches].y0 = y + t.height;
			branches[nBranches].z0 = z;
			branches[nBranches].x1 = bx; branches[nBranches].y1 = by;
			branches[nBranches].z1 = bz;
			branches[nBranches].leafR = 2;
			nBranches++;
		}
	}
	// Trunk-side leaves + branches + trunk.
	bigTrunkColumn(t, x, y, z, t.height - t.n);
	for (int i = 0; i < nBranches; i++) {
		bigLeafDisc(t, branches[i].x1, branches[i].y1, branches[i].z1, 2.0f, LEAF_ID());
		bigSegment(t, branches[i].x0, branches[i].y0, branches[i].z0,
			branches[i].x1, branches[i].y1, branches[i].z1);
	}
	// Trunk.
	level->setTile(x, y - 1, z, Tile::dirt->id);
	for (int yy = y; yy < y + t.height; yy++) {
		int id = level->getTile(x, yy, z);
		if (id == 0 || id == LEAF_ID())
			level->setTileAndData(x, yy, z, LOG_ID(), 0);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Birch (k): height 5..7, leaves meta 2, log meta 2
// ---------------------------------------------------------------------------
static bool placeLeafyTree(Level* level, JavaRandom& rand, int x, int y, int z,
	int h, int leafMeta, int logMeta, int topStart, int topRadiusMode) {
	(void)topStart; (void)topRadiusMode;
	if (y < 1 || y + h + 1 > 128) return false;
	int canGrow = 1;
	for (int yy = y; yy <= y + 1 + h && canGrow; yy++) {
		int r = 1;
		if (yy == y) r = 0;
		if (yy >= y + 1 + h - 2) r = 2;
		for (int xx = x - r; xx <= x + r && canGrow; xx++)
			for (int zz = z - r; zz <= z + r && canGrow; zz++) {
				if (yy < 0 || yy >= 128) { canGrow = 0; break; }
				int id = level->getTile(xx, yy, zz);
				if (id != 0 && id != LEAF_ID()) canGrow = 0;
			}
	}
	if (!canGrow) return false;
	int soil = level->getTile(x, y - 1, z);
	if (soil != Tile::grass->id && soil != Tile::dirt->id) return false;
	if (y >= 128 - h - 1) return false;
	level->setTile(x, y - 1, z, Tile::dirt->id);
	for (int yy = y - 3 + h; yy <= y + h; yy++) {
		int off = yy - (y + h);
		int r = 1 - off / 2;
		for (int xx = x - r; xx <= x + r; xx++) {
			int dx = xx - x;
			for (int zz = z - r; zz <= z + r; zz++) {
				int dz = zz - z;
				if (sgnAbs(dx) == r && sgnAbs(dz) == r) {
					if (rand.nextInt(2) == 0) continue;
					if (off == 0) continue;
				}
				if (isOpaque(level->getTile(xx, yy, zz))) continue;
				level->setTileAndData(xx, yy, zz, LEAF_ID(), leafMeta);
			}
		}
	}
	for (int j = 0; j < h; j++) {
		int id = level->getTile(x, y + j, z);
		if (id == 0 || id == LEAF_ID())
			level->setTileAndData(x, y + j, z, LOG_ID(), logMeta);
	}
	return true;
}

bool placeBirch(Level* level, JavaRandom& rand, int x, int y, int z) {
	int h = rand.nextInt(3) + 5;
	return placeLeafyTree(level, rand, x, y, z, h, 2, 2, 0, 0);
}

// ---------------------------------------------------------------------------
// Taiga1 (pw): tall spruce 7..11, meta 1
// ---------------------------------------------------------------------------
bool placeTaiga1(Level* level, JavaRandom& rand, int x, int y, int z) {
	int h = rand.nextInt(5) + 7;
	int j = h - rand.nextInt(2) - 3;
	int kk = 1 + rand.nextInt(h - j + 1);
	if (y < 1 || y + h + 1 > 128) return false;
	int canGrow = 1;
	for (int yy = y; yy <= y + 1 + h && canGrow; yy++) {
		int r = (yy - y >= j) ? 0 : kk;
		for (int xx = x - r; xx <= x + r && canGrow; xx++)
			for (int zz = z - r; zz <= z + r && canGrow; zz++) {
				if (yy < 0 || yy >= 128) { canGrow = 0; break; }
				int id = level->getTile(xx, yy, zz);
				if (id != 0 && id != LEAF_ID()) canGrow = 0;
			}
	}
	if (!canGrow) return false;
	int soil = level->getTile(x, y - 1, z);
	if (soil != Tile::grass->id && soil != Tile::dirt->id) return false;
	if (y >= 128 - h - 1) return false;
	level->setTile(x, y - 1, z, Tile::dirt->id);
	// fixed cone pass like birch but meta 1
	for (int yy = y - 3 + h; yy <= y + h; yy++) {
		int off = yy - (y + h);
		int r = 1 - off / 2;
		for (int xx = x - r; xx <= x + r; xx++) {
			int dx = xx - x;
			for (int zz = z - r; zz <= z + r; zz++) {
				int dz = zz - z;
				if (sgnAbs(dx) == r && sgnAbs(dz) == r) {
					if (rand.nextInt(2) == 0) continue;
					if (off == 0) continue;
				}
				if (isOpaque(level->getTile(xx, yy, zz))) continue;
				level->setTileAndData(xx, yy, zz, LEAF_ID(), 1);
			}
		}
	}
	for (int i = 0; i < h; i++) {
		int id = level->getTile(x, y + i, z);
		if (id == 0 || id == LEAF_ID())
			level->setTileAndData(x, y + i, z, LOG_ID(), 1);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Taiga2 (ws): pine cone 6..9, meta 1
// ---------------------------------------------------------------------------
bool placeTaiga2(Level* level, JavaRandom& rand, int x, int y, int z) {
	int h = rand.nextInt(4) + 6;
	int j = 1 + rand.nextInt(2);
	int kk = 2 + rand.nextInt(2);
	(void)kk;
	if (y < 1 || y + h + 1 > 128) return false;
	int canGrow = 1;
	for (int yy = y; yy <= y + 1 + h && canGrow; yy++) {
		int r = 1;
		if (yy == y) r = 0;
		if (yy >= y + 1 + h - 2) r = 2;
		for (int xx = x - r; xx <= x + r && canGrow; xx++)
			for (int zz = z - r; zz <= z + r && canGrow; zz++) {
				if (yy < 0 || yy >= 128) { canGrow = 0; break; }
				int id = level->getTile(xx, yy, zz);
				if (id != 0 && id != LEAF_ID()) canGrow = 0;
			}
	}
	if (!canGrow) return false;
	int soil = level->getTile(x, y - 1, z);
	if (soil != Tile::grass->id && soil != Tile::dirt->id) return false;
	if (y >= 128 - h - 1) return false;
	level->setTile(x, y - 1, z, Tile::dirt->id);
	// descending layers from y+h down to y+j, radius shrinks upward
	int r = 1;
	for (int yy = y + h; yy >= y + j; yy--) {
		for (int xx = x - r; xx <= x + r; xx++) {
			int dx = xx - x;
			for (int zz = z - r; zz <= z + r; zz++) {
				int dz = zz - z;
				if (sgnAbs(dx) == r && sgnAbs(dz) == r) {
					if (rand.nextInt(2) == 0) continue;
				}
				if (isOpaque(level->getTile(xx, yy, zz))) continue;
				level->setTileAndData(xx, yy, zz, LEAF_ID(), 1);
			}
		}
		if (r < 2 && yy > y + j + 1) r++;
	}
	for (int i = 0; i < h; i++) {
		int id = level->getTile(x, y + i, z);
		if (id == 0 || id == LEAF_ID())
			level->setTileAndData(x, y + i, z, LOG_ID(), 1);
	}
	return true;
}

int pickTree(JavaRandom& rand, int treeKind) {
	switch (treeKind) {
	case 1: // taiga (g)
		if (rand.nextInt(3) == 0) return 3;
		return 4;
	case 2: // forest (rb)
		if (rand.nextInt(5) == 0) return 2;
		if (rand.nextInt(3) == 0) return 1;
		return 0;
	case 3: // rainforest (yj)
		if (rand.nextInt(3) == 0) return 1;
		return 0;
	default: // default (kd)
		if (rand.nextInt(10) == 0) return 1;
		return 0;
	}
}

bool placePickedTree(Level* level, JavaRandom& rand, int kind, int x, int y, int z) {
	switch (kind) {
	case 1: return placeBigTree(level, rand, x, y, z);
	case 2: return placeBirch(level, rand, x, y, z);
	case 3: return placeTaiga1(level, rand, x, y, z);
	case 4: return placeTaiga2(level, rand, x, y, z);
	default: return placeOak(level, rand, x, y, z);
	}
}

// ---------------------------------------------------------------------------
// Small features
// ---------------------------------------------------------------------------
static bool canStayOn(int soilId, int plantId) {
	(void)plantId;
	return soilId == Tile::grass->id || soilId == Tile::dirt->id;
}

bool placeFlowers(Level* level, JavaRandom& rand, int x, int y, int z, int flowerId) {
	for (int i = 0; i < 64; i++) {
		int xx = x + rand.nextInt(8) - rand.nextInt(8);
		int yy = y + rand.nextInt(4) - rand.nextInt(4);
		int zz = z + rand.nextInt(8) - rand.nextInt(8);
		if (level->isEmptyTile(xx, yy, zz) && canStayOn(level->getTile(xx, yy - 1, zz), flowerId))
			level->setTile(xx, yy, zz, flowerId);
	}
	return true;
}

bool placeTallGrass(Level* level, JavaRandom& rand, int x, int y, int z, int meta) {
	while ((level->isEmptyTile(x, y, z) || level->getTile(x, y, z) == LEAF_ID()) && y > 0)
		y--;
	for (int i = 0; i < 128; i++) {
		int xx = x + rand.nextInt(8) - rand.nextInt(8);
		int yy = y + rand.nextInt(4) - rand.nextInt(4);
		int zz = z + rand.nextInt(8) - rand.nextInt(8);
		if (level->isEmptyTile(xx, yy, zz) && canStayOn(level->getTile(xx, yy - 1, zz), BB_TALLGRASS))
			level->setTileAndData(xx, yy, zz, Tile::tallgrass->id, meta);
	}
	return true;
}

bool placeDeadBush(Level* level, JavaRandom& rand, int x, int y, int z) {
	// No dead-bush block in PE: run the identical RNG walk, skip placement.
	while ((level->isEmptyTile(x, y, z) || level->getTile(x, y, z) == LEAF_ID()) && y > 0)
		y--;
	for (int i = 0; i < 4; i++) {
		int xx = x + rand.nextInt(8) - rand.nextInt(8);
		int yy = y + rand.nextInt(4) - rand.nextInt(4);
		int zz = z + rand.nextInt(8) - rand.nextInt(8);
		(void)xx; (void)yy; (void)zz;
	}
	return true;
}

bool placeReeds(Level* level, JavaRandom& rand, int x, int y, int z) {
	for (int i = 0; i < 20; i++) {
		int xx = x + rand.nextInt(4) - rand.nextInt(4);
		int zz = z + rand.nextInt(4) - rand.nextInt(4);
		if (level->isEmptyTile(xx, y, zz)) {
			bool nearWater = false;
			if (level->getMaterial(xx - 1, y - 1, zz) == Material::water) nearWater = true;
			if (level->getMaterial(xx + 1, y - 1, zz) == Material::water) nearWater = true;
			if (level->getMaterial(xx, y - 1, zz - 1) == Material::water) nearWater = true;
			if (level->getMaterial(xx, y - 1, zz + 1) == Material::water) nearWater = true;
			if (nearWater) {
				int h = 2 + rand.nextInt(rand.nextInt(3) + 1);
				for (int k = 0; k < h; k++) {
					// canStay: reeds below or soil adjacency; approximate with PE rules:
					// place while air and (k==0 ? nearWater : reeds below).
					if (!level->isEmptyTile(xx, y + k, zz)) break;
					if (k > 0 && level->getTile(xx, y + k - 1, zz) != Tile::reeds->id) break;
					level->setTile(xx, y + k, zz, Tile::reeds->id);
				}
			}
		}
	}
	return true;
}

bool placeCactus(Level* level, JavaRandom& rand, int x, int y, int z) {
	for (int i = 0; i < 10; i++) {
		int xx = x + rand.nextInt(8) - rand.nextInt(8);
		int yy = y + rand.nextInt(4) - rand.nextInt(4);
		int zz = z + rand.nextInt(8) - rand.nextInt(8);
		if (level->isEmptyTile(xx, yy, zz)) {
			int h = 1 + rand.nextInt(rand.nextInt(3) + 1);
			for (int k = 0; k < h; k++) {
				if (!level->isEmptyTile(xx, yy + k, zz)) break;
				if (k > 0 && level->getTile(xx, yy + k - 1, zz) != Tile::cactus->id) break;
				// canStay approximation: sand below for k==0
				if (k == 0 && level->getTile(xx, yy - 1, zz) != Tile::sand->id) break;
				level->setTile(xx, yy + k, zz, Tile::cactus->id);
			}
		}
	}
	return true;
}

// Vein walker shared by clay and ores (mirrors WorldGenMinable/WorldGenClay).
static bool placeVein(Level* level, JavaRandom& rand, int x, int y, int z,
	int size, int placeId, int replaceId) {
	float ang = rand.nextFloat() * 3.1415927f;
	double x1 = (double)(x + 8) + sin(ang) * (double)size / 8.0;
	double x2 = (double)(x + 8) - sin(ang) * (double)size / 8.0;
	double z1 = (double)(z + 8) + cos(ang) * (double)size / 8.0;
	double z2 = (double)(z + 8) - cos(ang) * (double)size / 8.0;
	double y1 = (double)(y + rand.nextInt(3) + 2);
	double y2 = (double)(y + rand.nextInt(3) + 2);
	for (int i = 0; i <= size; i++) {
		double cx = x1 + (x2 - x1) * (double)i / (double)size;
		double cy = y1 + (y2 - y1) * (double)i / (double)size;
		double cz = z1 + (z2 - z1) * (double)i / (double)size;
		double scale = rand.nextDouble() * (double)size / 16.0;
		double r = (sin((double)i * 3.1415927 / (double)size) + 1.0) * scale + 1.0;
		int x0 = (int)floor(cx - r / 2.0);
		int y0 = (int)floor(cy - r / 2.0);
		int z0 = (int)floor(cz - r / 2.0);
		int x3 = (int)floor(cx + r / 2.0);
		int y3 = (int)floor(cy + r / 2.0);
		int z3 = (int)floor(cz + r / 2.0);
		for (int xx = x0; xx <= x3; xx++) {
			double dx = ((double)xx + 0.5 - cx) / (r / 2.0);
			if (dx * dx >= 1.0) continue;
			for (int yy = y0; yy <= y3; yy++) {
				double dy = ((double)yy + 0.5 - cy) / (r / 2.0);
				if (dx * dx + dy * dy >= 1.0) continue;
				for (int zz = z0; zz <= z3; zz++) {
					double dz = ((double)zz + 0.5 - cz) / (r / 2.0);
					if (dx * dx + dy * dy + dz * dz >= 1.0) continue;
					if (level->getTile(xx, yy, zz) == replaceId)
						level->setTile(xx, yy, zz, placeId);
				}
			}
		}
	}
	return true;
}

bool placeClay(Level* level, JavaRandom& rand, int x, int y, int z, int size) {
	if (level->getMaterial(x, y, z) != Material::water) return false;
	return placeVein(level, rand, x, y, z, size, Tile::clay->id, Tile::sand->id);
}

bool placeMinable(Level* level, JavaRandom& rand, int x, int y, int z, int oreId, int size) {
	return placeVein(level, rand, x, y, z, size, oreId, Tile::rock->id);
}

bool placeLake(Level* level, JavaRandom& rand, int x, int y, int z, int liquidId) {
	x -= 8;
	z -= 8;
	while (level->isEmptyTile(x, y, z) && y > 0)
		y--;
	y -= 4;
	bool mask[16 * 16 * 8];
	for (int i = 0; i < 16 * 16 * 8; i++)
		mask[i] = false;
	int blobs = rand.nextInt(4) + 4;
	for (int b = 0; b < blobs; b++) {
		double rx = rand.nextDouble() * 6.0 + 3.0;
		double ry = rand.nextDouble() * 4.0 + 2.0;
		double rz = rand.nextDouble() * 6.0 + 3.0;
		double cx = (double)x + rand.nextDouble() * (16.0 - rx - 2.0) + 1.0 + rx / 2.0;
		double cy = (double)y + rand.nextDouble() * (8.0 - ry - 4.0) + 2.0 + ry / 2.0;
		double cz = (double)z + rand.nextDouble() * (16.0 - rz - 2.0) + 1.0 + rz / 2.0;
		for (int xx = 1; xx < 15; xx++)
			for (int zz = 1; zz < 15; zz++)
				for (int yy = 1; yy < 7; yy++) {
					double dx = ((double)xx - cx) / (rx / 2.0);
					double dy = ((double)yy - cy) / (ry / 2.0);
					double dz = ((double)zz - cz) / (rz / 2.0);
					if (dx * dx + dy * dy + dz * dz < 1.0)
						mask[(xx * 16 + zz) * 8 + yy] = true;
				}
	}
	for (int xx = 0; xx < 16; xx++)
		for (int zz = 0; zz < 16; zz++)
			for (int yy = 0; yy < 8; yy++) {
				bool edge = !mask[(xx * 16 + zz) * 8 + yy]
					&& ((xx > 0 && mask[((xx - 1) * 16 + zz) * 8 + yy])
						|| (xx < 15 && mask[((xx + 1) * 16 + zz) * 8 + yy])
						|| (zz > 0 && mask[(xx * 16 + zz - 1) * 8 + yy])
						|| (zz < 15 && mask[(xx * 16 + zz + 1) * 8 + yy])
						|| (yy > 0 && mask[(xx * 16 + zz) * 8 + yy - 1])
						|| (yy < 7 && mask[(xx * 16 + zz) * 8 + yy + 1]));
				if (!edge) continue;
				const Material* m = level->getMaterial(x + xx, y + yy, z + zz);
				if (yy >= 4 && m->isLiquid()) return false;
				if (yy < 4 && !m->isSolid() && level->getTile(x + xx, y + yy, z + zz) != liquidId)
					return false;
			}
	for (int xx = 0; xx < 16; xx++)
		for (int zz = 0; zz < 16; zz++)
			for (int yy = 0; yy < 8; yy++) {
				if (!mask[(xx * 16 + zz) * 8 + yy]) continue;
				if (yy >= 4)
					level->setTileNoUpdate(x + xx, y + yy, z + zz, 0);
				else
					level->setTileNoUpdate(x + xx, y + yy, z + zz, liquidId);
			}
	for (int xx = 0; xx < 16; xx++)
		for (int zz = 0; zz < 16; zz++)
			for (int yy = 4; yy < 8; yy++) {
				if (!mask[(xx * 16 + zz) * 8 + yy]) continue;
				if (level->getTile(x + xx, y + yy - 1, z + zz) == Tile::dirt->id
					&& level->getBrightness(LightLayer::Sky, x + xx, y + yy, z + zz) > 0)
					level->setTileNoUpdate(x + xx, y + yy - 1, z + zz, Tile::grass->id);
			}
	if (Tile::tiles[liquidId] && Tile::tiles[liquidId]->material == Material::lava) {
		for (int xx = 0; xx < 16; xx++)
			for (int zz = 0; zz < 16; zz++)
				for (int yy = 0; yy < 8; yy++) {
					if (!mask[(xx * 16 + zz) * 8 + yy]) continue;
					bool edge = (xx == 0 || xx == 15 || zz == 0 || zz == 15 || yy == 0 || yy == 7)
						|| !mask[((xx + 1) * 16 + zz) * 8 + yy] || !mask[((xx - 1) * 16 + zz) * 8 + yy]
						|| !mask[(xx * 16 + zz + 1) * 8 + yy] || !mask[(xx * 16 + zz - 1) * 8 + yy]
						|| !mask[(xx * 16 + zz) * 8 + yy + 1] || !mask[(xx * 16 + zz) * 8 + yy - 1];
					(void)edge;
					if ((yy < 4 || rand.nextInt(2) != 0)
						&& level->getTile(x + xx, y + yy, z + zz) != 0
						&& level->getMaterial(x + xx, y + yy, z + zz)->isSolid())
						level->setTileNoUpdate(x + xx, y + yy, z + zz, Tile::rock->id);
				}
	}
	return true;
}

bool placeSpring(Level* level, JavaRandom& rand, int x, int y, int z, int liquidId) {
	if (level->getTile(x, y + 1, z) != Tile::rock->id) return false;
	if (level->getTile(x, y - 1, z) != Tile::rock->id) return false;
	int id = level->getTile(x, y, z);
	if (id != 0 && id != Tile::rock->id) return false;
	int stone = 0, air = 0;
	if (level->getTile(x - 1, y, z) == Tile::rock->id) stone++;
	if (level->getTile(x + 1, y, z) == Tile::rock->id) stone++;
	if (level->getTile(x, y, z - 1) == Tile::rock->id) stone++;
	if (level->getTile(x, y, z + 1) == Tile::rock->id) stone++;
	if (level->isEmptyTile(x - 1, y, z)) air++;
	if (level->isEmptyTile(x + 1, y, z)) air++;
	if (level->isEmptyTile(x, y, z - 1)) air++;
	if (level->isEmptyTile(x, y, z + 1)) air++;
	if (stone == 3 && air == 1) {
		level->setTile(x, y, z, liquidId);
		return true;
	}
	return false;
}

bool placePumpkin(Level* level, JavaRandom& rand, int x, int y, int z) {
	// Tile::pumpkin is declared but never registered in this PE build;
	// resolve by id and skip placement if absent (RNG walk stays identical).
	Tile* pumpkin = (86 >= 0 && 86 < Tile::NUM_BLOCK_TYPES) ? Tile::tiles[86] : NULL;
	for (int i = 0; i < 64; i++) {
		int xx = x + rand.nextInt(8) - rand.nextInt(8);
		int yy = y + rand.nextInt(4) - rand.nextInt(4);
		int zz = z + rand.nextInt(8) - rand.nextInt(8);
		if (level->isEmptyTile(xx, yy, zz) && level->getTile(xx, yy - 1, zz) == Tile::grass->id) {
			int meta = rand.nextInt(4);
			if (pumpkin)
				level->setTileAndData(xx, yy, zz, pumpkin->id, meta);
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// Dungeon (er). Spawner block skipped (PE tile 52 is NULL); mob-type RNG kept.
// ---------------------------------------------------------------------------
static ItemInstance* dungeonLoot(JavaRandom& rand, int& auxOut) {
	auxOut = 0;
	// NOTE: only Items actually registered in this PE build are referenced
	// (saddle/bucket/golden apple/redstone/records are commented out in
	// Item.cpp and would not link). RNG consumption is identical either way.
	int i = rand.nextInt(11);
	if (i == 0) return NULL; // saddle (not registered)
	if (i == 1) return Item::ironIngot ? new ItemInstance(Item::ironIngot, rand.nextInt(4) + 1) : NULL;
	if (i == 2) return Item::bread ? new ItemInstance(Item::bread) : NULL;
	if (i == 3) return Item::wheat ? new ItemInstance(Item::wheat, rand.nextInt(4) + 1) : NULL;
	if (i == 4) return Item::sulphur ? new ItemInstance(Item::sulphur, rand.nextInt(4) + 1) : NULL;
	if (i == 5) return Item::string ? new ItemInstance(Item::string, rand.nextInt(4) + 1) : NULL;
	if (i == 6) return NULL; // bucket (not registered)
	if (i == 7) {
		if (rand.nextInt(100) == 0)
			return NULL; // golden apple (not registered)
		return NULL;
	}
	if (i == 8) {
		if (rand.nextInt(2) == 0) {
			rand.nextInt(4); // keep stream identical (redstone count, not registered)
			return NULL;
		}
		return NULL;
	}
	if (i == 9) {
		if (rand.nextInt(10) == 0) {
			rand.nextInt(2); // keep stream identical (record variant, not registered)
			return NULL;
		}
		return NULL;
	}
	if (i == 10)
		return Item::dye_powder ? new ItemInstance(Item::dye_powder, 1, 3) : NULL;
	return NULL;
}

bool placeDungeon(Level* level, JavaRandom& rand, int x, int y, int z) {
	int h = 3;
	int w = rand.nextInt(2) + 2;
	int d = rand.nextInt(2) + 2;
	int openings = 0;
	for (int xx = x - w - 1; xx <= x + w + 1; xx++) {
		for (int yy = y - 1; yy <= y + h + 1; yy++) {
			for (int zz = z - d - 1; zz <= z + d + 1; zz++) {
				const Material* m = level->getMaterial(xx, yy, zz);
				if (yy == y - 1 && !m->isSolid()) return false;
				if (yy == y + h + 1 && !m->isSolid()) return false;
				if (xx == x - w - 1 || xx == x + w + 1 || zz == z - d - 1 || zz == z + d + 1) {
					if (yy != y) continue;
					if (!level->isEmptyTile(xx, yy, zz)) continue;
					if (!level->isEmptyTile(xx, yy + 1, zz)) continue;
					openings++;
				}
			}
		}
	}
	if (openings < 1 || openings > 5) return false;
	for (int xx = x - w - 1; xx <= x + w + 1; xx++) {
		for (int yy = y + h; yy >= y - 1; yy--) {
			for (int zz = z - d - 1; zz <= z + d + 1; zz++) {
				if (xx == x - w - 1 || yy == y - 1 || zz == z - d - 1
					|| xx == x + w + 1 || yy == y + h + 1 || zz == z + d + 1) {
					if (yy < 0) continue;
					if (!level->getMaterial(xx, yy - 1, zz)->isSolid()) continue;
					if (yy == y - 1) {
						if (rand.nextInt(4) != 0)
							level->setTile(xx, yy, zz, Tile::mossStone->id);
						else
							level->setTile(xx, yy, zz, Tile::stoneBrick->id); // beta cobble (id 4)
					} else {
						level->setTile(xx, yy, zz, 0);
					}
				}
			}
		}
	}
	for (int a = 0; a < 2; a++) {
		for (int t = 0; t < 3; t++) {
			int cx = x + rand.nextInt(2 * w + 1) - w;
			int cz = z + rand.nextInt(2 * d + 1) - d;
			if (!level->isEmptyTile(cx, y, cz)) continue;
			int solids = 0;
			if (level->getMaterial(cx - 1, y, cz)->isSolid()) solids++;
			if (level->getMaterial(cx + 1, y, cz)->isSolid()) solids++;
			if (level->getMaterial(cx, y, cz - 1)->isSolid()) solids++;
			if (level->getMaterial(cx, y, cz + 1)->isSolid()) solids++;
			if (solids != 1) continue;
			level->setTile(cx, y, cz, Tile::chest->id);
			TileEntity* te = level->getTileEntity(cx, y, cz);
			ChestTileEntity* chest = dynamic_cast<ChestTileEntity*>(te);
			if (chest) {
				int size = chest->getContainerSize();
				for (int s = 0; s < 8; s++) {
					int aux = 0;
					ItemInstance* item = dungeonLoot(rand, aux);
					if (item) {
						chest->setItem(rand.nextInt(size), item);
						delete item;
					}
				}
			} else {
				// No tile entity (shouldn't happen): still consume identical RNG.
				for (int s = 0; s < 8; s++) {
					int aux = 0;
					ItemInstance* item = dungeonLoot(rand, aux);
					delete item;
					rand.nextInt(27);
				}
			}
		}
	}
	// Spawner: PE has no spawner tile entity/block data — consume mob-type RNG only.
	rand.nextInt(4);
	return true;
}

} // namespace Beta173Features

