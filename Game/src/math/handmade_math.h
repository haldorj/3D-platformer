#pragma once

#include <cmath>
#include <compare>

struct V2
{
    float x;
    float y;

    auto operator<=>(const V2&) const = default;
};

struct V3
{
    float x;
    float y;
    float z;

    auto operator<=>(const V3&) const = default;
};

struct V4
{
    float x;
    float y;
    float z;
    float w;

    auto operator<=>(const V4&) const = default;
};

struct IV4
{
    int x;
    int y;
    int z;
    int w;

    auto operator<=>(const IV4&) const = default;
};

struct Quat
{
    float x;
    float y;
    float z;
    float w;

    auto operator<=>(const Quat&) const = default;
};

struct M4
{
    float m[4][4] = {};

    auto operator<=>(const M4&) const = default;
};

inline float DegreesToRadians(const float degrees)
{
    // 3.14159265359f / 180.0f = 0.01745329252f
    return degrees * (0.01745329252f);
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
    return {
        a.x + b.x,
        a.y + b.y
    };
};

inline V2 operator+=(V2& a, const V2& b)
{
    a = a + b;
    return a;
};

inline V2 operator-(const V2& a, const V2& b)
{
    return {
        a.x - b.x,
        a.y - b.y
    };
};

inline V2 operator-=(V2& a, const V2& b)
{
    a = a - b;
    return a;
};

inline V2 operator*(const V2& a, const float b)
{
    return {
        a.x * b,
        a.y * b
    };
};

inline V2 Cross(const V2& a, const V2& b)
{
    return {
        a.x * b.y - a.y * b.x,
        a.y * b.x - a.x * b.y
    };
};

inline float Dot(const V2& a, const V2& b)
{
    return a.x * b.x + a.y * b.y;
};

inline V2 Normalize(const V2& a)
{
    float length = sqrtf(a.x * a.x + a.y * a.y);

    if (length == 0.0f)
    {
        return {0.0f, 0.0f};
    }

    return {a.x / length, a.y / length};
};

//////////////////////////////////////////////////////////////////////////////
//								VECTOR 3									//
//////////////////////////////////////////////////////////////////////////////

inline V3 operator+(const V3& a, const V3& b)
{
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
};

inline V3 operator+=(V3& a, const V3& b)
{
    a = a + b;
    return a;
};

inline V3 operator-(const V3& a, const V3& b)
{
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
};

inline V3 operator-=(V3& a, const V3& b)
{
    a = a - b;
    return a;
};

inline V3 operator*(const V3& a, const float b)
{
    return {
        a.x * b,
        a.y * b,
        a.z * b
    };
}

inline V3 Cross(const V3& a, const V3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
};

inline float Dot(const V3& a, const V3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
};

inline float Length(const V3& a)
{
    return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
};

inline V3 Normalize(const V3& a)
{
    const float length = Length(a);

    if (length == 0.0f)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    return {a.x / length, a.y / length, a.z / length};
};

inline V2 Normalize2D(const V3& a)
{
    const float length = sqrtf(a.x * a.x + a.y * a.y);

    if (length == 0.0f)
    {
        return {0.0f, 0.0f};
    }

    return {a.x / length, a.y / length};
};

static V3 V3Lerp(const V3& from, const V3& to, float t)
{
    V3 result = from + (to - from) * t;
    return result;
}

//////////////////////////////////////////////////////////////////////////////
//								VECTOR 4									//
//////////////////////////////////////////////////////////////////////////////

//

//////////////////////////////////////////////////////////////////////////////
//								QUATERNIONS									//
//////////////////////////////////////////////////////////////////////////////

inline Quat operator *(const Quat& q, const float s)
{
    return {
        q.x * s,
        q.y * s,
        q.z * s,
        q.w * s
    };
}

inline Quat operator +(const Quat& q1, const Quat& q2)
{
    return {
        q1.x + q2.x,
        q1.y + q2.y,
        q1.z + q2.z,
        q1.w + q2.w
    };
}

// Creates a quaternion that represents a rotation (in radians) around an axis.
// NOTE: Axis must be normalized.
inline Quat QuatFromAxisAngle(const V3 axis, const float radians)
{
    Quat result;
    const float halfAngle = radians * 0.5f;
    result.w = cosf(halfAngle);
    const float s = sinf(halfAngle);
    result.x = axis.x * s;
    result.y = axis.y * s;
    result.z = axis.z * s;
    return result;
}

inline float Dot(const Quat& from, const Quat& to)
{
    const float dot = from.x * to.x +
        from.y * to.y +
        from.z * to.z +
        from.w * to.w;

    return dot;
}

inline Quat NormalizeQuat(const Quat& q)
{
    const float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (length == 0.0f)
        return {0.0f, 0.0f, 0.0f, 1.0f};

    const float inv = 1.0f / length;
    const Quat result = {.x = q.x * inv, .y = q.y * inv, .z = q.z * inv, .w = q.w * inv};

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
        to1 = {-to.x, -to.y, -to.z, -to.w};
    }

    // Clamp the dot to avoid NaN from acos
    dot = std::clamp(dot, -1.0f, 1.0f);

    // If quaternions are close, use linear interpolation (Lerp) to avoid division by zero
    const float epsilon = 1e-5f;
    if (dot > 1.0f - epsilon)
    {
        // Linear interpolation
        Quat result = {
            from.x + t * (to1.x - from.x),
            from.y + t * (to1.y - from.y),
            from.z + t * (to1.z - from.z),
            from.w + t * (to1.w - from.w)
        };
        return NormalizeQuat(result);
    }

    // Compute the angle between them
    float theta = acosf(dot);
    float sinTheta = sinf(theta);

    float w1 = sinf((1.0f - t) * theta) / sinTheta;
    float w2 = sinf(t * theta) / sinTheta;

    Quat result = {
        from.x * w1 + to1.x * w2,
        from.y * w1 + to1.y * w2,
        from.z * w1 + to1.z * w2,
        from.w * w1 + to1.w * w2
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
            result.m[i][j] = a.m[i][j] + b.m[i][j];
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
            result.m[i][j] = a.m[i][j] - b.m[i][j];
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
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}

inline M4 MatrixTranspose(const M4& in)
{
    M4 result;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.m[i][j] = in.m[j][i];
        }
    }
    return result;
};

inline M4 MatrixIdentity()
{
    M4 result = {};
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;
    return result;
};

inline M4 MatrixInverse(const M4& in)
{
    M4 result = MatrixIdentity();

    // Transpose the upper 3�3 rotation/scale part
    result.m[0][0] = in.m[0][0];
    result.m[0][1] = in.m[1][0];
    result.m[0][2] = in.m[2][0];
    result.m[1][0] = in.m[0][1];
    result.m[1][1] = in.m[1][1];
    result.m[1][2] = in.m[2][1];
    result.m[2][0] = in.m[0][2];
    result.m[2][1] = in.m[1][2];
    result.m[2][2] = in.m[2][2];

    // Invert translation
    result.m[3][0] = -(in.m[3][0] * result.m[0][0] + in.m[3][1] * result.m[1][0] + in.m[3][2] * result.m[2][0]);
    result.m[3][1] = -(in.m[3][0] * result.m[0][1] + in.m[3][1] * result.m[1][1] + in.m[3][2] * result.m[2][1]);
    result.m[3][2] = -(in.m[3][0] * result.m[0][2] + in.m[3][1] * result.m[1][2] + in.m[3][2] * result.m[2][2]);
    result.m[3][3] = 1.0f;

    return result;
}

inline M4 MatrixLookAt(const V3& eye, const V3& at, const V3& up)
{
    const V3 zaxis = Normalize(at - eye); // The cameras "forward" vector.
    const V3 xaxis = Normalize(Cross(up, zaxis)); // The cameras "right" vector.
    const V3 yaxis = Cross(zaxis, xaxis); // The cameras "up" vector.
    M4 viewMatrix = {};
    viewMatrix.m[0][0] = xaxis.x;
    viewMatrix.m[1][0] = xaxis.y;
    viewMatrix.m[2][0] = xaxis.z;
    viewMatrix.m[3][0] = -Dot(xaxis, eye);

    viewMatrix.m[0][1] = yaxis.x;
    viewMatrix.m[1][1] = yaxis.y;
    viewMatrix.m[2][1] = yaxis.z;
    viewMatrix.m[3][1] = -Dot(yaxis, eye);

    viewMatrix.m[0][2] = zaxis.x;
    viewMatrix.m[1][2] = zaxis.y;
    viewMatrix.m[2][2] = zaxis.z;
    viewMatrix.m[3][2] = -Dot(zaxis, eye);

    viewMatrix.m[3][3] = 1.0f;
    return viewMatrix;
};

inline M4 MatrixOrthographic(const float width, const float height,
                             const float nearPlane, const float farPlane)
{
    // https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-d3dxmatrixorthorh

    M4 result = {};
    result.m[0][0] = 2.0f / width;
    result.m[1][1] = 2.0f / height;
    result.m[2][2] = 1.0f / (nearPlane - farPlane);
    result.m[3][2] = nearPlane / (nearPlane - farPlane);
    result.m[3][3] = 1.0f;

    return result;
}

inline M4 MatrixOrthographicBL(const float width, const float height,
                               const float nearPlane, const float farPlane)
{
    M4 m = {};

    m.m[0][0] = 2.0f / (width);
    m.m[1][1] = 2.0f / (height);
    m.m[2][2] = 1.0f / (nearPlane - farPlane);

    m.m[3][0] = -(width) / (width);
    m.m[3][1] = -(height) / (height);
    m.m[3][2] = -nearPlane / (farPlane - nearPlane);
    m.m[3][3] = 1.0f;

    return m;
}

inline M4 MatrixOrthographicTL(float width, float height,
                               float nearPlane, float farPlane)
{
    M4 m = {};

    m.m[0][0] = 2.0f / width;
    m.m[1][1] = -2.0f / height;
    m.m[2][2] = 1.0f / (nearPlane - farPlane);

    m.m[3][0] = -1.0f;
    m.m[3][1] = 1.0f;
    m.m[3][2] = -nearPlane / (farPlane - nearPlane);
    m.m[3][3] = 1.0f;

    return m;
}

inline M4 MatrixPerspective(const float fovY, const float aspect,
                            const float nearPlane, const float farPlane)
{
    M4 result = {};

    const float f = 1.0f / tanf(fovY / 2.0f);
    result.m[0][0] = f / aspect;
    result.m[1][1] = f;
    result.m[2][2] = farPlane / (farPlane - nearPlane);
    result.m[2][3] = 1.0f;
    result.m[3][2] = (-nearPlane * farPlane) / (farPlane - nearPlane);
    result.m[3][3] = 0.0f;

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

    result.m[0][0] = 1;
    result.m[1][1] = c;
    result.m[1][2] = -s;
    result.m[2][1] = s;
    result.m[2][2] = c;
    result.m[3][3] = 1.0f;

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

    result.m[0][0] = c;
    result.m[0][2] = s;
    result.m[1][1] = 1;
    result.m[2][0] = -s;
    result.m[2][2] = c;
    result.m[3][3] = 1.0f;

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

    result.m[0][0] = c;
    result.m[0][1] = -s;
    result.m[1][0] = s;
    result.m[1][1] = c;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;

    return result;
}


inline M4 MatrixTranslation(const V3& in)
{
    M4 result = MatrixIdentity();

    // [1][0][0][x]
    // [0][1][0][y]
    // [0][0][1][z]
    // [0][0][0][1]

    result.m[3][0] = in.x;
    result.m[3][1] = in.y;
    result.m[3][2] = in.z;
    result.m[3][3] = 1.0f;

    return result;
};

inline M4 MatrixScaling(const V3& in)
{
    M4 result = {};

    // [x][0][0][0]
    // [0][y][0][0]
    // [0][0][z][0]
    // [0][0][0][1]

    result.m[0][0] = in.x;
    result.m[1][1] = in.y;
    result.m[2][2] = in.z;
    result.m[3][3] = 1.0f;

    return result;
};

inline M4 MatrixFromQuaternion(const Quat& q)
{
    M4 result = {};
    const float xx = q.x * q.x;
    const float yy = q.y * q.y;
    const float zz = q.z * q.z;
    const float xy = q.x * q.y;
    const float xz = q.x * q.z;
    const float yz = q.y * q.z;
    const float wx = q.w * q.x;
    const float wy = q.w * q.y;
    const float wz = q.w * q.z;

    result.m[0][0] = 1.0f - 2.0f * (yy + zz);
    result.m[0][1] = 2.0f * (xy - wz);
    result.m[0][2] = 2.0f * (xz + wy);
    result.m[0][3] = 0.0f;

    result.m[1][0] = 2.0f * (xy + wz);
    result.m[1][1] = 1.0f - 2.0f * (xx + zz);
    result.m[1][2] = 2.0f * (yz - wx);
    result.m[1][3] = 0.0f;

    result.m[2][0] = 2.0f * (xz - wy);
    result.m[2][1] = 2.0f * (yz + wx);
    result.m[2][2] = 1.0f - 2.0f * (xx + yy);
    result.m[2][3] = 0.0f;

    result.m[3][0] = 0.0f;
    result.m[3][1] = 0.0f;
    result.m[3][2] = 0.0f;
    result.m[3][3] = 1.0f;

    return result;
}
