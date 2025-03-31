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

// 爆炸轴计算策略基类
class ExplosionAxisStrategy {
public:
    virtual ~ExplosionAxisStrategy() = default;
    virtual Vec3 computeAxis(const std::vector<Vertex>& meshVertices) = 0;
    virtual std::string getName() const = 0;
    
    // 获取是否使用PCL优化的状态
    virtual bool isPCLOptimized() const { return false; }
    
    // 工厂方法，根据策略名称创建对应的策略实例
    static std::shared_ptr<ExplosionAxisStrategy> create(const std::string& strategyName);
    
    // 列出所有可用的策略名称
    static std::vector<std::string> getAvailableStrategies();
};

// 组合式策略 - 按优先级尝试不同方法：旋转对称性 -> 反射对称性 -> PCA
class CombinedStrategy : public ExplosionAxisStrategy {
public:
    CombinedStrategy(const std::string& rotationDetectorName = "Mitra", 
                    const std::string& reflectionDetectorName = "Mitra");
    ~CombinedStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "Combined Strategy"; }
    
private:
    std::unique_ptr<RotationalSymmetryDetector> m_rotationDetector;
    std::unique_ptr<ReflectiveSymmetryDetector> m_reflectionDetector;
    std::unique_ptr<PCAAnalyzer> m_pcaAnalyzer;
};

// PCL优化的组合式策略 - 使用PCL库实现的检测器提高性能
class PCLOptimizedStrategy : public ExplosionAxisStrategy {
public:
    PCLOptimizedStrategy();
    ~PCLOptimizedStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "PCL Optimized Strategy"; }
    bool isPCLOptimized() const override { return true; }
    
private:
    std::unique_ptr<RotationalSymmetryDetector> m_rotationDetector;
    std::unique_ptr<ReflectiveSymmetryDetector> m_reflectionDetector;
    std::unique_ptr<PCAAnalyzer> m_pcaAnalyzer;
};

// Eigen优化的组合式策略 - 使用Eigen库实现的检测器提高性能和兼容性
class EigenStrategy : public ExplosionAxisStrategy {
public:
    EigenStrategy();
    ~EigenStrategy();
    
    Vec3 computeAxis(const std::vector<Vertex>& meshVertices) override;
    std::string getName() const override { return "Eigen Strategy"; }
    
private:
    std::unique_ptr<RotationalSymmetryDetector> m_rotationDetector;
    std::unique_ptr<ReflectiveSymmetryDetector> m_reflectionDetector;
    std::unique_ptr<PCAAnalyzer> m_pcaAnalyzer;
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
    
private:
    // 私有构造函数（单例模式）
    ExplosionAxisManager();
    
    // 禁止拷贝和赋值（单例模式）
    ExplosionAxisManager(const ExplosionAxisManager&) = delete;
    ExplosionAxisManager& operator=(const ExplosionAxisManager&) = delete;
    
    std::shared_ptr<ExplosionAxisStrategy> m_currentStrategy;
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

} // namespace MC

#endif // MC_EXPLOSION_AXIS_STRATEGY_H