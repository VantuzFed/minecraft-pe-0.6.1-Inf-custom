#include "Beta173TerrainGen.h"

#include "Beta173Features.h"

using namespace Beta173Features;

Beta173TerrainGen::Beta173TerrainGen(int64_t seed)
	: m_rand(seed), m_biomes(seed) {
	// Constructor order mirrors yf(fd,long): k,l,m,n,o,a(mob),b,c sharing one RNG.
	JavaRandom r(seed);
	m_noiseGen1 = BetaOctaves(r, 16);
	m_noiseGen2 = BetaOctaves(r, 16);
	m_noiseGen3 = BetaOctaves(r, 8);
	m_noiseGen4 = BetaOctaves(r, 4);
	m_noiseGen5 = BetaOctaves(r, 4);
	m_mobNoise = BetaOctaves(r, 10);
	m_noiseGen6 = BetaOctaves(r, 16);
	m_noiseC = BetaOctaves(r, 8);
	m_sand.assign(256, 0.0);
	m_gravel.assign(256, 0.0);
	m_stoneNoise.assign(256, 0.0);
}

void Beta173TerrainGen::generateChunk(int chunkX, int chunkZ, unsigned char* blocks) {
	// Mirrors yf.b(int,int) chunk factory seeding (wrapping long math).
	uint64_t s = (uint64_t)(int64_t)chunkX * (uint64_t)341873128712LL
		+ (uint64_t)(int64_t)chunkZ * (uint64_t)132897987541LL;
	m_rand.setSeed((int64_t)s);
	m_biomes.getBiomeBlock(m_biomeBlock, chunkX * 16, chunkZ * 16, 16, 16);
	generateTerrain(chunkX, chunkZ, blocks);
	replaceBlocksForBiome(chunkX, chunkZ, blocks);
}

void Beta173TerrainGen::generateTerrain(int chunkX, int chunkZ, unsigned char* blocks) {
	const int cell = 4;
	const int sea = 64;
	const int xs = cell + 1; // 5
	const int ys = 17;
	const int zs = cell + 1; // 5
	std::vector<double> dens;
	initializeNoiseField(dens, chunkX * cell, 0, chunkZ * cell, xs, ys, zs);

	const double* temps = m_biomes.temperatures.empty() ? NULL : &m_biomes.temperatures[0];
	for (int cx = 0; cx < cell; cx++) {
		for (int cz = 0; cz < cell; cz++) {
			for (int yCell = 0; yCell < 16; yCell++) {
				double v000 = dens[((cx + 0) * zs + (cz + 0)) * ys + (yCell + 0)];
				double v001 = dens[((cx + 0) * zs + (cz + 1)) * ys + (yCell + 0)];
				double v100 = dens[((cx + 1) * zs + (cz + 0)) * ys + (yCell + 0)];
				double v101 = dens[((cx + 1) * zs + (cz + 1)) * ys + (yCell + 0)];
				double d000 = (dens[((cx + 0) * zs + (cz + 0)) * ys + (yCell + 1)] - v000) * 0.125;
				double d001 = (dens[((cx + 0) * zs + (cz + 1)) * ys + (yCell + 1)] - v001) * 0.125;
				double d100 = (dens[((cx + 1) * zs + (cz + 0)) * ys + (yCell + 1)] - v100) * 0.125;
				double d101 = (dens[((cx + 1) * zs + (cz + 1)) * ys + (yCell + 1)] - v101) * 0.125;
				for (int subY = 0; subY < 8; subY++) {
					double xa = v000;
					double xb = v001;
					double dxa = (v100 - v000) * 0.25;
					double dxb = (v101 - v001) * 0.25;
					for (int subX = 0; subX < 4; subX++) {
						// NOTE: subZ is NOT part of the initial index; it advances
						// via byteIdx += 128 per subZ iteration (matches original).
						int byteIdx = ((subX + cx * 4) << 11) | ((cz * 4) << 7) | (yCell * 8 + subY);
						double za = xa;
						double dza = (xb - xa) * 0.25;
						for (int subZ = 0; subZ < 4; subZ++) {
							double temper = temps ? temps[(cx * 4 + subX) * 16 + (cz * 4 + subZ)] : 1.0;
							int id = 0;
							int y = yCell * 8 + subY;
							if (y < sea) {
								if (temper < 0.5) {
									if (y >= sea - 1)
										id = BB_ICE;
									else
										id = BB_WATER;
								} else {
									id = BB_WATER;
								}
							}
							if (za > 0.0)
								id = BB_STONE;
							blocks[byteIdx] = (unsigned char)id;
							byteIdx += 128;
							za += dza;
						}
						xa += dxa;
						xb += dxb;
					}
					v000 += d000; v001 += d001; v100 += d100; v101 += d101;
				}
			}
		}
	}
}

void Beta173TerrainGen::initializeNoiseField(std::vector<double>& out,
	int x0, int y0, int z0, int xs, int ys, int zs) {
	out.assign((size_t)xs * (size_t)ys * (size_t)zs, 0.0);
	const double d0 = 684.412;
	const double d1 = 684.412;

	m_mobNoise.fill2D(m_g, x0, z0, xs, zs, 1.121, 1.121);
	m_noiseGen6.fill2D(m_h, x0, z0, xs, zs, 200.0, 200.0);
	m_noiseGen3.fill(m_d, (double)x0, (double)y0, (double)z0, xs, ys, zs,
		d0 / 80.0, d1 / 160.0, d0 / 80.0);
	m_noiseGen1.fill(m_e, (double)x0, (double)y0, (double)z0, xs, ys, zs, d0, d1, d0);
	m_noiseGen2.fill(m_f, (double)x0, (double)y0, (double)z0, xs, ys, zs, d0, d1, d0);

	const double* tempArr = m_biomes.temperatures.empty() ? NULL : &m_biomes.temperatures[0];
	const double* rainArr = m_biomes.humidities.empty() ? NULL : &m_biomes.humidities[0];
	size_t idx3D = 0;
	size_t idx2D = 0;
	int step = 16 / xs;
	for (int xi = 0; xi < xs; xi++) {
		int xSample = xi * step + step / 2;
		for (int zi = 0; zi < zs; zi++) {
			int zSample = zi * step + step / 2;
			double temp = tempArr ? tempArr[xSample * 16 + zSample] : 1.0;
			double humidXtemp = (rainArr ? rainArr[xSample * 16 + zSample] : 0.5) * temp;
			double fall = 1.0 - humidXtemp;
			fall = fall * fall;
			fall = fall * fall;
			fall = 1.0 - fall;

			double depth = (m_g[idx2D] + 256.0) / 512.0;
			depth = depth * fall;
			if (depth > 1.0) depth = 1.0;

			double mount = m_h[idx2D] / 8000.0;
			if (mount < 0.0)
				mount = -mount * 0.3;
			mount = mount * 3.0 - 2.0;
			if (mount < 0.0) {
				mount = mount / 2.0;
				if (mount < -1.0) mount = -1.0;
				mount = mount / 1.4;
				mount = mount / 2.0;
				depth = 0.0;
			} else {
				if (mount > 1.0) mount = 1.0;
				mount = mount / 8.0;
			}
			if (depth < 0.0) depth = 0.0;
			depth = depth + 0.5;
			mount = mount * (double)ys / 16.0;
			double base = (double)ys / 2.0 + mount * 4.0;
			idx2D++;

			for (int y = 0; y < ys; y++) {
				double dens = 0.0;
				double yFall = ((double)y - base) * 12.0 / depth;
				if (yFall < 0.0) yFall = yFall * 4.0;
				double low = m_e[idx3D] / 512.0;
				double upp = m_f[idx3D] / 512.0;
				double t = (m_d[idx3D] / 10.0 + 1.0) / 2.0;
				if (t < 0.0) dens = low;
				else if (t > 1.0) dens = upp;
				else dens = low + (upp - low) * t;
				dens = dens - yFall;
				if (y > ys - 4) {
					float f2 = (float)(y - (ys - 4)) / 3.0f;
					dens = dens * (1.0 - (double)f2) + (-10.0) * (double)f2;
				}
				out[idx3D] = dens;
				idx3D++;
			}
		}
	}
}

// Mirrors yf.replaceBlocksForBiome(int,int,byte[],kd[]).
void Beta173TerrainGen::replaceBlocksForBiome(int chunkX, int chunkZ, unsigned char* blocks) {
	const int sea = 64;
	const double s = 0.03125;
	// NOTE: sand/stone buffers use dims (16,16,1) — the 3D bulk path with
	// Y varying and Z fixed (original quirk, verified in bytecode).
	// Buffer layout is [ix*16+iy]; read as [z + x*16] below.
	// NOTE 2: the y/z origins are (chunkZ*16, 0.0), NOT (0.0, chunkZ*16)
	// (verified in bytecode: y=iload_2*16, z=dconst_0).
	m_noiseGen4.fill(m_sand, (double)(chunkX * 16), (double)(chunkZ * 16), 0.0,
		16, 16, 1, s, s, 1.0);
	m_noiseGen4.fill(m_gravel, (double)(chunkX * 16), 109.0134, (double)(chunkZ * 16),
		16, 1, 16, s, 1.0, s);
	m_noiseGen5.fill(m_stoneNoise, (double)(chunkX * 16), (double)(chunkZ * 16), 0.0,
		16, 16, 1, s * 2.0, s * 2.0, s * 2.0);

	// Column visit order matters for the RNG stream: outer Z, inner X (original).
	for (int z = 0; z < 16; z++) {
		for (int x = 0; x < 16; x++) {
			// NOTE: surface arrays are stored [X*16+Z] (outer X); this loop
			// reads them as [z + x*16], matching the original's access order.
			const BetaBiome* biome = m_biomeBlock[z + x * 16];
			if (!biome) biome = &BetaBiomeSource::BIOMES[BETA_PLAINS];
			int sandFlag = (m_sand[z + x * 16] + m_rand.nextDouble() * 0.2 > 0.0) ? 1 : 0;
			int gravFlag = (m_gravel[z + x * 16] + m_rand.nextDouble() * 0.2 > 3.0) ? 1 : 0;
			int depthNoise = (int)(m_stoneNoise[z + x * 16] / 3.0 + 3.0 + m_rand.nextDouble() * 0.25);
			int depth = -1;
			int top = biome->topId;
			int filler = biome->fillerId;
			for (int y = 127; y >= 0; y--) {
				int idx = (x * 16 + z) * 128 + y;
				if (y <= 0 + m_rand.nextInt(5)) {
					blocks[idx] = (unsigned char)BB_BEDROCK;
					continue;
				}
				int cur = blocks[idx];
				if (cur == 0) {
					depth = -1;
					continue;
				}
				if (cur != BB_STONE) continue;
				if (depth == -1) {
					if (depthNoise <= 0) {
						top = 0;
						filler = BB_STONE;
					} else if (y >= sea - 4 && y <= sea + 1) {
						top = biome->topId;
						filler = biome->fillerId;
						if (gravFlag != 0) { top = 0; filler = BB_GRAVEL; }
						if (sandFlag != 0) { top = BB_SAND; filler = BB_SAND; }
					}
					if (y < sea && top == 0)
						top = BB_WATER;
					depth = depthNoise;
					if (y >= sea - 1)
						blocks[idx] = (unsigned char)top;
					else
						blocks[idx] = (unsigned char)filler;
				} else if (depth > 0) {
					depth--;
					blocks[idx] = (unsigned char)filler;
					if (depth == 0 && filler == BB_SAND) {
						depth = m_rand.nextInt(4);
						filler = BB_SANDSTONE;
					}
				}
			}
		}
	}
}

