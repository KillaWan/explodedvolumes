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

} // namespace MC