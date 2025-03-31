#include "explosionaxis/explosion_axis_strategy.h"
#include "explosionaxis/mitra_rotational_symmetry_detector.h" 
#include "explosionaxis/mitra_reflective_symmetry_detector.h"
#include "explosionaxis/pcl_rotational_symmetry_detector.h"
#include "explosionaxis/pcl_reflective_symmetry_detector.h"
#include "explosionaxis/eigen_rotational_symmetry_detector.h"
#include "explosionaxis/eigen_reflective_symmetry_detector.h"
#include "explosionaxis/pca_analyzer.h"
#include <iostream>

namespace MC {

// 工厂方法实现
std::shared_ptr<ExplosionAxisStrategy> ExplosionAxisStrategy::create(const std::string& strategyName) {
    if (strategyName == "Combined Strategy") {
        return std::make_shared<CombinedStrategy>();
    }
    else if (strategyName == "PCL Optimized Strategy") {
        return std::make_shared<PCLOptimizedStrategy>();
    }
    else if (strategyName == "Eigen Strategy") {
        return std::make_shared<EigenStrategy>();
    }
    return std::make_shared<CombinedStrategy>();
}

// 获取可用策略列表
std::vector<std::string> ExplosionAxisStrategy::getAvailableStrategies() {
    return {"Combined Strategy", "PCL Optimized Strategy", "Eigen Strategy"};
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
    std::cout << "Use combined strategies detecting explosion axis." << std::endl;
    
    // 步骤1: 检测旋转对称性
    Vec3 rotationAxis;
    int symmetryOrder;
    if (m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder)) {
        std::cout << "Use rotational axis (order " << symmetryOrder << ")" << std::endl;
        return rotationAxis;
    }
    
    // 步骤2: 如果没有旋转对称性，检测反射对称性
    Vec3 reflectiveNormal;
    if (m_reflectionDetector->detect(meshVertices, reflectiveNormal)) {
        std::cout << "Use reflection axis." << std::endl;
        return m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
    }
    
    // 步骤3: 如果没有任何对称性，使用3D PCA
    std::cout << "No symmetry detected, using 3D PCA." << std::endl;
    return m_pcaAnalyzer->compute3DPCA(meshVertices);
}

// PCLOptimizedStrategy 构造函数
PCLOptimizedStrategy::PCLOptimizedStrategy() {
    // 创建PCL优化的旋转对称性检测器
    m_rotationDetector = std::make_unique<SimplePCLRotationalSymmetryDetector>();
    
    // 创建PCL优化的反射对称性检测器
    m_reflectionDetector = std::make_unique<SimplePCLReflectiveSymmetryDetector>();
    
    // 创建PCA分析器
    m_pcaAnalyzer = std::make_unique<PCAAnalyzer>();
    
    std::cout << "Created PCL Optimized Strategy using PCL detectors" << std::endl;
}

// PCLOptimizedStrategy 析构函数
PCLOptimizedStrategy::~PCLOptimizedStrategy() {
    // 智能指针会自动处理内存释放
}

// PCLOptimizedStrategy 计算爆炸轴实现
Vec3 PCLOptimizedStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Using PCL Optimized Strategy for explosion axis detection" << std::endl;
    
    // 步骤1: 检测旋转对称性
    Vec3 rotationAxis;
    int symmetryOrder;
    
    if (m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder)) {
        std::cout << "Using PCL detected rotational symmetry axis (order " << symmetryOrder << ")" << std::endl;
        return rotationAxis;
    }
    
    // 步骤2: 如果没有旋转对称性，检测反射对称性
    Vec3 reflectiveNormal;
    if (m_reflectionDetector->detect(meshVertices, reflectiveNormal)) {
        std::cout << "Using PCL detected reflective symmetry plane" << std::endl;
        return m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
    }
    
    // 步骤3: 如果没有任何对称性，使用3D PCA
    std::cout << "No symmetry detected with PCL, using 3D PCA" << std::endl;
    return m_pcaAnalyzer->compute3DPCA(meshVertices);
}

// EigenStrategy 构造函数
EigenStrategy::EigenStrategy() {
    // 创建Eigen优化的旋转对称性检测器
    m_rotationDetector = std::make_unique<EigenRotationalSymmetryDetector>();
    
    // 创建Eigen优化的反射对称性检测器
    m_reflectionDetector = std::make_unique<EigenReflectiveSymmetryDetector>();
    
    // 创建PCA分析器
    m_pcaAnalyzer = std::make_unique<PCAAnalyzer>();
    
    std::cout << "Created Eigen Strategy using Eigen detectors" << std::endl;
}

// EigenStrategy 析构函数
EigenStrategy::~EigenStrategy() {
    // 智能指针会自动处理内存释放
}

// EigenStrategy 计算爆炸轴实现
Vec3 EigenStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Using Eigen Strategy for explosion axis detection" << std::endl;
    
    // 步骤1: 检测旋转对称性
    Vec3 rotationAxis;
    int symmetryOrder;
    
    if (m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder)) {
        std::cout << "Using Eigen detected rotational symmetry axis (order " << symmetryOrder << ")" << std::endl;
        return rotationAxis;
    }
    
    // 步骤2: 如果没有旋转对称性，检测反射对称性
    Vec3 reflectiveNormal;
    if (m_reflectionDetector->detect(meshVertices, reflectiveNormal)) {
        std::cout << "Using Eigen detected reflective symmetry plane" << std::endl;
        return m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
    }
    
    // 步骤3: 如果没有任何对称性，使用3D PCA
    std::cout << "No symmetry detected with Eigen, using 3D PCA" << std::endl;
    return m_pcaAnalyzer->compute3DPCA(meshVertices);
}

// 单例模式获取ExplosionAxisManager实例
ExplosionAxisManager& ExplosionAxisManager::getInstance() {
    static ExplosionAxisManager instance;
    return instance;
}

// ExplosionAxisManager 构造函数
ExplosionAxisManager::ExplosionAxisManager() {
    // 默认使用Eigen优化策略以获得更好的性能和兼容性
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