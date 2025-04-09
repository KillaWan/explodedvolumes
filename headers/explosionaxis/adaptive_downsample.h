#ifndef MC_ADAPTIVE_DOWNSAMPLE_H
#define MC_ADAPTIVE_DOWNSAMPLE_H

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/common/common.h>
#include <iostream>

namespace MC
{

    inline pcl::PointCloud<pcl::PointXYZ>::Ptr adaptiveDownsample(
        const pcl::PointCloud<pcl::PointXYZ>::Ptr &cloud,
        size_t targetPointCount = 5000,
        float minLeafSize = 0.01f,
        float maxLeafSize = 0.2f)
    {

        if (cloud->empty())
        {
            std::cerr << "Empty cloud passed to adaptiveDownsample" << std::endl;
            return cloud;
        }
        if (cloud->size() <= targetPointCount)
        {
            return cloud;
        }

        pcl::PointXYZ minPt, maxPt;
        pcl::getMinMax3D(*cloud, minPt, maxPt);

        // calculate diagonal length
        float diagonalLength = std::sqrt(
            std::pow(maxPt.x - minPt.x, 2) +
            std::pow(maxPt.y - minPt.y, 2) +
            std::pow(maxPt.z - minPt.z, 2));

        // estimate initial leaf size
        float initialLeafSize = diagonalLength / 50.0f;
        initialLeafSize = std::max(minLeafSize, std::min(maxLeafSize, initialLeafSize));
        // binary search to find the right leaf size
        float lowerBound = minLeafSize;
        float upperBound = maxLeafSize;
        float currentLeafSize = initialLeafSize;
        pcl::PointCloud<pcl::PointXYZ>::Ptr result(new pcl::PointCloud<pcl::PointXYZ>());
        for (int attempt = 0; attempt < 10; ++attempt)
        {
            pcl::VoxelGrid<pcl::PointXYZ> vg;
            vg.setInputCloud(cloud);
            vg.setLeafSize(currentLeafSize, currentLeafSize, currentLeafSize);
            vg.filter(*result);

            std::cout << "Leaf size " << currentLeafSize << " resulted in "
                      << result->size() << " points" << std::endl;

            if (std::abs(static_cast<int>(result->size()) - static_cast<int>(targetPointCount)) < targetPointCount * 0.1)
            {
                break;
            }

            // adjust leaf size
            if (result->size() > targetPointCount)
            {
                lowerBound = currentLeafSize;
                currentLeafSize = (currentLeafSize + upperBound) / 2.0f;
            }
            else
            {
                upperBound = currentLeafSize;
                currentLeafSize = (currentLeafSize + lowerBound) / 2.0f;
            }

            if (upperBound - lowerBound < 0.001f)
            {
                break;
            }
        }

        std::cout << "Final downsampling: from " << cloud->size() << " to "
                  << result->size() << " points with leaf size " << currentLeafSize << std::endl;

        return result;
    }
}

#endif // MC_ADAPTIVE_DOWNSAMPLE_H