#include "Beta173Caves.h"

#include "BetaMath.h"
#include <cmath>
#include <cstdio>

// ---------------------------------------------------------------------------
// Beta173Caves (fv + sq)
// ---------------------------------------------------------------------------
void Beta173Caves::generate(int chunkX, int chunkZ, int64_t worldSeed, unsigned char* blocks) {
	m_rand.setSeed(worldSeed);
	// NOTE: Java long arithmetic wraps on overflow; use unsigned math to match.
	uint64_t odd1 = (uint64_t)(m_rand.nextLong() / 2L * 2L + 1L);
	uint64_t odd2 = (uint64_t)(m_rand.nextLong() / 2L * 2L + 1L);
	for (int x = chunkX - m_range; x <= chunkX + m_range; x++) {
		for (int z = chunkZ - m_range; z <= chunkZ + m_range; z++) {
			uint64_t s = (uint64_t)(int64_t)x * odd1 + (uint64_t)(int64_t)z * odd2;
			m_rand.setSeed((int64_t)(s ^ (uint64_t)worldSeed));
			generateCaveChunk(x, z, chunkX, chunkZ, blocks);
		}
	}
}

void Beta173Caves::generateCaveChunk(int x, int z, int cx, int cz, unsigned char* blocks) {
	// NOTE: triple-nested nextInt with NO trailing +1 (verified in bytecode
	// and empirically: miscounting this shifts the whole RNG stream).
	int branches = m_rand.nextInt(m_rand.nextInt(m_rand.nextInt(40) + 1) + 1);
	if (m_rand.nextInt(15) != 0) branches = 0;
#ifdef BETA173_CAVE_DEBUG
	fprintf(stderr, "branches=%d\n", branches);
#endif
	for (int i = 0; i < branches; i++) {
		double px = (double)(x * 16 + m_rand.nextInt(16));
		double py = (double)m_rand.nextInt(m_rand.nextInt(120) + 8);
		double pz = (double)(z * 16 + m_rand.nextInt(16));
		int count = 1;
		if (m_rand.nextInt(4) == 0) {
			generateRoom(cx, cz, blocks, px, py, pz);
			count += m_rand.nextInt(4);
		}
#ifdef BETA173_CAVE_DEBUG
		fprintf(stderr, "branch %d: %f %f %f c=%d\n", i, px, py, pz, count);
#endif
		for (int j = 0; j < count; j++) {
			float yaw = m_rand.nextFloat() * 3.1415927f * 2.0f;
			float pitch = (m_rand.nextFloat() - 0.5f) * 2.0f / 8.0f;
			float rad = m_rand.nextFloat() * 2.0f + m_rand.nextFloat();
			generateCaveNode(cx, cz, blocks, px, py, pz, rad, yaw, pitch, 0, 0, 1.0);
		}
	}
}

void Beta173Caves::generateRoom(int chunkX, int chunkZ, unsigned char* blocks,
	double x, double y, double z) {
	generateCaveNode(chunkX, chunkZ, blocks, x, y, z,
		1.0f + m_rand.nextFloat() * 6.0f, 0.0f, 0.0f, -1, -1, 0.5);
}

void Beta173Caves::generateCaveNode(int chunkX, int chunkZ, unsigned char* blocks,
	double caveX, double caveY, double caveZ, float radius, float yaw, float pitch,
	int pos, int len, double yScale) {

	double centerX = (double)(chunkX * 16 + 8);
	double centerZ = (double)(chunkZ * 16 + 8);
	float yawDelta = 0.0f;
	float pitchDelta = 0.0f;
	JavaRandom r(m_rand.nextLong());
	if (len <= 0) {
		int range = m_range * 16 - 16;
		len = range - r.nextInt(range / 4);
	}
	bool isInitial = false;
	if (pos == -1) {
		pos = len / 2;
		isInitial = true;
	}
	int splitAt = r.nextInt(len / 2) + len / 4;
	bool makeRoom = (r.nextInt(6) == 0);
	while (pos < len) {
		double width = 1.5 + (double)(BetaMath::sin((float)pos * 3.1415927f / (float)len) * radius * 1.0f);
		double height = width * yScale;
		float cosPitch = BetaMath::cos(pitch);
		float sinPitch = BetaMath::sin(pitch);
		caveX += (double)(BetaMath::cos(yaw) * cosPitch);
		caveY += (double)sinPitch;
		caveZ += (double)(BetaMath::sin(yaw) * cosPitch);
		if (makeRoom) pitch *= 0.92f;
		else pitch *= 0.7f;
		pitch += pitchDelta * 0.1f;
		yaw += yawDelta * 0.1f;
		pitchDelta *= 0.9f;
		yawDelta *= 0.75f;
		pitchDelta += (r.nextFloat() - r.nextFloat()) * r.nextFloat() * 2.0f;
		yawDelta += (r.nextFloat() - r.nextFloat()) * r.nextFloat() * 4.0f;
		if (!isInitial && pos == splitAt && radius > 1.0f) {
			generateCaveNode(chunkX, chunkZ, blocks, caveX, caveY, caveZ,
				r.nextFloat() * 0.5f + 0.5f, yaw - 1.5707964f, pitch / 3.0f, pos, len, 1.0);
			generateCaveNode(chunkX, chunkZ, blocks, caveX, caveY, caveZ,
				r.nextFloat() * 0.5f + 0.5f, yaw + 1.5707964f, pitch / 3.0f, pos, len, 1.0);
			return;
		}
		if (!isInitial && r.nextInt(4) == 0) { pos++; continue; }
		double dx = caveX - centerX;
		double dz = caveZ - centerZ;
		double remaining = (double)(len - pos);
		double maxDist = (double)(radius + 2.0f + 16.0f);
		if (dx * dx + dz * dz - remaining * remaining > maxDist * maxDist) return;
		// NOTE: edge checks skip this step (pos++, continue), they do NOT
		// abort the tunnel (verified: javap offsets 508/527/546/568 -> 1207).
		if (caveX < centerX - 16.0 - width * 2.0) { pos++; continue; }
		if (caveZ < centerZ - 16.0 - width * 2.0) { pos++; continue; }
		if (caveX > centerX + 16.0 + width * 2.0) { pos++; continue; }
		if (caveZ > centerZ + 16.0 + width * 2.0) { pos++; continue; }
		int xMin = BetaMath::floor(caveX - width) - chunkX * 16 - 1;
		int xMax = BetaMath::floor(caveX + width) - chunkX * 16 + 1;
		int yMin = BetaMath::floor(caveY - height) - 1;
		int yMax = BetaMath::floor(caveY + height) + 1;
		int zMin = BetaMath::floor(caveZ - width) - chunkZ * 16 - 1;
		int zMax = BetaMath::floor(caveZ + width) - chunkZ * 16 + 1;
		if (xMin < 0) xMin = 0;
		if (xMax > 16) xMax = 16;
		if (yMin < 1) yMin = 1;
		if (yMax > 120) yMax = 120;
		if (zMin < 0) zMin = 0;
		if (zMax > 16) zMax = 16;
		bool hitWater = false;
		for (int lx = xMin; lx < xMax && !hitWater; lx++) {
			for (int lz = zMin; lz < zMax && !hitWater; lz++) {
				for (int ly = yMax + 1; ly >= yMin - 1; ly--) {
					if (ly < 0 || ly >= 128) continue;
					int idx = (lx * 16 + lz) * 128 + ly;
					int id = blocks[idx];
					if (id == 8 || id == 9) { hitWater = true; break; }
					if (ly == yMin - 1) { /* fall through */ }
					else if (lx != xMin && lx != xMax - 1 && lz != zMin && lz != zMax - 1) {
						ly = yMin;
					}
				}
			}
		}
		if (hitWater) { pos++; continue; }
		for (int lx = xMin; lx < xMax; lx++) {
			double nx = ((double)(lx + chunkX * 16) + 0.5 - caveX) / width;
			for (int lz = zMin; lz < zMax; lz++) {
				double nz = ((double)(lz + chunkZ * 16) + 0.5 - caveZ) / width;
				int idx = (lx * 16 + lz) * 128 + yMax;
				bool foundGrass = false;
				if (nx * nx + nz * nz < 1.0) {
					for (int ly = yMax - 1; ly >= yMin; ly--) {
						double ny = ((double)ly + 0.5 - caveY) / height;
						if (ny > -0.7 && nx * nx + ny * ny + nz * nz < 1.0) {
							int id = blocks[idx];
							if (id == 2) foundGrass = true;
							if (id == 1 || id == 3 || id == 2) {
								if (ly < 10)
									blocks[idx] = 10; // flowing lava (uu.D), verified in jar
								else {
									blocks[idx] = 0;
									if (foundGrass && blocks[idx - 1] == 3)
										blocks[idx - 1] = 2;
								}
							}
						}
						idx--;
					}
				}
			}
		}
		if (isInitial) return;
		pos++;
	}
}
