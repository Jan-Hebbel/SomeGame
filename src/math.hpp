#ifndef MATH_H
#define MATH_H

struct Vec2
{
	float x;
	float y;
};

struct Vec3
{
	float x;
	float y;
	float z;
};

struct Mat4
{
	float e[4][4];
};

//
// Vec2
//
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

inline Vec2 make_vec2(float x, float y) {
	Vec2 vec = { x, y };
	return vec;
}

Vec2 normalize(Vec2 v);

// 
// Vec3
//
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

inline Vec3 make_vec3(float x, float y, float z) {
	Vec3 vec = { x, y, z };
	return vec;
}

Vec3 normalize(Vec3 v);

// 
// Mat4
//
inline Mat4 operator*(Mat4 a, Mat4 b)
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

inline Mat4 identity()
{
	Mat4 r = {};
	for (int i = 0; i < 4; ++i)
	{
		r.e[i][i] = 1.0f;
	}
	return r;
}

inline Mat4 orthographic_projection(float l, float r, float b, float t, float n, float f)
{
	/*
		| 2.0/(r-l)    0            0         0 |
		| 0            2.0/(t-b)    0         0 |
		| 0            0            1.0/(f-n) 0 |
		| -(r+l)/(r-l) -(t+b)/(t-b) -n/(f-n)  1 |

		NOTE: this matrix needs to be transformed before use in the (GLSL) shader
	*/
	Mat4 result = {};
	result.e[0][0] = 2.0f / (r - l);
	result.e[1][1] = 2.0f / (t - b);
	result.e[2][2] = 1.0f / (f - n);
	result.e[3][0] = -(r + l) / (r - l);
	result.e[3][1] = -(t + b) / (t - b);
	result.e[3][2] = -n / (f - n);
	result.e[3][3] = 1.0f;
	return result;
}

Mat4 transpose(Mat4 m);
Mat4 translate(Vec3 pos);
Mat4 translate(Mat4 matrix, Vec3 pos);

#endif
