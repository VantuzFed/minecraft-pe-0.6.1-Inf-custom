#ifndef BETA173_LEVEL_SOURCE_H__
#define BETA173_LEVEL_SOURCE_H__

// Overworld chunk generator ported from Minecraft Beta 1.7.3
// ChunkProviderGenerate (yf), transcribed from the original jar.
// Uses JavaRandom + BetaNoise + BetaBiomeSource so identical seeds give
// identical terrain to the original game.

#include <string>
#include <vector>

#include "../chunk/ChunkSource.h"
#include "Beta173Biome.h"
#include "Beta173Features.h"
#include "Beta173TerrainGen.h"
#include "../../../util/JavaRandom.h"
#include "../../../util/Random.h"
#include "../MobSpawner.h"

class Level;
class LevelChunk;

class Beta173LevelSource : public ChunkSource {
public:
	Beta173LevelSource(Level* level, int64_t seed, bool spawnMobs);
	~Beta173LevelSource();

	bool hasChunk(int x, int z) { return true; }
	LevelChunk* create(int x, int z);
	LevelChunk* getChunk(int x, int z);
	void postProcess(ChunkSource* parent, int x, int z);
	bool tick() { return false; }
	bool shouldSave() { return true; }
	Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);
	std::string gatherStats() { return "Beta173LevelSource"; }

private:
	// Nearest PE biome for mob spawning only (never affects terrain RNG).
	const Biome* betaToPeBiome(const BetaBiome* b);

	Level* m_level;
	int64_t m_seed;
	bool m_spawnMobs;
	JavaRandom m_rand; // populate stream (reseeded per chunk, like the original)
	Beta173TerrainGen m_terrain;
	Beta173Caves m_caves;
	std::vector<double> m_snowTemp;

	// Mob spawning bridge: separate MT randomness, never touches the JavaRandom stream.
	Random m_mobRand;
};

#endif
