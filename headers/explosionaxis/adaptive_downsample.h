#ifndef MC_ADAPTIVE_DOWNSAMPLE_H
#define MC_ADAPTIVE_DOWNSAMPLE_H

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/common/common.h>
#include <iostream>

namespace MC {

/**
 * 使用自适应体素大小对点云进行下采样
 * 
 * @param cloud 输入点云
 * @param targetPointCount 目标点数量
 * @param minLeafSize 最小叶子大小
 * @param maxLeafSize 最大叶子大小
 * @return 下采样后的点云
 */
inline pcl::PointCloud<pcl::PointXYZ>::Ptr adaptiveDownsample(
    const pcl::PointCloud<pcl::PointXYZ>::Ptr& cloud,
    size_t targetPointCount = 5000, 
    float minLeafSize = 0.01f,
    float maxLeafSize = 0.2f) {
    
    if (cloud->empty()) {
        std::cerr << "Empty cloud passed to adaptiveDownsample" << std::endl;
        return cloud;
    }
    
    // 如果点云已经小于目标点数，直接返回
    if (cloud->size() <= targetPointCount) {
        return cloud;
    }
    
    // 计算点云边界框
    pcl::PointXYZ minPt, maxPt;
    pcl::getMinMax3D(*cloud, minPt, maxPt);
    
    // 计算边界框对角线长度
    float diagonalLength = std::sqrt(
        std::pow(maxPt.x - minPt.x, 2) +
        std::pow(maxPt.y - minPt.y, 2) +
        std::pow(maxPt.z - minPt.z, 2)
    );
    
    // 估计初始叶子大小
    float initialLeafSize = diagonalLength / 50.0f;
    initialLeafSize = std::max(minLeafSize, std::min(maxLeafSize, initialLeafSize));
    
    // 二分搜索找到合适的叶子大小
    float lowerBound = minLeafSize;
    float upperBound = maxLeafSize;
    float currentLeafSize = initialLeafSize;
    pcl::PointCloud<pcl::PointXYZ>::Ptr result(new pcl::PointCloud<pcl::PointXYZ>());
    
    // 最多尝试10次
    for (int attempt = 0; attempt < 10; ++attempt) {
        pcl::VoxelGrid<pcl::PointXYZ> vg;
        vg.setInputCloud(cloud);
        vg.setLeafSize(currentLeafSize, currentLeafSize, currentLeafSize);
        vg.filter(*result);
        
        std::cout << "Leaf size " << currentLeafSize << " resulted in " 
                  << result->size() << " points" << std::endl;
        
        // 检查是否接近目标点数
        if (std::abs(static_cast<int>(result->size()) - static_cast<int>(targetPointCount)) < targetPointCount * 0.1) {
            // 在10%误差范围内，接受结果
            break;
        }
        
        // 调整叶子大小
        if (result->size() > targetPointCount) {
            // 太多点，增加叶子大小
            lowerBound = currentLeafSize;
            currentLeafSize = (currentLeafSize + upperBound) / 2.0f;
        } else {
            // 太少点，减小叶子大小
            upperBound = currentLeafSize;
            currentLeafSize = (currentLeafSize + lowerBound) / 2.0f;
        }
        
        // 检查是否达到边界
        if (upperBound - lowerBound < 0.001f) {
            break;
        }
    }
    
    std::cout << "Final downsampling: from " << cloud->size() << " to " 
              << result->size() << " points with leaf size " << currentLeafSize << std::endl;
    
    return result;
}

} // namespace MC

#endif // MC_ADAPTIVE_DOWNSAMPLE_H