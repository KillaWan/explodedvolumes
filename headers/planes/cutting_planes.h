#pragma once

#include "data.h"
#include "selecting_planes.h"
#include <vector>

namespace MC
{

    // 表示平面与网格交线的一条线段
    struct IntersectionSegment
    {
        Vertex start;
        Vertex end;
        int planeIndex; // 对应的切割平面索引
    };

    // 包含所有交线的结构
    struct PlaneIntersection
    {
        std::vector<IntersectionSegment> segments;
        bool visible; // 是否可见
    };

    // 表示模型被切割后的一段
    struct ModelSegment
    {
        int startIndex;    // 在原始网格索引数组中的起始位置
        int indexCount;    // 索引数量
        Vec3 displacement; // 爆炸时的位移
    };

    // 计算切割平面与网格的交线
    PlaneIntersection computePlaneIntersections(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes);

    // 三角形与平面相交检测
    bool trianglePlaneIntersection(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        const CuttingPlane &plane,
        IntersectionSegment &segment);

    // 计算点到平面的距离
    float pointToPlaneDistance(const Vertex &v, const CuttingPlane &plane);

    // 创建用于显示交线的VAO/VBO
    void setupIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO);

    // 更新交线VAO/VBO
    void updateIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO);

    // 计算模型爆炸分段
    std::vector<ModelSegment> computeModelSegments(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance);

    // 计算顶点在爆炸轴上的投影
    float projectVertexOnAxis(const Vertex &vertex, const Vec3 &axis);

    // 设置爆炸后模型的VAO/VBO/EBO
    void setupExplodedMesh(
        const Mesh &mesh,
        const std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO);

    // 更新爆炸状态
    void updateExplosionState(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance,
        bool explodeEnabled,
        std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO);

} // namespace MC