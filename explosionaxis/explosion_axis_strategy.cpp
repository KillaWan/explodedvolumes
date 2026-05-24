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
    : m_detector(std::make_shared<SimplePCLRotationalSymmetryDetector>()) {
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
    Vec3 axis = {0, 0, 1}; // default z axis
    int symmetryOrder = 0;
    
    bool success = m_detector->detect(meshVertices, axis, symmetryOrder);
    
    // unpdate detection status
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(success);
    
    if (!success) {
        std::cout << "Rotational symmetry detection failed. Falling back to previous axis." << std::endl;
        // return last successful axis as fallback
        return ExplosionAxisManager::getInstance().getLastSuccessfulAxis();
    }
    
    // detection successful, save this axis
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}

//-------------- Reflective Strategy --------------//
ReflectiveStrategy::ReflectiveStrategy() 
    : m_detector(std::make_shared<SimplePCLReflectiveSymmetryDetector>()) {
}

ReflectiveStrategy::~ReflectiveStrategy() = default;

void ReflectiveStrategy::applyConfig(const ExplosionAxisConfig& config) {
    if (m_detector) {
        m_detector->setSampleCount(config.mirrorSampleCount);
        m_detector->useCustomAxis(config.useCustomMirrorNormal);
        m_detector->setCustomAxis(config.mirrorNormal);
        
        std::cout << "Applied config to reflective strategy: "
                  << "samples=" << config.mirrorSampleCount
                  << ", custom normal=" << (config.useCustomMirrorNormal ? "yes" : "no")
                  << std::endl;
    }
}

Vec3 ReflectiveStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    Vec3 normal = {0, 0, 1}; // default normal is the z-axis
    
    bool success = m_detector->detect(meshVertices, normal);
    
    // update detection status
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(success);
    
    if (!success) {
        std::cout << "Reflective symmetry detection failed. Falling back to previous axis." << std::endl;
        return ExplosionAxisManager::getInstance().getLastSuccessfulAxis();
    }
    
    // detection successful, save this axis
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
    // PCA is always successful, so we can directly compute and return the axis
    ExplosionAxisManager::getInstance().setDetectionStatus(true);
    
    // analyze principal axis using PCA
    Vec3 axis = m_analyzer->analyzePrincipalAxis(meshVertices);
    
    // save this axis as the last successful one
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
    // step 1: try rotational symmetry detection
    Vec3 axis;
    int symmetryOrder;
    
    std::cout << "Attempting to detect rotational symmetry..." << std::endl;
    bool rotSuccess = m_rotationDetector->detect(meshVertices, axis, symmetryOrder);
    
    // update detection status
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(rotSuccess);
    
    if (rotSuccess) {
        std::cout << "Found rotational symmetry axis: (" << axis.x << ", " << axis.y << ", " << axis.z << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // step 2: if no rotational symmetry, try reflective symmetry
    std::cout << "Attempting to detect reflective symmetry..." << std::endl;
    bool refSuccess = m_reflectionDetector->detect(meshVertices, axis);
    
    // update
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(refSuccess);
    
    if (refSuccess) {
        std::cout << "Found reflective symmetry plane normal: (" << axis.x << ", " << axis.y << ", " << axis.z << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // step 3: if no symmetry detected, use PCA
    std::cout << "No symmetry detected. Using PCA analysis..." << std::endl;
    
    // PCA is always successful
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
    
    // create PCL optimized reflective symmetry detector
    m_reflectionDetector = std::make_unique<SimplePCLReflectiveSymmetryDetector>();
    
    // create PCA analyzer
    m_pcaAnalyzer = std::make_unique<PCAAnalyzer>();
    std::cout << "Creating PCL optimized strategy (placeholder implementation)" << std::endl;
}

void PCLOptimizedStrategy::applyConfig(const ExplosionAxisConfig& config) {
    // configure rotation detector
    if (m_rotationDetector) {
        m_rotationDetector->setSampleCount(config.rotationSampleCount);
        m_rotationDetector->setSymmetryOrder(config.rotationSymmetryOrder);
        m_rotationDetector->useCustomAxis(config.useCustomRotationAxis);
        m_rotationDetector->setCustomAxis(config.rotationAxis);
    }
    // configure reflection detector
    if (m_reflectionDetector) {
        m_reflectionDetector->setSampleCount(config.mirrorSampleCount);
        m_reflectionDetector->useCustomAxis(config.useCustomMirrorNormal);
        m_reflectionDetector->setCustomAxis(config.mirrorNormal);
    }
    // configure PCA analyzer
    if (m_pcaAnalyzer) {
        m_pcaAnalyzer->setUseLongestAxis(config.useLongestAxis);
    }
    std::cout << "Applied config to PCL Optimized Strategy" << std::endl;
}

PCLOptimizedStrategy::~PCLOptimizedStrategy() = default;

// PCLOptimizedStrategy computeAxis implementation
Vec3 PCLOptimizedStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Using PCL Optimized Strategy for explosion axis detection" << std::endl;
    
    // step 1: try rotational symmetry detection
    Vec3 rotationAxis;
    int symmetryOrder;
    
    bool rotSuccess = m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder);
    
    // update detection status
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(rotSuccess);
    
    if (rotSuccess) {
        std::cout << "Using PCL detected rotational symmetry axis (order " << symmetryOrder << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(rotationAxis);
        return rotationAxis;
    }
    
    // step 2: if no rotational symmetry, try reflective symmetry
    Vec3 reflectiveNormal;
    bool refSuccess = m_reflectionDetector->detect(meshVertices, reflectiveNormal);
    
    // update detection status
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(refSuccess);
    
    if (refSuccess) {
        std::cout << "Using PCL detected reflective symmetry plane" << std::endl;
        Vec3 axis = m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // step 3: if no symmetry detected, use 3D PCA
    std::cout << "No symmetry detected with PCL, using 3D PCA" << std::endl;
    
    // PCA is always successful
    ExplosionAxisManager::getInstance().setDetectionStatus(true);
    
    Vec3 axis = m_pcaAnalyzer->compute3DPCA(meshVertices);
    ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
    return axis;
}

//-------------- EigenStrategy --------------//

// EigenStrategy constructor
EigenStrategy::EigenStrategy() {
    // create Eigen optimized rotational symmetry detector
    m_rotationDetector = std::make_unique<EigenRotationalSymmetryDetector>();
    
    // create Eigen optimized reflective symmetry detector
    m_reflectionDetector = std::make_unique<EigenReflectiveSymmetryDetector>();
    
    // create PCA analyzer
    m_pcaAnalyzer = std::make_unique<PCAAnalyzer>();
    
    std::cout << "Created Eigen Strategy using Eigen detectors" << std::endl;
}

// EigenStrategy destructor
EigenStrategy::~EigenStrategy() {
    // unique_ptr will automatically clean up
}

void EigenStrategy::applyConfig(const ExplosionAxisConfig& config) {
    // Apply rotational symmetry detector configuration
    std::cout << "Applied config to combined strategy" << std::endl;
}

// EigenStrategy computeAxis implementation
Vec3 EigenStrategy::computeAxis(const std::vector<Vertex>& meshVertices) {
    std::cout << "Using Eigen Strategy for explosion axis detection" << std::endl;
    
    // step 1: try rotational symmetry detection
    Vec3 rotationAxis;
    int symmetryOrder;
    
    bool rotSuccess = m_rotationDetector->detect(meshVertices, rotationAxis, symmetryOrder);
    
    // update detection status
    ExplosionAxisManager::getInstance().setRotationalDetectionStatus(rotSuccess);
    
    if (rotSuccess) {
        std::cout << "Using Eigen detected rotational symmetry axis (order " << symmetryOrder << ")" << std::endl;
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(rotationAxis);
        return rotationAxis;
    }
    
    // step 2: if no rotational symmetry, try reflective symmetry
    Vec3 reflectiveNormal;
    bool refSuccess = m_reflectionDetector->detect(meshVertices, reflectiveNormal);
    
    // update detection status
    ExplosionAxisManager::getInstance().setReflectiveDetectionStatus(refSuccess);
    
    if (refSuccess) {
        std::cout << "Using Eigen detected reflective symmetry plane" << std::endl;
        Vec3 axis = m_pcaAnalyzer->compute2DPCAOnPlane(meshVertices, reflectiveNormal);
        ExplosionAxisManager::getInstance().setLastSuccessfulAxis(axis);
        return axis;
    }
    
    // step 3: if no symmetry detected, use 3D PCA
    std::cout << "No symmetry detected with Eigen, using 3D PCA" << std::endl;
    
    // PCA is always successful
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
    m_currentStrategy = std::make_shared<PCAStrategy>();
    m_config.strategyName = "PCA Strategy";
}

void ExplosionAxisManager::setStrategy(const std::string& strategyName) {
    m_currentStrategy = ExplosionAxisStrategy::create(strategyName);
    m_config.strategyName = strategyName;
    
    // reset detection status for the new strategy
    m_config.rotationalDetectionSuccessful = true;
    m_config.reflectiveDetectionSuccessful = true;
    m_config.lastDetectionSuccessful = true;
    
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
    if (m_config.useCustomExplosionAxis) {
        std::cout << "Using custom explosion axis: (" 
                  << m_config.customExplosionAxis.x << ", " 
                  << m_config.customExplosionAxis.y << ", " 
                  << m_config.customExplosionAxis.z << ")" << std::endl;
        
        // directly return the custom axis from config
        return m_config.customExplosionAxis;
    }
    
    // if no custom axis, use the current strategy to compute the explosion axis
    if (m_currentStrategy) {
        std::cout << "Using strategy '" << m_currentStrategy->getName() << "' to compute explosion axis" << std::endl;
        return m_currentStrategy->computeAxis(meshVertices);
    }
    
    // fallback if no strategy is set
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
