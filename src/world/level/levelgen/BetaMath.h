#ifndef BETA_MATH_H__
#define BETA_MATH_H__

// Port of beta 1.7.3 MathHelper (in): 65536-entry sine table.
// table[i] = (float)sin(i * PI * 2 / 65536)  [double math, then float]
// sin(f) = table[(int)(f * 10430.378f) & 65535]
// cos(f) = table[(int)(f * 10430.378f + 16384.0f) & 65535]

#include <cmath>

class BetaMath {
public:
	static const float* sinTable() {
		static float table[65536];
		static bool init = false;
		if (!init) {
			for (int i = 0; i < 65536; i++)
				table[i] = (float)::sin((double)i * 3.141592653589793 * 2.0 / 65536.0);
			init = true;
		}
		return table;
	}
	static float sin(float f) {
		return sinTable()[((int)(f * 10430.378f)) & 65535];
	}
	static float cos(float f) {
		return sinTable()[((int)(f * 10430.378f + 16384.0f)) & 65535];
	}
	static int floor(double d) {
		int i = (int)d;
		return (d < (double)i) ? i - 1 : i;
	}
	static int floor(float f) {
		int i = (int)f;
		return (f < (float)i) ? i - 1 : i;
	}
};

#endif
