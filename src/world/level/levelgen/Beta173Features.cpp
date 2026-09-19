#include "Beta173Features.h"

#include <cmath>
#include <vector>

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
// Big oak (ih). Faithful port from the original jar:
//  - the generator keeps an INTERNAL Random seeded with exactly one
//    nextLong() from the populate stream, so every big-tree attempt shifts
//    the populate stream by one nextLong no matter the outcome;
//  - populate calls a(1,1,1) first, which sets m=12, n=5, j=k=1.0;
//  - e() checks soil/clearance (shortening height when blocked far up);
//  - a() lays out branches, b() stacks leaves at their ends,
//    c() draws the trunk, d() draws the branch segments.
// ---------------------------------------------------------------------------
struct BigTreeState {
	Level* level;
	JavaRandom b;    // internal RNG, seeded from the populate stream
	int ox, oy, oz;  // d[3] origin
	int height;      // e
	int trunkTop;    // f
	int n;           // leafDistanceLimit (5 after a(1,1,1))
	struct Node { int x, y, z, baseY; };
	std::vector<Node> nodes; // o
};

// Mirrors ih.a(int)F. Float arithmetic like the original.
static float bigLayerSize(int height, int y) {
	if ((double)y < (double)height * 0.3)
		return -1.618f;
	float half = (float)height / 2.0f;
	float dy = (float)height / 2.0f - (float)y;
	float r;
	if (dy == 0.0f) r = half;
	else if (fabsf(dy) >= half) r = 0.0f;
	else r = (float)sqrt(pow((double)fabsf(half), 2.0) - pow((double)fabsf(dy), 2.0));
	return r * 0.5f;
}

// Mirrors ih.b(int)F: leaf-stack radius profile.
static float bigLeafProfile(int n, int y) {
	if (y < 0 || y >= n) return -1.0f;
	if (y == 0 || y == n - 1) return 2.0f;
	return 3.0f;
}

// Mirrors ih.a(int,int,int,float,byte,int) for the XZ plane (axis 1).
static void bigLeafDisc(Level* level, int x, int y, int z, float radius) {
	int r = (int)((double)radius + 0.618);
	for (int dx = -r; dx <= r; dx++) {
		for (int dz = -r; dz <= r; dz++) {
			double dd = sqrt(pow((double)sgnAbs(dx) + 0.5, 2.0)
				+ pow((double)sgnAbs(dz) + 0.5, 2.0));
			if (dd > (double)radius) continue;
			int id = level->getTile(x + dx, y, z + dz);
			if (id == 0 || id == BB_LEAVES)
				level->setTile(x + dx, y, z + dz, LEAF_ID());
		}
	}
}

// Mirrors ih.a(int,int,int): n leaf layers with the b(int) profile.
static void bigLeafStack(Level* level, int x, int y, int z, int n) {
	for (int i = 0; i < n; i++) {
		float r = bigLeafProfile(n, i);
		if (r < 0.0f) continue;
		bigLeafDisc(level, x, y + i, z, r);
	}
}

// Dominant-axis helper shared by the line check/draw (otherCoordPairs).
static int bigDominantAxis(int dx, int dy, int dz) {
	int d[3] = { dx, dy, dz };
	int best = 0;
	for (int i = 1; i < 3; i++)
		if (sgnAbs(d[i]) > sgnAbs(d[best])) best = i;
	return best;
}

// Mirrors ih.a(int[],int[])I: -1 when the whole line is air/leaves,
// otherwise the walked distance where it got blocked.
static int bigCheckLine(Level* level,
	int x0, int y0, int z0, int x1, int y1, int z1) {
	static const int PAIRS[6] = { 2, 0, 0, 1, 2, 1 };
	int dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
	int d[3] = { dx, dy, dz };
	int s[3] = { x0, y0, z0 };
	int dom = bigDominantAxis(dx, dy, dz);
	if (d[dom] == 0) return -1;
	int a2 = PAIRS[dom], a3 = PAIRS[dom + 3];
	int sign = d[dom] > 0 ? 1 : -1;
	double r2 = (double)d[a2] / (double)d[dom];
	double r3 = (double)d[a3] / (double)d[dom];
	int end = d[dom] + sign;
	for (int i = 0; i != end; i += sign) {
		int p[3];
		p[dom] = s[dom] + i;
		p[a2] = BetaMath::floor((double)s[a2] + (double)i * r2 + 0.5);
		p[a3] = BetaMath::floor((double)s[a3] + (double)i * r3 + 0.5);
		int id = level->getTile(p[0], p[1], p[2]);
		if (id != 0 && id != BB_LEAVES) return sgnAbs(i);
	}
	return -1;
}

// Mirrors ih.a(int[],int[],int)V: unconditional log line along the same walk.
static void bigDrawLine(Level* level,
	int x0, int y0, int z0, int x1, int y1, int z1) {
	static const int PAIRS[6] = { 2, 0, 0, 1, 2, 1 };
	int dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
	int d[3] = { dx, dy, dz };
	int s[3] = { x0, y0, z0 };
	int dom = bigDominantAxis(dx, dy, dz);
	if (d[dom] == 0) return;
	int a2 = PAIRS[dom], a3 = PAIRS[dom + 3];
	int sign = d[dom] > 0 ? 1 : -1;
	double r2 = (double)d[a2] / (double)d[dom];
	double r3 = (double)d[a3] / (double)d[dom];
	int end = d[dom] + sign;
	for (int i = 0; i != end; i += sign) {
		int p[3];
		p[dom] = s[dom] + i;
		p[a2] = BetaMath::floor((double)s[a2] + (double)i * r2 + 0.5);
		p[a3] = BetaMath::floor((double)s[a3] + (double)i * r3 + 0.5);
		level->setTile(p[0], p[1], p[2], LOG_ID());
	}
}

// Mirrors ih.e()Z: grass/dirt soil, trunk column must be clear
// (shortens the height when blocked 6+ up).
static bool bigValidLocation(BigTreeState& t) {
	Level* level = t.level;
	int soil = level->getTile(t.ox, t.oy - 1, t.oz);
	if (soil != Tile::grass->id && soil != Tile::dirt->id) return false;
	int dist = bigCheckLine(level,
		t.ox, t.oy, t.oz, t.ox, t.oy + t.height - 1, t.oz);
	if (dist == -1) return true;
	if (dist < 6) return false;
	t.height = dist;
	return true;
}

// Mirrors ih.a()V: branch layout from the top down.
static void bigLayout(BigTreeState& t) {
	t.trunkTop = (int)((double)t.height * 0.618);
	if (t.trunkTop >= t.height) t.trunkTop = t.height - 1;
	int branchCount = (int)(1.382 + pow((double)t.height / 13.0, 2.0));
	if (branchCount < 1) branchCount = 1;
	t.nodes.clear();
	t.nodes.reserve((size_t)branchCount * (size_t)t.height);
	int top = t.oy + t.height - t.n;
	for (int lvl = top, rel = lvl - t.oy; rel >= 0; --lvl, --rel) {
		float taper = bigLayerSize(t.height, rel);
		if (taper < 0.0f) continue;
		for (int i = 0; i < branchCount; i++) {
			double spread = (double)taper * ((double)t.b.nextFloat() + 0.328);
			double ang = (double)t.b.nextFloat() * 2.0 * 3.14159;
			int bx = BetaMath::floor(spread * sin(ang) + (double)t.ox + 0.5);
			int bz = BetaMath::floor(spread * cos(ang) + (double)t.oz + 0.5);
			double dist = sqrt(pow((double)sgnAbs(t.ox - bx), 2.0)
				+ pow((double)sgnAbs(t.oz - bz), 2.0));
			double lowered = (double)lvl - dist * 0.381;
			int baseY = (lowered > (double)(t.oy + t.trunkTop))
				? (t.oy + t.trunkTop) : (int)lowered;
			if (bigCheckLine(t.level, t.ox, baseY, t.oz, bx, lvl, bz) != -1)
				continue;
			BigTreeState::Node nd;
			nd.x = bx; nd.y = lvl; nd.z = bz; nd.baseY = baseY;
			t.nodes.push_back(nd);
		}
	}
}

bool placeBigTree(Level* level, JavaRandom& rand, int x, int y, int z) {
	BigTreeState t;
	t.level = level;
	// The original funnels all branch randomness through an internal
	// Random: exactly one nextLong() leaves the populate stream here.
	t.b.setSeed(rand.nextLong());
	t.ox = x; t.oy = y; t.oz = z;
	t.height = 0;
	t.trunkTop = 0;
	// pg.a(1,1,1) as done by populate for every tree: m=12, n=5, j=k=1.0.
	t.n = 5;
	t.height = 5 + t.b.nextInt(12);
	if (!bigValidLocation(t)) return false;
	bigLayout(t);
	for (size_t i = 0; i < t.nodes.size(); i++) // b(): leaf stacks
		bigLeafStack(level, t.nodes[i].x, t.nodes[i].y, t.nodes[i].z, t.n);
	bigDrawLine(level, x, y, z, x, y + t.trunkTop, z); // c(): trunk
	for (size_t i = 0; i < t.nodes.size(); i++) { // d(): branches
		if ((double)(t.nodes[i].baseY - y) < (double)t.height * 0.2) continue;
		bigDrawLine(level, x, t.nodes[i].baseY, z,
			t.nodes[i].x, t.nodes[i].y, t.nodes[i].z);
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
// Taiga1 (pw): tall spruce 7..11, meta 1. Leaf layers carry no RNG:
// corners are cut deterministically, so the whole tree costs exactly
// the 3 header nextInts no matter the outcome.
// ---------------------------------------------------------------------------
bool placeTaiga1(Level* level, JavaRandom& rand, int x, int y, int z) {
	int h = rand.nextInt(5) + 7;
	int j = h - rand.nextInt(2) - 3;
	int kk = 1 + rand.nextInt(h - j + 1);
	if (y < 1 || y + h + 1 > 128) return false;
	int canGrow = 1;
	for (int yy = y; yy <= y + 1 + h && canGrow; yy++) {
		int r = (yy - y >= j) ? kk : 0;
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
	// Cone widening downward: radius 0 at the tip, up to kk.
	int r = 0;
	for (int yy = y + h; yy >= y + j; yy--) {
		for (int xx = x - r; xx <= x + r; xx++) {
			for (int zz = z - r; zz <= z + r; zz++) {
				int dx = sgnAbs(xx - x);
				int dz = sgnAbs(zz - z);
				if (dx == r && dz == r && r > 0) continue;
				if (isOpaque(level->getTile(xx, yy, zz))) continue;
				level->setTileAndData(xx, yy, zz, LEAF_ID(), 1);
			}
		}
		if (r >= 1 && yy == y + j + 1) r--;
		else if (r < kk) r++;
	}
	for (int i = 0; i < h - 1; i++) {
		int id = level->getTile(x, y + i, z);
		if (id == 0 || id == LEAF_ID())
			level->setTileAndData(x, y + i, z, LOG_ID(), 1);
	}
	return true;
}

// ---------------------------------------------------------------------------
// Taiga2 (ws): pine 6..9, meta 1. Layer radii walk r/top/old with one
// nextInt(2) header; the trunk is shortened by nextInt(3).
// ---------------------------------------------------------------------------
bool placeTaiga2(Level* level, JavaRandom& rand, int x, int y, int z) {
	int h = rand.nextInt(4) + 6;
	int j = 1 + rand.nextInt(2);
	int kk = 2 + rand.nextInt(2);
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
	int r = rand.nextInt(2);
	int top = 1;
	int old = 0;
	for (int i = 0; i <= h - j; i++) {
		int yy = y + h - i;
		for (int xx = x - r; xx <= x + r; xx++) {
			for (int zz = z - r; zz <= z + r; zz++) {
				int dx = sgnAbs(xx - x);
				int dz = sgnAbs(zz - z);
				if (dx == r && dz == r && r > 0) continue;
				if (isOpaque(level->getTile(xx, yy, zz))) continue;
				level->setTileAndData(xx, yy, zz, LEAF_ID(), 1);
			}
		}
		if (r < top) r++;
		else {
			r = old;
			old = 1;
			if (++top > kk) top = kk;
		}
	}
	int d = rand.nextInt(3);
	for (int i = 0; i < h - d; i++) {
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

