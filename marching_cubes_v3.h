#ifndef MARCHING_CUBES_V3_H
#define MARCHING_CUBES_V3_H

#include <vector>
#include <cmath>

// 基本的3D向量结构
struct Vec3 {
    float x, y, z;
    Vec3() : x(0.f), y(0.f), z(0.f) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

// 顶点结构（可与 Vec3 合并使用，根据需要扩展法线、颜色等信息）
struct Vertex {
    float x, y, z;
};

// 网格数据结构：存储顶点数组和三角形索引数组
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

// 定义三角形索引类型（可根据需要更改为 size_t）
typedef unsigned int IndexType;

static const Vec3 cubeCornerOffsets[8];

// 内联函数：根据 (x, y, z) 下标从体数据中取值
// 采用公式：data[x + dims[0]*(y + dims[1]*z)]
inline float getVolumeValue(const float *data, int x, int y, int z, const int dims[3])
{
    return data[x + dims[0]*(y + dims[1]*z)];
}

// 线性插值函数：给定等值面阈值和两个端点及其对应的体数据值，计算交点位置
Vec3 interpolate(float isoLevel, const Vec3 &p1, const Vec3 &p2, float valp1, float valp2);

// 针对体数据中一个单元格（体素）进行多边形化处理，将计算得到的交点存入 triangleVerts
// triangleVerts 中存储的是该单元格内所有边上计算得到的交点，后续可根据 triTable 生成三角形
void polygoniseCube(const float *volume, const int dims[3],
                    int cellX, int cellY, int cellZ,
                    float isoLevel,
                    std::vector<Vec3> &triangleVerts);

void calculateMeshBounds(const std::vector<Vertex> &vertices,Vec3 &min_bounds, Vec3 &max_bounds, Vec3 &center);

// 从体数据中提取网格的接口（marching cubes 算法）
// 参数：体数据、尺寸数组、等值面阈值；返回提取出的网格
void marchingCubesFull(const float *volume, const int dims[3],
    float isoLevel,
    std::vector<Vertex> &outVertices,
    std::vector<IndexType> &outIndices);

#endif // MARCHING_CUBES_V3_H
