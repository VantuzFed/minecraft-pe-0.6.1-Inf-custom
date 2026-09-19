#ifndef BETA173_CAVES_H__
#define BETA173_CAVES_H__

// Port of MapGenBase (fv) + MapGenCaves (sq) from Beta 1.7.3, transcribed
// from the original jar. Carves directly into the chunk byte buffer
// (layout (x*16+z)*128+y, x/z 0..15, y 0..127). Level-free.

#include <stdint.h>
#include "../../../util/JavaRandom.h"

class Beta173Caves {
public:
	Beta173Caves() : m_range(8) {}

	// Mirrors fv.a(cl,fd,int,int,byte[]): seed derivation + neighbor walk.
	void generate(int chunkX, int chunkZ, int64_t worldSeed, unsigned char* blocks);

	// Test hook: seed the stream and run one tunnel node directly.
	void testNode(int64_t rngSeed, int cx, int cz, unsigned char* blocks,
		double x, double y, double z, float rad, float yaw, float pitch,
		int pos, int len, double yScale) {
		m_rand.setSeed(rngSeed);
		generateCaveNode(cx, cz, blocks, x, y, z, rad, yaw, pitch, pos, len, yScale);
	}

	// Test hook: seed the stream and run one neighbor dispatch directly.
	void testDispatch(int64_t rngSeed, int nx, int nz, int ccx, int ccz, unsigned char* blocks) {
		m_rand.setSeed(rngSeed);
		generateCaveChunk(nx, nz, ccx, ccz, blocks);
	}

private:
	void generateCaveChunk(int x, int z, int chunkX, int chunkZ, unsigned char* blocks);
	void generateCaveNode(int chunkX, int chunkZ, unsigned char* blocks,
		double x, double y, double z, float radius, float yaw, float pitch,
		int pos, int len, double yScale);
	void generateRoom(int chunkX, int chunkZ, unsigned char* blocks,
		double x, double y, double z);

	int m_range; // == 8
	JavaRandom m_rand;
};

#endif
