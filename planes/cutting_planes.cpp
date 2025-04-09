#include "planes/cutting_planes.h"
#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <glad/glad.h>
#include <omp.h>
#ifdef _WIN32
#include <mutex>
#endif

namespace MC
{

    // calculate the distance between point and plane
    float pointToPlaneDistance(const Vertex &v, const CuttingPlane &plane)
    {
        return (v.x - plane.origin.x) * plane.normal.x +
               (v.y - plane.origin.y) * plane.normal.y +
               (v.z - plane.origin.z) * plane.normal.z;
    }

    // calclute linear interpolation between two points
    Vertex interpolateVertex(const Vertex &v1, const Vertex &v2, float t)
    {
        Vertex result;
        result.x = v1.x + t * (v2.x - v1.x);
        result.y = v1.y + t * (v2.y - v1.y);
        result.z = v1.z + t * (v2.z - v1.z);
        return result;
    }

    // calculate the projection of the vertex on the explode axis
    float projectVertexOnAxis(const Vertex &vertex, const Vec3 &axis)
    {
        return vertex.x * axis.x + vertex.y * axis.y + vertex.z * axis.z;
    }


    bool trianglePlaneIntersection(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        const CuttingPlane &plane,
        IntersectionSegment &segment)
    {

        float d0 = pointToPlaneDistance(v0, plane);
        float d1 = pointToPlaneDistance(v1, plane);
        float d2 = pointToPlaneDistance(v2, plane);

        // if all points are on the same side of the plane, then there is no intersection
        if ((d0 > 0 && d1 > 0 && d2 > 0) || (d0 < 0 && d1 < 0 && d2 < 0))
        {
            return false;
        }

        bool v0OnPlane = std::abs(d0) < 1e-6f;
        bool v1OnPlane = std::abs(d1) < 1e-6f;
        bool v2OnPlane = std::abs(d2) < 1e-6f;

        std::vector<Vertex> intersections;

        // 检查每条边是否与平面相交
        if (!v0OnPlane && !v1OnPlane && d0 * d1 <= 0.0f)
        {
            // v0-v1 边与平面相交
            float t = d0 / (d0 - d1);
            intersections.push_back(interpolateVertex(v0, v1, t));
        }

        if (!v1OnPlane && !v2OnPlane && d1 * d2 <= 0.0f)
        {
            float t = d1 / (d1 - d2);
            intersections.push_back(interpolateVertex(v1, v2, t));
        }

        if (!v2OnPlane && !v0OnPlane && d2 * d0 <= 0.0f)
        {
            float t = d2 / (d2 - d0);
            intersections.push_back(interpolateVertex(v2, v0, t));
        }

        // if the vertex is on a plane
        if (v0OnPlane)
        {
            intersections.push_back(v0);
        }
        if (v1OnPlane)
        {
            intersections.push_back(v1);
        }
        if (v2OnPlane)
        {
            intersections.push_back(v2);
        }

        // remove duplicate intersections
        if (intersections.size() > 1)
        {
            for (size_t i = 0; i < intersections.size(); i++)
            {
                for (size_t j = i + 1; j < intersections.size();)
                {
                    float dx = intersections[i].x - intersections[j].x;
                    float dy = intersections[i].y - intersections[j].y;
                    float dz = intersections[i].z - intersections[j].z;
                    float distSq = dx * dx + dy * dy + dz * dz;

                    if (distSq < 1e-10f)
                    {
                        // 删除重复点
                        intersections.erase(intersections.begin() + j);
                    }
                    else
                    {
                        j++;
                    }
                }
            }
        }

        if (intersections.size() >= 2)
        {
            segment.start = intersections[0];
            segment.end = intersections[1];
            return true;
        }
        else if (intersections.size() == 1)
        {
            segment.start = intersections[0];
            segment.end = intersections[0];
         
            segment.end.x += 0.0001f;
            segment.end.y += 0.0001f;
            segment.end.z += 0.0001f;
            return true;
        }

        return false;
    }

    // 计算切割平面与网格的交线


    PlaneIntersection computePlaneIntersections(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes)
    {
        PlaneIntersection intersection;
        intersection.visible = false;

        // 检查网格是否有效
        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            std::cerr << "Empty mesh, cannot compute plane intersections" << std::endl;
            return intersection;
        }

        // 使用线程安全的向量
        std::vector<IntersectionSegment> threadSafeSegments;
        std::mutex segmentMutex;

// 并行计算交线
#pragma omp parallel for
        for (size_t planeIdx = 0; planeIdx < planes.size(); planeIdx++)
        {
            const CuttingPlane &plane = planes[planeIdx];
            std::vector<IntersectionSegment> localSegments;

            // 遍历所有三角形
            for (size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                // 获取三角形的三个顶点
                const Vertex &v0 = mesh.vertices[mesh.indices[i]];
                const Vertex &v1 = mesh.vertices[mesh.indices[i + 1]];
                const Vertex &v2 = mesh.vertices[mesh.indices[i + 2]];

                // 检查三角形与平面的交线
                IntersectionSegment segment;
                if (trianglePlaneIntersection(v0, v1, v2, plane, segment))
                {
                    segment.planeIndex = planeIdx;
                    localSegments.push_back(segment);
                }
            }

// 线程安全地合并结果
#pragma omp critical
            {
                threadSafeSegments.insert(
                    threadSafeSegments.end(),
                    localSegments.begin(),
                    localSegments.end());
            }
        }

        intersection.segments = std::move(threadSafeSegments);

        std::cout << "Computed " << intersection.segments.size()
                  << " intersection segments for " << planes.size() << " planes" << std::endl;

        return intersection;
    }

    // 创建用于显示交线的VAO/VBO
    void setupIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO)
    {

        // 生成VAO和VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);

        // 将所有线段顶点放入一个数组
        std::vector<Vertex> lineVertices;
        for (const auto &segment : intersection.segments)
        {
            lineVertices.push_back(segment.start);
            lineVertices.push_back(segment.end);
        }

        // 绑定并填充VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(Vertex),
                     lineVertices.data(), GL_STATIC_DRAW);

        // 设置顶点属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // 更新交线VAO/VBO
    void updateIntersectionVAO(
        const PlaneIntersection &intersection,
        unsigned int &VAO,
        unsigned int &VBO)
    {

        // 将所有线段顶点放入一个数组
        std::vector<Vertex> lineVertices;
        for (const auto &segment : intersection.segments)
        {
            lineVertices.push_back(segment.start);
            lineVertices.push_back(segment.end);
        }

        // 更新VBO数据
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(Vertex),
                     lineVertices.data(), GL_STATIC_DRAW);
    }

    // 计算模型爆炸分段
    std::vector<ModelSegment> computeModelSegments(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis,
        float explodeDistance)
    {

        std::vector<ModelSegment> segments;

        if (planes.empty() || mesh.vertices.empty() || mesh.indices.empty())
        {
            // 如果没有切割平面或网格为空，将整个模型作为一个分段
            ModelSegment wholeMesh;
            wholeMesh.startIndex = 0;
            wholeMesh.indexCount = mesh.indices.size();
            wholeMesh.displacement = {0, 0, 0}; // 不移动
            segments.push_back(wholeMesh);
            return segments;
        }

        // 对切割平面按照位置排序
        std::vector<CuttingPlane> sortedPlanes = planes;
        std::sort(sortedPlanes.begin(), sortedPlanes.end(),
                  [&explosionAxis](const CuttingPlane &a, const CuttingPlane &b)
                  {
                      float projA = a.origin.x * explosionAxis.x +
                                    a.origin.y * explosionAxis.y +
                                    a.origin.z * explosionAxis.z;
                      float projB = b.origin.x * explosionAxis.x +
                                    b.origin.y * explosionAxis.y +
                                    b.origin.z * explosionAxis.z;
                      return projA < projB;
                  });

        // 计算每个三角形所属的分段
        std::vector<int> triangleSegments(mesh.indices.size() / 3, -1);
        std::vector<float> planePositions(sortedPlanes.size());

        // 计算每个平面在爆炸轴上的投影位置
        for (size_t i = 0; i < sortedPlanes.size(); ++i)
        {
            planePositions[i] = sortedPlanes[i].origin.x * explosionAxis.x +
                                sortedPlanes[i].origin.y * explosionAxis.y +
                                sortedPlanes[i].origin.z * explosionAxis.z;
        }

        // 分析每个三角形，确定它属于哪个分段
        for (size_t triIndex = 0; triIndex < mesh.indices.size() / 3; ++triIndex)
        {
            // 获取三角形的三个顶点
            const Vertex &v0 = mesh.vertices[mesh.indices[triIndex * 3]];
            const Vertex &v1 = mesh.vertices[mesh.indices[triIndex * 3 + 1]];
            const Vertex &v2 = mesh.vertices[mesh.indices[triIndex * 3 + 2]];

            // 计算三角形中心点在爆炸轴上的投影
            float centerProj = (projectVertexOnAxis(v0, explosionAxis) +
                                projectVertexOnAxis(v1, explosionAxis) +
                                projectVertexOnAxis(v2, explosionAxis)) /
                               3.0f;

            // 确定三角形所在的分段
            int segmentIndex = 0;
            for (size_t planeIdx = 0; planeIdx < planePositions.size(); ++planeIdx)
            {
                if (centerProj < planePositions[planeIdx])
                {
                    break;
                }
                segmentIndex++;
            }

            triangleSegments[triIndex] = segmentIndex;
        }

        // 统计每个分段的三角形数量
        std::map<int, int> segmentTriangleCounts;
        for (int segIndex : triangleSegments)
        {
            segmentTriangleCounts[segIndex]++;
        }

        // 创建临时索引数组，按分段重新组织
        std::vector<IndexType> newIndices;
        newIndices.reserve(mesh.indices.size());

        // 记录每个分段在新索引数组中的起始位置
        std::vector<int> segmentStartIndices(segmentTriangleCounts.size(), 0);
        int segmentCount = segmentTriangleCounts.size();

        // 为每个分段分配空间
        for (int i = 0; i < segmentCount; ++i)
        {
            if (i > 0)
            {
                segmentStartIndices[i] = segmentStartIndices[i - 1] + segmentTriangleCounts[i - 1] * 3;
            }
            newIndices.resize(newIndices.size() + segmentTriangleCounts[i] * 3);
        }

        // 填充新索引数组
        std::vector<int> segmentCurrentIndices = segmentStartIndices;
        for (size_t triIndex = 0; triIndex < triangleSegments.size(); ++triIndex)
        {
            int segIdx = triangleSegments[triIndex];
            int newStartIdx = segmentCurrentIndices[segIdx];

            // 复制三角形的三个索引
            newIndices[newStartIdx] = mesh.indices[triIndex * 3];
            newIndices[newStartIdx + 1] = mesh.indices[triIndex * 3 + 1];
            newIndices[newStartIdx + 2] = mesh.indices[triIndex * 3 + 2];

            segmentCurrentIndices[segIdx] += 3;
        }

        // 计算分段位移
        segments.resize(segmentCount);

        // 确定固定点
        int fixedSegmentIndex;
        if (segmentCount % 2 == 1)
        {
            // 奇数段，中间段固定
            fixedSegmentIndex = segmentCount / 2;
        }
        else
        {
            // 偶数段，中间两段之间的切面位置固定
            fixedSegmentIndex = segmentCount / 2 - 1;
        }

        // 计算每个分段的位移
        for (int i = 0; i < segmentCount; ++i)
        {
            segments[i].startIndex = segmentStartIndices[i];
            segments[i].indexCount = segmentTriangleCounts[i] * 3;

            // 计算位移
            float displacement = (i - fixedSegmentIndex) * explodeDistance;
            segments[i].displacement.x = explosionAxis.x * displacement;
            segments[i].displacement.y = explosionAxis.y * displacement;
            segments[i].displacement.z = explosionAxis.z * displacement;
        }

        std::cout << "Model divided into " << segmentCount << " segments for explosion" << std::endl;

        return segments;
    }

    // 设置爆炸后模型的VAO/VBO/EBO
    void setupExplodedMesh(
        const Mesh &mesh,
        const std::vector<ModelSegment> &segments,
        unsigned int &VAO,
        unsigned int &VBO,
        unsigned int &EBO)
    {

        // 如果已经创建了VAO，则先删除
        if (VAO != 0)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        // 创建新的顶点和索引数组
        std::vector<Vertex> explodedVertices;
        std::vector<IndexType> explodedIndices;

        // 计算需要的总容量
        size_t totalVertexCount = mesh.vertices.size();
        size_t totalIndexCount = 0;
        for (const auto &segment : segments)
        {
            totalIndexCount += segment.indexCount;
        }

        explodedVertices.reserve(totalVertexCount);
        explodedIndices.reserve(totalIndexCount);

        // 复制顶点
        for (const auto &vertex : mesh.vertices)
        {
            explodedVertices.push_back(vertex);
        }

        // 计算分段顶点所需的偏移量
        std::vector<Vec3> vertexOffsets(explodedVertices.size(), Vec3(0, 0, 0));

        // 为每个分段中的顶点设置偏移量
        for (const auto &segment : segments)
        {
            for (int i = 0; i < segment.indexCount; ++i)
            {
                IndexType idx = mesh.indices[segment.startIndex + i];
                vertexOffsets[idx] = segment.displacement;
            }
        }

        // 应用偏移量到顶点
        for (size_t i = 0; i < explodedVertices.size(); ++i)
        {
            explodedVertices[i].x += vertexOffsets[i].x;
            explodedVertices[i].y += vertexOffsets[i].y;
            explodedVertices[i].z += vertexOffsets[i].z;
        }

        // 使用原始索引
        explodedIndices = mesh.indices;

        // 创建VAO和VBO
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // 绑定顶点缓冲
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, explodedVertices.size() * sizeof(Vertex),
                     explodedVertices.data(), GL_STATIC_DRAW);

        // 设置顶点属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        // 绑定索引缓冲
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, explodedIndices.size() * sizeof(IndexType),
                     explodedIndices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

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
        unsigned int &EBO)
    {

        if (explodeEnabled)
        {
            // 计算模型分段
            segments = computeModelSegments(mesh, planes, explosionAxis, explodeDistance);

            // 设置爆炸后的模型
            setupExplodedMesh(mesh, segments, VAO, VBO, EBO);
        }
        else
        {
            // 非爆炸状态，使用原始网格
            if (VAO != 0)
            {
                glDeleteVertexArrays(1, &VAO);
                glDeleteBuffers(1, &VBO);
                glDeleteBuffers(1, &EBO);
                VAO = VBO = EBO = 0;
            }

            // 清空分段信息
            segments.clear();
        }
    }

} // namespace MC
