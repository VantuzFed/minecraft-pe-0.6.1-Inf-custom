#include "Alpha112TerrainGen.h"

#include "Beta173Features.h"

using namespace Beta173Features;

Alpha112TerrainGen::Alpha112TerrainGen(int64_t seed)
	: m_rand(seed), m_snowCovered(false) {
	// Constructor order mirrors nw(cn,long): k,l,m,n,o,a,b,c sharing one RNG.
	JavaRandom r(seed);
	m_noiseK = Alpha112Octaves(r, 16);
	m_noiseL = Alpha112Octaves(r, 16);
	m_noiseM = Alpha112Octaves(r, 8);
	m_noiseN = Alpha112Octaves(r, 4);
	m_noiseO = Alpha112Octaves(r, 4);
	m_noiseA = Alpha112Octaves(r, 10);
	m_noiseB = Alpha112Octaves(r, 16);
	m_noiseC = Alpha112Octaves(r, 8);
	m_sand.assign(256, 0.0);
	m_gravel.assign(256, 0.0);
	m_stoneNoise.assign(256, 0.0);
}

void Alpha112TerrainGen::generateChunk(int chunkX, int chunkZ, unsigned char* blocks) {
	// Mirrors nw.b(int,int) chunk factory seeding (wrapping long math).
	uint64_t s = (uint64_t)(int64_t)chunkX * (uint64_t)341873128712LL
		+ (uint64_t)(int64_t)chunkZ * (uint64_t)132897987541LL;
	m_rand.setSeed((int64_t)s);
	generateTerrain(chunkX, chunkZ, blocks);
	replaceSurface(chunkX, chunkZ, blocks);
}

void Alpha112TerrainGen::generateTerrain(int chunkX, int chunkZ, unsigned char* blocks) {
	const int cell = 4;
	const int sea = 64;
	const int xs = cell + 1; // 5
	const int ys = 17;
	const int zs = cell + 1; // 5
	std::vector<double> dens;
	initializeNoiseField(dens, chunkX * cell, 0, chunkZ * cell, xs, ys, zs);

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
						int byteIdx = ((subX + cx * 4) << 11) | ((cz * 4) << 7) | (yCell * 8 + subY);
						double za = xa;
						double dza = (xb - xa) * 0.25;
						for (int subZ = 0; subZ < 4; subZ++) {
							int id = 0;
							int y = yCell * 8 + subY;
							if (y < sea) {
								if (m_snowCovered && y >= sea - 1)
									id = BB_ICE;
								else
									id = BB_WATER;
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

void Alpha112TerrainGen::initializeNoiseField(std::vector<double>& out,
	int x0, int y0, int z0, int xs, int ys, int zs) {
	out.assign((size_t)xs * (size_t)ys * (size_t)zs, 0.0);
	const double d0 = 684.412;
	const double d1 = 684.412;

	m_noiseA.fill(m_g, (double)x0, (double)y0, (double)z0, xs, 1, zs, 1.0, 0.0, 1.0);
	m_noiseB.fill(m_h, (double)x0, (double)y0, (double)z0, xs, 1, zs, 100.0, 0.0, 100.0);
	m_noiseM.fill(m_d, (double)x0, (double)y0, (double)z0, xs, ys, zs,
		d0 / 80.0, d1 / 160.0, d0 / 80.0);
	m_noiseK.fill(m_e, (double)x0, (double)y0, (double)z0, xs, ys, zs, d0, d1, d0);
	m_noiseL.fill(m_f, (double)x0, (double)y0, (double)z0, xs, ys, zs, d0, d1, d0);

	size_t idx3D = 0;
	size_t idx2D = 0;
	for (int xi = 0; xi < xs; xi++) {
		for (int zi = 0; zi < zs; zi++) {
			// No biome humidity falloff in Alpha: depth is raw.
			double depth = (m_g[idx2D] + 256.0) / 512.0;
			if (depth > 1.0) depth = 1.0;

			double mount = m_h[idx2D] / 8000.0;
			if (mount < 0.0)
				mount = -mount;
			mount = mount * 3.0 - 3.0;
			if (mount < 0.0) {
				mount = mount / 2.0;
				if (mount < -1.0) mount = -1.0;
				mount = mount / 1.4;
				mount = mount / 2.0;
				depth = 0.0;
			} else {
				if (mount > 1.0) mount = 1.0;
				mount = mount / 6.0;
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

// Mirrors nw.b(int,int,byte[]) surface builder.
void Alpha112TerrainGen::replaceSurface(int chunkX, int chunkZ, unsigned char* blocks) {
	const int sea = 64;
	const double s = 0.03125;
	// r: (x*16, z*16, 0) dims (16,16,1), read [x*16+z].
	// s(gravel): (z*16, 109.0134, x*16) dims (16,1,16) - NOTE swapped axes.
	// t: (x*16, z*16, 0) dims (16,16,1) at double scale.
	m_noiseN.fill(m_sand, (double)(chunkX * 16), (double)(chunkZ * 16), 0.0,
		16, 16, 1, s, s, 1.0);
	m_noiseN.fill(m_gravel, (double)(chunkZ * 16), 109.0134, (double)(chunkX * 16),
		16, 1, 16, s, 1.0, s);
	m_noiseO.fill(m_stoneNoise, (double)(chunkX * 16), (double)(chunkZ * 16), 0.0,
		16, 16, 1, s * 2.0, s * 2.0, s * 2.0);

	// Column visit order matters for the RNG stream: outer X, inner Z (original).
	for (int x = 0; x < 16; x++) {
		for (int z = 0; z < 16; z++) {
			int sandFlag = (m_sand[x + z * 16] + m_rand.nextDouble() * 0.2 > 0.0) ? 1 : 0;
			int gravFlag = (m_gravel[x + z * 16] + m_rand.nextDouble() * 0.2 > 3.0) ? 1 : 0;
			int depthNoise = (int)(m_stoneNoise[x + z * 16] / 3.0 + 3.0 + m_rand.nextDouble() * 0.25);
			int depth = -1;
			int top = BB_GRASS;
			int filler = BB_DIRT;
			for (int y = 127; y >= 0; y--) {
				int idx = (x * 16 + z) * 128 + y;
				if (y <= 0 + m_rand.nextInt(6) - 1) {
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
						top = BB_GRASS;
						filler = BB_DIRT;
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
					// NOTE: no sandstone transition in Alpha.
				}
			}
		}
	}
}
