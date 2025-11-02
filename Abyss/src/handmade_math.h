#pragma once
#include <math.h>

struct V2
{
	float X, Y;
};

struct V3
{
	float X, Y, Z;
};

struct V4
{
	float X, Y, Z, W;
};

struct M4
{
	float M[4][4] = { 0 };
};

//////////////////////////////////////////////////////////////////////////////
//								VECTOR 2									//
//////////////////////////////////////////////////////////////////////////////

inline V2 operator+(const V2& a, const V2& b)
{	
	return {a.X + b.X,
			a.Y + b.Y};
};

inline V2 operator+=(V2& a, const V2& b)
{	
	a = a + b;
	return a;
};

inline V2 operator-(const V2& a, const V2& b)
{	
	return {a.X - b.X,
			a.Y - b.Y};
};

inline V2 operator-=(V2& a, const V2& b)
{	
	a = a - b;
	return a;
};

inline V2 operator*(const V2& a, float b)
{	
	return {a.X * b,
			a.Y * b};
};

inline V2 Cross(const V2& a, const V2& b)
{
	return {a.X * b.Y - a.Y * b.X,
			a.Y * b.X - a.X * b.Y};
};

inline float Dot(const V2& a, const V2& b)
{
	return a.X * b.X + a.Y * b.Y;
};

inline V2 Normalize(const V2& a)
{
	float length = sqrtf(a.X * a.X + a.Y * a.Y);

	if (length == 0.0f)
	{
		return { 0.0f, 0.0f };
	}

	return {a.X / length, a.Y / length};
};

//////////////////////////////////////////////////////////////////////////////
//								VECTOR 3									//
//////////////////////////////////////////////////////////////////////////////

inline V3 operator+(const V3& a, const V3& b) 
{	
	return {a.X + b.X,
			a.Y + b.Y,
			a.Z + b.Z};
};

inline V3 operator+=(V3& a, const V3& b)
{	
	a = a + b;
	return a;
};

inline V3 operator-(const V3& a, const V3& b) 
{	
	return {a.X - b.X,
			a.Y - b.Y,
			a.Z - b.Z};
};

inline V3 operator-=(V3& a, const V3& b)
{	
	a = a - b;
	return a;
};

inline V3 operator*(const V3& a, float b) 
{	
	return {a.X * b,
			a.Y * b,
			a.Z * b};
};

inline bool operator==(const V3& a, const V3& b)
{
	return ( a.X == b.X && a.Y == b.Y && a.Z == b.Z );
};

inline bool operator!=(const V3& a, const V3& b)
{
	return (a.X != b.X || a.Y != b.Y || a.Z != b.Z);
};

inline V3 Cross(const V3& a, const V3& b)
{
	return { a.Y * b.Z - a.Z * b.Y,
			a.Z * b.X - a.X * b.Z,
			a.X * b.Y - a.Y * b.X };
};

inline float Dot(const V3& a, const V3& b)
{
	return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
};

inline V3 Normalize(const V3& a)
{
	float length = sqrtf(a.X * a.X + a.Y * a.Y + a.Z * a.Z);

	if (length == 0.0f)
	{
		return {0.0f, 0.0f, 0.0f};
	}

	return {a.X / length, a.Y / length, a.Z / length};
};

inline V2 Normalize2D(const V3& a)
{
	float length = sqrtf(a.X * a.X + a.Y * a.Y);

	if (length == 0.0f)
	{
		return { 0.0f, 0.0f };
	}

	return {a.X / length, a.Y / length};
};

inline float Length(const V3& a)
{
	return sqrtf(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
};

//////////////////////////////////////////////////////////////////////////////
//								VECTOR 4									//
//////////////////////////////////////////////////////////////////////////////

//

//////////////////////////////////////////////////////////////////////////////
//								QUATERNIONS									//
//////////////////////////////////////////////////////////////////////////////

struct Quat
{
	float X, Y, Z, W;
};

// Creates a quaternion that represents a rotation (in radians) around an axis.
// NOTE: Axis must be normalized.
inline Quat QuatFromAxisAngle(V3 axis, float radians)
{
	Quat result = {};
	float halfAngle = radians * 0.5f;
	result.W = cosf(halfAngle);
	float s = sinf(halfAngle);
	result.X = axis.X * s;
	result.Y = axis.Y * s;
	result.Z = axis.Z * s;
	return result;
}

//////////////////////////////////////////////////////////////////////////////
//								MATRIX 4x4									//
//////////////////////////////////////////////////////////////////////////////

inline M4 operator+(const M4& a, const M4& b)
{
	M4 result = {};
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			result.M[i][j] = a.M[i][j] + b.M[i][j];
		}
	}

	return result;
}

inline M4 operator-(const M4& a, const M4& b)
{
	M4 result = {};
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			result.M[i][j] = a.M[i][j] - b.M[i][j];
		}
	}

	return result;
}

inline M4 operator*(const M4& a, const M4& b)
{
    M4 result = {};
    for (int i = 0; i < 4; i++) 
    {
        for (int j = 0; j < 4; j++) 
        {
            for (int k = 0; k < 4; k++) 
            {
                result.M[i][j] += a.M[i][k] * b.M[k][j];
            }
        }
    }
    return result;
}			

inline M4 MatrixTranspose(const M4& in)
{
	M4 out;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			out.M[i][j] = in.M[j][i];
		}
	}
	return out;
};

inline M4 MatrixIdentity()
{
	M4 result = {};
	result.M[0][0] = 1.0f;
	result.M[1][1] = 1.0f;
	result.M[2][2] = 1.0f;
	result.M[3][3] = 1.0f;
	return result;
};

inline M4 MatrixLookAt(const V3& eye, const V3& at, const V3& up)
{
	V3 zaxis = Normalize(at - eye);			// The cameras "forward" vector.
	V3 xaxis = Normalize(Cross(up, zaxis)); // The cameras "right" vector.
	V3 yaxis = Cross(zaxis, xaxis);			// The cameras "up" vector.
	M4 viewMatrix = {};
	viewMatrix.M[0][0] = xaxis.X;
	viewMatrix.M[1][0] = xaxis.Y;
	viewMatrix.M[2][0] = xaxis.Z;
	viewMatrix.M[3][0] = -Dot(xaxis, eye);

	viewMatrix.M[0][1] = yaxis.X;
	viewMatrix.M[1][1] = yaxis.Y;
	viewMatrix.M[2][1] = yaxis.Z;
	viewMatrix.M[3][1] = -Dot(yaxis, eye);

	viewMatrix.M[0][2] = zaxis.X;
	viewMatrix.M[1][2] = zaxis.Y;
	viewMatrix.M[2][2] = zaxis.Z;
	viewMatrix.M[3][2] = -Dot(zaxis, eye);

	viewMatrix.M[3][3] = 1.0f;
	return viewMatrix;
};

inline M4 MatrixOrthographic(float width, float height, float zn, float zf)
{
	// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-d3dxmatrixorthorh

	M4 result = {};
	result.M[0][0] = 2.0f / width;
	result.M[1][1] = 2.0f / height;
	result.M[2][2] = 1.0f / (zn - zf);
	result.M[3][2] = zn / (zn - zf);
	result.M[3][3] = 1.0f;

	return result;
}

inline M4 MatrixPerspective(float fovY, float aspect, float near, float far)
{
	M4 result = {};

	float f = 1.0f / tanf(fovY / 2.0f);
	result.M[0][0] = f / aspect;
	result.M[1][1] = f;
	result.M[2][2] = far / (far - near);
	result.M[2][3] = 1.0f;
	result.M[3][2] = (-near * far) / (far - near);
	result.M[3][3] = 0.0f;

	return result;
};

inline M4 MatrixRotationX(float angle)
{
	M4 result = {};

	float c = cosf(angle);
	float s = sinf(angle);

	// [1][0][0][0]
	// [0][c][-s][0]
	// [0][s][c][0]
	// [0][0][0][1]

	result.M[0][0] = 1;
	result.M[1][1] = c; result.M[1][2] = -s;
	result.M[2][1] = s; result.M[2][2] = c;
	result.M[3][3] = 1.0f;

	return result;
}

inline M4 MatrixRotationY(float angle)
{
	M4 result = {};
	
	float c = cosf(angle);
	float s = sinf(angle);

	// [c][0][s][0]
	// [0][1][0][0]
	// [-s][0][c][0]
	// [0][0][0][1]

	result.M[0][0] = c; result.M[0][2] = s;
	result.M[1][1] = 1;
	result.M[2][0] = -s; result.M[2][2] = c;
	result.M[3][3] = 1.0f;

	return result;
}

inline M4 MatrixRotationZ(float angle)
{
	M4 result = {};
	
	float c = cosf(angle);
	float s = sinf(angle);

	// [c][-s][0][0]
	// [s][c][0][0]
	// [0][0][1][0]
	// [0][0][0][1]

	result.M[0][0] = c; result.M[0][1] = -s;
	result.M[1][0] = s; result.M[1][1] = c;
	result.M[2][2] = 1.0f;
	result.M[3][3] = 1.0f;

	return result;
}


inline M4 MatrixTranslation(float x, float y, float z)
{
	M4 result = MatrixIdentity();

	// [1][0][0][x]
	// [0][1][0][y]
	// [0][0][1][z]
	// [0][0][0][1]

	result.M[3][0] = x;
	result.M[3][1] = y;
	result.M[3][2] = z;
	result.M[3][3] = 1.0f;

	return result;
};

inline M4 MatrixScaling(float x, float y, float z)
{
	M4 result = {};

	// [x][0][0][0]
	// [0][y][0][0]
	// [0][0][z][0]
	// [0][0][0][1]

	result.M[0][0] = x;
	result.M[1][1] = y;
	result.M[2][2] = z;
	result.M[3][3] = 1.0f;

	return result;
};

inline M4 MatrixFromQuaternion(const Quat& q)
{
	M4 result = {};
	float xx = q.X * q.X;
	float yy = q.Y * q.Y;
	float zz = q.Z * q.Z;
	float xy = q.X * q.Y;
	float xz = q.X * q.Z;
	float yz = q.Y * q.Z;
	float wx = q.W * q.X;
	float wy = q.W * q.Y;
	float wz = q.W * q.Z;

	result.M[0][0] = 1.0f - 2.0f * (yy + zz);
	result.M[0][1] = 2.0f * (xy - wz);
	result.M[0][2] = 2.0f * (xz + wy);
	result.M[0][3] = 0.0f;

	result.M[1][0] = 2.0f * (xy + wz);
	result.M[1][1] = 1.0f - 2.0f * (xx + zz);
	result.M[1][2] = 2.0f * (yz - wx);
	result.M[1][3] = 0.0f;

	result.M[2][0] = 2.0f * (xz - wy);
	result.M[2][1] = 2.0f * (yz + wx);
	result.M[2][2] = 1.0f - 2.0f * (xx + yy);
	result.M[2][3] = 0.0f;

	result.M[3][0] = 0.0f;
	result.M[3][1] = 0.0f;
	result.M[3][2] = 0.0f;
	result.M[3][3] = 1.0f;

	return result;
}
