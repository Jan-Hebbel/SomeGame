#include "math.hpp"

#include "types.hpp"

#include <math.h>

Vec2 normalize(Vec2 v) {
	Vec2 r = {};
	if (v.x == 0 && v.y == 0) return r;
	float length = sqrtf(v.x * v.x + v.y * v.y);
	r.x = v.x / length;
	r.y = v.y / length;
	return r;
}

Vec3 normalize(Vec3 v) {
	Vec3 r = {};
	if (v.x == 0 && v.y == 0 && v.z == 0) return r;
	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	r.x = v.x / length;
	r.y = v.y / length;
	r.z = v.z / length;
	return r;
}

Mat4 transpose(Mat4 m)
{
	Mat4 r = {};
	for (uint i = 0; i < 4; ++i)
	{
		for (uint j = 0; j < 4; ++j)
		{
			r.e[i][j] = m.e[j][i];
		}
	}
	return r;
}

Mat4 translate(Vec3 pos)
{
	Mat4 r = identity();
	r.e[0][3] = pos.x;
	r.e[1][3] = pos.y;
	r.e[2][3] = pos.z;
	return r;
}

Mat4 translate(Mat4 matrix, Vec3 pos)
{
	matrix.e[0][3] += pos.x;
	matrix.e[1][3] += pos.y;
	matrix.e[2][3] += pos.z;
	return matrix;
}
