#include "explosionaxis/explosion_axis_strategy.h"
#include "explosionaxis/mitra_rotational_symmetry_detector.h" 
#include "explosionaxis/mirta_reflective_symmetry_detector.h"
#include "explosionaxis/pca_analyzer.h"
#include <iostream>

namespace MC {

// 工厂方法实现
std::shared_ptr<ExplosionAxisStrategy> ExplosionAxisStrategy::create(const std::string& strategyName) {
    if (strategyName == "Combined Strategy") {
        return std::make_shared<CombinedStrategy>();
    }
    // 默认使用组合策略
    return std::make_shared<CombinedStrategy>();
}

// 获取可用策略列表
std::vector<std::string> ExplosionAxisStrategy::getAvailableStrategies() {
    return {"Combined Strategy"};
}

// CombinedStrategy 构造函数
CombinedStrategy::CombinedStrategy(const std::string& rotationDetectorName, 
                                   const std::string& reflectionDetectorName) {
    // 创建旋转对称性检测器
    if (rotationDetectorName == "Mitra") {
        m_rotationDetector = std::make_unique<MitraRotationalSymmetryDetector>();
    } else {
        // 其他检测器实现...
        m_rotationDetector = std::make_unique<MitraRotationalSymmetryDetector>();
    }
    
    // 创建反射对称性检测器
    if (reflectionDetectorName == "Mitra") {
        m_reflectionDetector = std::make_unique<MitraReflectiveSymmetryDetector>();
    } else {
        // 其他检测器实现...
        m_reflectionDetector = std::make_unique<MitraReflectiveSymmetryDetector>();
    }
    
    // 创建PCA分析器
    m_pcaAnalyzer = std::make_unique<PCAAnalyzer>();
}

// CombinedStrategy 析构函数
CombinedStrategy::~CombinedStrategy() {
    // 智能指针会自动处理内存释放
}

// CombinedStrategy 计算爆炸轴实现
Vec3 CombinedStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "使用组合策略计算爆炸轴" << std::endl;
    
    // 步骤1: 检测旋转对称性
    Vec3 rotationAxis;
    int symmetryOrder;
    if (m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder)) {
        std::cout << "使用旋转对称轴（阶数 " << symmetryOrder << "）" << std::endl;
        return rotationAxis;
    }
    
    // 步骤2: 如果没有旋转对称性，检测反射对称性
    Vec3 reflectiveNormal;
    if (m_reflectionDetector->detect(meshVertices, reflectiveNormal)) {
        std::cout << "使用镜面对称性和2D PCA" << std::endl;
        return m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
    }
    
    // 步骤3: 如果没有任何对称性，使用3D PCA
    std::cout << "未检测到对称性，使用3D PCA" << std::endl;
    return m_pcaAnalyzer->compute3DPCA(meshVertices);
}

// 单例模式获取ExplosionAxisManager实例
ExplosionAxisManager& ExplosionAxisManager::getInstance() {
    static ExplosionAxisManager instance;
    return instance;
}

// ExplosionAxisManager 构造函数
ExplosionAxisManager::ExplosionAxisManager() {
    // 默认使用组合策略
    m_currentStrategy = std::make_shared<CombinedStrategy>();
}

// 设置策略（通过名称）
void ExplosionAxisManager::setStrategy(const std::string& strategyName) {
    m_currentStrategy = ExplosionAxisStrategy::create(strategyName);
}

// 设置策略（通过对象）
void ExplosionAxisManager::setStrategy(std::shared_ptr<ExplosionAxisStrategy> strategy) {
    m_currentStrategy = strategy;
}

// 计算爆炸轴（使用顶点数组）
Vec3 ExplosionAxisManager::computeExplosionAxis(const std::vector<Vertex>& meshVertices) {
    if (!m_currentStrategy) {
        std::cerr << "未设置爆炸轴计算策略" << std::endl;
        return Vec3(0, 0, 1); // 默认使用 Z 轴
    }
    return m_currentStrategy->computeAxis(meshVertices);
}

// 计算爆炸轴（使用整个网格）
Vec3 ExplosionAxisManager::computeExplosionAxis(const Mesh& mesh) {
    return computeExplosionAxis(mesh.vertices);
}

// 获取当前策略名称
std::string ExplosionAxisManager::getCurrentStrategyName() const {
    return m_currentStrategy ? m_currentStrategy->getName() : "None";
}

// 获取可用策略列表
std::vector<std::string> ExplosionAxisManager::getAvailableStrategies() const {
    return ExplosionAxisStrategy::getAvailableStrategies();
}

} // namespace MC