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

inline Vec2 operator-(Vec2 v);
inline Vec2 operator+(Vec2 v1, Vec2 v2);
inline Vec2 operator-(Vec2 v1, Vec2 v2);
inline Vec2 operator*(float s, Vec2 v);

inline Vec3 operator-(Vec3 v);
inline Vec3 operator+(Vec3 v1, Vec3 v2);
inline Vec3 operator-(Vec3 v1, Vec3 v2);
inline Vec3 operator*(float s, Vec3 v);

Mat4 operator*(Mat4 a, Mat4 b);
Mat4 identity();
Mat4 orthographic_projection(float l, float r, float b, float t, float n, float f);
Mat4 transpose(Mat4 m);
Mat4 translate(Vec3 pos);
Mat4 translate(Mat4 matrix, Vec3 pos);

#endif
