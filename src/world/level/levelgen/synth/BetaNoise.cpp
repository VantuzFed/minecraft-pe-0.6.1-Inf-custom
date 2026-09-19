#include "BetaNoise.h"

#include <cmath>

// ---------------------------------------------------------------------------
// BetaImprovedNoise (am)
// ---------------------------------------------------------------------------

BetaImprovedNoise::BetaImprovedNoise()
	: m_xOff(0.0), m_yOff(0.0), m_zOff(0.0) {
	for (int i = 0; i < 512; i++)
		m_perm[i] = 0;
}

BetaImprovedNoise::BetaImprovedNoise(JavaRandom& rand)
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

double BetaImprovedNoise::lerp(double t, double a, double b) {
	return a + t * (b - a);
}

double BetaImprovedNoise::grad2D(int hash, double x, double y) {
	int h = hash & 15;
	double gx = (double)(1 - ((h & 8) >> 3)) * x;
	double gy;
	if (h < 4)
		gy = 0.0;
	else if (h == 12 || h == 14)
		gy = x;
	else
		gy = y;
	double t1 = ((h & 1) == 0) ? gx : -gx;
	double t2 = ((h & 2) == 0) ? gy : -gy;
	return t1 + t2;
}

double BetaImprovedNoise::grad3D(int hash, double x, double y, double z) {
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

double BetaImprovedNoise::eval3D(double x, double y, double z) const {
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

void BetaImprovedNoise::fill(double* out, double x0, double y0, double z0,
	int xs, int ys, int zs,
	double xScale, double yScale, double zScale, double freq) const {

	if (ys == 1) {
		int h0 = 0, h1 = 0, h2 = 0, h3 = 0;
		double l0 = 0.0, l1 = 0.0;
		int outIdx = 0;
		double weight = 1.0 / freq;
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
				h0 = m_perm[xm] + 0;
				h1 = m_perm[h0] + zm;
				h2 = m_perm[xm + 1] + 0;
				h3 = m_perm[h2] + zm;
				l0 = lerp(xFade,
					grad2D(m_perm[h1], xFrac, zFrac),
					grad3D(m_perm[h3], xFrac - 1.0, 0.0, zFrac));
				l1 = lerp(xFade,
					grad3D(m_perm[h1 + 1], xFrac, 0.0, zFrac - 1.0),
					grad3D(m_perm[h3 + 1], xFrac - 1.0, 0.0, zFrac - 1.0));
				double v = lerp(zFade, l0, l1);
				out[outIdx] = out[outIdx] + v * weight;
				outIdx++;
			}
		}
		return;
	}

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
// BetaOctaves (uf)
// ---------------------------------------------------------------------------

BetaOctaves::BetaOctaves() {
}

BetaOctaves::BetaOctaves(JavaRandom& rand, int octaves) {
	m_gens.reserve(octaves);
	for (int i = 0; i < octaves; i++)
		m_gens.push_back(BetaImprovedNoise(rand));
}

double BetaOctaves::sample2D(double x, double y) const {
	double sum = 0.0;
	double freq = 1.0;
	for (size_t i = 0; i < m_gens.size(); i++) {
		sum = sum + m_gens[i].eval2D(x * freq, y * freq) / freq;
		freq = freq / 2.0;
	}
	return sum;
}

void BetaOctaves::fill(std::vector<double>& out, double x, double y, double z,
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

// ---------------------------------------------------------------------------
// BetaNoise2D (cc)
// ---------------------------------------------------------------------------

const int BetaNoise2D::GRAD[12][3] = {
	{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0},
	{1, 0, 1}, {-1, 0, 1}, {1, 0, -1}, {-1, 0, -1},
	{0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}
};
const double BetaNoise2D::F2 = 0.5 * (std::sqrt(3.0) - 1.0);
const double BetaNoise2D::G2 = (3.0 - std::sqrt(3.0)) / 6.0;

BetaNoise2D::BetaNoise2D()
	: m_xOff(0.0), m_yOff(0.0) {
	for (int i = 0; i < 512; i++)
		m_perm[i] = 0;
}

BetaNoise2D::BetaNoise2D(JavaRandom& rand)
	: m_xOff(rand.nextDouble() * 256.0),
	  m_yOff(rand.nextDouble() * 256.0) {
	// NOTE: original calls nextDouble() a third time for field c (unused by bulk);
	// consume it to keep RNG streams identical.
	rand.nextDouble();
	for (int i = 0; i < 512; i++)
		m_perm[i] = 0;
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

void BetaNoise2D::fill(double* out, double x0, double z0, int xs, int zs,
	double xScale, double zScale, double amp) const {
	int outIdx = 0;
	for (int ix = 0; ix < xs; ix++) {
		double X = (x0 + (double)ix) * xScale + m_xOff;
		for (int iz = 0; iz < zs; iz++) {
			double Z = (z0 + (double)iz) * zScale + m_yOff;
			double s = (X + Z) * F2;
			int i = fastFloor(X + s);
			int j = fastFloor(Z + s);
			double t = (double)(i + j) * G2;
			double x0c = X - ((double)i - t);
			double z0c = Z - ((double)j - t);
			int i1, j1;
			if (x0c > z0c) { i1 = 1; j1 = 0; }
			else { i1 = 0; j1 = 1; }
			double x1 = x0c - (double)i1 + G2;
			double z1 = z0c - (double)j1 + G2;
			double x2 = x0c - 1.0 + 2.0 * G2;
			double z2 = z0c - 1.0 + 2.0 * G2;
			int ii = i & 255;
			int jj = j & 255;
			double n0, n1, n2;
			double t0 = 0.5 - x0c * x0c - z0c * z0c;
			if (t0 < 0.0)
				n0 = 0.0;
			else {
				t0 *= t0;
				n0 = t0 * t0 * dot(GRAD[m_perm[ii + m_perm[jj]] % 12], x0c, z0c);
			}
			double t1 = 0.5 - x1 * x1 - z1 * z1;
			if (t1 < 0.0)
				n1 = 0.0;
			else {
				t1 *= t1;
				n1 = t1 * t1 * dot(GRAD[m_perm[ii + i1 + m_perm[jj + j1]] % 12], x1, z1);
			}
			double t2 = 0.5 - x2 * x2 - z2 * z2;
			if (t2 < 0.0)
				n2 = 0.0;
			else {
				t2 *= t2;
				n2 = t2 * t2 * dot(GRAD[m_perm[ii + 1 + m_perm[jj + 1]] % 12], x2, z2);
			}
			out[outIdx] = out[outIdx] + 70.0 * (n0 + n1 + n2) * amp;
			outIdx++;
		}
	}
}

// ---------------------------------------------------------------------------
// BetaOctaves2D (ug)
// ---------------------------------------------------------------------------

BetaOctaves2D::BetaOctaves2D() {
}

BetaOctaves2D::BetaOctaves2D(JavaRandom& rand, int octaves) {
	m_gens.reserve(octaves);
	for (int i = 0; i < octaves; i++)
		m_gens.push_back(BetaNoise2D(rand));
}

void BetaOctaves2D::fill(std::vector<double>& out, double x, double z, int xs, int zs,
	double xScale, double zScale, double freqMult, double divMult) const {
	xScale = xScale / 1.5;
	zScale = zScale / 1.5;
	size_t need = (size_t)xs * (size_t)zs;
	if (out.size() < need)
		out.assign(need, 0.0);
	else {
		out.assign(out.size(), 0.0);
	}
	double divisor = 1.0;
	double freq = 1.0;
	for (size_t i = 0; i < m_gens.size(); i++) {
		m_gens[i].fill(&out[0], x, z, xs, zs,
			xScale * freq, zScale * freq, 0.55 / divisor);
		freq = freq * freqMult;
		divisor = divisor * divMult;
	}
}
