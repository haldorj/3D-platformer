#pragma once

#include <cmath>

static constexpr float PI_32 = 3.14159265359f;

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

struct IV4
{
	int32_t X, Y, Z, W;
};

struct Quat
{
	float X, Y, Z, W;
};

struct M4
{
	float M[4][4] = { 0 };
};

inline float DegreesToRadians(const float degrees)
{
	// 3.14159265359f / 180.0f = 0.01745329252f
	return degrees * (0.01745329251f);
}

inline float RadiansToDegrees(const float radians)
{
	// 3.14159265359f / 180.0f = 0.01745329252f
	return radians / 0.01745329252f;
}

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

inline V2 operator*(const V2& a, const float b)
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

inline V3 operator*(const V3& a, const float b) 
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
	const float length = sqrtf(a.X * a.X + a.Y * a.Y + a.Z * a.Z);

	if (length == 0.0f)
	{
		return {0.0f, 0.0f, 0.0f};
	}

	return {a.X / length, a.Y / length, a.Z / length};
};

inline V2 Normalize2D(const V3& a)
{
	const float length = sqrtf(a.X * a.X + a.Y * a.Y);

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

inline V3 V3Lerp(const V3& from, const V3& to, float t)
{
	return from * (1.0f - t) + to * t;
}

//////////////////////////////////////////////////////////////////////////////
//								VECTOR 4									//
//////////////////////////////////////////////////////////////////////////////

//

//////////////////////////////////////////////////////////////////////////////
//								QUATERNIONS									//
//////////////////////////////////////////////////////////////////////////////

inline Quat operator * (const Quat& q, const float s)
{
	return {q.X * s,
			q.Y * s,
			q.Z * s,
			q.W * s};
}

inline Quat operator + (const Quat& q1, const Quat& q2)
{
	return { q1.X + q2.X,
			q1.Y + q2.Y,
			q1.Z + q2.Z,
			q1.W + q2.W };
}

// Creates a quaternion that represents a rotation (in radians) around an axis.
// NOTE: Axis must be normalized.
inline Quat QuatFromAxisAngle(const V3 axis, const float radians)
{
	Quat result = {};
	const float halfAngle = radians * 0.5f;
	result.W = cosf(halfAngle);
	const float s = sinf(halfAngle);
	result.X = axis.X * s;
	result.Y = axis.Y * s;
	result.Z = axis.Z * s;
	return result;
}

inline float Dot(const Quat& from, const Quat& to)
{
	float dot = from.X * to.X + 
				from.Y * to.Y + 
				from.Z * to.Z + 
				from.W * to.W;

	return dot;
}

inline Quat NormalizeQuat(const Quat& q)
{
	float len = sqrtf(q.X * q.X + q.Y * q.Y + q.Z * q.Z + q.W * q.W);
	if (len == 0.0f) return { 0,0,0,1 };
	float inv = 1.0f / len;
	Quat result = { q.X * inv, q.Y * inv, q.Z * inv, q.W * inv };

	return result;
}

inline Quat Slerp(const Quat& from, const Quat& to, float t)
{
	float dot = Dot(from, to);

	// If dot < 0, slerp the opposite quaternion to take the shortest path
	Quat to1 = to;
	if (dot < 0.0f)
	{
		dot = -dot;
		to1 = { -to.X, -to.Y, -to.Z, -to.W };
	}

	// Clamp the dot to avoid NaN from acos
	dot = std::clamp(dot, -1.0f, 1.0f);

	// If quaternions are close, use linear interpolation (Lerp) to avoid division by zero
	const float epsilon = 1e-5f;
	if (dot > 1.0f - epsilon)
	{
		// Linear interpolation
		Quat result = {
			from.X + t * (to1.X - from.X),
			from.Y + t * (to1.Y - from.Y),
			from.Z + t * (to1.Z - from.Z),
			from.W + t * (to1.W - from.W)
		};
		return NormalizeQuat(result);
	}

	// Compute the angle between them
	float theta = acosf(dot);
	float sinTheta = sinf(theta);

	float w1 = sinf((1.0f - t) * theta) / sinTheta;
	float w2 = sinf(t * theta) / sinTheta;

	Quat result = {
		from.X * w1 + to1.X * w2,
		from.Y * w1 + to1.Y * w2,
		from.Z * w1 + to1.Z * w2,
		from.W * w1 + to1.W * w2
	};

	return NormalizeQuat(result);
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
	const V3 zaxis = Normalize(at - eye);				// The cameras "forward" vector.
	const V3 xaxis = Normalize(Cross(up, zaxis));		// The cameras "right" vector.
	const V3 yaxis = Cross(zaxis, xaxis);				// The cameras "up" vector.
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

inline M4 MatrixOrthographic(const float width, const float height, 
	const float nearPlane, const float farPlane)
{
	// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-d3dxmatrixorthorh

	M4 result = {};
	result.M[0][0] = 2.0f / width;
	result.M[1][1] = 2.0f / height;
	result.M[2][2] = 1.0f / (nearPlane - farPlane);
	result.M[3][2] = nearPlane / (nearPlane - farPlane);
	result.M[3][3] = 1.0f;

	return result;
}

inline M4 MatrixOrthographicBL(const float width, const float height,
                               const float nearPlane, const float farPlane)
{
	M4 m = {};

	m.M[0][0] = 2.0f / (width);
	m.M[1][1] = 2.0f / (height);
	m.M[2][2] = 1.0f / (nearPlane - farPlane);

	m.M[3][0] = -(width) / (width);
	m.M[3][1] = -(height) / (height);
	m.M[3][2] = -nearPlane / (farPlane - nearPlane);
	m.M[3][3] = 1.0f;

	return m;
}

inline M4 MatrixOrthographicTL(float width, float height,
	float nearPlane, float farPlane)
{
	M4 m = {};

	m.M[0][0] = 2.0f / width;
	m.M[1][1] = -2.0f / height;
	m.M[2][2] = 1.0f / (nearPlane - farPlane);

	m.M[3][0] = -1.0f;
	m.M[3][1] = 1.0f;
	m.M[3][2] = -nearPlane / (farPlane - nearPlane);
	m.M[3][3] = 1.0f;

	return m;
}

inline M4 MatrixPerspective(const float fovY, const float aspect,
	const float nearPlane, const float farPlane)
{
	M4 result = {};

	const float f = 1.0f / tanf(fovY / 2.0f);
	result.M[0][0] = f / aspect;
	result.M[1][1] = f;
	result.M[2][2] = farPlane / (farPlane - nearPlane);	
	result.M[2][3] = 1.0f;
	result.M[3][2] = (-nearPlane * farPlane) / (farPlane - nearPlane);
	result.M[3][3] = 0.0f;

	return result;
};

inline M4 MatrixRotationX(const float angle)
{
	M4 result = {};

	const float c = cosf(angle);
	const float s = sinf(angle);

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

inline M4 MatrixRotationY(const float angle)
{
	M4 result = {};

	const float c = cosf(angle);
	const float s = sinf(angle);

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

inline M4 MatrixRotationZ(const float angle)
{
	M4 result = {};

	const float c = cosf(angle);
	const float s = sinf(angle);

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


inline M4 MatrixTranslation(const float x, const float y, const float z)
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

inline M4 MatrixScaling(const float x, const float y, const float z)
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
	const float xx = q.X * q.X;
	const float yy = q.Y * q.Y;
	const float zz = q.Z * q.Z;
	const float xy = q.X * q.Y;
	const float xz = q.X * q.Z;
	const float yz = q.Y * q.Z;
	const float wx = q.W * q.X;
	const float wy = q.W * q.Y;
	const float wz = q.W * q.Z;

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
