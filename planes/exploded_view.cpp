#include "planes/exploded_view.h"
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

    // 将三角形分配到相应的段 - 使用空间哈希表优化
    void assignTriangleToSegment(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        int segmentIndex,
        std::vector<ExplodedSegment> &segments,
        std::vector<std::unordered_map<Vertex, size_t, VertexHash, VertexEqual>> &vertexMaps)
    {
        ExplodedSegment &segment = segments[segmentIndex];
        auto &vertexMap = vertexMaps[segmentIndex];
        std::array<size_t, 3> newIndices;

        for (int i = 0; i < 3; i++)
        {
            const Vertex &v = (i == 0) ? v0 : ((i == 1) ? v1 : v2);

            // 使用哈希表查找顶点，时间复杂度O(1)
            auto it = vertexMap.find(v);
            if (it != vertexMap.end())
            {
                // 使用现有顶点
                newIndices[i] = it->second;
            }
            else
            {
                // 添加新顶点
                newIndices[i] = segment.vertices.size();
                segment.vertices.push_back(v);
                vertexMap[v] = newIndices[i];
            }
        }

        segment.indices.push_back(newIndices[0]);
        segment.indices.push_back(newIndices[1]);
        segment.indices.push_back(newIndices[2]);
    }

    // 计算段的中心点
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

    // 优化的计算爆炸视图函数 - 一体化处理
    ExplodedView computeExplodedView(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explosionDistance)
    {
        ExplodedView result;
        result.explosionDistance = explosionDistance; // 存储爆炸距离

        // 计时开始
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

        // 创建顶点映射数组 - 每个段一个哈希表
        std::vector<std::unordered_map<Vertex, size_t, VertexHash, VertexEqual>>
            vertexMaps(planes.size() + 1);

        // 预计算顶点在爆炸轴上的投影，避免重复计算
        std::vector<float> vertexProjections(mesh.vertices.size());

#pragma omp parallel for
        for (size_t i = 0; i < mesh.vertices.size(); i++)
        {
            vertexProjections[i] = projectVertexOnAxis(mesh.vertices[i], explosionAxis);
        }

        // 预先为每个片段的顶点和索引容器分配空间
        for (auto &segment : result.segments)
        {
            segment.vertices.reserve(mesh.vertices.size() / result.segments.size() * 2);
            segment.indices.reserve(mesh.indices.size() / result.segments.size() * 2);
        }

        // 遍历所有三角形，将其分配到相应的片段
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            const Vertex &v0 = mesh.vertices[mesh.indices[i]];
            const Vertex &v1 = mesh.vertices[mesh.indices[i + 1]];
            const Vertex &v2 = mesh.vertices[mesh.indices[i + 2]];

            // 使用预计算的投影
            float p0 = vertexProjections[mesh.indices[i]];
            float p1 = vertexProjections[mesh.indices[i + 1]];
            float p2 = vertexProjections[mesh.indices[i + 2]];

            // 找到三角形中心在爆炸轴上的投影
            float centerProj = (p0 + p1 + p2) / 3.0f;

            // 确定该三角形属于哪个片段 - 使用二分查找优化
            auto it = std::lower_bound(
                sortedPlaneIndices.begin(),
                sortedPlaneIndices.end(),
                std::make_pair(centerProj, static_cast<size_t>(0)));

            int segmentIndex = static_cast<int>(std::distance(sortedPlaneIndices.begin(), it));

            // 分配三角形到相应的片段
            assignTriangleToSegment(v0, v1, v2, segmentIndex, result.segments, vertexMaps);
        }

        // 清理不再需要的哈希表，释放内存
        vertexMaps.clear();

// 计算每个片段的中心点
#pragma omp parallel for
        for (size_t i = 0; i < result.segments.size(); i++)
        {
            result.segments[i].center = computeSegmentCenter(result.segments[i]);
        }

        // 直接在此函数中计算位移，而不是调用单独的函数
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

            // 设置中心片段位移为0
            result.segments[centerSegmentIndex].displacement = Vec3(0, 0, 0);

// 计算其他片段的位移
// 向左（爆炸轴负方向）的片段
#pragma omp parallel for
            for (int i = 0; i < centerSegmentIndex; i++)
            {
                float distance = explosionDistance * (centerSegmentIndex - i);
                result.segments[i].displacement.x = -explosionAxis.x * distance;
                result.segments[i].displacement.y = -explosionAxis.y * distance;
                result.segments[i].displacement.z = -explosionAxis.z * distance;
            }

// 向右（爆炸轴正方向）的片段
#pragma omp parallel for
            for (int i = centerSegmentIndex + 1; i < numSegments; i++)
            {
                float distance = explosionDistance * (i - centerSegmentIndex);
                result.segments[i].displacement.x = explosionAxis.x * distance;
                result.segments[i].displacement.y = explosionAxis.y * distance;
                result.segments[i].displacement.z = explosionAxis.z * distance;
            }
        }

// 设置每个片段的OpenGL缓冲
#pragma omp parallel for
        for (size_t i = 0; i < result.segments.size(); i++)
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
// #include "planes/exploded_view.h"
// #include <algorithm>
// #include <iostream>
// #include <unordered_map>
// #include <unordered_set>
// #include <cmath>
// #include <glad/glad.h>

// namespace MC
// {
//     // 比较两个顶点是否近似相等
//     bool verticesEqual(const Vertex &v1, const Vertex &v2, float epsilon = 1e-5f)
//     {
//         return std::abs(v1.x - v2.x) < epsilon &&
//                std::abs(v1.y - v2.y) < epsilon &&
//                std::abs(v1.z - v2.z) < epsilon;
//     }

//     // 将三角形分配到相应的段
//     void assignTriangleToSegment(
//         const Vertex &v0, const Vertex &v1, const Vertex &v2,
//         int segmentIndex,
//         std::vector<ExplodedSegment> &segments,
//         std::unordered_map<int, std::unordered_map<size_t, size_t>> &vertexMaps)
//     {
//         ExplodedSegment &segment = segments[segmentIndex];
//         auto &vertexMap = vertexMaps[segmentIndex];
//         std::array<size_t, 3> newIndices;

//         for (int i = 0; i < 3; i++)
//         {
//             const Vertex &v = (i == 0) ? v0 : ((i == 1) ? v1 : v2);
//             bool found = false;
//             size_t existingIndex = 0;

//             for (size_t j = 0; j < segment.vertices.size(); j++)
//             {
//                 if (verticesEqual(segment.vertices[j], v))
//                 {
//                     found = true;
//                     existingIndex = j;
//                     break;
//                 }
//             }

//             if (found)
//             {
//                 newIndices[i] = existingIndex;
//             }
//             else
//             {
//                 newIndices[i] = segment.vertices.size();
//                 segment.vertices.push_back(v);
//             }
//         }

//         segment.indices.push_back(newIndices[0]);
//         segment.indices.push_back(newIndices[1]);
//         segment.indices.push_back(newIndices[2]);
//     }

//     // 计算段的中心点
//     Vec3 computeSegmentCenter(const ExplodedSegment &segment)
//     {
//         if (segment.vertices.empty())
//         {
//             return Vec3(0, 0, 0);
//         }

//         Vec3 center(0, 0, 0);
//         for (const auto &v : segment.vertices)
//         {
//             center.x += v.x;
//             center.y += v.y;
//             center.z += v.z;
//         }

//         float count = static_cast<float>(segment.vertices.size());
//         center.x /= count;
//         center.y /= count;
//         center.z /= count;

//         return center;
//     }

//     // 修改后的计算爆炸视图函数 - 一体化处理
//     ExplodedView computeExplodedView(
//         const Mesh &mesh,
//         const std::vector<CuttingPlane> &planes,
//         const Vec3 &explosionAxis,
//         float explosionDistance) // 添加默认值参数
//     {
//         ExplodedView result;
//         result.explosionDistance = explosionDistance; // 存储爆炸距离

//         // 如果没有切割平面，则整个模型作为一个片段
//         if (planes.empty())
//         {
//             ExplodedSegment segment;
//             segment.vertices = mesh.vertices;
//             segment.indices = mesh.indices;
//             segment.center = mesh.center;
//             segment.displacement = Vec3(0, 0, 0);

//             result.segments.push_back(segment);
//             return result;
//         }

//         // 对切割平面按照在爆炸轴上的位置排序
//         std::vector<std::pair<float, size_t>> sortedPlaneIndices;
//         for (size_t i = 0; i < planes.size(); i++)
//         {
//             float proj = planes[i].distance;
//             sortedPlaneIndices.push_back({proj, i});
//         }

//         std::sort(sortedPlaneIndices.begin(), sortedPlaneIndices.end());

//         // 初始化片段（n个平面会产生n+1个片段）
//         result.segments.resize(planes.size() + 1);
//         std::unordered_map<int, std::unordered_map<size_t, size_t>> vertexMaps;

//         // 遍历所有三角形，将其分配到相应的片段
//         for (size_t i = 0; i < mesh.indices.size(); i += 3)
//         {
//             const Vertex &v0 = mesh.vertices[mesh.indices[i]];
//             const Vertex &v1 = mesh.vertices[mesh.indices[i + 1]];
//             const Vertex &v2 = mesh.vertices[mesh.indices[i + 2]];

//             // 计算三角形三个顶点在爆炸轴上的投影
//             float p0 = projectVertexOnAxis(v0, explosionAxis);
//             float p1 = projectVertexOnAxis(v1, explosionAxis);
//             float p2 = projectVertexOnAxis(v2, explosionAxis);

//             // 找到三角形中心在爆炸轴上的投影
//             float centerProj = (p0 + p1 + p2) / 3.0f;

//             // 确定该三角形属于哪个片段
//             int segmentIndex = 0;
//             while (segmentIndex < sortedPlaneIndices.size() &&
//                    centerProj > planes[sortedPlaneIndices[segmentIndex].second].distance)
//             {
//                 segmentIndex++;
//             }

//             // 分配三角形到相应的片段
//             assignTriangleToSegment(v0, v1, v2, segmentIndex, result.segments, vertexMaps);
//         }

//         // 计算每个片段的中心点
//         for (auto &segment : result.segments)
//         {
//             segment.center = computeSegmentCenter(segment);
//         }

//         // 直接在此函数中计算位移，而不是调用单独的函数
//         int numSegments = result.segments.size();
//         if (numSegments <= 1)
//         {
//             // 只有一个片段，不需要爆炸
//             if (!result.segments.empty())
//             {
//                 result.segments[0].displacement = Vec3(0, 0, 0);
//             }
//         }
//         else
//         {
//             // 确定固定的中心片段
//             int centerSegmentIndex;
//             if (numSegments % 2 == 1)
//             {
//                 // 奇数个片段，中间片段固定
//                 centerSegmentIndex = numSegments / 2;
//             }
//             else
//             {
//                 // 偶数个片段，选择中间两个的第一个固定
//                 centerSegmentIndex = numSegments / 2 - 1;
//             }

//             // 设置中心片段位移为0
//             result.segments[centerSegmentIndex].displacement = Vec3(0, 0, 0);

//             // 计算其他片段的位移
//             // 向左（爆炸轴负方向）的片段
//             for (int i = centerSegmentIndex - 1; i >= 0; i--)
//             {
//                 float distance = explosionDistance * (centerSegmentIndex - i);
//                 result.segments[i].displacement.x = -explosionAxis.x * distance;
//                 result.segments[i].displacement.y = -explosionAxis.y * distance;
//                 result.segments[i].displacement.z = -explosionAxis.z * distance;
//             }

//             // 向右（爆炸轴正方向）的片段
//             for (int i = centerSegmentIndex + 1; i < numSegments; i++)
//             {
//                 float distance = explosionDistance * (i - centerSegmentIndex);
//                 result.segments[i].displacement.x = explosionAxis.x * distance;
//                 result.segments[i].displacement.y = explosionAxis.y * distance;
//                 result.segments[i].displacement.z = explosionAxis.z * distance;
//             }
//         }

//         // 设置每个片段的OpenGL缓冲
//         for (auto &segment : result.segments)
//         {
//             setupSegmentMesh(segment);
//         }

//         return result;
//     }

//     // 保留这个函数用于后续更新爆炸距离
//     void updateExplodedViewDisplacements(
//         ExplodedView &explodedView,
//         const Vec3 &explosionAxis,
//         float explosionDistance)
//     {
//         explodedView.explosionDistance = explosionDistance;
//         auto &segments = explodedView.segments;
//         int numSegments = segments.size();

//         if (numSegments <= 1)
//         {
//             // 只有一个片段，不需要爆炸
//             if (!segments.empty())
//             {
//                 segments[0].displacement = Vec3(0, 0, 0);
//             }
//             return;
//         }

//         // 确定固定的中心片段
//         int centerSegmentIndex;
//         if (numSegments % 2 == 1)
//         {
//             // 奇数个片段，中间片段固定
//             centerSegmentIndex = numSegments / 2;
//         }
//         else
//         {
//             // 偶数个片段，选择中间两个的第一个固定
//             centerSegmentIndex = numSegments / 2 - 1;
//         }

//         // 设置中心片段位移为0
//         segments[centerSegmentIndex].displacement = Vec3(0, 0, 0);

//         // 计算其他片段的位移
//         // 向左（爆炸轴负方向）的片段
//         for (int i = centerSegmentIndex - 1; i >= 0; i--)
//         {
//             float distance = explosionDistance * (centerSegmentIndex - i);
//             segments[i].displacement.x = -explosionAxis.x * distance;
//             segments[i].displacement.y = -explosionAxis.y * distance;
//             segments[i].displacement.z = -explosionAxis.z * distance;
//         }

//         // 向右（爆炸轴正方向）的片段
//         for (int i = centerSegmentIndex + 1; i < numSegments; i++)
//         {
//             float distance = explosionDistance * (i - centerSegmentIndex);
//             segments[i].displacement.x = explosionAxis.x * distance;
//             segments[i].displacement.y = explosionAxis.y * distance;
//             segments[i].displacement.z = explosionAxis.z * distance;
//         }

//         // 更新每个片段的OpenGL缓冲
//         for (auto &segment : segments)
//         {
//             updateSegmentMesh(segment);
//         }
//     }

//     // 设置片段的VAO/VBO/EBO
//     void setupSegmentMesh(ExplodedSegment &segment)
//     {
//         if (segment.vertices.empty() || segment.indices.empty())
//         {
//             return;
//         }

//         glGenVertexArrays(1, &segment.VAO);
//         glGenBuffers(1, &segment.VBO);
//         glGenBuffers(1, &segment.EBO);

//         glBindVertexArray(segment.VAO);

//         glBindBuffer(GL_ARRAY_BUFFER, segment.VBO);
//         glBufferData(GL_ARRAY_BUFFER, segment.vertices.size() * sizeof(Vertex),
//                      segment.vertices.data(), GL_STATIC_DRAW);

//         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, segment.EBO);
//         glBufferData(GL_ELEMENT_ARRAY_BUFFER, segment.indices.size() * sizeof(IndexType),
//                      segment.indices.data(), GL_STATIC_DRAW);

//         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
//         glEnableVertexAttribArray(0);

//         glBindVertexArray(0);
//     }

//     // 更新片段的网格数据
//     void updateSegmentMesh(ExplodedSegment &segment)
//     {
//         if (segment.vertices.empty() || segment.indices.empty())
//         {
//             return;
//         }

//         // 应用位移到顶点
//         std::vector<Vertex> displacedVertices = segment.vertices;
//         for (auto &v : displacedVertices)
//         {
//             v.x += segment.displacement.x;
//             v.y += segment.displacement.y;
//             v.z += segment.displacement.z;
//         }

//         glBindBuffer(GL_ARRAY_BUFFER, segment.VBO);
//         glBufferData(GL_ARRAY_BUFFER, displacedVertices.size() * sizeof(Vertex),
//                      displacedVertices.data(), GL_STATIC_DRAW);
//     }

//     // 清理爆炸视图资源
//     void cleanupExplodedView(ExplodedView &explodedView)
//     {
//         for (auto &segment : explodedView.segments)
//         {
//             if (segment.VAO != 0)
//             {
//                 glDeleteVertexArrays(1, &segment.VAO);
//                 segment.VAO = 0;
//             }
//             if (segment.VBO != 0)
//             {
//                 glDeleteBuffers(1, &segment.VBO);
//                 segment.VBO = 0;
//             }
//             if (segment.EBO != 0)
//             {
//                 glDeleteBuffers(1, &segment.EBO);
//                 segment.EBO = 0;
//             }
//         }a

//         explodedView.segments.clear();
//     }

// } // namespace MC
