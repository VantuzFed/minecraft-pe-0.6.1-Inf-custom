#include "Alpha112Noise.h"

#include <cstddef>

// ---------------------------------------------------------------------------
// Alpha112Perlin (v)
// ---------------------------------------------------------------------------

Alpha112Perlin::Alpha112Perlin()
	: m_xOff(0.0), m_yOff(0.0), m_zOff(0.0) {
	for (int i = 0; i < 512; i++)
		m_perm[i] = 0;
}

Alpha112Perlin::Alpha112Perlin(JavaRandom& rand)
	: m_xOff(rand.nextDouble() * 256.0),
	  m_yOff(rand.nextDouble() * 256.0),
	  m_zOff(rand.nextDouble() * 256.0) {
	for (int i = 0; i < 256; i++)
		m_perm[i] = i;
	for (int i = 0; i < 256; i++) {
		int j = rand.nextInt(256 - i) + i;
		int tmp = m_perm[i];
		m_perm[i] = m_perm[j];
		m_perm[j] = tmp;
		m_perm[i + 256] = m_perm[i];
	}
}

double Alpha112Perlin::lerp(double t, double a, double b) {
	return a + t * (b - a);
}

double Alpha112Perlin::grad3D(int hash, double x, double y, double z) {
	int h = hash & 15;
	double u = (h < 8) ? x : y;
	double v;
	if (h < 4)
		v = y;
	else if (h == 12 || h == 14)
		v = x;
	else
		v = z;
	double t1 = ((h & 1) == 0) ? u : -u;
	double t2 = ((h & 2) == 0) ? v : -v;
	return t1 + t2;
}

double Alpha112Perlin::eval3D(double x, double y, double z) const {
	double xf = x + m_xOff;
	double yf = y + m_yOff;
	double zf = z + m_zOff;
	int xi = (int)xf;
	int yi = (int)yf;
	int zi = (int)zf;
	if (xf < (double)xi) xi--;
	if (yf < (double)yi) yi--;
	if (zf < (double)zi) zi--;
	int xm = xi & 255;
	int ym = yi & 255;
	int zm = zi & 255;
	xf -= (double)xi;
	yf -= (double)yi;
	zf -= (double)zi;
	double u = fade(xf);
	double v = fade(yf);
	double w = fade(zf);
	int A = m_perm[xm] + ym;
	int AA = m_perm[A] + zm;
	int AB = m_perm[A + 1] + zm;
	int B = m_perm[xm + 1] + ym;
	int BA = m_perm[B] + zm;
	int BB = m_perm[B + 1] + zm;
	double x00 = lerp(u, grad3D(m_perm[AA], xf, yf, zf), grad3D(m_perm[BA], xf - 1.0, yf, zf));
	double x10 = lerp(u, grad3D(m_perm[AB], xf, yf - 1.0, zf), grad3D(m_perm[BB], xf - 1.0, yf - 1.0, zf));
	double xy0 = lerp(v, x00, x10);
	double x01 = lerp(u, grad3D(m_perm[AA + 1], xf, yf, zf - 1.0), grad3D(m_perm[BA + 1], xf - 1.0, yf, zf - 1.0));
	double x11 = lerp(u, grad3D(m_perm[AB + 1], xf, yf - 1.0, zf - 1.0), grad3D(m_perm[BB + 1], xf - 1.0, yf - 1.0, zf - 1.0));
	double xy1 = lerp(v, x01, x11);
	return lerp(w, xy0, xy1);
}

void Alpha112Perlin::fill(double* out, double x0, double y0, double z0,
	int xs, int ys, int zs,
	double xScale, double yScale, double zScale, double freq) const {

	int outIdx = 0;
	double weight = 1.0 / freq;
	int cachedY = -1;
	int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
	double x00 = 0.0, x10 = 0.0, x01 = 0.0, x11 = 0.0;
	for (int ix = 0; ix < xs; ix++) {
		double X = (x0 + (double)ix) * xScale + m_xOff;
		int xi = (int)X;
		if (X < (double)xi) xi--;
		int xm = xi & 255;
		double xFrac = X - (double)xi;
		double xFade = fade(xFrac);
		for (int iz = 0; iz < zs; iz++) {
			double Z = (z0 + (double)iz) * zScale + m_zOff;
			int zi = (int)Z;
			if (Z < (double)zi) zi--;
			int zm = zi & 255;
			double zFrac = Z - (double)zi;
			double zFade = fade(zFrac);
			for (int iy = 0; iy < ys; iy++) {
				double Y = (y0 + (double)iy) * yScale + m_yOff;
				int yi = (int)Y;
				if (Y < (double)yi) yi--;
				int ym = yi & 255;
				double yFrac = Y - (double)yi;
				double yFade = fade(yFrac);
				if (iy == 0 || ym != cachedY) {
					cachedY = ym;
					A = m_perm[xm] + ym;
					AA = m_perm[A] + zm;
					AB = m_perm[A + 1] + zm;
					B = m_perm[xm + 1] + ym;
					BA = m_perm[B] + zm;
					BB = m_perm[B + 1] + zm;
					x00 = lerp(xFade, grad3D(m_perm[AA], xFrac, yFrac, zFrac), grad3D(m_perm[BA], xFrac - 1.0, yFrac, zFrac));
					x10 = lerp(xFade, grad3D(m_perm[AB], xFrac, yFrac - 1.0, zFrac), grad3D(m_perm[BB], xFrac - 1.0, yFrac - 1.0, zFrac));
					x01 = lerp(xFade, grad3D(m_perm[AA + 1], xFrac, yFrac, zFrac - 1.0), grad3D(m_perm[BA + 1], xFrac - 1.0, yFrac, zFrac - 1.0));
					x11 = lerp(xFade, grad3D(m_perm[AB + 1], xFrac, yFrac - 1.0, zFrac - 1.0), grad3D(m_perm[BB + 1], xFrac - 1.0, yFrac - 1.0, zFrac - 1.0));
				}
				double y0v = lerp(yFade, x00, x10);
				double y1v = lerp(yFade, x01, x11);
				double f = lerp(zFade, y0v, y1v);
				out[outIdx] = out[outIdx] + f * weight;
				outIdx++;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Alpha112Octaves (lp)
// ---------------------------------------------------------------------------

Alpha112Octaves::Alpha112Octaves() {
}

Alpha112Octaves::Alpha112Octaves(JavaRandom& rand, int octaves) {
	m_gens.reserve(octaves);
	for (int i = 0; i < octaves; i++)
		m_gens.push_back(Alpha112Perlin(rand));
}

double Alpha112Octaves::sample2D(double x, double y) const {
	double sum = 0.0;
	double freq = 1.0;
	for (size_t i = 0; i < m_gens.size(); i++) {
		sum = sum + m_gens[i].eval2D(x * freq, y * freq) / freq;
		freq = freq / 2.0;
	}
	return sum;
}

void Alpha112Octaves::fill(std::vector<double>& out, double x, double y, double z,
	int xs, int ys, int zs,
	double xScale, double yScale, double zScale) const {
	out.assign((size_t)xs * (size_t)ys * (size_t)zs, 0.0);
	double freq = 1.0;
	for (size_t i = 0; i < m_gens.size(); i++) {
		m_gens[i].fill(&out[0], x, y, z, xs, ys, zs,
			xScale * freq, yScale * freq, zScale * freq, freq);
		freq = freq / 2.0;
	}
}
