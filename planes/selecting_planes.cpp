#include "planes/selecting_planes.h"
#include <algorithm>
#include <iostream>

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkOBBTree.h>

namespace MC
{

    // 将Mesh转换为VTK PolyData格式（用于自适应方法）
    static vtkSmartPointer<vtkPolyData> meshToVtkPolyData(const Mesh &mesh)
    {
        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
        for (const auto &vertex : mesh.vertices)
        {
            points->InsertNextPoint(vertex.x, vertex.y, vertex.z);
        }

        vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
        for (size_t i = 0; i < mesh.indices.size(); i += 3)
        {
            vtkIdType pointIds[3] = {
                mesh.indices[i],
                mesh.indices[i + 1],
                mesh.indices[i + 2]};
            cells->InsertNextCell(3, pointIds);
        }

        vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
        polyData->SetPoints(points);
        polyData->SetPolys(cells);

        return polyData;
    }

    // 自适应切割平面方法 - 使用OBB树
    std::vector<CuttingPlane> generateAdaptiveCuttingPlanes(
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
            normalizedAxis.x = 0.0f;
            normalizedAxis.y = 0.0f;
            normalizedAxis.z = 1.0f;
        }
    
        // 将网格转换为VTK格式 - 保留此部分以获取模型尺寸信息
        vtkSmartPointer<vtkPolyData> polyData = meshToVtkPolyData(mesh);
        vtkSmartPointer<vtkOBBTree> obbTree = vtkSmartPointer<vtkOBBTree>::New();
        obbTree->SetDataSet(polyData);
        obbTree->BuildLocator();
    
        double cornerOBB[3], maxOBB[3], midOBB[3], minOBB[3], sizeOBB[3];
        obbTree->ComputeOBB(polyData->GetPoints(), cornerOBB, maxOBB, midOBB, minOBB, sizeOBB);
    
        // 关键修改：直接使用标准化后的爆炸轴作为切割平面法线
        Vec3 cuttingAxis = normalizedAxis;
    
        // 计算模型在切割轴（爆炸轴）上的投影范围
        float minProj = std::numeric_limits<float>::max();
        float maxProj = std::numeric_limits<float>::lowest();
        std::vector<float> projections;
    
        for (const auto &vertex : mesh.vertices)
        {
            float proj =
                vertex.x * cuttingAxis.x +
                vertex.y * cuttingAxis.y +
                vertex.z * cuttingAxis.z;
    
            projections.push_back(proj);
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }
    
        // 对投影值进行排序，用于后续自适应切割
        std::sort(projections.begin(), projections.end());
    
        // 自适应确定切割位置
        std::vector<float> cutPositions;
    
        // 根据顶点分布，选择切割位置
        for (int i = 1; i <= numPlanes; ++i)
        {
            // 根据顶点数量的百分比位置选择切割点
            size_t index = (i * projections.size()) / (numPlanes + 1);
            if (index < projections.size())
            {
                cutPositions.push_back(projections[index]);
            }
        }
    
        // 创建切割平面
        for (const float &position : cutPositions)
        {
            CuttingPlane plane;
            plane.normal = cuttingAxis;  // 使用爆炸轴作为平面法线
            plane.distance = position;
    
            // 计算平面上的一点
            plane.origin.x = mesh.center.x + cuttingAxis.x * (position - (minProj + maxProj) / 2.0f);
            plane.origin.y = mesh.center.y + cuttingAxis.y * (position - (minProj + maxProj) / 2.0f);
            plane.origin.z = mesh.center.z + cuttingAxis.z * (position - (minProj + maxProj) / 2.0f);
    
            planes.push_back(plane);
        }
    
        std::cout << "Generated " << planes.size() << " cutting planes perpendicular to explosion axis" << std::endl;
    
        return planes;
    }

} // namespace MC