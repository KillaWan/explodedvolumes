#pragma once

#include "data.h"
#include "cutting_planes.h"
#include <vector>
#include <glm/glm.hpp>

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

namespace MC
{
    // 表示爆炸模型中的一个片段
    struct ExplodedSegment
    {
        std::vector<Vertex> vertices;   // 片段的顶点
        std::vector<IndexType> indices; // 片段的索引
        Vec3 center;                    // 片段的中心点
        Vec3 displacement;              // 爆炸视图中该片段的位移
        unsigned int VAO = 0;           // 渲染所需的VAO
        unsigned int VBO = 0;           // 渲染所需的VBO
        unsigned int EBO = 0;           // 渲染所需的EBO

        // VTK多边形数据指针，用于高级处理
        vtkSmartPointer<vtkPolyData> vtkPolyData;
    };

    // 爆炸视图数据结构
    struct ExplodedView
    {
        std::vector<ExplodedSegment> segments; // 切割后的所有片段
        bool enabled = false;                  // 是否启用爆炸视图
        float explosionDistance = 2.0f;        // 爆炸后片段之间的距离
    };

    // 计算爆炸视图
    ExplodedView computeExplodedView(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis);

    // 更新爆炸视图的位移
    void updateExplodedViewDisplacements(
        ExplodedView &explodedView,
        const Vec3 &explosionAxis,
        float explosionDistance);

    // 设置片段的VAO/VBO/EBO
    void setupSegmentMesh(ExplodedSegment &segment);

    // 更新片段的网格数据
    void updateSegmentMesh(ExplodedSegment &segment);

    // 清理爆炸视图资源
    void cleanupExplodedView(ExplodedView &explodedView);

} // namespace MC