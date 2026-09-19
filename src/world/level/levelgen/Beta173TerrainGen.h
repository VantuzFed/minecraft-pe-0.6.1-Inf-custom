#ifndef BETA173_TERRAIN_GEN_H__
#define BETA173_TERRAIN_GEN_H__

// Pure (Level-free) part of the Beta 1.7.3 overworld generator:
// biome-seeded noises, density field, surface builder. Mirrors
// yf.generateTerrain / initializeNoiseField / replaceBlocksForBiome.
// Kept separate so chunk bytes can be verified against the original.

#include <vector>
#include "Beta173Biome.h"
#include "synth/BetaNoise.h"
#include "../../../util/JavaRandom.h"

class Beta173TerrainGen {
public:
	explicit Beta173TerrainGen(int64_t seed);

	// Mirrors yf.b(int,int) chunk factory (minus caves/LevelChunk):
	// reseeds the stream, builds biome block, terrain and surface.
	// blocks must hold 16*16*128 bytes, layout (x*16+z)*128+y.
	void generateChunk(int chunkX, int chunkZ, unsigned char* blocks);

	// Test hook: terrain only (no surface pass).
	void generateTerrainOnly(int chunkX, int chunkZ, unsigned char* blocks) {
		uint64_t s = (uint64_t)(int64_t)chunkX * (uint64_t)341873128712LL
			+ (uint64_t)(int64_t)chunkZ * (uint64_t)132897987541LL;
		m_rand.setSeed((int64_t)s);
		m_biomes.getBiomeBlock(m_biomeBlock, chunkX * 16, chunkZ * 16, 16, 16);
		generateTerrain(chunkX, chunkZ, blocks);
	}

	// Exposed for tests / level source use.
	JavaRandom m_rand;
	BetaBiomeSource m_biomes;
	std::vector<const BetaBiome*> m_biomeBlock;

	// Tree-density point sample (shares the stateless octave object).
	double sampleTreeDensity(double x, double z) const {
		return m_noiseC.sample2D(x, z);
	}

private:
	void generateTerrain(int chunkX, int chunkZ, unsigned char* blocks);
	void initializeNoiseField(std::vector<double>& out,
		int x0, int y0, int z0, int xs, int ys, int zs);
	void replaceBlocksForBiome(int chunkX, int chunkZ, unsigned char* blocks);

	BetaOctaves m_noiseGen1; // 16
	BetaOctaves m_noiseGen2; // 16
	BetaOctaves m_noiseGen3; // 8
	BetaOctaves m_noiseGen4; // 4
	BetaOctaves m_noiseGen5; // 4
	BetaOctaves m_mobNoise;  // 10
	BetaOctaves m_noiseGen6; // 16
	BetaOctaves m_noiseC;    // 8

	std::vector<double> m_density;
	std::vector<double> m_sand;
	std::vector<double> m_gravel;
	std::vector<double> m_stoneNoise;
	std::vector<double> m_d, m_e, m_f, m_g, m_h;
};

#endif
