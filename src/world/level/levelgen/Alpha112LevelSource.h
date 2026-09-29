#ifndef ALPHA112_LEVEL_SOURCE_H__
#define ALPHA112_LEVEL_SOURCE_H__

// Overworld chunk generator ported from Minecraft Alpha 1.1.2
// ChunkProviderGenerate (nw), transcribed from the original jar.
// Uses JavaRandom + Alpha112Noise so identical seeds give identical
// terrain to the original game. Caves are shared with Beta 1.7.3
// (verified identical in bytecode: same counts, walk, carve set).
// No biomes: one global grass/dirt surface, SnowCovered off.

#include <string>

#include "../chunk/ChunkSource.h"
#include "Beta173Features.h"
#include "Beta173Caves.h"
#include "Alpha112TerrainGen.h"
#include "../../../util/JavaRandom.h"
#include "../../../util/Random.h"
#include "../MobSpawner.h"

class Level;
class LevelChunk;

class Alpha112LevelSource : public ChunkSource {
public:
	Alpha112LevelSource(Level* level, int64_t seed, bool spawnMobs);
	~Alpha112LevelSource();

	bool hasChunk(int x, int z) { return true; }
	LevelChunk* create(int x, int z);
	LevelChunk* getChunk(int x, int z);
	void postProcess(ChunkSource* parent, int x, int z);
	bool tick() { return false; }
	bool shouldSave() { return true; }
	Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);
	std::string gatherStats() { return "Alpha112LevelSource"; }

private:
	Level* m_level;
	int64_t m_seed;
	bool m_spawnMobs;
	JavaRandom m_rand; // populate stream (reseeded per chunk, like the original)
	Alpha112TerrainGen m_terrain;
	Beta173Caves m_caves;

	// Mob spawning bridge: separate MT randomness, never touches the JavaRandom stream.
	Random m_mobRand;
};

#endif
