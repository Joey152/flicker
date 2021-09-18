#pragma once

void vec3_add(float v[static 3], float x, float y, float z);
float vec3_length(float a[static 3]);
void vec3_normalize(float v[static 3]);
void vec3_cross(float v[static 3], float a[static 3], float b[static 3]);
float vec3_dot(float a[static 3], float b[static 3]);

void mat4_mul(float m[4][4], float a[4][4], float b[4][4]);
void mat4_view(float m[static 4][4], float pos[static 3], float cos_yaw, float sin_yaw, float cos_pitch, float sin_pitch);
void mat4_perspective(float m[static 4][4], float aspect, float fovy, float n, float f);
