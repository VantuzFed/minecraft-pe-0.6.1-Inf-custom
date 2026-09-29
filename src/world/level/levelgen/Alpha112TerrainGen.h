#ifndef ALPHA112_TERRAIN_GEN_H__
#define ALPHA112_TERRAIN_GEN_H__

// Pure (Level-free) part of the Alpha 1.1.2 overworld generator:
// octave noises, density field, surface builder. Mirrors nw.a/b
// (generateTerrain / initializeNoiseField / replaceBlocksForBiome).
// Kept separate so chunk bytes can be verified against the original.
//
// Unlike Beta 1.7.3 there are no biomes: one global grass/dirt surface,
// global SnowCovered flag (default off) for ice instead of water.

#include <vector>
#include "synth/Alpha112Noise.h"
#include "../../../util/JavaRandom.h"

class Alpha112TerrainGen {
public:
	explicit Alpha112TerrainGen(int64_t seed);

	// Mirrors nw.b(int,int) chunk factory (minus caves/LevelChunk):
	// reseeds the stream, builds terrain and surface.
	// blocks must hold 16*16*128 bytes, layout (x*16+z)*128+y.
	void generateChunk(int chunkX, int chunkZ, unsigned char* blocks);

	// Test hook: terrain only (no surface pass).
	void generateTerrainOnly(int chunkX, int chunkZ, unsigned char* blocks) {
		uint64_t s = (uint64_t)(int64_t)chunkX * (uint64_t)341873128712LL
			+ (uint64_t)(int64_t)chunkZ * (uint64_t)132897987541LL;
		m_rand.setSeed((int64_t)s);
		generateTerrain(chunkX, chunkZ, blocks);
	}

	// Exposed for tests / level source use.
	JavaRandom m_rand;
	bool m_snowCovered;

	// Tree-density point sample (shares the stateless octave object).
	double sampleTreeDensity(double x, double z) const {
		return m_noiseC.sample2D(x, z);
	}

private:
	void generateTerrain(int chunkX, int chunkZ, unsigned char* blocks);
	void initializeNoiseField(std::vector<double>& out,
		int x0, int y0, int z0, int xs, int ys, int zs);
	void replaceSurface(int chunkX, int chunkZ, unsigned char* blocks);

	Alpha112Octaves m_noiseK; // 16 (low)
	Alpha112Octaves m_noiseL; // 16 (upper)
	Alpha112Octaves m_noiseM; // 8  (selector)
	Alpha112Octaves m_noiseN; // 4  (sand/gravel)
	Alpha112Octaves m_noiseO; // 4  (depth noise)
	Alpha112Octaves m_noiseA; // 10 (depth base)
	Alpha112Octaves m_noiseB; // 16 (mountain)
	Alpha112Octaves m_noiseC; // 8  (trees)

	std::vector<double> m_density;
	std::vector<double> m_sand;
	std::vector<double> m_gravel;
	std::vector<double> m_stoneNoise;
	std::vector<double> m_d, m_e, m_f, m_g, m_h;
};

#endif
