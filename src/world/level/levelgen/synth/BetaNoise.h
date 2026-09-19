#ifndef BETA_NOISE_H__
#define BETA_NOISE_H__

// Bit-compatible port of Minecraft Beta 1.7.3 noise classes, transcribed
// from the original jar (javap -c -p):
//   am -> BetaImprovedNoise  (single improved-Perlin octave)
//   uf -> BetaOctaves        (3D octave stack over BetaImprovedNoise)
//   cc -> BetaNoise2D        (2D simplex noise)
//   ug -> BetaOctaves2D      (2D octave stack over BetaNoise2D)
//
// All randomness comes from JavaRandom so identical seeds give identical
// output to the original game.

#include <vector>
#include "../../../../util/JavaRandom.h"

class BetaImprovedNoise {
public:
	BetaImprovedNoise();
	explicit BetaImprovedNoise(JavaRandom& rand);

	static double lerp(double t, double a, double b);
	static double grad2D(int hash, double x, double y);
	static double grad3D(int hash, double x, double y, double z);
	static double fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

	double eval3D(double x, double y, double z) const;
	double eval2D(double x, double y) const { return eval3D(x, y, 0.0); }

	// Adds weighted noise into out[] (out must hold xs*ys*zs entries).
	// Mirrors am.a(double[],double,double,double,int,int,int,double,double,double,double)
	// including the ySize==1 2D-slice path and the Ym caching quirk.
	void fill(double* out, double x0, double y0, double z0,
		int xs, int ys, int zs,
		double xScale, double yScale, double zScale, double freq) const;

private:
	int m_perm[512];
	double m_xOff, m_yOff, m_zOff;
};

class BetaOctaves {
public:
	BetaOctaves();
	BetaOctaves(JavaRandom& rand, int octaves);

	double sample2D(double x, double y) const;

	// Mirrors uf.a(double[],double,double,double,int,int,int,double,double,double).
	// out is resized to xs*ys*zs and zero-filled first.
	void fill(std::vector<double>& out, double x, double y, double z,
		int xs, int ys, int zs,
		double xScale, double yScale, double zScale) const;

	// Heightmap wrapper: y=10.0, ySize=1, yScale=1.0
	void fill2D(std::vector<double>& out, double x, double z,
		int xs, int zs, double xScale, double zScale) const {
		fill(out, x, 10.0, z, xs, 1, zs, xScale, 1.0, zScale);
	}

private:
	std::vector<BetaImprovedNoise> m_gens;
};

class BetaNoise2D {
public:
	BetaNoise2D();
	explicit BetaNoise2D(JavaRandom& rand);

	// Mirrors cc.a(double[],double,double,int,int,double,double,double).
	// out must hold xs*zs entries; values are ADDED ( caller zeroes first).
	void fill(double* out, double x0, double z0, int xs, int zs,
		double xScale, double zScale, double amp) const;

private:
	static int fastFloor(double d) { return d > 0.0 ? (int)d : (int)d - 1; }
	static double dot(const int* g, double x, double y) { return (double)g[0] * x + (double)g[1] * y; }

	static const int GRAD[12][3];
	static const double F2;
	static const double G2;

	int m_perm[512];
	double m_xOff, m_yOff;
};

class BetaOctaves2D {
public:
	BetaOctaves2D();
	BetaOctaves2D(JavaRandom& rand, int octaves);

	// Mirrors ug 7-arg (divMult defaults to 0.5).
	void fill(std::vector<double>& out, double x, double z, int xs, int zs,
		double xScale, double zScale, double extra) const {
		fill(out, x, z, xs, zs, xScale, zScale, extra, 0.5);
	}
	// Mirrors ug 8-arg. out is resized if smaller than xs*zs, else zero-filled.
	void fill(std::vector<double>& out, double x, double z, int xs, int zs,
		double xScale, double zScale, double freqMult, double divMult) const;

private:
	std::vector<BetaNoise2D> m_gens;
};

#endif
