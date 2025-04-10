#include "planes/exploded_view.h"
#include "planes/cutting_planes.h" // 引入切割平面定义，可使用其中的interpolateVertex函数
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <glad/glad.h>
#ifdef _WIN32
#include <array>
#endif

namespace MC
{
    // 声明外部函数
    extern float pointToPlaneDistance(const Vertex &v, const CuttingPlane &plane);
    extern Vertex interpolateVertex(const Vertex &v1, const Vertex &v2, float t);
    // 顶点哈希结构 - 用于快速查找顶点
    struct VertexHash
    {
        float epsilon;

        VertexHash(float e = 1e-5f) : epsilon(e) {}

        size_t operator()(const Vertex &v) const
        {
            // 将坐标量化以减少浮点误差影响
            int x = static_cast<int>(v.x / epsilon);
            int y = static_cast<int>(v.y / epsilon);
            int z = static_cast<int>(v.z / epsilon);

            // 使用空间哈希函数组合坐标
            // 这些质数有助于减少哈希冲突
            return static_cast<size_t>(x * 73856093 ^ y * 19349663 ^ z * 83492791);
        }
    };

    // 顶点近似相等判断
    struct VertexEqual
    {
        float epsilon;

        VertexEqual(float e = 1e-5f) : epsilon(e) {}

        bool operator()(const Vertex &v1, const Vertex &v2) const
        {
            return std::abs(v1.x - v2.x) < epsilon &&
                   std::abs(v1.y - v2.y) < epsilon &&
                   std::abs(v1.z - v2.z) < epsilon;
        }
    };

    // 计算点到平面的有符号距离 - 使用已有的pointToPlaneDistance函数
    float signedDistanceToPlane(const Vertex &vertex, const CuttingPlane &plane)
    {
        return pointToPlaneDistance(vertex, plane);
    }

    // 不再定义interpolateVertex函数，而是使用cutting_planes.cpp中的版本
    // Vertex interpolateVertex(const Vertex& v1, const Vertex& v2, float t) 函数在cutting_planes.cpp中已定义

    // 三角形切割函数 - 将一个三角形根据平面切割成多个三角形并分配到两侧
    void cutTriangleByPlane(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        const CuttingPlane &plane,
        std::vector<Vertex> &positiveVertices,
        std::vector<IndexType> &positiveIndices,
        std::vector<Vertex> &negativeVertices,
        std::vector<IndexType> &negativeIndices,
        std::unordered_map<Vertex, size_t, VertexHash, VertexEqual> &positiveVertexMap,
        std::unordered_map<Vertex, size_t, VertexHash, VertexEqual> &negativeVertexMap)
    {
        // 计算每个顶点到平面的有符号距离
        float d0 = signedDistanceToPlane(v0, plane);
        float d1 = signedDistanceToPlane(v1, plane);
        float d2 = signedDistanceToPlane(v2, plane);

        // 定义一个小的epsilon值，用于判断顶点是否在平面上
        const float epsilon = 1e-6f;

        // 确定每个顶点相对于平面的位置
        bool v0Positive = d0 > epsilon;
        bool v1Positive = d1 > epsilon;
        bool v2Positive = d2 > epsilon;
        bool v0OnPlane = std::abs(d0) <= epsilon;
        bool v1OnPlane = std::abs(d1) <= epsilon;
        bool v2OnPlane = std::abs(d2) <= epsilon;

        int positiveCount = (v0Positive ? 1 : 0) + (v1Positive ? 1 : 0) + (v2Positive ? 1 : 0);
        int onPlaneCount = (v0OnPlane ? 1 : 0) + (v1OnPlane ? 1 : 0) + (v2OnPlane ? 1 : 0);

        // 添加顶点到相应的集合，并返回索引
        auto addVertexToCollection = [](
                                         const Vertex &v,
                                         std::vector<Vertex> &vertices,
                                         std::unordered_map<Vertex, size_t, VertexHash, VertexEqual> &vertexMap) -> size_t
        {
            auto it = vertexMap.find(v);
            if (it != vertexMap.end())
            {
                return it->second;
            }

            size_t index = vertices.size();
            vertices.push_back(v);
            vertexMap[v] = index;
            return index;
        };

        // 处理不同的情况
        if (onPlaneCount == 3 || (positiveCount == 0 && onPlaneCount == 0) || (positiveCount == 3))
        {
            // 1. 三角形完全在平面上，或者完全在一侧
            const std::vector<Vertex> &verts = {v0, v1, v2};
            auto &targetVertices = (positiveCount > 0 || onPlaneCount == 3) ? positiveVertices : negativeVertices;
            auto &targetIndices = (positiveCount > 0 || onPlaneCount == 3) ? positiveIndices : negativeIndices;
            auto &targetMap = (positiveCount > 0 || onPlaneCount == 3) ? positiveVertexMap : negativeVertexMap;

            std::array<size_t, 3> indices;
            for (int i = 0; i < 3; ++i)
            {
                indices[i] = addVertexToCollection(verts[i], targetVertices, targetMap);
            }

            targetIndices.push_back(indices[0]);
            targetIndices.push_back(indices[1]);
            targetIndices.push_back(indices[2]);
            return;
        }

        // 2. 三角形被平面切割
        // 将顶点按照其相对于平面的位置分类
        std::vector<Vertex> posVerts, negVerts, onPlaneVerts;
        std::vector<int> posIndices, negIndices, onPlaneIndices;

        if (v0OnPlane)
        {
            onPlaneVerts.push_back(v0);
            onPlaneIndices.push_back(0);
        }
        else if (v0Positive)
        {
            posVerts.push_back(v0);
            posIndices.push_back(0);
        }
        else
        {
            negVerts.push_back(v0);
            negIndices.push_back(0);
        }

        if (v1OnPlane)
        {
            onPlaneVerts.push_back(v1);
            onPlaneIndices.push_back(1);
        }
        else if (v1Positive)
        {
            posVerts.push_back(v1);
            posIndices.push_back(1);
        }
        else
        {
            negVerts.push_back(v1);
            negIndices.push_back(1);
        }

        if (v2OnPlane)
        {
            onPlaneVerts.push_back(v2);
            onPlaneIndices.push_back(2);
        }
        else if (v2Positive)
        {
            posVerts.push_back(v2);
            posIndices.push_back(2);
        }
        else
        {
            negVerts.push_back(v2);
            negIndices.push_back(2);
        }

        // 计算平面与三角形边的交点
        std::vector<Vertex> intersections;

        auto calculateIntersection = [&](const Vertex &va, const Vertex &vb, float da, float db)
        {
            if (std::abs(da - db) < epsilon)
                return;
            float t = da / (da - db);
            if (t >= 0.0f && t <= 1.0f)
            {
                intersections.push_back(interpolateVertex(va, vb, t));
            }
        };

        if (!v0OnPlane && !v1OnPlane && (v0Positive != v1Positive))
        {
            calculateIntersection(v0, v1, d0, d1);
        }

        if (!v1OnPlane && !v2OnPlane && (v1Positive != v2Positive))
        {
            calculateIntersection(v1, v2, d1, d2);
        }

        if (!v2OnPlane && !v0OnPlane && (v2Positive != v0Positive))
        {
            calculateIntersection(v2, v0, d2, d0);
        }

        // 移除重复的交点
        if (intersections.size() > 1)
        {
            for (size_t i = 0; i < intersections.size(); ++i)
            {
                for (size_t j = i + 1; j < intersections.size();)
                {
                    if (VertexEqual()(intersections[i], intersections[j]))
                    {
                        intersections.erase(intersections.begin() + j);
                    }
                    else
                    {
                        ++j;
                    }
                }
            }
        }

        // 合并平面上的点和交点
        for (const auto &v : onPlaneVerts)
        {
            intersections.push_back(v);
        }

        // 确保至少有两个交点
        if (intersections.size() < 2)
        {
            // 可能是精度问题导致的边缘情况，将完整三角形分配到一侧
            const std::vector<Vertex> &verts = {v0, v1, v2};
            auto &targetVertices = (positiveCount >= 2) ? positiveVertices : negativeVertices;
            auto &targetIndices = (positiveCount >= 2) ? positiveIndices : negativeIndices;
            auto &targetMap = (positiveCount >= 2) ? positiveVertexMap : negativeVertexMap;

            std::array<size_t, 3> indices;
            for (int i = 0; i < 3; ++i)
            {
                indices[i] = addVertexToCollection(verts[i], targetVertices, targetMap);
            }

            targetIndices.push_back(indices[0]);
            targetIndices.push_back(indices[1]);
            targetIndices.push_back(indices[2]);
            return;
        }

        // 根据交点和分类的顶点生成新的三角形
        // 1. Positive side
        if (!posVerts.empty() || !onPlaneVerts.empty())
        {
            std::vector<Vertex> allPosVerts(posVerts);
            allPosVerts.insert(allPosVerts.end(), intersections.begin(), intersections.end());

            // 简化处理：假设顶点数目较少，可以直接生成三角形
            if (allPosVerts.size() == 3)
            {
                // 一个简单的三角形
                size_t idx0 = addVertexToCollection(allPosVerts[0], positiveVertices, positiveVertexMap);
                size_t idx1 = addVertexToCollection(allPosVerts[1], positiveVertices, positiveVertexMap);
                size_t idx2 = addVertexToCollection(allPosVerts[2], positiveVertices, positiveVertexMap);

                positiveIndices.push_back(idx0);
                positiveIndices.push_back(idx1);
                positiveIndices.push_back(idx2);
            }
            else if (allPosVerts.size() == 4)
            {
                // 一个四边形，需要分成两个三角形
                size_t idx0 = addVertexToCollection(allPosVerts[0], positiveVertices, positiveVertexMap);
                size_t idx1 = addVertexToCollection(allPosVerts[1], positiveVertices, positiveVertexMap);
                size_t idx2 = addVertexToCollection(allPosVerts[2], positiveVertices, positiveVertexMap);
                size_t idx3 = addVertexToCollection(allPosVerts[3], positiveVertices, positiveVertexMap);

                // 三角形1
                positiveIndices.push_back(idx0);
                positiveIndices.push_back(idx1);
                positiveIndices.push_back(idx2);

                // 三角形2
                positiveIndices.push_back(idx0);
                positiveIndices.push_back(idx2);
                positiveIndices.push_back(idx3);
            }
            // 可以根据需要扩展处理更复杂的多边形
        }

        // 2. Negative side
        if (!negVerts.empty() || !onPlaneVerts.empty())
        {
            std::vector<Vertex> allNegVerts(negVerts);
            allNegVerts.insert(allNegVerts.end(), intersections.begin(), intersections.end());

            if (allNegVerts.size() == 3)
            {
                // 一个简单的三角形
                size_t idx0 = addVertexToCollection(allNegVerts[0], negativeVertices, negativeVertexMap);
                size_t idx1 = addVertexToCollection(allNegVerts[1], negativeVertices, negativeVertexMap);
                size_t idx2 = addVertexToCollection(allNegVerts[2], negativeVertices, negativeVertexMap);

                negativeIndices.push_back(idx0);
                negativeIndices.push_back(idx1);
                negativeIndices.push_back(idx2);
            }
            else if (allNegVerts.size() == 4)
            {
                // 一个四边形，需要分成两个三角形
                size_t idx0 = addVertexToCollection(allNegVerts[0], negativeVertices, negativeVertexMap);
                size_t idx1 = addVertexToCollection(allNegVerts[1], negativeVertices, negativeVertexMap);
                size_t idx2 = addVertexToCollection(allNegVerts[2], negativeVertices, negativeVertexMap);
                size_t idx3 = addVertexToCollection(allNegVerts[3], negativeVertices, negativeVertexMap);

                // 三角形1
                negativeIndices.push_back(idx0);
                negativeIndices.push_back(idx1);
                negativeIndices.push_back(idx2);

                // 三角形2
                negativeIndices.push_back(idx0);
                negativeIndices.push_back(idx2);
                negativeIndices.push_back(idx3);
            }
            // 可以根据需要扩展处理更复杂的多边形
        }
    }

    // Segment center
    Vec3 computeSegmentCenter(const ExplodedSegment &segment)
    {
        if (segment.vertices.empty())
        {
            return Vec3(0, 0, 0);
        }

        Vec3 center(0, 0, 0);
        for (const auto &v : segment.vertices)
        {
            center.x += v.x;
            center.y += v.y;
            center.z += v.z;
        }

        float count = static_cast<float>(segment.vertices.size());
        center.x /= count;
        center.y /= count;
        center.z /= count;

        return center;
    }

    // 优化的计算爆炸视图函数 - 实现真正的网格切割
    ExplodedView computeExplodedView(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explosionDistance)
    {
        ExplodedView result;
        result.explosionDistance = explosionDistance;

        // Start timing
        auto startTime = std::chrono::high_resolution_clock::now();

        // 如果没有切割平面，则整个模型作为一个片段
        if (planes.empty())
        {
            ExplodedSegment segment;
            segment.vertices = mesh.vertices;
            segment.indices = mesh.indices;
            segment.center = mesh.center;
            segment.displacement = Vec3(0, 0, 0);

            result.segments.push_back(segment);

            // 设置OpenGL缓冲
            setupSegmentMesh(segment);

            return result;
        }

        // 对切割平面按照在爆炸轴上的位置排序
        std::vector<std::pair<float, size_t>> sortedPlaneIndices;
        sortedPlaneIndices.reserve(planes.size());

        for (size_t i = 0; i < planes.size(); i++)
        {
            float proj = planes[i].distance;
            sortedPlaneIndices.emplace_back(proj, i);
        }

        std::sort(sortedPlaneIndices.begin(), sortedPlaneIndices.end());

        // 初始化片段（n个平面会产生n+1个片段）
        result.segments.resize(planes.size() + 1);

        // 为每个片段创建顶点映射
        std::vector<std::unordered_map<Vertex, size_t, VertexHash, VertexEqual>> vertexMaps(planes.size() + 1);

        // 使用多阶段切割算法
        // 首先，将整个网格视为一个片段
        std::vector<Vertex> currentVertices = mesh.vertices;
        std::vector<IndexType> currentIndices = mesh.indices;

        // 对每个切割平面进行处理
        for (size_t planeIdx = 0; planeIdx < sortedPlaneIndices.size(); ++planeIdx)
        {
            const CuttingPlane &plane = planes[sortedPlaneIndices[planeIdx].second];

            // 切割后的两部分（平面正面和负面）
            std::vector<Vertex> positiveVertices, negativeVertices;
            std::vector<IndexType> positiveIndices, negativeIndices;
            std::unordered_map<Vertex, size_t, VertexHash, VertexEqual> posVertexMap, negVertexMap;

            // 对当前片段中的每个三角形进行切割
            for (size_t i = 0; i < currentIndices.size(); i += 3)
            {
                const Vertex &v0 = currentVertices[currentIndices[i]];
                const Vertex &v1 = currentVertices[currentIndices[i + 1]];
                const Vertex &v2 = currentVertices[currentIndices[i + 2]];

                // 切割三角形并将结果添加到正面和负面集合中
                cutTriangleByPlane(v0, v1, v2, plane,
                                   positiveVertices, positiveIndices,
                                   negativeVertices, negativeIndices,
                                   posVertexMap, negVertexMap);
            }

            // 将正面部分保存为当前片段，继续处理下一个平面
            result.segments[planeIdx].vertices = std::move(negativeVertices);
            result.segments[planeIdx].indices = std::move(negativeIndices);

            // 更新当前片段为切割后的正面部分
            currentVertices = std::move(positiveVertices);
            currentIndices = std::move(positiveIndices);
        }

        // 最后一个片段是最后一次切割后剩余的正面部分
        result.segments[planes.size()].vertices = std::move(currentVertices);
        result.segments[planes.size()].indices = std::move(currentIndices);

// 计算每个片段的中心点
#pragma omp parallel for
        for (size_t i = 0; i < result.segments.size(); ++i)
        {
            if (!result.segments[i].vertices.empty())
            {
                result.segments[i].center = computeSegmentCenter(result.segments[i]);
            }
        }

        // 计算爆炸位移
        int numSegments = result.segments.size();
        if (numSegments <= 1)
        {
            // 只有一个片段，不需要爆炸
            if (!result.segments.empty())
            {
                result.segments[0].displacement = Vec3(0, 0, 0);
            }
        }
        else
        {
            // 确定固定的中心片段
            int centerSegmentIndex;
            if (numSegments % 2 == 1)
            {
                // 奇数个片段，中间片段固定
                centerSegmentIndex = numSegments / 2;
            }
            else
            {
                // 偶数个片段，选择中间两个的第一个固定
                centerSegmentIndex = numSegments / 2 - 1;
            }

// 设置各片段的位移
#pragma omp parallel for
            for (int i = 0; i < numSegments; ++i)
            {
                if (i < centerSegmentIndex)
                {
                    // 向左（爆炸轴负方向）的片段
                    float distance = explosionDistance * (centerSegmentIndex - i);
                    result.segments[i].displacement.x = -explosionAxis.x * distance;
                    result.segments[i].displacement.y = -explosionAxis.y * distance;
                    result.segments[i].displacement.z = -explosionAxis.z * distance;
                }
                else if (i > centerSegmentIndex)
                {
                    // 向右（爆炸轴正方向）的片段
                    float distance = explosionDistance * (i - centerSegmentIndex);
                    result.segments[i].displacement.x = explosionAxis.x * distance;
                    result.segments[i].displacement.y = explosionAxis.y * distance;
                    result.segments[i].displacement.z = explosionAxis.z * distance;
                }
                else
                {
                    // 中心片段
                    result.segments[i].displacement = Vec3(0, 0, 0);
                }
            }
        }

        // 移除空的片段
        result.segments.erase(
            std::remove_if(result.segments.begin(), result.segments.end(),
                           [](const ExplodedSegment &segment)
                           {
                               return segment.vertices.empty() || segment.indices.empty();
                           }),
            result.segments.end());

// 设置每个片段的OpenGL缓冲
#pragma omp parallel for
        for (size_t i = 0; i < result.segments.size(); ++i)
        {
            setupSegmentMesh(result.segments[i]);
        }

        // 计时结束
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "Exploded view computation completed in " << duration
                  << " ms with " << result.segments.size() << " segments." << std::endl;

        return result;
    }

    // 更新爆炸视图的位移
    void updateExplodedViewDisplacements(
        ExplodedView &explodedView,
        const Vec3 &explosionAxis,
        float explosionDistance)
    {
        explodedView.explosionDistance = explosionDistance;
        auto &segments = explodedView.segments;
        int numSegments = segments.size();

        if (numSegments <= 1)
        {
            // 只有一个片段，不需要爆炸
            if (!segments.empty())
            {
                segments[0].displacement = Vec3(0, 0, 0);
            }
            return;
        }

        // 确定固定的中心片段
        int centerSegmentIndex;
        if (numSegments % 2 == 1)
        {
            // 奇数个片段，中间片段固定
            centerSegmentIndex = numSegments / 2;
        }
        else
        {
            // 偶数个片段，选择中间两个的第一个固定
            centerSegmentIndex = numSegments / 2 - 1;
        }

        // 设置中心片段位移为0
        segments[centerSegmentIndex].displacement = Vec3(0, 0, 0);

// 计算其他片段的位移 - 并行处理
#pragma omp parallel sections
        {
#pragma omp section
            {
                // 向左（爆炸轴负方向）的片段
                for (int i = centerSegmentIndex - 1; i >= 0; i--)
                {
                    float distance = explosionDistance * (centerSegmentIndex - i);
                    segments[i].displacement.x = -explosionAxis.x * distance;
                    segments[i].displacement.y = -explosionAxis.y * distance;
                    segments[i].displacement.z = -explosionAxis.z * distance;
                }
            }

#pragma omp section
            {
                // 向右（爆炸轴正方向）的片段
                for (int i = centerSegmentIndex + 1; i < numSegments; i++)
                {
                    float distance = explosionDistance * (i - centerSegmentIndex);
                    segments[i].displacement.x = explosionAxis.x * distance;
                    segments[i].displacement.y = explosionAxis.y * distance;
                    segments[i].displacement.z = explosionAxis.z * distance;
                }
            }
        }

// 更新每个片段的OpenGL缓冲 - 并行处理
#pragma omp parallel for
        for (size_t i = 0; i < segments.size(); i++)
        {
            updateSegmentMesh(segments[i]);
        }
    }

    // 设置片段的VAO/VBO/EBO
    void setupSegmentMesh(ExplodedSegment &segment)
    {
        if (segment.vertices.empty() || segment.indices.empty())
        {
            return;
        }

        // 如果已经有VAO，先删除
        if (segment.VAO != 0)
        {
            glDeleteVertexArrays(1, &segment.VAO);
            glDeleteBuffers(1, &segment.VBO);
            glDeleteBuffers(1, &segment.EBO);
            segment.VAO = segment.VBO = segment.EBO = 0;
        }

        glGenVertexArrays(1, &segment.VAO);
        glGenBuffers(1, &segment.VBO);
        glGenBuffers(1, &segment.EBO);

        glBindVertexArray(segment.VAO);

        // 创建包含位移的顶点数据
        std::vector<Vertex> displacedVertices = segment.vertices;
        for (auto &v : displacedVertices)
        {
            v.x += segment.displacement.x;
            v.y += segment.displacement.y;
            v.z += segment.displacement.z;
        }

        glBindBuffer(GL_ARRAY_BUFFER, segment.VBO);
        glBufferData(GL_ARRAY_BUFFER, displacedVertices.size() * sizeof(Vertex),
                     displacedVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, segment.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, segment.indices.size() * sizeof(IndexType),
                     segment.indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // 更新片段的网格数据
    void updateSegmentMesh(ExplodedSegment &segment)
    {
        if (segment.vertices.empty() || segment.indices.empty() || segment.VAO == 0)
        {
            return;
        }

        // 应用位移到顶点
        std::vector<Vertex> displacedVertices = segment.vertices;
        for (auto &v : displacedVertices)
        {
            v.x += segment.displacement.x;
            v.y += segment.displacement.y;
            v.z += segment.displacement.z;
        }

        glBindBuffer(GL_ARRAY_BUFFER, segment.VBO);
        glBufferData(GL_ARRAY_BUFFER, displacedVertices.size() * sizeof(Vertex),
                     displacedVertices.data(), GL_STATIC_DRAW);
    }

    // 清理爆炸视图资源
    void cleanupExplodedView(ExplodedView &explodedView)
    {
        for (auto &segment : explodedView.segments)
        {
            if (segment.VAO != 0)
            {
                glDeleteVertexArrays(1, &segment.VAO);
                segment.VAO = 0;
            }
            if (segment.VBO != 0)
            {
                glDeleteBuffers(1, &segment.VBO);
                segment.VBO = 0;
            }
            if (segment.EBO != 0)
            {
                glDeleteBuffers(1, &segment.EBO);
                segment.EBO = 0;
            }
        }

        explodedView.segments.clear();
    }

} // namespace MC