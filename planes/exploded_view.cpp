#include "planes/exploded_view.h"
#include "data.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPlane.h>
#include <vtkCutter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkClipPolyData.h>

// 在 exploded_view.cpp 头部添加
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vtkCell.h>
#include <vtkCellType.h>

namespace MC
{

    // 将Mesh转换为VTK格式
    vtkSmartPointer<vtkPolyData> convertMeshToVTKPolyData(const Mesh &mesh)
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

    // 使用VTK进行切割的新实现
    ExplodedView computeExplodedView(
        const Mesh &mesh,
        const std::vector<CuttingPlane> &planes,
        const Vec3 &explosionAxis)
    {
        ExplodedView explodedView;
        explodedView.explosionDistance = 35.0f;
        float segmentGap = explodedView.explosionDistance;

        // 片段间的间距
        float segmentSpacing = segmentGap;

        // 将原始网格转换为VTK格式
        vtkSmartPointer<vtkPolyData> originalPolyData = convertMeshToVTKPolyData(mesh);

        // 没有切割平面时，只返回原始模型
        if (planes.empty())
        {
            ExplodedSegment segment;
            segment.vtkPolyData = originalPolyData;

            // 复制顶点和索引
            for (const auto &vertex : mesh.vertices)
            {
                segment.vertices.push_back(vertex);
            }
            segment.indices = mesh.indices;

            // 无位移
            segment.displacement = {0.0f, 0.0f, 0.0f};
            setupSegmentMesh(segment);
            explodedView.segments.push_back(segment);

            return explodedView;
        }

        // 使用二叉树思路来进行切割
        std::vector<vtkSmartPointer<vtkPolyData>> segmentList;
        segmentList.push_back(originalPolyData);

        // 对每个切割平面
        for (const CuttingPlane &plane : planes)
        {
            // 获取当前所有片段，准备进一步切割
            std::vector<vtkSmartPointer<vtkPolyData>> currentSegments = segmentList;
            segmentList.clear(); // 清空列表准备存储新切割的片段

            // 创建VTK平面
            vtkSmartPointer<vtkPlane> vtkClippingPlane = vtkSmartPointer<vtkPlane>::New();
            vtkClippingPlane->SetOrigin(plane.origin.x, plane.origin.y, plane.origin.z);
            vtkClippingPlane->SetNormal(plane.normal.x, plane.normal.y, plane.normal.z);

            // 对当前每个片段进行切割
            for (auto segment : currentSegments)
            {
                // 正面切割
                vtkSmartPointer<vtkClipPolyData> clipperPositive = vtkSmartPointer<vtkClipPolyData>::New();
                clipperPositive->SetInputData(segment);
                clipperPositive->SetClipFunction(vtkClippingPlane);
                clipperPositive->InsideOutOff(); // 保留平面正面的部分
                clipperPositive->Update();

                // 负面切割
                vtkSmartPointer<vtkClipPolyData> clipperNegative = vtkSmartPointer<vtkClipPolyData>::New();
                clipperNegative->SetInputData(segment);
                clipperNegative->SetClipFunction(vtkClippingPlane);
                clipperNegative->InsideOutOn(); // 保留平面负面的部分
                clipperNegative->Update();

                // 将两个切割结果添加到片段列表中
                vtkSmartPointer<vtkPolyData> positiveOutput = clipperPositive->GetOutput();
                vtkSmartPointer<vtkPolyData> negativeOutput = clipperNegative->GetOutput();

                // 只添加有效的片段（包含面片的片段）
                if (positiveOutput->GetNumberOfCells() > 0)
                {
                    segmentList.push_back(positiveOutput);
                }

                if (negativeOutput->GetNumberOfCells() > 0)
                {
                    segmentList.push_back(negativeOutput);
                }
            }
        }

        // 创建最终的爆炸视图片段
        int segmentCount = segmentList.size();
        for (int i = 0; i < segmentCount; i++)
        {
            vtkSmartPointer<vtkPolyData> segmentPolyData = segmentList[i];
            ExplodedSegment explodedSegment;
            explodedSegment.vtkPolyData = segmentPolyData;

            // 复制顶点
            for (vtkIdType j = 0; j < segmentPolyData->GetNumberOfPoints(); j++)
            {
                double point[3];
                segmentPolyData->GetPoint(j, point);

                Vertex v;
                v.x = point[0];
                v.y = point[1];
                v.z = point[2];
                explodedSegment.vertices.push_back(v);
            }

            // 复制面片索引
            for (vtkIdType j = 0; j < segmentPolyData->GetNumberOfCells(); j++)
            {
                vtkCell *cell = segmentPolyData->GetCell(j);

                // 确认是三角形
                if (cell->GetCellType() == VTK_TRIANGLE)
                {
                    for (int k = 0; k < 3; k++) // 三角形有3个顶点
                    {
                        explodedSegment.indices.push_back(cell->GetPointId(k));
                    }
                }
            }

            // 计算位移
            // 将片段均匀分布在爆炸轴上，并加入间距
            float normalizedIndex = (segmentCount > 1) ? 2.0f * ((float)i / (segmentCount - 1) - 0.5f) : 0.0f;

            // 添加片段间距：计算相对于中心的索引距离，乘以片段间距
            int centerIndex = segmentCount / 2;
            int indexDistance = std::abs(i - centerIndex);
            float gapDisplacement = indexDistance * segmentSpacing;

            // 总位移 = 标准化分布位移 + 间隔位移（保持符号一致）
            float displacementFactor = -1.0f * normalizedIndex * explodedView.explosionDistance;
            if (displacementFactor != 0)
            {
                displacementFactor += (displacementFactor > 0 ? gapDisplacement : -gapDisplacement);
            }

            explodedSegment.displacement.x = explosionAxis.x * displacementFactor;
            explodedSegment.displacement.y = explosionAxis.y * displacementFactor;
            explodedSegment.displacement.z = explosionAxis.z * displacementFactor;

            // 设置OpenGL缓冲
            setupSegmentMesh(explodedSegment);
            explodedView.segments.push_back(explodedSegment);
        }

        return explodedView;
    }
    // 设置片段的VAO/VBO/EBO
    void setupSegmentMesh(ExplodedSegment &segment)
    {
        // 如果已经有VAO，先删除
        if (segment.VAO != 0)
        {
            glDeleteVertexArrays(1, &segment.VAO);
            glDeleteBuffers(1, &segment.VBO);
            glDeleteBuffers(1, &segment.EBO);
        }

        // 创建VAO/VBO/EBO
        glGenVertexArrays(1, &segment.VAO);
        glGenBuffers(1, &segment.VBO);
        glGenBuffers(1, &segment.EBO);

        glBindVertexArray(segment.VAO);

        // 绑定顶点缓冲
        glBindBuffer(GL_ARRAY_BUFFER, segment.VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     segment.vertices.size() * sizeof(Vertex),
                     segment.vertices.data(),
                     GL_STATIC_DRAW);

        // 绑定索引缓冲
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, segment.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     segment.indices.size() * sizeof(IndexType),
                     segment.indices.data(),
                     GL_STATIC_DRAW);

        // 设置顶点属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // 更新爆炸视图位移
    void MC::updateExplodedViewDisplacements(
        ExplodedView &explodedView,
        const Vec3 &explosionAxis,
        float explosionDistance)
    {
        // 保存爆炸距离设置
        explodedView.explosionDistance = explosionDistance;
        int segmentCount = explodedView.segments.size();
        
        if (segmentCount <= 1) {
            return; // 没有片段或只有一个片段，不需要爆炸
        }
        
        // 确保爆炸轴是单位向量
        Vec3 normalizedAxis = explosionAxis;
        float length = std::sqrt(
            normalizedAxis.x * normalizedAxis.x +
            normalizedAxis.y * normalizedAxis.y +
            normalizedAxis.z * normalizedAxis.z);
            
        if (length > 0.0001f) {
            normalizedAxis.x /= length;
            normalizedAxis.y /= length;
            normalizedAxis.z /= length;
        }
        
        // 这里使用与原始computeExplodedView中完全相同的计算逻辑
        float segmentSpacing = explosionDistance;
        
        for (int i = 0; i < segmentCount; i++) {
            auto &segment = explodedView.segments[i];
            
            // 复制原始computeExplodedView中相同的位移计算逻辑:
            // 1. 计算标准化索引 (-0.5到0.5的范围)
            float normalizedIndex = (segmentCount > 1) ? 2.0f * ((float)i / (segmentCount - 1) - 0.5f) : 0.0f;
            
            // 2. 计算相对于中心的索引距离
            int centerIndex = segmentCount / 2;
            int indexDistance = std::abs(i - centerIndex);
            
            // 3. 基于间距计算额外偏移
            float gapDisplacement = indexDistance * segmentSpacing;
            
            // 4. 总位移 = 标准化分布位移 + 间隔位移（保持符号一致）
            float displacementFactor = -1.0f * normalizedIndex * explosionDistance;
            if (displacementFactor != 0) {
                displacementFactor += (displacementFactor > 0 ? gapDisplacement : -gapDisplacement);
            }
            
            // 应用位移
            segment.displacement.x = normalizedAxis.x * displacementFactor;
            segment.displacement.y = normalizedAxis.y * displacementFactor;
            segment.displacement.z = normalizedAxis.z * displacementFactor;
        }
    }

    // 清理爆炸视图资源
    void cleanupExplodedView(ExplodedView &explodedView)
    {
        for (auto &segment : explodedView.segments)
        {
            if (segment.VAO != 0)
            {
                glDeleteVertexArrays(1, &segment.VAO);
                glDeleteBuffers(1, &segment.VBO);
                glDeleteBuffers(1, &segment.EBO);

                segment.VAO = 0;
                segment.VBO = 0;
                segment.EBO = 0;
            }
        }
        explodedView.segments.clear();
    }

} // namespace MC