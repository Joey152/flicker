#pragma once

struct VertexPos {
    float x;
    float y;
    float z;
} __attribute__((__packed__));

struct VertexColor {
    float r;
    float g;
    float b;
} __attribute__((__packed__));

struct Vertex {
    struct VertexPos pos;
};

