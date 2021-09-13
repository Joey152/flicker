#include "common/linmath.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

void vec3_add(float v[static 3], float x, float y, float z) {
    v[0] += x;
    v[1] += y;
    v[2] += z;
}

float vec3_length(float a[static 3]) {
    return sqrt(
        powf(a[0], 2) +
        powf(a[1], 2) +
        powf(a[2], 2)
    );
}

void vec3_normalize(float v[static 3]) {
    float l = vec3_length(v);

    v[0] = v[0] / l;
    v[1] = v[1] / l;
    v[2] = v[2] / l;
}

void vec3_cross(float v[static 3], float a[static 3], float b[static 3]) {
    float temp[3] = {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };

    memcpy(v, temp, sizeof temp);
}

float vec3_dot(float a[static 3], float b[static 3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void mat4_mul(float m[static 4][4], float a[static 4][4], float b[static 4][4]) {
    //float **temp = calloc(4 * 4, sizeof **temp);
    float temp[4][4];

    for (size_t row = 0; row < 4; row++) {
        for (size_t col = 0; col < 4; col++) {
            temp[row][col] = 0.0f;
            for (size_t offset = 0; offset < 4; offset++) {
                temp[row][col] += a[row][offset] * b[offset][col];
            }
        }
    }

    memcpy(m, temp, 4 * 4 * sizeof **temp);
}

void
mat4_view(float m[static 4][4], float pos[static 3], float pitch, float yaw) {
    float cos_yaw = cosf(yaw);
    float sin_yaw = sinf(yaw);
    float cos_pitch = cosf(pitch);
    float sin_pitch = sinf(pitch);

    float xaxis[3] = {cos_yaw, 0.0f, -sin_yaw};
    float yaxis[3] = {sin_yaw * sin_pitch, cos_pitch, cos_yaw * sin_pitch};
    float zaxis[3] = {sin_yaw * cos_pitch, -sin_pitch, cos_yaw * cos_pitch};

    float posx = -vec3_dot(xaxis, pos);
    float posy = -vec3_dot(yaxis, pos);
    float posz = -vec3_dot(zaxis, pos);

//    m[0][0] = xaxis[0];
//    m[0][1] = xaxis[1];
//    m[0][2] = xaxis[2];
//    m[0][3] = 0.0f;
//    m[1][0] = yaxis[0];
//    m[1][1] = yaxis[1];
//    m[1][2] = yaxis[2];
//    m[1][3] = 0.0f;
//    m[2][0] = zaxis[0];
//    m[2][1] = zaxis[1];
//    m[2][2] = zaxis[2];
//    m[2][3] = 0.0f;
//    m[3][0] = posx;
//    m[3][1] = posy;
//    m[3][2] = posz;
//    m[3][3] = 1.0f;

    m[0][0] = xaxis[0];
    m[0][1] = yaxis[0];
    m[0][2] = zaxis[0];
    m[0][3] = 0.0f;
    m[1][0] = xaxis[1];
    m[1][1] = yaxis[1];
    m[1][2] = zaxis[1];
    m[1][3] = 0.0f;
    m[2][0] = xaxis[2];
    m[2][1] = yaxis[2];
    m[2][2] = zaxis[2];
    m[2][3] = 0.0f;
    m[3][0] = posx;
    m[3][1] = posy;
    m[3][2] = posz;
    m[3][3] = 1.0f;
}

void mat4_perspective(float m[static 4][4], float aspect, float fovy, float n, float f) {
    float x = 1.0f / aspect * tanf(fovy/2.0f);
    float y = 1.0f / tanf(fovy/2.0f);
    float a = (f + n) / (n - f);
    float b = (2.0f * f * n) / (n - f);

    float p[4][4] = {
        { x,   0.0, 0.0, 0.0 },
        { 0.0, y,   0.0, 0.0 },
        { 0.0, 0.0, -a,  1.0 },
        { 0.0, 0.0, b,   0.0 },
    };

    float c[4][4] = {
        { 1.0, 0.0, 0.0, 0.0 },
        { 0.0, -1.0, 0.0, 0.0 },
        { 0.0, 0.0, 0.5, 0.0 },
        { 0.0, 0.0, 0.5, 1.0 },
    };

    mat4_mul(m, p, c);
}
