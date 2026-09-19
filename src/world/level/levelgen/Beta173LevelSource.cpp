#include "Beta173LevelSource.h"

#include <cstring>

#include "../Level.h"
#include "../ChunkPos.h"
#include "../biome/Biome.h"
#include "../biome/BiomeSource.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../tile/TallGrass.h"
#include "../material/Material.h"
#include "../../../util/Random.h"

using namespace Beta173Features;

Beta173LevelSource::Beta173LevelSource(Level* level, int64_t seed, bool spawnMobs)
	: m_level(level), m_seed(seed), m_spawnMobs(spawnMobs), m_rand(seed),
	  m_terrain(seed), m_mobRand((long)seed) {
}

Beta173LevelSource::~Beta173LevelSource() {
}

// Nearest PE biome for mob spawning only (never affects terrain RNG).
const Biome* Beta173LevelSource::betaToPeBiome(const BetaBiome* b) {
	if (!b) return Biome::plains;
	switch (b - &BetaBiomeSource::BIOMES[0]) {
	case BETA_RAINFOREST: return Biome::rainForest;
	case BETA_SWAMPLAND: return Biome::swampland;
	case BETA_SEASONAL: return Biome::seasonalForest;
	case BETA_FOREST: return Biome::forest;
	case BETA_SAVANNA: return Biome::savanna;
	case BETA_SHRUBLAND: return Biome::shrubland;
	case BETA_TAIGA: return Biome::taiga;
	case BETA_DESERT: return Biome::desert;
	case BETA_PLAINS: return Biome::plains;
	case BETA_ICEDESERT: return Biome::iceDesert;
	case BETA_TUNDRA: return Biome::tundra;
	default: return Biome::plains;
	}
}

Biome::MobList Beta173LevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
	const BetaBiome* b = m_terrain.m_biomes.getBiome(x, z);
	Biome* pb = const_cast<Biome*>(betaToPeBiome(b));
	if (!pb) return Biome::MobList();
	return pb->getMobs(mobCategory);
}

LevelChunk* Beta173LevelSource::create(int x, int z) {
	return getChunk(x, z);
}

LevelChunk* Beta173LevelSource::getChunk(int xOffs, int zOffs) {
	unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
	memset(blocks, 0, LevelChunk::ChunkBlockCount);
	LevelChunk* chunk = new LevelChunk(m_level, blocks, xOffs, zOffs);

	m_terrain.generateChunk(xOffs, zOffs, blocks);
	m_caves.generate(xOffs, zOffs, m_seed, blocks);
	chunk->recalcHeightmap(false);
	return chunk;
}

// Mirrors yf.initializeNoiseField(double[],int,int,int,int,int,int).
void Beta173LevelSource::postProcess(ChunkSource* parent, int xt, int zt) {
	(void)parent;
	m_level->isGeneratingTerrain = true;

	int xo = xt * 16;
	int zo = zt * 16;
	const BetaBiome* biome = m_terrain.m_biomes.getBiome(xo + 16, zo + 16);

	m_rand.setSeed(m_seed);
	int64_t l1 = m_rand.nextLong() / 2L * 2L + 1L;
	int64_t l2 = m_rand.nextLong() / 2L * 2L + 1L;
	{
		uint64_t s = (uint64_t)(int64_t)xt * (uint64_t)l1 + (uint64_t)(int64_t)zt * (uint64_t)l2;
		m_rand.setSeed((int64_t)(s ^ (uint64_t)m_seed));
	}

	if (m_rand.nextInt(4) == 0) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeLake(m_level, m_rand, x, y, z, Tile::calmWater->id);
	}
	if (m_rand.nextInt(8) == 0) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(m_rand.nextInt(120) + 8);
		int z = zo + m_rand.nextInt(16) + 8;
		if (y < 64 || m_rand.nextInt(10) == 0)
			placeLake(m_level, m_rand, x, y, z, Tile::calmLava->id);
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
		placeMinable(m_level, m_rand, x, y, z, Tile::emeraldOre->id, 7); // beta diamond (id 56)
	}
	for (int k = 0; k < 1; k++) {
		int x = xo + m_rand.nextInt(16);
		int y = m_rand.nextInt(16) + m_rand.nextInt(16);
		int z = zo + m_rand.nextInt(16);
		placeMinable(m_level, m_rand, x, y, z, Tile::lapisOre->id, 6);
	}

	double treeNoise = m_terrain.sampleTreeDensity((double)xo * 0.5, (double)zo * 0.5);
	int treeBase = (int)((treeNoise / 8.0 + m_rand.nextDouble() * 4.0 + 4.0) / 3.0);
	int treeCount = 0;
	if (m_rand.nextInt(10) == 0) treeCount++;
	if (biome == &BetaBiomeSource::BIOMES[BETA_FOREST]) treeCount += treeBase + 5;
	if (biome == &BetaBiomeSource::BIOMES[BETA_RAINFOREST]) treeCount += treeBase + 5;
	if (biome == &BetaBiomeSource::BIOMES[BETA_SEASONAL]) treeCount += treeBase + 2;
	if (biome == &BetaBiomeSource::BIOMES[BETA_TAIGA]) treeCount += treeBase + 5;
	if (biome == &BetaBiomeSource::BIOMES[BETA_DESERT]) treeCount += -20;
	if (biome == &BetaBiomeSource::BIOMES[BETA_TUNDRA]) treeCount += -20;
	if (biome == &BetaBiomeSource::BIOMES[BETA_PLAINS]) treeCount += -20;
	for (int k = 0; k < treeCount; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int z = zo + m_rand.nextInt(16) + 8;
		int kind = pickTree(m_rand, biome->treeKind);
		int y = m_level->getHeightmap(x, z);
		placePickedTree(m_level, m_rand, kind, x, y, z);
	}

	int flowerCount = 0;
	if (biome == &BetaBiomeSource::BIOMES[BETA_FOREST]) flowerCount = 2;
	if (biome == &BetaBiomeSource::BIOMES[BETA_SEASONAL]) flowerCount = 4;
	if (biome == &BetaBiomeSource::BIOMES[BETA_TAIGA]) flowerCount = 2;
	if (biome == &BetaBiomeSource::BIOMES[BETA_PLAINS]) flowerCount = 3;
	for (int k = 0; k < flowerCount; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeFlowers(m_level, m_rand, x, y, z, Tile::flower->id);
	}

	int grassCount = 0;
	if (biome == &BetaBiomeSource::BIOMES[BETA_FOREST]) grassCount = 2;
	if (biome == &BetaBiomeSource::BIOMES[BETA_RAINFOREST]) grassCount = 10;
	if (biome == &BetaBiomeSource::BIOMES[BETA_SEASONAL]) grassCount = 2;
	if (biome == &BetaBiomeSource::BIOMES[BETA_TAIGA]) grassCount = 1;
	if (biome == &BetaBiomeSource::BIOMES[BETA_PLAINS]) grassCount = 10;
	for (int k = 0; k < grassCount; k++) {
		int type = 1;
		if (biome == &BetaBiomeSource::BIOMES[BETA_RAINFOREST] && m_rand.nextInt(3) != 0)
			type = 2;
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		// Beta fern meta 2 -> PE FERN meta (visual only, no RNG impact).
		placeTallGrass(m_level, m_rand, x, y, z, type == 2 ? TallGrass::FERN : TallGrass::TALL_GRASS);
	}

	int shrubCount = 0;
	if (biome == &BetaBiomeSource::BIOMES[BETA_DESERT]) shrubCount = 2;
	for (int k = 0; k < shrubCount; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeDeadBush(m_level, m_rand, x, y, z);
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
	if (m_rand.nextInt(32) == 0) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placePumpkin(m_level, m_rand, x, y, z);
	}
	int cactusCount = 0;
	if (biome == &BetaBiomeSource::BIOMES[BETA_DESERT]) cactusCount += 10;
	for (int k = 0; k < cactusCount; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(128);
		int z = zo + m_rand.nextInt(16) + 8;
		placeCactus(m_level, m_rand, x, y, z);
	}

	for (int k = 0; k < 50; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(m_rand.nextInt(120) + 8);
		int z = zo + m_rand.nextInt(16) + 8;
		placeSpring(m_level, m_rand, x, y, z, Tile::calmWater->id);
	}
	for (int k = 0; k < 20; k++) {
		int x = xo + m_rand.nextInt(16) + 8;
		int y = m_rand.nextInt(m_rand.nextInt(m_rand.nextInt(112) + 8) + 8);
		int z = zo + m_rand.nextInt(16) + 8;
		placeSpring(m_level, m_rand, x, y, z, Tile::calmLava->id);
	}

	if (m_spawnMobs && !m_level->isClientSide) {
		const Biome* pb = betaToPeBiome(biome);
		m_mobRand.setSeed((long)((int64_t)xt * 341873128712LL + (int64_t)zt * 132897987541LL + m_seed));
		MobSpawner::postProcessSpawnMobs(m_level, const_cast<Biome*>(pb), xo + 8, zo + 8, 16, 16, &m_mobRand);
	}

	m_terrain.m_biomes.getTemperatureBlock(m_snowTemp, xo + 8, zo + 8, 16, 16);
	for (int x = xo + 8; x < xo + 8 + 16; x++) {
		for (int z = zo + 8; z < zo + 8 + 16; z++) {
			int ix = x - (xo + 8);
			int iz = z - (zo + 8);
			int y = m_level->getHeightmap(x, z);
			double t = m_snowTemp[ix * 16 + iz] - (double)(y - 64) / 64.0 * 0.3;
			if (t < 0.5 && y > 0 && y < 128 && m_level->isEmptyTile(x, y, z)
				&& m_level->getMaterial(x, y - 1, z)->blocksMotion()
				&& m_level->getMaterial(x, y - 1, z) != Material::ice)
				m_level->setTile(x, y, z, Tile::topSnow->id);
		}
	}

	m_level->isGeneratingTerrain = false;
}
