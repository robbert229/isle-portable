#pragma once

#include "miniwin/d3drm.h"

#include <math.h>

typedef D3DVALUE Matrix3x3[3][3];

inline D3DVECTOR TransformNormal(const D3DVECTOR& v, const Matrix3x3& m)
{
	return {
		v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
		v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
		v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
	};
}

inline D3DVECTOR Normalize(const D3DVECTOR& v)
{
	float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len > 0.0f) {
		float invLen = 1.0f / len;
		return {v.x * invLen, v.y * invLen, v.z * invLen};
	}
	return {0, 0, 0};
}

inline float DotProduct(const D3DVECTOR& a, const D3DVECTOR& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline D3DVECTOR CrossProduct(const D3DVECTOR& a, const D3DVECTOR& b)
{
	return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline D3DVECTOR TransformPoint(const D3DVECTOR& p, const D3DRMMATRIX4D& m)
{
	return {
		p.x * m[0][0] + p.y * m[1][0] + p.z * m[2][0] + m[3][0],
		p.x * m[0][1] + p.y * m[1][1] + p.z * m[2][1] + m[3][1],
		p.x * m[0][2] + p.y * m[1][2] + p.z * m[2][2] + m[3][2]
	};
}

inline void MultiplyMatrix(D3DRMMATRIX4D& out, const D3DRMMATRIX4D& a, const D3DRMMATRIX4D& b)
{
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			out[row][col] = 0.0;
			for (int k = 0; k < 4; ++k) {
				out[row][col] += a[row][k] * b[k][col];
			}
		}
	}
}

inline void IdentityMatrix(D3DRMMATRIX4D out)
{
	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) {
			out[row][col] = row == col ? 1.0f : 0.0f;
		}
	}
}

// Inverse of an affine row-vector matrix (last column 0, 0, 0, 1). Returns
// false and the identity for a singular matrix.
inline bool InvertAffineMatrix(D3DRMMATRIX4D out, const D3DRMMATRIX4D m)
{
	float c00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
	float c01 = m[1][2] * m[2][0] - m[1][0] * m[2][2];
	float c02 = m[1][0] * m[2][1] - m[1][1] * m[2][0];
	float det = m[0][0] * c00 + m[0][1] * c01 + m[0][2] * c02;
	if (fabsf(det) < 1e-12f) {
		IdentityMatrix(out);
		return false;
	}
	float inv = 1.0f / det;
	out[0][0] = c00 * inv;
	out[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * inv;
	out[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * inv;
	out[1][0] = c01 * inv;
	out[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * inv;
	out[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * inv;
	out[2][0] = c02 * inv;
	out[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * inv;
	out[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * inv;
	for (int col = 0; col < 3; ++col) {
		out[3][col] = -(m[3][0] * out[0][col] + m[3][1] * out[1][col] + m[3][2] * out[2][col]);
		out[col][3] = 0.0f;
	}
	out[3][3] = 1.0f;
	return true;
}

inline D3DVECTOR TransformDirection(const D3DVECTOR& v, const D3DRMMATRIX4D m)
{
	return {
		v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0],
		v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1],
		v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2]
	};
}
