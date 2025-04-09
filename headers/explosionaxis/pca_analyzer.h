#ifndef MC_PCA_ANALYZER_H
#define MC_PCA_ANALYZER_H

#include <vector>
#include "data.h"
#include "explosionaxis/vector_ops.h"

namespace MC
{

    // PCA Analyzer
    class PCAAnalyzer
    {
    public:
        PCAAnalyzer() = default;
        ~PCAAnalyzer() = default;

        Vec3 analyzePrincipalAxis(const std::vector<Vertex> &meshVertices);

        void setUseLongestAxis(bool use) { m_useLongestAxis = use; }
        bool isUsingLongestAxis() const { return m_useLongestAxis; }

        Vec3 compute3DPCA(const std::vector<Vertex> &meshVertices);
        Vec3 compute2DPCAOnPlane(const std::vector<Vertex> &meshVertices, const Vec3 &planeNormal);

    private:
        bool m_useLongestAxis = true;
    };

}

#endif // MC_PCA_ANALYZER_H