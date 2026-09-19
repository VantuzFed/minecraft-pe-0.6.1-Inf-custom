#include "Beta173Biome.h"

// kd static{} transcription (colors are decimal ints from bytecode):
//   1 a = new yj().b(588342).a("Rainforest").a(2094168)
//   2 b = new tf().b(522674).a("Swampland").a(9154376)
//   3 c = new kd().b(10215459).a("Seasonal Forest")
//   4 d = new rb().b(353825).a("Forest").a(5159473)
//   5 e = new fs().b(14278691).a("Savanna")
//   6 f = new kd().b(10595616).a("Shrubland")
//   7 g = new g().b(3060051).a("Taiga").b().a(8107825)
//   8 h = new fs().b(16421912).a("Desert").e()
//   9 i = new fs().b(16767248).a("Plains")
//  10 j = new fs().b(16772499).a("Ice Desert").b().e().a(12899129)
//  11 k = new kd().b(5762041).a("Tundra").b().a(12899129)
//  12 l = new t().b(16711680).a("Hell").e()
//  13 m = new ry().b(8421631).a("Sky").e()
// kd() defaults: p=top=grass(2), q=filler=dirt(3), r=5169201, rain enabled, no snow.
// b(int)=grass color o, a(String)=name, b()=snow, e()=no rain, a(int)=water color r.
// Tree kinds: yj=rainforest(3), g=taiga(1), rb=forest(2), else default(0).

// NOTE: kd.a() (lookup-table build, called from static init) forces
// desert and ice-desert top/filler to sand (verified against the jar).
const BetaBiome BetaBiomeSource::BIOMES[BETA_COUNT] = {
	{ "Rainforest",     588342,   2, 3, 2094168,  false, false, 3 }, // 0
	{ "Swampland",      522674,   2, 3, 9154376,  false, false, 0 }, // 1
	{ "SeasonalForest", 10215459, 2, 3, 5169201,  false, false, 0 }, // 2
	{ "Forest",         353825,   2, 3, 5159473,  false, false, 2 }, // 3
	{ "Savanna",        14278691, 2, 3, 5169201,  false, false, 0 }, // 4
	{ "Shrubland",      10595616, 2, 3, 5169201,  false, false, 0 }, // 5
	{ "Taiga",          3060051,  2, 3, 8107825,  true,  false, 1 }, // 6
	{ "Desert",         16421912, 12, 12, 5169201, false, true,  0 }, // 7
	{ "Plains",         16767248, 2, 3, 5169201,  false, false, 0 }, // 8
	{ "Ice Desert",     16772499, 12, 12, 12899129, true, true,  0 }, // 9
	{ "Tundra",         5762041,  2, 3, 12899129, true,  false, 0 }, // 10
	{ "Hell",           16711680, 87, 87, 5169201, false, true,  0 }, // 11
	{ "Sky",            8421631,  2, 3, 5169201,  false, true,  0 }, // 12
};

const BetaBiome* BetaBiomeSource::s_table[64 * 64];
bool BetaBiomeSource::s_tableBuilt = false;

void BetaBiomeSource::buildTable() {
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
			s_table[i + j * 64] = lookupFloat((float)i / 63.0f, (float)j / 63.0f);
	s_tableBuilt = true;
}

// Mirrors kd.a(float,float) exactly (float arithmetic, fcmpg/fcmpl semantics:
// < uses fcmpg (NaN -> greater), > uses fcmpl (NaN -> less); inputs here are never NaN).
const BetaBiome* BetaBiomeSource::lookupFloat(float f0, float f1) {
	f1 = f1 * f0;
	if (f0 < 0.1f) return &BIOMES[BETA_TUNDRA];
	if (f1 < 0.2f) {
		if (f0 < 0.5f) return &BIOMES[BETA_TUNDRA];
		if (f0 < 0.95f) return &BIOMES[BETA_SAVANNA];
		return &BIOMES[BETA_DESERT];
	}
	if (f1 > 0.5f && f0 < 0.7f) return &BIOMES[BETA_SWAMPLAND];
	if (f0 < 0.5f) return &BIOMES[BETA_TAIGA];
	if (f0 < 0.97f) {
		if (f1 < 0.35f) return &BIOMES[BETA_SHRUBLAND];
		return &BIOMES[BETA_FOREST];
	}
	if (f1 < 0.45f) return &BIOMES[BETA_PLAINS];
	if (f1 < 0.9f) return &BIOMES[BETA_SEASONAL];
	return &BIOMES[BETA_RAINFOREST];
}

// Mirrors kd.a(double,double): 64x64 lookup table.
const BetaBiome* BetaBiomeSource::lookup(double temp, double humid) {
	if (!s_tableBuilt) buildTable();
	int i = (int)(temp * 63.0);
	int j = (int)(humid * 63.0);
	if (i < 0) i = 0; if (i > 63) i = 63;
	if (j < 0) j = 0; if (j > 63) j = 63;
	return s_table[i + j * 64];
}

BetaBiomeSource::BetaBiomeSource(int64_t seed) {
	JavaRandom rT(seed * 9871L);
	m_tempNoise = BetaOctaves2D(rT, 4);
	JavaRandom rH(seed * 39811L);
	m_humidNoise = BetaOctaves2D(rH, 4);
	JavaRandom rN(seed * 543321L);
	m_extraNoise = BetaOctaves2D(rN, 2);
}

double BetaBiomeSource::getTemperature(int x, int z) {
	std::vector<double> t;
	m_tempNoise.fill(t, (double)x, (double)z, 1, 1,
		0.02500000037252903, 0.02500000037252903, 0.5);
	return t[0];
}

double* BetaBiomeSource::getTemperatureBlock(std::vector<double>& out, int x, int z, int w, int h) {
	size_t need = (size_t)w * (size_t)h;
	if (out.size() < need) out.assign(need, 0.0);
	m_tempNoise.fill(out, (double)x, (double)z, w, h,
		0.02500000037252903, 0.02500000037252903, 0.25);
	m_extraNoise.fill(m_noise, (double)x, (double)z, w, h, 0.25, 0.25, 0.5882352941176471);
	size_t idx = 0;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			double d9 = m_noise[idx] * 1.1 + 0.5;
			double d11 = 0.01;
			double d13 = 1.0 - d11;
			double d15 = (out[idx] * 0.15 + 0.7) * d13 + d9 * d11;
			d15 = 1.0 - (1.0 - d15) * (1.0 - d15);
			if (d15 < 0.0) d15 = 0.0;
			if (d15 > 1.0) d15 = 1.0;
			out[idx] = d15;
			idx++;
		}
	}
	return &out[0];
}

const BetaBiome* BetaBiomeSource::getBiome(int x, int z) {
	std::vector<const BetaBiome*> b;
	getBiomeBlock(b, x, z, 1, 1);
	return b[0];
}

const BetaBiome** BetaBiomeSource::getBiomeBlock(std::vector<const BetaBiome*>& out, int x, int z, int w, int h) {
	size_t need = (size_t)w * (size_t)h;
	if (out.size() < need) out.assign(need, NULL);
	// NOTE: mirrors the original quirk — both noise size args are w, not (w,h).
	m_tempNoise.fill(temperatures, (double)x, (double)z, w, w,
		0.02500000037252903, 0.02500000037252903, 0.25);
	m_humidNoise.fill(humidities, (double)x, (double)z, w, w,
		0.05000000074505806, 0.05000000074505806, 0.3333333333333333);
	m_extraNoise.fill(m_noise, (double)x, (double)z, w, w,
		0.25, 0.25, 0.5882352941176471);
	// Guard: original reads w*h entries from w*w arrays (callers always use
	// square regions, but keep memory safe anyway).
	size_t need2 = (size_t)w * (size_t)w;
	size_t need3 = (size_t)w * (size_t)h;
	size_t big = need2 > need3 ? need2 : need3;
	if (temperatures.size() < big) temperatures.assign(big, 0.0);
	if (humidities.size() < big) humidities.assign(big, 0.0);
	if (m_noise.size() < big) m_noise.assign(big, 0.0);
	size_t idx = 0;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			double d9 = m_noise[idx] * 1.1 + 0.5;
			double d11 = 0.01;
			double d13 = 1.0 - d11;
			double d15 = (temperatures[idx] * 0.15 + 0.7) * d13 + d9 * d11;
			d11 = 0.002;
			d13 = 1.0 - d11;
			double d17 = (humidities[idx] * 0.15 + 0.5) * d13 + d9 * d11;
			d15 = 1.0 - (1.0 - d15) * (1.0 - d15);
			if (d15 < 0.0) d15 = 0.0;
			if (d15 > 1.0) d15 = 1.0;
			if (d17 < 0.0) d17 = 0.0;
			if (d17 > 1.0) d17 = 1.0;
			temperatures[idx] = d15;
			humidities[idx] = d17;
			out[idx] = lookup(d15, d17);
			idx++;
		}
	}
	return &out[0];
}
