#ifndef MATH_H
#define MATH_H

struct Vec2
{
	float x;
	float y;
};

inline Vec2 operator-(Vec2 v)
{
	Vec2 result;
	result.x = -v.x;
	result.y = -v.y;
	return result;
}

inline Vec2 operator+(Vec2 v1, Vec2 v2)
{
	Vec2 result;
	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	return result;
}

inline Vec2 operator-(Vec2 v1, Vec2 v2)
{
	Vec2 result;
	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	return result;
}

inline Vec2 operator*(float s, Vec2 v)
{
	Vec2 result;
	result.x = s * v.x;
	result.y = s * v.y;
	return result;
}

struct Vec3
{
	float x;
	float y;
	float z;
};

inline Vec3 operator-(Vec3 v)
{
	Vec3 result;
	result.x = -v.x;
	result.y = -v.y;
	result.z = -v.z;
	return result;
}

inline Vec3 operator+(Vec3 v1, Vec3 v2)
{
	Vec3 result;
	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;
	return result;
}

inline Vec3 operator-(Vec3 v1, Vec3 v2)
{
	Vec3 result;
	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;
	return result;
}

inline Vec3 operator*(float s, Vec3 v)
{
	Vec3 result;
	result.x = s * v.x;
	result.y = s * v.y;
	result.z = s * v.z;
	return result;
}

struct Mat4
{
	float e[4][4];
};

Mat4 operator*(Mat4 a, Mat4 b)
{
	Mat4 r = {};
	for (int i = 0; i < 4; ++i) // rows of a
	{
		for (int j = 0; j < 4; ++j) // columns of b
		{
			for (int k = 0; k < 4; ++k) // going through the row of a and column of b
			{
				r.e[i][j] += a.e[i][k] * b.e[k][j];
			}
		}
	}
	return r;
}

#endif