#ifndef ALPHA112_NOISE_H__
#define ALPHA112_NOISE_H__

// Alpha 1.1.2 octave/perlin noise, transcribed from the original jar:
//   v  -> Alpha112Perlin   (single improved-Perlin octave)
//   lp -> Alpha112Octaves  (3D octave stack over Alpha112Perlin)
//   bk -> base (empty)
//
// Same family as BetaImprovedNoise/BetaOctaves (perm-512, *256 offsets,
// same shuffle, fade, grad, octave halving, YM-cache) with one crucial
// difference: Alpha's bulk fill has NO ySize==1 2D-slice fast path —
// every call runs the full 3D evaluation (verified in a112_v.txt).
// Beta's slice produces different values, so the Beta classes cannot be
// reused here.
//
// All randomness comes from JavaRandom so identical seeds give identical
// output to the original game.

#include <vector>
#include "../../../../util/JavaRandom.h"

class Alpha112Perlin {
public:
	Alpha112Perlin();
	explicit Alpha112Perlin(JavaRandom& rand);

	static double lerp(double t, double a, double b);
	static double grad3D(int hash, double x, double y, double z);
	static double fade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }

	double eval3D(double x, double y, double z) const;
	double eval2D(double x, double y) const { return eval3D(x, y, 0.0); }

	// Adds weighted noise into out[] (out must hold xs*ys*zs entries).
	// Mirrors v.a(double[],double,double,double,int,int,int,double,double,double,double):
	// always the 3D path, including the Ym caching quirk.
	void fill(double* out, double x0, double y0, double z0,
		int xs, int ys, int zs,
		double xScale, double yScale, double zScale, double freq) const;

private:
	int m_perm[512];
	double m_xOff, m_yOff, m_zOff;
};

class Alpha112Octaves {
public:
	Alpha112Octaves();
	Alpha112Octaves(JavaRandom& rand, int octaves);

	double sample2D(double x, double y) const;

	// Mirrors lp.a(double[],double,double,double,int,int,int,double,double,double).
	// out is resized to xs*ys*zs and zero-filled first.
	void fill(std::vector<double>& out, double x, double y, double z,
		int xs, int ys, int zs,
		double xScale, double yScale, double zScale) const;

private:
	std::vector<Alpha112Perlin> m_gens;
};

#endif
