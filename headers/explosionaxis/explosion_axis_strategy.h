#ifndef MC_EXPLOSION_AXIS_STRATEGY_H
#define MC_EXPLOSION_AXIS_STRATEGY_H

#include <vector>
#include <memory>
#include <string>
#include "data.h"


namespace MC {

// 前向声明
class RotationalSymmetryDetector;
class ReflectiveSymmetryDetector;
class PCAAnalyzer;
class MitraRotationalSymmetryDetector;
class MitraReflectiveSymmetryDetector;
class SimplePCLRotationalSymmetryDetector;
class SimplePCLReflectiveSymmetryDetector;
class EigenRotationalSymmetryDetector;
class EigenReflectiveSymmetryDetector;

// 爆炸轴设置结构
struct ExplosionAxisConfig {
    // 策略选择
    std::string strategyName = "PCA Strategy";

    // 旋转对称性参数
    int rotationSampleCount = 100;
    int rotationSymmetryOrder = 4;
    Vec3 rotationAxis = {0.0f, 0.0f, 1.0f};
    bool useCustomRotationAxis = false;
    
    // 镜像对称性参数
    int mirrorSampleCount = 100;
    Vec3 mirrorNormal = {0.0f, 0.0f, 1.0f};
    bool useCustomMirrorNormal = false;

    // PCA参数
    bool useLongestAxis = true;
    
    // 可用的策略列表
    static std::vector<std::string> getStrategyNames() {
        return {"Rotational Symmetry", "Reflective Symmetry", "PCA (Longest Axis)", "Combined"};
    }
    
    // 将UI策略名转换为内部策略名
    static std::string convertToInternalName(const std::string& uiName) {
        if (uiName == "Rotational Symmetry") return "Rotational Strategy";
        if (uiName == "Reflective Symmetry") return "Reflective Strategy";
        if (uiName == "PCA (Longest Axis)") return "PCA Strategy";
        if (uiName == "Combined") return "PCL Optimized Strategy";
        return uiName; // 如果已经是内部名称，则直接返回
    }
    
    // 将内部策略名转换为UI策略名
    static std::string convertToUIName(const std::string& internalName) {
        if (internalName == "Rotational Strategy") return "Rotational Symmetry";
        if (internalName == "Reflective Strategy") return "Reflective Symmetry";
        if (internalName == "PCA Strategy") return "PCA (Longest Axis)";
        if (internalName == "PCL Optimized Strategy") return "Combined";
        return internalName; // 如果未找到匹配，则直接返回
    }

    bool lastDetectionSuccessful = true;
    
    // 可以选择为每种策略添加单独的状态
    bool rotationalDetectionSuccessful = true;
    bool reflectiveDetectionSuccessful = true;
    bool pcaDetectionSuccessful = true;
};

// 爆炸轴计算策略基类
class ExplosionAxisStrategy {
public:
    virtual ~ExplosionAxisStrategy() = default;
    virtual Vec3 computeAxis(const std::vector<Vertex>& meshVertices) = 0;
    virtual std::string getName() const = 0;
    
    // 获取是否使用PCL优化的状态
    virtual bool isPCLOptimized() const { return false; }
    
    // 应用配置
    virtual void applyConfig(const ExplosionAxisConfig& config) {}
    
    // 工厂方法，根据策略名称创建对应的策略实例
    static std::shared_ptr<ExplosionAxisStrategy> create(const std::string& strategyName);
    
    // 列出所有可用的策略名称
    static std::vector<std::string> getAvailableStrategies();
};

// 旋转对称性策略
class RotationalStrategy : public ExplosionAxisStrategy {
public:
    RotationalStrategy();
    ~RotationalStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "Rotational Strategy"; }
    
    // 应用配置
    void applyConfig(const ExplosionAxisConfig& config) override;
    
private:
    std::shared_ptr<SimplePCLRotationalSymmetryDetector> m_detector;
};

// 反射对称性策略
class ReflectiveStrategy : public ExplosionAxisStrategy {
public:
    ReflectiveStrategy();
    ~ReflectiveStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "Reflective Strategy"; }
    
    // 应用配置
    void applyConfig(const ExplosionAxisConfig& config) override;
    
private:
    std::shared_ptr<SimplePCLReflectiveSymmetryDetector> m_detector;
};

// PCA策略
class PCAStrategy : public ExplosionAxisStrategy {
public:
    PCAStrategy();
    ~PCAStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "PCA Strategy"; }
    
    // 应用配置
    void applyConfig(const ExplosionAxisConfig& config) override;
    
private:
    std::shared_ptr<PCAAnalyzer> m_analyzer;
    bool m_useLongestAxis = true;
};

// 组合式策略 - 按优先级尝试不同方法：旋转对称性 -> 反射对称性 -> PCA
class CombinedStrategy : public ExplosionAxisStrategy {
public:
    CombinedStrategy();
    ~CombinedStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "Combined Strategy"; }
    
    // 应用配置
    void applyConfig(const ExplosionAxisConfig& config) override;
    
private:
    std::shared_ptr<MitraRotationalSymmetryDetector> m_rotationDetector;
    std::shared_ptr<MitraReflectiveSymmetryDetector> m_reflectionDetector;
    std::shared_ptr<PCAAnalyzer> m_pcaAnalyzer;
};

// PCL优化的组合式策略 - 使用PCL库实现的检测器提高性能
class PCLOptimizedStrategy : public ExplosionAxisStrategy {
public:
    PCLOptimizedStrategy();
    ~PCLOptimizedStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "PCL Optimized Strategy"; }
    bool isPCLOptimized() const override { return true; }
    
    // 应用配置
    void applyConfig(const ExplosionAxisConfig& config) override;
    
private:
    std::shared_ptr<SimplePCLRotationalSymmetryDetector> m_rotationDetector;
    std::shared_ptr<SimplePCLReflectiveSymmetryDetector> m_reflectionDetector;
    std::shared_ptr<PCAAnalyzer> m_pcaAnalyzer;
};

// Eigen优化的组合式策略 - 使用Eigen库实现的检测器提高性能和兼容性
class EigenStrategy : public ExplosionAxisStrategy {
public:
    EigenStrategy();
    ~EigenStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "Eigen Strategy"; }
    
    // 应用配置
    void applyConfig(const ExplosionAxisConfig& config) override;
    
private:
    std::shared_ptr<EigenRotationalSymmetryDetector> m_rotationDetector;
    std::shared_ptr<EigenReflectiveSymmetryDetector> m_reflectionDetector;
    std::shared_ptr<PCAAnalyzer> m_pcaAnalyzer;
};

// 爆炸轴计算管理器 - 单例模式
class ExplosionAxisManager {
public:
    // 获取单例实例
    static ExplosionAxisManager& getInstance();
    
    // 设置当前使用的策略
    void setStrategy(const std::string& strategyName);
    void setStrategy(std::shared_ptr<ExplosionAxisStrategy> strategy);
    
    // 使用当前策略计算爆炸轴
    Vec3 computeExplosionAxis(const std::vector<Vertex>& meshVertices);
    Vec3 computeExplosionAxis(const Mesh& mesh);
    
    // 获取当前策略名称
    std::string getCurrentStrategyName() const;
    std::vector<std::string> getAvailableStrategies() const;
    
    // 检查当前策略是否使用PCL优化
    bool isPCLOptimized() const;
    
    // 获取当前配置
    ExplosionAxisConfig& getConfig() { return m_config; }
    const ExplosionAxisConfig& getConfig() const { return m_config; }
    
    // 应用配置到当前策略
    void applyConfig(const ExplosionAxisConfig& config);

    // 获取上一次成功的爆炸轴
    const Vec3& getLastSuccessfulAxis() const { return m_lastSuccessfulAxis; }
    
    // 设置上一次成功的爆炸轴
    void setLastSuccessfulAxis(const Vec3& axis) { m_lastSuccessfulAxis = axis; }

    // 检测是否成功的方法
    bool wasLastDetectionSuccessful() const { return m_config.lastDetectionSuccessful; }
    
    // 设置检测状态
    void setDetectionStatus(bool success) { 
        m_config.lastDetectionSuccessful = success; 
    }
    
    // 为各种策略设置具体的检测状态
    void setRotationalDetectionStatus(bool success) {
        m_config.rotationalDetectionSuccessful = success;
        // 同时更新总体状态
        m_config.lastDetectionSuccessful = success;
    }
    
    void setReflectiveDetectionStatus(bool success) {
        m_config.reflectiveDetectionSuccessful = success;
        // 同时更新总体状态
        m_config.lastDetectionSuccessful = success;
    }
    
private:
    // 私有构造函数（单例模式）
    ExplosionAxisManager();
    
    // 禁止拷贝和赋值（单例模式）
    ExplosionAxisManager(const ExplosionAxisManager&) = delete;
    ExplosionAxisManager& operator=(const ExplosionAxisManager&) = delete;
    
    std::shared_ptr<ExplosionAxisStrategy> m_currentStrategy;
    ExplosionAxisConfig m_config;
    Vec3 m_lastSuccessfulAxis = {0.0f, 0.0f, 1.0f};

};

// 暴露给main函数的API函数
inline Vec3 computeExplosionAxis(const std::vector<Vertex>& meshVertices) {
    return ExplosionAxisManager::getInstance().computeExplosionAxis(meshVertices);
}

inline Vec3 computeExplosionAxis(const Mesh& mesh) {
    return ExplosionAxisManager::getInstance().computeExplosionAxis(mesh);
}

inline std::string getCurrentExplosionStrategyName() {
    return ExplosionAxisManager::getInstance().getCurrentStrategyName();
}

inline void setExplosionStrategy(const std::string& strategyName) {
    ExplosionAxisManager::getInstance().setStrategy(strategyName);
}

inline std::vector<std::string> getAvailableExplosionStrategies() {
    return ExplosionAxisManager::getInstance().getAvailableStrategies();
}

// 爆炸轴配置相关函数
inline ExplosionAxisConfig& getExplosionAxisConfig() {
    return ExplosionAxisManager::getInstance().getConfig();
}

inline void applyExplosionAxisConfig(const ExplosionAxisConfig& config) {
    ExplosionAxisManager::getInstance().applyConfig(config);
}

inline bool isCurrentStrategyPCLOptimized() {
    return ExplosionAxisManager::getInstance().isPCLOptimized();
}

} // namespace MC

#endif // MC_EXPLOSION_AXIS_STRATEGY_H