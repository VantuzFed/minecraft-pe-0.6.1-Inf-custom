#ifndef BETA173_BIOME_H__
#define BETA173_BIOME_H__

// Port of Minecraft Beta 1.7.3 WorldChunkManager (xv) + BiomeGenBase (kd),
// transcribed from the original jar. Uses BetaOctaves2D noise so identical
// seeds give identical biomes to the original game.
//
// Biome indices match kd static fields a..m:
//   0 Rainforest(yj) 1 Swampland(tf) 2 SeasonalForest(kd) 3 Forest(rb)
//   4 Savanna(fs) 5 Shrubland(kd) 6 Taiga(g) 7 Desert(fs) 8 Plains(fs)
//   9 IceDesert(fs) 10 Tundra(kd) 11 Hell(t) 12 Sky(ry)

#include <string>
#include <vector>
#include "synth/BetaNoise.h"
#include "../../../util/JavaRandom.h"

struct BetaBiome {
	const char* name;
	int color;        // kd.o (grass tint base)
	int topId;        // kd.p (top block, beta id)
	int fillerId;     // kd.q (filler block, beta id)
	int waterColor;   // kd.r
	bool snowy;       // kd.b() — snowfall
	bool noRain;      // kd.e() — no precipitation at all
	int treeKind;     // 0 default(oak/big), 1 taiga, 2 forest, 3 rainforest
};

// rain/snow helper: mirrors BiomeGenBase temperature use in populate.
enum BetaBiomeId {
	BETA_RAINFOREST = 0,
	BETA_SWAMPLAND = 1,
	BETA_SEASONAL = 2,
	BETA_FOREST = 3,
	BETA_SAVANNA = 4,
	BETA_SHRUBLAND = 5,
	BETA_TAIGA = 6,
	BETA_DESERT = 7,
	BETA_PLAINS = 8,
	BETA_ICEDESERT = 9,
	BETA_TUNDRA = 10,
	BETA_HELL = 11,
	BETA_SKY = 12,
	BETA_COUNT = 13
};

class BetaBiomeSource {
public:
	explicit BetaBiomeSource(int64_t seed);

	static const BetaBiome BIOMES[BETA_COUNT];

	// Mirrors xv.a(int,int): single biome at block coords.
	const BetaBiome* getBiome(int x, int z);
	// Mirrors xv.b(int,int): single temperature value.
	double getTemperature(int x, int z);
	// Mirrors xv.a(double[],int,int,int,int): temperature block.
	double* getTemperatureBlock(std::vector<double>& out, int x, int z, int w, int h);
	// Mirrors xv.a(kd[],int,int,int,int): biome block (also refreshes temp/humid arrays).
	const BetaBiome** getBiomeBlock(std::vector<const BetaBiome*>& out, int x, int z, int w, int h);

	// Last computed arrays (mirrors xv.a/b/c fields). Valid after getBiomeBlock.
	std::vector<double> temperatures;
	std::vector<double> humidities;

	static const BetaBiome* lookup(double temp, double humid);
	static const BetaBiome* lookupFloat(float temp, float humid);

private:
	BetaOctaves2D m_tempNoise;   // 4 octaves, seed*9871
	BetaOctaves2D m_humidNoise;  // 4 octaves, seed*39811
	BetaOctaves2D m_extraNoise;  // 2 octaves, seed*543321
	std::vector<double> m_noise;
	std::vector<const BetaBiome*> m_biomes;
	static const BetaBiome* s_table[64 * 64];
	static bool s_tableBuilt;
	static void buildTable();
};

#endif
