#ifndef BETA173_FEATURES_H__
#define BETA173_FEATURES_H__

// Placement routines ported from Minecraft Beta 1.7.3 WorldGenerator
// subclasses (.transcribed from the original jar). All randomness flows
// through JavaRandom so identical seeds give identical decoration.
//
// Beta block IDs are mapped to PE Tile IDs (classic IDs match 1:1 except
// diamond ore 56 -> emeraldOre 56, fern meta 2 -> PE FERN meta 3).
//
// Known intentional deviations (no PE counterpart, RNG stream preserved):
//   - dead bushes (block 32): positions computed, block skipped
//   - mob spawner block (tile 52 is NULL in PE): RNG consumed, block skipped
//   - missing loot items (saddle/bucket/etc. are commented out in PE Item.cpp):
//     RNG consumed, null items skipped like the original does

#include <vector>
#include "../../../util/JavaRandom.h"
#include "Beta173Biome.h"

class Level;

namespace Beta173Features {

// Beta block ids used by the generator (== PE tile ids, see mapping above).
enum BetaBlock {
	BB_AIR = 0,
	BB_STONE = 1,
	BB_GRASS = 2,
	BB_DIRT = 3,
	BB_BEDROCK = 7,
	BB_WATER = 9,
	BB_LAVA = 11,
	BB_SAND = 12,
	BB_GRAVEL = 13,
	BB_GOLD_ORE = 14,
	BB_IRON_ORE = 15,
	BB_COAL_ORE = 16,
	BB_LOG = 17,
	BB_LEAVES = 18,
	BB_SANDSTONE = 24,
	BB_TALLGRASS = 31,
	BB_FLOWER_Y = 37,
	BB_FLOWER_R = 38,
	BB_MUSHROOM_B = 39,
	BB_MUSHROOM_R = 40,
	BB_MOSSY = 48,
	BB_COBBLE = 4,
	BB_CHEST = 54,
	BB_DIAMOND_ORE = 56,
	BB_REDSTONE_ORE = 73,
	BB_SNOW = 78,
	BB_ICE = 79,
	BB_CACTUS = 81,
	BB_CLAY = 82,
	BB_REEDS = 83,
	BB_PUMPKIN = 86,
	BB_LAPIS_ORE = 21
};

// Beta opaqueCubeLookup approximation for PE ids (false = see-through).
bool isOpaque(int tileId);

// Trees. Return true if grown.
bool placeOak(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeBigTree(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeBirch(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeTaiga1(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeTaiga2(Level* level, JavaRandom& rand, int x, int y, int z);

// Biome tree selector: mirrors kd.a(Random) + overrides (consumes rand).
// treeKind: 0 default, 1 taiga(g), 2 forest(rb), 3 rainforest(yj).
// Returns 0=oak 1=bigtree 2=birch 3=taiga1 4=taiga2.
int pickTree(JavaRandom& rand, int treeKind);
bool placePickedTree(Level* level, JavaRandom& rand, int kind, int x, int y, int z);

// Small features (always return true like the originals).
bool placeFlowers(Level* level, JavaRandom& rand, int x, int y, int z, int flowerId);
bool placeTallGrass(Level* level, JavaRandom& rand, int x, int y, int z, int meta);
bool placeDeadBush(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeReeds(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeCactus(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeClay(Level* level, JavaRandom& rand, int x, int y, int z, int size);
bool placeMinable(Level* level, JavaRandom& rand, int x, int y, int z, int oreId, int size);
bool placeLake(Level* level, JavaRandom& rand, int x, int y, int z, int liquidId);
bool placeDungeon(Level* level, JavaRandom& rand, int x, int y, int z);
bool placePumpkin(Level* level, JavaRandom& rand, int x, int y, int z);
bool placeSpring(Level* level, JavaRandom& rand, int x, int y, int z, int liquidId);

} // namespace Beta173Features

#include "Beta173Caves.h"

#endif
