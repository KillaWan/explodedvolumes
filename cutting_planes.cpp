#include "headers/cutting_planes.h"
#include <algorithm>
#include <iostream>
#include <glad/glad.h>

namespace MC
{

    // 计算点到平面的距离
    float pointToPlaneDistance(const Vertex &v, const CuttingPlane &plane)
    {
        return (v.x - plane.origin.x) * plane.normal.x +
               (v.y - plane.origin.y) * plane.normal.y +
               (v.z - plane.origin.z) * plane.normal.z;
    }

    // 计算两点之间的线性插值
    Vertex interpolateVertex(const Vertex &v1, const Vertex &v2, float t)
    {
        Vertex result;
        result.x = v1.x + t * (v2.x - v1.x);
        result.y = v1.y + t * (v2.y - v1.y);
        result.z = v1.z + t * (v2.z - v1.z);
        return result;
    }

    // 三角形与平面相交检测
    bool trianglePlaneIntersection(
        const Vertex &v0, const Vertex &v1, const Vertex &v2,
        const CuttingPlane &plane,
        IntersectionSegment &segment)
    {

        // 计算三个顶点到平面的距离
        float d0 = pointToPlaneDistance(v0, plane);
        float d1 = pointToPlaneDistance(v1, plane);
        float d2 = pointToPlaneDistance(v2, plane);

        // 如果所有点都在平面同一侧，则不相交
        if ((d0 > 0 && d1 > 0 && d2 > 0) || (d0 < 0 && d1 < 0 && d2 < 0))
        {
            return false;
        }

        // 如果任何点在平面上（距离为0），需要特殊处理
        bool v0OnPlane = std::abs(d0) < 1e-6f;
        bool v1OnPlane = std::abs(d1) < 1e-6f;
        bool v2OnPlane = std::abs(d2) < 1e-6f;

        // 收集交点
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
            // v1-v2 边与平面相交
            float t = d1 / (d1 - d2);
            intersections.push_back(interpolateVertex(v1, v2, t));
        }

        if (!v2OnPlane && !v0OnPlane && d2 * d0 <= 0.0f)
        {
            // v2-v0 边与平面相交
            float t = d2 / (d2 - d0);
            intersections.push_back(interpolateVertex(v2, v0, t));
        }

        // 处理顶点在平面上的情况
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

        // 去除重复的交点（可能由于浮点精度问题导致）
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

        // 如果找到两个不同的交点，则保存为一个线段
        if (intersections.size() >= 2)
        {
            segment.start = intersections[0];
            segment.end = intersections[1];
            return true;
        }
        else if (intersections.size() == 1)
        {
            // 只有一个交点，可能是顶点与平面相交，这种情况下不生成线段
            // 或者也可以生成一个很短的线段来表示这个点
            segment.start = intersections[0];
            segment.end = intersections[0];
            // 添加一个微小的偏移以便能够渲染
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
        intersection.visible = false; // 默认不可见

        // 检查网格是否有效
        if (mesh.vertices.empty() || mesh.indices.empty())
        {
            std::cerr << "Empty mesh, cannot compute plane intersections" << std::endl;
            return intersection;
        }

        // 为每个平面计算交线
        for (size_t planeIdx = 0; planeIdx < planes.size(); planeIdx++)
        {
            const CuttingPlane &plane = planes[planeIdx];

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
                    intersection.segments.push_back(segment);
                }
            }
        }

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

} // namespace MC