#ifndef MC_EXPLOSION_AXIS_STRATEGY_H
#define MC_EXPLOSION_AXIS_STRATEGY_H

#include <vector>
#include <memory>
#include <string>
#include "data.h"

namespace MC
{

    class RotationalSymmetryDetector;
    class ReflectiveSymmetryDetector;
    class PCAAnalyzer;
    class MitraRotationalSymmetryDetector;
    class MitraReflectiveSymmetryDetector;
    class SimplePCLRotationalSymmetryDetector;
    class SimplePCLReflectiveSymmetryDetector;
    class EigenRotationalSymmetryDetector;
    class EigenReflectiveSymmetryDetector;

    // explode axis settings structure
    struct ExplosionAxisConfig
    {
        std::string strategyName = "PCA Strategy";

        // custom axis
        bool useCustomExplosionAxis = false;
        Vec3 customExplosionAxis = {0.0f, 1.0f, 0.0f};

        // rotational symmetry parameters
        int rotationSampleCount = 100;
        int rotationSymmetryOrder = 4;
        Vec3 rotationAxis = {0.0f, 0.0f, 1.0f};
        bool useCustomRotationAxis = false;

        // mirror symmetry parameters
        int mirrorSampleCount = 100;
        Vec3 mirrorNormal = {0.0f, 0.0f, 1.0f};
        bool useCustomMirrorNormal = false;

        // PCA
        bool useLongestAxis = true;

        // avaliable strategies list
        static std::vector<std::string> getStrategyNames()
        {
            return {"Rotational Symmetry", "Reflective Symmetry", "PCA (Longest Axis)", "Combined"};
        }

        static std::string convertToInternalName(const std::string &uiName)
        {
            if (uiName == "Rotational Symmetry")
                return "Rotational Strategy";
            if (uiName == "Reflective Symmetry")
                return "Reflective Strategy";
            if (uiName == "PCA (Longest Axis)")
                return "PCA Strategy";
            if (uiName == "Combined")
                return "PCL Optimized Strategy";
            return uiName;
        }

        static std::string convertToUIName(const std::string &internalName)
        {
            if (internalName == "Rotational Strategy")
                return "Rotational Symmetry";
            if (internalName == "Reflective Strategy")
                return "Reflective Symmetry";
            if (internalName == "PCA Strategy")
                return "PCA (Longest Axis)";
            if (internalName == "PCL Optimized Strategy")
                return "Combined";
            return internalName;
        }

        bool lastDetectionSuccessful = true;
        bool rotationalDetectionSuccessful = true;
        bool reflectiveDetectionSuccessful = true;
        bool pcaDetectionSuccessful = true;
    };

    // ExplosionAxisStrategy
    class ExplosionAxisStrategy
    {
    public:
        virtual ~ExplosionAxisStrategy() = default;
        virtual Vec3 computeAxis(const std::vector<Vertex> &meshVertices) = 0;
        virtual std::string getName() const = 0;
        virtual bool isPCLOptimized() const { return false; }
        virtual void applyConfig(const ExplosionAxisConfig &config) {}
        static std::shared_ptr<ExplosionAxisStrategy> create(const std::string &strategyName);
        static std::vector<std::string> getAvailableStrategies();
    };

    // RotationalStrategy
    class RotationalStrategy : public ExplosionAxisStrategy
    {
    public:
        RotationalStrategy();
        ~RotationalStrategy();
        Vec3 computeAxis(const std::vector<Vertex> &meshVertices) override;
        std::string getName() const override { return "Rotational Strategy"; }
        void applyConfig(const ExplosionAxisConfig &config) override;

    private:
        std::shared_ptr<SimplePCLRotationalSymmetryDetector> m_detector;
    };

    // ReflectiveStrategy
    class ReflectiveStrategy : public ExplosionAxisStrategy
    {
    public:
        ReflectiveStrategy();
        ~ReflectiveStrategy();
        Vec3 computeAxis(const std::vector<Vertex> &meshVertices) override;
        std::string getName() const override { return "Reflective Strategy"; }
        void applyConfig(const ExplosionAxisConfig &config) override;

    private:
        std::shared_ptr<SimplePCLReflectiveSymmetryDetector> m_detector;
    };

    // PCAStrategy
    class PCAStrategy : public ExplosionAxisStrategy
    {
    public:
        PCAStrategy();
        ~PCAStrategy();
        Vec3 computeAxis(const std::vector<Vertex> &meshVertices) override;
        std::string getName() const override { return "PCA Strategy"; }
        void applyConfig(const ExplosionAxisConfig &config) override;

    private:
        std::shared_ptr<PCAAnalyzer> m_analyzer;
        bool m_useLongestAxis = true;
    };

    // CombinedStrategy. priority：rotational -> reflective -> PCA
    class CombinedStrategy : public ExplosionAxisStrategy
    {
    public:
        CombinedStrategy();
        ~CombinedStrategy();
        Vec3 computeAxis(const std::vector<Vertex> &meshVertices) override;
        std::string getName() const override { return "Combined Strategy"; }
        void applyConfig(const ExplosionAxisConfig &config) override;

    private:
        std::shared_ptr<MitraRotationalSymmetryDetector> m_rotationDetector;
        std::shared_ptr<MitraReflectiveSymmetryDetector> m_reflectionDetector;
        std::shared_ptr<PCAAnalyzer> m_pcaAnalyzer;
    };

    // PCLOptimizedStrategy
    class PCLOptimizedStrategy : public ExplosionAxisStrategy
    {
    public:
        PCLOptimizedStrategy();
        ~PCLOptimizedStrategy();
        Vec3 computeAxis(const std::vector<Vertex> &meshVertices) override;
        std::string getName() const override { return "PCL Optimized Strategy"; }
        bool isPCLOptimized() const override { return true; }
        void applyConfig(const ExplosionAxisConfig &config) override;

    private:
        std::shared_ptr<SimplePCLRotationalSymmetryDetector> m_rotationDetector;
        std::shared_ptr<SimplePCLReflectiveSymmetryDetector> m_reflectionDetector;
        std::shared_ptr<PCAAnalyzer> m_pcaAnalyzer;
    };

    // EigenStrategy
    class EigenStrategy : public ExplosionAxisStrategy
    {
    public:
        EigenStrategy();
        ~EigenStrategy();
        Vec3 computeAxis(const std::vector<Vertex> &meshVertices) override;
        std::string getName() const override { return "Eigen Strategy"; }
        void applyConfig(const ExplosionAxisConfig &config) override;

    private:
        std::shared_ptr<EigenRotationalSymmetryDetector> m_rotationDetector;
        std::shared_ptr<EigenReflectiveSymmetryDetector> m_reflectionDetector;
        std::shared_ptr<PCAAnalyzer> m_pcaAnalyzer;
    };

    // ExplosionAxisManager
    class ExplosionAxisManager
    {
    public:
        static ExplosionAxisManager &getInstance();
        void setStrategy(const std::string &strategyName);
        void setStrategy(std::shared_ptr<ExplosionAxisStrategy> strategy);
        Vec3 computeExplosionAxis(const std::vector<Vertex> &meshVertices);
        Vec3 computeExplosionAxis(const Mesh &mesh);

        std::string getCurrentStrategyName() const;
        std::vector<std::string> getAvailableStrategies() const;

        bool isPCLOptimized() const;

        ExplosionAxisConfig &getConfig() { return m_config; }
        const ExplosionAxisConfig &getConfig() const { return m_config; }

        void applyConfig(const ExplosionAxisConfig &config);

        const Vec3 &getLastSuccessfulAxis() const { return m_lastSuccessfulAxis; }
        void setLastSuccessfulAxis(const Vec3 &axis) { m_lastSuccessfulAxis = axis; }

        bool wasLastDetectionSuccessful() const { return m_config.lastDetectionSuccessful; }

        void setDetectionStatus(bool success)
        {
            m_config.lastDetectionSuccessful = success;
        }

        void setRotationalDetectionStatus(bool success)
        {
            m_config.rotationalDetectionSuccessful = success;
            m_config.lastDetectionSuccessful = success;
        }

        void setReflectiveDetectionStatus(bool success)
        {
            m_config.reflectiveDetectionSuccessful = success;
            m_config.lastDetectionSuccessful = success;
        }

    private:
        ExplosionAxisManager();

        ExplosionAxisManager(const ExplosionAxisManager &) = delete;
        ExplosionAxisManager &operator=(const ExplosionAxisManager &) = delete;

        std::shared_ptr<ExplosionAxisStrategy> m_currentStrategy;
        ExplosionAxisConfig m_config;
        Vec3 m_lastSuccessfulAxis = {0.0f, 0.0f, 1.0f};
    };

    inline Vec3 computeExplosionAxis(const std::vector<Vertex> &meshVertices)
    {
        return ExplosionAxisManager::getInstance().computeExplosionAxis(meshVertices);
    }

    inline Vec3 computeExplosionAxis(const Mesh &mesh)
    {
        return ExplosionAxisManager::getInstance().computeExplosionAxis(mesh);
    }

    inline std::string getCurrentExplosionStrategyName()
    {
        return ExplosionAxisManager::getInstance().getCurrentStrategyName();
    }

    inline void setExplosionStrategy(const std::string &strategyName)
    {
        ExplosionAxisManager::getInstance().setStrategy(strategyName);
    }

    inline std::vector<std::string> getAvailableExplosionStrategies()
    {
        return ExplosionAxisManager::getInstance().getAvailableStrategies();
    }

    inline ExplosionAxisConfig &getExplosionAxisConfig()
    {
        return ExplosionAxisManager::getInstance().getConfig();
    }

    inline void applyExplosionAxisConfig(const ExplosionAxisConfig &config)
    {
        ExplosionAxisManager::getInstance().applyConfig(config);
    }

    inline bool isCurrentStrategyPCLOptimized()
    {
        return ExplosionAxisManager::getInstance().isPCLOptimized();
    }

}

#endif // MC_EXPLOSION_AXIS_STRATEGY_H