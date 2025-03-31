#include "headers/selecting_planes.h"
#include "headers/data.h"
#include <algorithm>
#include <iostream>

namespace MC
{

    // 生成均匀分布的切割平面
    std::vector<CuttingPlane> generateUniformCuttingPlanes(
        const Mesh &mesh,
        const Vec3 &explosionAxis,
        int numPlanes)
    {

        std::vector<CuttingPlane> planes;

        // 归一化爆炸轴
        float axisLength = std::sqrt(
            explosionAxis.x * explosionAxis.x +
            explosionAxis.y * explosionAxis.y +
            explosionAxis.z * explosionAxis.z);

        Vec3 normalizedAxis;
        if (axisLength > 1e-6f)
        {
            normalizedAxis.x = explosionAxis.x / axisLength;
            normalizedAxis.y = explosionAxis.y / axisLength;
            normalizedAxis.z = explosionAxis.z / axisLength;
        }
        else
        {
            // 默认使用Z轴
            normalizedAxis.x = 0.0f;
            normalizedAxis.y = 0.0f;
            normalizedAxis.z = 1.0f;
        }

        // 计算模型在爆炸轴方向上的范围
        float minProj = std::numeric_limits<float>::max();
        float maxProj = std::numeric_limits<float>::lowest();

        for (const auto &vertex : mesh.vertices)
        {
            float proj = vertex.x * normalizedAxis.x +
                         vertex.y * normalizedAxis.y +
                         vertex.z * normalizedAxis.z;
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }

        // 模型在爆炸轴方向上的总长度
        float totalLength = maxProj - minProj;

        // 在长度方向上均匀分割成 numPlanes+1 份，在每个分割点放置切割平面
        // 但不在开始和结束点放置切割平面
        float step = totalLength / (numPlanes + 1);

        for (int i = 1; i <= numPlanes; i++)
        {
            // 分割点的位置 (从1到numPlanes，跳过0和numPlanes+1，即跳过开始和结束位置)
            float distance = minProj + i * step;

            CuttingPlane plane;
            plane.normal = normalizedAxis;
            plane.distance = distance;

            // 计算平面上的一点(在爆炸轴上)
            plane.origin.x = mesh.center.x + normalizedAxis.x * (distance - (minProj + maxProj) / 2.0f);
            plane.origin.y = mesh.center.y + normalizedAxis.y * (distance - (minProj + maxProj) / 2.0f);
            plane.origin.z = mesh.center.z + normalizedAxis.z * (distance - (minProj + maxProj) / 2.0f);

            planes.push_back(plane);
        }

        std::cout << "Generated " << planes.size() << " cutting planes along the explosion axis" << std::endl;

        return planes;
    }

    // 检查某个切割平面是否应该被保留（将在第二阶段实现）
    bool shouldKeepCuttingPlane(const CuttingPlane &plane, const Mesh &mesh)
    {
        // 目前简单返回 true，表示保留所有平面
        return true;
    }

} // namespace MC