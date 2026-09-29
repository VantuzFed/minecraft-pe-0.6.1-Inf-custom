#include "Alpha112LevelSource.h"

#include <cstring>

#include "../Level.h"
#include "../ChunkPos.h"
#include "../biome/Biome.h"
#include "../biome/BiomeSource.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../material/Material.h"
#include "../../../util/Random.h"

using namespace Beta173Features;

Alpha112LevelSource::Alpha112LevelSource(Level* level, int64_t seed, bool spawnMobs)
	: m_level(level), m_seed(seed), m_spawnMobs(spawnMobs), m_rand(seed),
	  m_terrain(seed), m_mobRand((long)seed) {
}

Alpha112LevelSource::~Alpha112LevelSource() {
}

Biome::MobList Alpha112LevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
	// No biomes in Alpha: plains mobs everywhere (never affects terrain RNG).
	Biome* pb = Biome::plains;
	if (!pb) return Biome::MobList();
	return pb->getMobs(mobCategory);
}

LevelChunk* Alpha112LevelSource::create(int x, int z) {
	return getChunk(x, z);
}

LevelChunk* Alpha112LevelSource::getChunk(int xOffs, int zOffs) {
	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	memset(blocks, 0, LevelChunk::ChunkBlockCount);
	LevelChunk* chunk = new LevelChunk(m_level, blocks, xOffs, zOffs);

	m_terrain.generateChunk(xOffs, zOffs, blocks);
	m_caves.generate(xOffs, zOffs, m_seed, blocks);
	chunk->recalcHeightmap(false);
	return chunk;
}

// Mirrors nw.a(aw,int,int) populate. Exact order matters for the RNG stream:
// dungeons, clay, ores, trees, flowers, reeds, cactus, springs.
void Alpha112LevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
	(void)parent;
	m_level->isGeneratingTerrain = true;

	int xo = xt * 16;
	int zo = zt * 16;

	m_rand.setSeed(m_seed);
	int64_t l1 = m_rand.nextLong() / 2L * 2L + 1L;
	int64_t l2 = m_rand.nextLong() / 2L * 2L + 1L;
	{
		uint64_t s = (uint64_t)(int64_t)xt * (uint64_t)l1 + (uint64_t)(int64_t)zt * (uint64_t)l2;
		m_rand.setSeed((int64_t)(s ^ (uint64_t)m_seed));
	}

	for (int k = 0; k < 8; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeDungeon(m_level, m_rand, x, y, z);
	}

	for (int k = 0; k < 10; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16);
		placeClay(m_level, m_rand, x, y, z, 32);
	}
	for (int k = 0; k < 20; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::dirt->id, 32);
	}
	for (int k = 0; k < 10; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::gravel->id, 32);
	}
	for (int k = 0; k < 20; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::coalOre->id, 16);
	}
	for (int k = 0; k < 20; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(64);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::ironOre->id, 8);
	}
	for (int k = 0; k < 2; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(32);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::goldOre->id, 8);
	}
	for (int k = 0; k < 8; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(16);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::redStoneOre->id, 7);
	}
	for (int k = 0; k < 1; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(16);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::emeraldOre->id, 7); // alpha diamond (id 56)
	}

	// Trees: base density plus a 1/10 bonus, oak unless 1/10 big tree.
	// No biome modifiers in Alpha.
	double treeNoise = m_terrain.sampleTreeDensity((double)xo * 0.5, (double)zo * 0.5);
	int treeCount = (int)((treeNoise / 8.0 + m_rand.nextDouble() * 4.0 + 4.0) / 3.0);
	if (treeCount < 0) treeCount = 0;
	if (m_rand.nextInt(10) == 0) treeCount++;
	bool bigTree = (m_rand.nextInt(10) == 0);
	for (int k = 0; k < treeCount; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int z = zo + m_rand.nextInt(16) + 8;
		int y = m_level->getHeightmap(x, z);
		if (bigTree)
			placeBigTree(m_level, m_rand, x, y, z);
		else
			placeOak(m_level, m_rand, x, y, z);
	}

	for (int k = 0; k < 2; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeFlowers(m_level, m_rand, x, y, z, Tile::flower->id);
	}
	if (m_rand.nextInt(2) == 0) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeFlowers(m_level, m_rand, x, y, z, Tile::rose->id);
	}
	if (m_rand.nextInt(4) == 0) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeFlowers(m_level, m_rand, x, y, z, Tile::mushroom1->id);
	}
	if (m_rand.nextInt(8) == 0) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeFlowers(m_level, m_rand, x, y, z, Tile::mushroom2->id);
	}

	for (int k = 0; k < 10; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeReeds(m_level, m_rand, x, y, z);
	}
	for (int k = 0; k < 1; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeCactus(m_level, m_rand, x, y, z);
	}

	for (int k = 0; k < 50; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(m_rand.nextInt(120) + 8);
		int z = zo + m_rand.nextInt(16) + 8;
		placeSpring(m_level, m_rand, x, y, z, Tile::water->id);
	}
	for (int k = 0; k < 20; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(m_rand.nextInt(m_rand.nextInt(112) + 8) + 8);
		int z = zo + m_rand.nextInt(16) + 8;
		placeSpring(m_level, m_rand, x, y, z, Tile::lava->id);
	}

	// NOTE: no snow cap (SnowCovered off) and no lakes/tallgrass in Alpha.

	if (m_spawnMobs && !m_level->isClientSide) {
		m_mobRand.setSeed((long)((int64_t)xt * 341873128712LL + (int64_t)zt * 132897987541LL + m_seed));
		MobSpawner::postProcessSpawnMobs(m_level, Biome::plains, xo + 8, zo + 8, 16, 16, &m_mobRand);
	}

	m_level->isGeneratingTerrain = false;
}
