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

// Factory method implementation
std::shared_ptr<ExplosionAxisStrategy> ExplosionAxisStrategy::create(const std::string& strategyName) {
    if (strategyName == "Rotational Strategy") {
        return std::make_shared<RotationalStrategy>();
    } else if (strategyName == "Reflective Strategy") {
        return std::make_shared<ReflectiveStrategy>();
    } else if (strategyName == "PCA Strategy") {
        return std::make_shared<PCAStrategy>();
    } else if (strategyName == "Combined Strategy") {
        return std::make_shared<CombinedStrategy>();
    } else if (strategyName == "PCL Optimized Strategy") {
        return std::make_shared<PCLOptimizedStrategy>();
    } else if (strategyName == "Eigen Strategy") {
        return std::make_shared<EigenStrategy>();
    }
    
    std::cerr << "Unknown strategy name: " << strategyName << ", using Combined Strategy instead.\n";
    return std::make_shared<CombinedStrategy>();
}

// List all available strategy names
std::vector<std::string> ExplosionAxisStrategy::getAvailableStrategies() {
    return {
        "Rotational Strategy",
        "Reflective Strategy",
        "PCA Strategy",
        "Combined Strategy",
        "PCL Optimized Strategy",
        "Eigen Strategy"
    };
}

//-------------- Rotational Strategy --------------//
RotationalStrategy::RotationalStrategy() 
    : m_detector(std::make_shared<MitraRotationalSymmetryDetector>()) {
}

RotationalStrategy::~RotationalStrategy() = default;

void RotationalStrategy::applyConfig(const ExplosionAxisConfig& config) {
    if (m_detector) {
        m_detector->setSampleCount(config.rotationSampleCount);
        m_detector->setSymmetryOrder(config.rotationSymmetryOrder);
        m_detector->useCustomAxis(config.useCustomRotationAxis);
        m_detector->setCustomAxis(config.rotationAxis);
        
        std::cout << "Applied config to rotational strategy: "
                  << "samples=" << config.rotationSampleCount
                  << ", order=" << config.rotationSymmetryOrder
                  << ", custom axis=" << (config.useCustomRotationAxis ? "yes" : "no")
                  << std::endl;
    }
}

Vec3 RotationalStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    Vec3 axis = {0, 0, 1}; // 默认轴为z轴
    int symmetryOrder = 0;
    
    bool success = m_detector->detect(meshVertices, axis, symmetryOrder);
    
    // 更新检测状态
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(success);
    
    if (!success) {
        std::cout << "Rotational symmetry detection failed. Falling back to previous axis." << std::endl;
        // 使用上一次成功的轴
        return ExplosionAxisManager::getInstance().getLastSuccessfulAxis();
    }
    
    // 检测成功，保存这个轴
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}

//-------------- Reflective Strategy --------------//
ReflectiveStrategy::ReflectiveStrategy() 
    : m_detector(std::make_shared<MitraReflectiveSymmetryDetector>()) {
}

ReflectiveStrategy::~ReflectiveStrategy() = default;

void ReflectiveStrategy::applyConfig(const ExplosionAxisConfig& config) {
    if (m_detector) {
        m_detector->setSampleCount(config.mirrorSampleCount);
        m_detector->useCustomNormal(config.useCustomMirrorNormal);
        m_detector->setCustomNormal(config.mirrorNormal);
        
        std::cout << "Applied config to reflective strategy: "
                  << "samples=" << config.mirrorSampleCount
                  << ", custom normal=" << (config.useCustomMirrorNormal ? "yes" : "no")
                  << std::endl;
    }
}

Vec3 ReflectiveStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    Vec3 normal = {0, 0, 1}; // 默认法向为z轴
    
    bool success = m_detector->detect(meshVertices, normal);
    
    // 更新检测状态
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(success);
    
    if (!success) {
        std::cout << "Reflective symmetry detection failed. Falling back to previous axis." << std::endl;
        return ExplosionAxisManager::getInstance().getLastSuccessfulAxis();
    }
    
    // 检测成功，保存这个轴
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(normal);
    return normal;
}

//-------------- PCA Strategy --------------//
PCAStrategy::PCAStrategy() 
    : m_analyzer(std::make_shared<PCAAnalyzer>()) {
}

PCAStrategy::~PCAStrategy() = default;

void PCAStrategy::applyConfig(const ExplosionAxisConfig& config) {
    if (m_analyzer) {
        m_analyzer->setUseLongestAxis(config.useLongestAxis);
        m_useLongestAxis = config.useLongestAxis;
        
        std::cout << "Applied config to PCA strategy: "
                  << "use longest axis=" << (config.useLongestAxis ? "yes" : "no")
                  << std::endl;
    }
}

Vec3 PCAStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    // PCA总是成功的
    ExplosionAxisManager::getInstance().setDetectionStatus(true);
    
    // 计算主轴
    Vec3 axis = m_analyzer->analyzePrincipalAxis(meshVertices);
    
    // 保存轴
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}

//-------------- Combined Strategy --------------//
CombinedStrategy::CombinedStrategy() 
    : m_rotationDetector(std::make_shared<MitraRotationalSymmetryDetector>()),
      m_reflectionDetector(std::make_shared<MitraReflectiveSymmetryDetector>()),
      m_pcaAnalyzer(std::make_shared<PCAAnalyzer>()) {
}

CombinedStrategy::~CombinedStrategy() = default;

void CombinedStrategy::applyConfig(const ExplosionAxisConfig& config) {
    // Apply rotational symmetry detector configuration
    if (m_rotationDetector) {
        m_rotationDetector->setSampleCount(config.rotationSampleCount);
        m_rotationDetector->setSymmetryOrder(config.rotationSymmetryOrder);
        m_rotationDetector->useCustomAxis(config.useCustomRotationAxis);
        m_rotationDetector->setCustomAxis(config.rotationAxis);
    }
    
    // Apply reflective symmetry detector configuration
    if (m_reflectionDetector) {
        m_reflectionDetector->setSampleCount(config.mirrorSampleCount);
        m_reflectionDetector->useCustomNormal(config.useCustomMirrorNormal);
        m_reflectionDetector->setCustomNormal(config.mirrorNormal);
    }
    
    // Apply PCA configuration
    if (m_pcaAnalyzer) {
        m_pcaAnalyzer->setUseLongestAxis(config.useLongestAxis);
    }
    
    std::cout << "Applied config to combined strategy" << std::endl;
}

Vec3 CombinedStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    // 优先尝试旋转对称性
    Vec3 axis;
    int symmetryOrder;
    
    std::cout << "Attempting to detect rotational symmetry..." << std::endl;
    bool rotSuccess = m_rotationDetector->detect(meshVertices, axis, symmetryOrder);
    
    // 更新旋转检测状态
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(rotSuccess);
    
    if (rotSuccess) {
        std::cout << "Found rotational symmetry axis: (" << axis.x << ", " << axis.y << ", " << axis.z << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // 如果旋转对称性检测失败，尝试反射对称性
    std::cout << "Attempting to detect reflective symmetry..." << std::endl;
    bool refSuccess = m_reflectionDetector->detect(meshVertices, axis);
    
    // 更新反射检测状态
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(refSuccess);
    
    if (refSuccess) {
        std::cout << "Found reflective symmetry plane normal: (" << axis.x << ", " << axis.y << ", " << axis.z << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // 如果两种对称性检测都失败，使用PCA
    std::cout << "No symmetry detected. Using PCA analysis..." << std::endl;
    
    // PCA总是成功的
    ExplosionAxisManager::getInstance().setDetectionStatus(true);
    
    axis = m_pcaAnalyzer->analyzePrincipalAxis(meshVertices);
    std::cout << "Using PCA principal axis: (" << axis.x << ", " << axis.y << ", " << axis.z << ")" << std::endl;
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}

//-------------- PCL Optimized Strategy --------------//
PCLOptimizedStrategy::PCLOptimizedStrategy() {
    // In a real implementation, an instance of the PCL detector would be created here
    m_rotationDetector = std::make_unique<SimplePCLRotationalSymmetryDetector>();
    
    // 创建Eigen优化的反射对称性检测器
    m_reflectionDetector = std::make_unique<SimplePCLReflectiveSymmetryDetector>();
    
    // 创建PCA分析器
    m_pcaAnalyzer = std::make_unique<PCAAnalyzer>();
    std::cout << "Creating PCL optimized strategy (placeholder implementation)" << std::endl;
}

void PCLOptimizedStrategy::applyConfig(const ExplosionAxisConfig& config) {
    // Apply rotational symmetry detector configuration
    std::cout << "Applied config to combined strategy" << std::endl;
}

PCLOptimizedStrategy::~PCLOptimizedStrategy() = default;

// PCLOptimizedStrategy 计算爆炸轴实现
Vec3 PCLOptimizedStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Using PCL Optimized Strategy for explosion axis detection" << std::endl;
    
    // 步骤1: 检测旋转对称性
    Vec3 rotationAxis;
    int symmetryOrder;
    
    bool rotSuccess = m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder);
    
    // 更新检测状态
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(rotSuccess);
    
    if (rotSuccess) {
        std::cout << "Using PCL detected rotational symmetry axis (order " << symmetryOrder << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(rotationAxis);
        return rotationAxis;
    }
    
    // 步骤2: 如果没有旋转对称性，检测反射对称性
    Vec3 reflectiveNormal;
    bool refSuccess = m_reflectionDetector->detect(meshVertices, reflectiveNormal);
    
    // 更新检测状态
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(refSuccess);
    
    if (refSuccess) {
        std::cout << "Using PCL detected reflective symmetry plane" << std::endl;
        Vec3 axis = m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // 步骤3: 如果没有任何对称性，使用3D PCA
    std::cout << "No symmetry detected with PCL, using 3D PCA" << std::endl;
    
    // PCA总是成功的
    ExplosionAxisManager::getInstance().setDetectionStatus(true);
    
    Vec3 axis = m_pcaAnalyzer->compute3DPCA(meshVertices);
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}

//-------------- EigenStrategy --------------//

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

void EigenStrategy::applyConfig(const ExplosionAxisConfig& config) {
    // Apply rotational symmetry detector configuration
    std::cout << "Applied config to combined strategy" << std::endl;
}

// EigenStrategy 计算爆炸轴实现
Vec3 EigenStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Using Eigen Strategy for explosion axis detection" << std::endl;
    
    // 步骤1: 检测旋转对称性
    Vec3 rotationAxis;
    int symmetryOrder;
    
    bool rotSuccess = m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder);
    
    // 更新检测状态
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(rotSuccess);
    
    if (rotSuccess) {
        std::cout << "Using Eigen detected rotational symmetry axis (order " << symmetryOrder << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(rotationAxis);
        return rotationAxis;
    }
    
    // 步骤2: 如果没有旋转对称性，检测反射对称性
    Vec3 reflectiveNormal;
    bool refSuccess = m_reflectionDetector->detect(meshVertices, reflectiveNormal);
    
    // 更新检测状态
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(refSuccess);
    
    if (refSuccess) {
        std::cout << "Using Eigen detected reflective symmetry plane" << std::endl;
        Vec3 axis = m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // 步骤3: 如果没有任何对称性，使用3D PCA
    std::cout << "No symmetry detected with Eigen, using 3D PCA" << std::endl;
    
    // PCA总是成功的
    ExplosionAxisManager::getInstance().setDetectionStatus(true);
    
    Vec3 axis = m_pcaAnalyzer->compute3DPCA(meshVertices);
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}
//-------------- Explosion Axis Manager --------------//
ExplosionAxisManager& ExplosionAxisManager::getInstance() {
    static ExplosionAxisManager instance;
    return instance;
}

ExplosionAxisManager::ExplosionAxisManager() {
    // Default to using the Combined Strategy
    m_currentStrategy = std::make_shared<CombinedStrategy>();
    m_config.strategyName = "Combined Strategy";
}

void ExplosionAxisManager::setStrategy(const std::string& strategyName) {
    m_currentStrategy = ExplosionAxisStrategy::create(strategyName);
    m_config.strategyName = strategyName;
    
    // Apply the current configuration to the new strategy
    m_currentStrategy->applyConfig(m_config);
    
    std::cout << "Switched to strategy: " << strategyName << std::endl;
}

void ExplosionAxisManager::setStrategy(std::shared_ptr<ExplosionAxisStrategy> strategy) {
    if (strategy) {
        m_currentStrategy = strategy;
        m_config.strategyName = strategy->getName();
        
        // Apply the current configuration to the new strategy
        m_currentStrategy->applyConfig(m_config);
        
        std::cout << "Switched to strategy: " << m_config.strategyName << std::endl;
    }
}

Vec3 ExplosionAxisManager::computeExplosionAxis(const std::vector<Vertex>& meshVertices) {
    if (m_currentStrategy) {
        std::cout << "Using strategy '" << m_currentStrategy->getName() << "' to compute explosion axis" << std::endl;
        return m_currentStrategy->computeAxis(meshVertices);
    }
    
    // Default return z-axis
    std::cout << "Warning: No valid strategy set, using default z-axis as explosion axis" << std::endl;
    return Vec3(0, 0, 1);
}

Vec3 ExplosionAxisManager::computeExplosionAxis(const Mesh& mesh) {
    return computeExplosionAxis(mesh.vertices);
}

std::string ExplosionAxisManager::getCurrentStrategyName() const {
    if (m_currentStrategy) {
        return m_currentStrategy->getName();
    }
    return "No strategy set";
}

std::vector<std::string> ExplosionAxisManager::getAvailableStrategies() const {
    return ExplosionAxisStrategy::getAvailableStrategies();
}

bool ExplosionAxisManager::isPCLOptimized() const {
    if (m_currentStrategy) {
        return m_currentStrategy->isPCLOptimized();
    }
    return false;
}

void ExplosionAxisManager::applyConfig(const ExplosionAxisConfig& config) {
    // Update stored configuration
    m_config = config;
    
    // If the strategy name has changed, switch strategies
    if (m_config.strategyName != m_currentStrategy->getName()) {
        setStrategy(m_config.strategyName);
    } else if (m_currentStrategy) {
        // Otherwise, apply the configuration to the current strategy
        m_currentStrategy->applyConfig(m_config);
    }
}

} // namespace MC
