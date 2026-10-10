#ifndef NET_MINECRAFT_CLIENT_RENDERER_SHADER_Matrix4_H__
#define NET_MINECRAFT_CLIENT_RENDERER_SHADER_Matrix4_H__

#include <cstring>
#include <cmath>

struct Matrix4 {
	float m[16];

	Matrix4() {
		identity();
	}

	explicit Matrix4(const float* data) {
		if (data) {
			std::memcpy(m, data, 16 * sizeof(float));
		} else {
			identity();
		}
	}

	void identity() {
		std::memset(m, 0, 16 * sizeof(float));
		m[0] = 1.0f;
		m[5] = 1.0f;
		m[10] = 1.0f;
		m[15] = 1.0f;
	}

	Matrix4 multiply(const Matrix4& b) const {
		Matrix4 res;
		for (int col = 0; col < 4; ++col) {
			for (int row = 0; row < 4; ++row) {
				float sum = 0.0f;
				for (int k = 0; k < 4; ++k) {
					sum += m[k * 4 + row] * b.m[col * 4 + k];
				}
				res.m[col * 4 + row] = sum;
			}
		}
		return res;
	}

	Matrix4 transpose() const {
		Matrix4 res;
		for (int c = 0; c < 4; ++c) {
			for (int r = 0; r < 4; ++r) {
				res.m[r * 4 + c] = m[c * 4 + r];
			}
		}
		return res;
	}

	Matrix4 inverse() const {
		Matrix4 inv;
		float invOut[16];
		const float* src = m;

		invOut[0] = src[5]  * src[10] * src[15] - 
		            src[5]  * src[11] * src[14] - 
		            src[9]  * src[6]  * src[15] + 
		            src[9]  * src[7]  * src[14] +
		            src[13] * src[6]  * src[11] - 
		            src[13] * src[7]  * src[10];

		invOut[4] = -src[4]  * src[10] * src[15] + 
		             src[4]  * src[11] * src[14] + 
		             src[8]  * src[6]  * src[15] - 
		             src[8]  * src[7]  * src[14] - 
		             src[12] * src[6]  * src[11] + 
		             src[12] * src[7]  * src[10];

		invOut[8] = src[4]  * src[9] * src[15] - 
		            src[4]  * src[11] * src[13] - 
		            src[8]  * src[5] * src[15] + 
		            src[8]  * src[7] * src[13] + 
		            src[12] * src[5] * src[11] - 
		            src[12] * src[7] * src[9];

		invOut[12] = -src[4]  * src[9] * src[14] + 
		              src[4]  * src[10] * src[13] +
		              src[8]  * src[5] * src[14] - 
		              src[8]  * src[6] * src[13] - 
		              src[12] * src[5] * src[10] + 
		              src[12] * src[6] * src[9];

		invOut[1] = -src[1]  * src[10] * src[15] + 
		             src[1]  * src[11] * src[14] + 
		             src[9]  * src[2] * src[15] - 
		             src[9]  * src[3] * src[14] - 
		             src[13] * src[2] * src[11] + 
		             src[13] * src[3] * src[10];

		invOut[5] = src[0]  * src[10] * src[15] - 
		            src[0]  * src[11] * src[14] - 
		            src[8]  * src[2] * src[15] + 
		            src[8]  * src[3] * src[14] + 
		            src[12] * src[2] * src[11] - 
		            src[12] * src[3] * src[10];

		invOut[9] = -src[0]  * src[9] * src[15] + 
		             src[0]  * src[11] * src[13] + 
		             src[8]  * src[1] * src[15] - 
		             src[8]  * src[3] * src[13] - 
		             src[12] * src[1] * src[11] + 
		             src[12] * src[3] * src[9];

		invOut[13] = src[0]  * src[9] * src[14] - 
		             src[0]  * src[10] * src[13] - 
		             src[8]  * src[1] * src[14] + 
		             src[8]  * src[2] * src[13] + 
		             src[12] * src[1] * src[10] - 
		             src[12] * src[2] * src[9];

		invOut[2] = src[1]  * src[6] * src[15] - 
		            src[1]  * src[7] * src[14] - 
		            src[5]  * src[2] * src[15] + 
		            src[5]  * src[3] * src[14] + 
		            src[13] * src[2] * src[7] - 
		            src[13] * src[3] * src[6];

		invOut[6] = -src[0]  * src[6] * src[15] + 
		             src[0]  * src[7] * src[14] + 
		             src[4]  * src[2] * src[15] - 
		             src[4]  * src[3] * src[14] - 
		             src[12] * src[2] * src[7] + 
		             src[12] * src[3] * src[6];

		invOut[10] = src[0]  * src[5] * src[15] - 
		             src[0]  * src[7] * src[13] - 
		             src[4]  * src[1] * src[15] + 
		             src[4]  * src[3] * src[13] + 
		             src[12] * src[1] * src[7] - 
		             src[12] * src[3] * src[5];

		invOut[14] = -src[0]  * src[5] * src[14] + 
		              src[0]  * src[6] * src[13] + 
		              src[4]  * src[1] * src[14] - 
		              src[4]  * src[2] * src[13] - 
		              src[12] * src[1] * src[6] + 
		              src[12] * src[2] * src[5];

		invOut[3] = -src[1] * src[6] * src[11] + 
		             src[1] * src[7] * src[10] + 
		             src[5] * src[2] * src[11] - 
		             src[5] * src[3] * src[10] - 
		             src[9] * src[2] * src[7] + 
		             src[9] * src[3] * src[6];

		invOut[7] = src[0] * src[6] * src[11] - 
		            src[0] * src[7] * src[10] - 
		            src[4] * src[2] * src[11] + 
		            src[4] * src[3] * src[10] + 
		            src[8] * src[2] * src[7] - 
		            src[8] * src[3] * src[6];

		invOut[11] = -src[0] * src[5] * src[11] + 
		              src[0] * src[7] * src[9] + 
		              src[4] * src[1] * src[11] - 
		              src[4] * src[3] * src[9] - 
		              src[8] * src[1] * src[7] + 
		              src[8] * src[3] * src[5];

		invOut[15] = src[0] * src[5] * src[10] - 
		             src[0] * src[6] * src[9] - 
		             src[4] * src[1] * src[10] + 
		             src[4] * src[2] * src[9] + 
		             src[8] * src[1] * src[6] - 
		             src[8] * src[2] * src[5];

		float det = src[0] * invOut[0] + src[1] * invOut[4] + src[2] * invOut[8] + src[3] * invOut[12];
		if (std::abs(det) < 1e-8f) {
			inv.identity();
			return inv;
		}

		det = 1.0f / det;
		for (int i = 0; i < 16; ++i) {
			inv.m[i] = invOut[i] * det;
		}
		return inv;
	}
};

#endif /* NET_MINECRAFT_CLIENT_RENDERER_SHADER_Matrix4_H__ */
