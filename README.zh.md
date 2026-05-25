# ExplodedVolumes

[English](README.md) | [中文](README.zh.md) | [Nederlands](README.nl.md)

![Introduction](images/Screenshot_20240130_115627.png)

**ExplodedVolumes** 是一个基于 C++17 与 OpenGL 的体数据交互式可视化程序。它读取 NIfTI 体数据，利用 Marching Cubes 提取等值面，计算或选择爆炸轴，构造切割平面，将表面划分为若干组成部分，并在普通视图与爆炸视图两种模式下进行渲染。

本项目以源代码形式发布，同时作为 TU Eindhoven 课程项目的一部分。第三方头文件与特定平台的二进制库并未随仓库一并提供。编译之前，使用者需要依据自己的操作系统、CPU 架构与编译器工具链，重新构造本地依赖目录。

---

## 目录

- [1. 项目背景](#1-项目背景)
- [2. 结果展示](#2-结果展示)
- [3. 功能概述](#3-功能概述)
- [4. 方法流程](#4-方法流程)
- [5. 源代码结构](#5-源代码结构)
- [6. 依赖模型](#6-依赖模型)
  - [6.1 仅发布源代码的原则](#61-仅发布源代码的原则)
  - [6.2 共享依赖目录结构](#62-共享依赖目录结构)
  - [6.3 平台相关二进制文件规则](#63-平台相关二进制文件规则)
- [7. Windows 编译](#7-windows-编译)
- [8. macOS 编译](#8-macos-编译)
- [9. 运行程序](#9-运行程序)
- [10. 交互说明](#10-交互说明)
- [11. VS Code 配置说明](#11-vs-code-配置说明)
- [12. 已知限制](#12-已知限制)
- [13. 第三方组件与参考文献](#13-第三方组件与参考文献)

---

## 1. 项目背景

许多体数据包含难以仅从外部等值面充分理解的内部结构。爆炸视图的基本作用，是将一个表面按一定顺序切分为若干组成部分，并沿指定方向对这些部分进行平移。这样得到的可视化结果能够暴露隐藏几何结构，同时保留分离部分之间原有的空间关系。

在本实现中，输入数据是以 NIfTI 格式存储的科学或医学体数据。程序首先提取三角化等值面，然后估计候选爆炸方向，生成切割平面，将网格划分为不同组成部分，最后在交互式 OpenGL 查看器中渲染结果。渲染风格刻意强化边缘，以提高曲面结构的可读性。

---

## 2. 结果展示

### 主界面

![Main Interface](images/Iguana_1.png)

*主窗口，包括 Marching Cubes 控制面板、爆炸轴设置以及爆炸视图控制项。*

### 爆炸视图

| 普通视图 | 爆炸视图 |
|:---:|:---:|
| ![Iguana Normal](images/Iguana_1.png) | ![Iguana Exploded](images/Iguana_2.png) |
| ![chris_t1 Normal](images/chris_t1_1.png) | ![chris_t1 Exploded](images/chris_t1_2.png) |
| ![CT_Abdo Normal](images/CT_Abdo_1.png) | ![CT_Abdo Exploded](images/CT_Abdo_2.png) |
| ![CT_Philips Normal](images/CT_Philips_1.png) | ![CT_Philips Exploded](images/CT_Philips_2.png) |

*多个数据集在普通渲染模式与爆炸视图模式下的对比。*

---

## 3. 功能概述

当前实现提供以下功能：

- 通过原生文件对话框读取 `.nii` 格式的 NIfTI 体数据文件；
- 使用 Marching Cubes 提取三角化等值面；
- 在 OpenGL 3.3 Core Profile 上下文中渲染普通视图与爆炸视图；
- 通过 PCA、对称性检测以及组合策略估计或选择爆炸轴；
- 构造切割平面，并将网格划分为可分离的爆炸视图组成部分；
- 交互式调整等值面阈值与爆炸距离；
- 可视化爆炸轴；
- 通过鼠标旋转与缩放相机；
- 使用基于 framebuffer 的后处理过程实现边缘增强渲染；
- 通过 Dear ImGui 暴露运行时控制界面。

---

## 4. 方法流程

从数据流的角度看，本实现基于如下流程：

```text
NIfTI volume
    ↓
VolumeData representation
    ↓
Marching Cubes iso-surface extraction
    ↓
Explosion-axis estimation or selection
    ↓
Cutting-plane generation and selection
    ↓
Surface segmentation
    ↓
Normal or exploded-view rendering
    ↓
Post-processing and ImGui overlay
```

活动渲染循环位于 `main.cpp`。渲染逻辑与用户界面构造主要实现在 `visual.cpp` 中，后处理过程由 `post_processor.cpp` 负责。切割平面与爆炸视图逻辑位于 `planes/` 目录下，爆炸轴估计逻辑位于 `explosionaxis/` 目录下。

---

## 5. 源代码结构

本项目的源代码版本大致具有如下结构：

```text
.
├── main.cpp                         # 程序入口与活动渲染循环
├── data.cpp                         # NIfTI 读取与体数据工具
├── marching_cubes.cpp               # Marching Cubes 等值面提取
├── visual.cpp                       # GLFW/OpenGL/ImGui 渲染与界面
├── post_processor.cpp               # 基于 FBO 的后处理流程
├── glad.c                           # GLAD OpenGL 加载器实现
├── nifti1_io.c                      # NIfTI 读取器实现
├── znzlib.c                         # NIfTI 底层文件 I/O 辅助代码
├── file_dialog.h                    # portable-file-dialogs 封装
├── headers/
│   ├── data.h
│   ├── marching_cubes.h
│   ├── visual.h
│   ├── post_processor.h
│   ├── explosionaxis/               # 爆炸轴策略头文件
│   └── planes/                      # 切割平面与爆炸视图头文件
├── explosionaxis/                   # 爆炸轴策略实现
├── planes/                          # 切割平面、选择与爆炸视图代码
├── imgui/                           # 项目使用的 Dear ImGui 实现文件
└── dependencies/                    # 本地依赖目录，由使用者重建
```

通常情况下，`dependencies/` 目录不应提交到仓库。该目录应根据下文的平台相关编译说明在本地创建。

输入体数据不必存放在仓库中。若需要公开发布，应只包含允许公开传播、许可证明确且在必要时已经完成去标识化处理的体数据文件。

---

## 6. 依赖模型

### 6.1 仅发布源代码的原则

本项目以源代码形式发布，不包含第三方二进制依赖。这样可以避免随仓库附带的二进制库与不同编译器、操作系统或 CPU 架构不兼容。

具体来说，这意味着：

- 不应假设一个干净的源码包中已经包含填充好的依赖目录；
- 依赖目录需要在本地重新构造；
- Windows 编译只使用 Windows 兼容的库；
- macOS 编译只使用 macOS 兼容的库；
- 不应混用来自不同操作系统、CPU 架构或编译器工具链的二进制产物。

### 6.2 共享依赖目录结构

本文档中的编译命令假定如下本地目录结构。也可以使用等价路径，但对应的 include 与 library 参数需要相应调整。

```text
dependencies/
├── include/
│   ├── GLFW/                        # GLFW 头文件
│   ├── glad/                        # glad.h
│   ├── KHR/                         # khrplatform.h
│   ├── glm/                         # GLM 头文件
│   ├── Eigen/                       # Eigen 头文件，或使用外部 Eigen include 路径
│   ├── imgui.h
│   ├── imconfig.h
│   ├── imgui_internal.h
│   ├── imstb_rectpack.h
│   ├── imstb_textedit.h
│   ├── imstb_truetype.h
│   ├── imgui_impl_glfw.h
│   ├── imgui_impl_opengl3.h
│   ├── nifti1.h
│   ├── nifti1_io.h
│   ├── nifti1_io_version.h
│   ├── znzlib.h
│   ├── znzlib_version.h
│   └── portable-file-dialogs.h
└── library/
    └── platform-specific libraries, such as GLFW and OpenMP runtimes
```

程序使用的组件如下：

| 依赖 | 作用 | 典型处理方式 |
|---|---|---|
| C++17 compiler | 编译程序 | Clang、MinGW-w64 或 MSVC |
| OpenGL 3.3 | 渲染后端 | 由平台与显卡驱动提供 |
| GLFW | 窗口、OpenGL 上下文与输入 | 头文件 + 平台相关库 |
| GLAD | OpenGL 函数加载器 | 编译 `glad.c`，提供匹配的 `glad/` 与 `KHR/` 头文件 |
| Dear ImGui | 运行时 GUI | 编译随项目使用的 ImGui `.cpp` 文件，提供匹配头文件 |
| GLM | 图形数学 | 头文件库 |
| Eigen | 爆炸轴估计中的线性代数 | 头文件库 |
| NIfTI C files | 体数据读取 | 编译 `nifti1_io.c` 与 `znzlib.c`，提供匹配头文件 |
| portable-file-dialogs | 原生文件对话框 | 单头文件 |
| OpenMP | CPU 并行循环 | 编译器参数与运行时库 |
| zlib | 可选的压缩 NIfTI 支持 | 当启用 `HAVE_ZLIB` 或本地 NIfTI 配置需要时链接 |
| PCL / VTK / Boost | 当前 Makefile 使用的几何与渲染支持 | macOS 上通过 Homebrew 安装 |

### 6.3 平台相关二进制文件规则

已编译库必须与目标平台和编译器 ABI 匹配。

| 文件类型 | 常见上下文 |
|---|---|
| `.a` | Windows MinGW-w64 或兼容 GCC 风格工具链 |
| `.lib` | Windows MSVC |
| `.dll` | Windows 运行时库 |
| `.dylib` | macOS 动态库 |
| `.framework` | macOS 系统或框架依赖 |

例如，MSVC 不应链接 MinGW 的 `.a` 文件，macOS 的 `.dylib` 文件也不能用于 Windows 编译。Apple Silicon 编译需要 arm64 兼容依赖，Intel macOS 编译需要 x86_64 兼容依赖。

---

## 7. Windows 编译

Windows 编译应仅使用 Windows 兼容的头文件与库。不要复用 macOS `.dylib` 文件、Homebrew 路径或 macOS framework 参数。

创建本地依赖目录：

```powershell
mkdir dependencies
mkdir dependencies\include
mkdir dependencies\library
```

将[第 6.2 节](#62-共享依赖目录结构)列出的头文件放入 `dependencies\include`，并将匹配的 Windows GLFW/OpenMP 运行时库放入 `dependencies\library`。

### MinGW-w64

在项目根目录下通过 PowerShell 运行：

```powershell
$Sources = @(
  "glad.c", "znzlib.c", "nifti1_io.c",
  "imgui\imgui.cpp", "imgui\imgui_draw.cpp", "imgui\imgui_impl_glfw.cpp",
  "imgui\imgui_impl_opengl3.cpp", "imgui\imgui_tables.cpp", "imgui\imgui_widgets.cpp",
  "explosionaxis\eigen_reflective_symmetry_detector.cpp",
  "explosionaxis\eigen_rotational_symmetry_detector.cpp",
  "explosionaxis\explosion_axis_strategy.cpp",
  "explosionaxis\mitra_reflective_symmetry_detector.cpp",
  "explosionaxis\mitra_rotational_symmetry_detector.cpp",
  "explosionaxis\pca_analyzer.cpp",
  "explosionaxis\pcl_reflective_symmetry_detector.cpp",
  "explosionaxis\pcl_rotational_symmetry_detector.cpp",
  "explosionaxis\vector_ops.cpp",
  "planes\cutting_planes.cpp", "planes\exploded_view.cpp", "planes\selecting_planes.cpp",
  "data.cpp", "main.cpp", "marching_cubes.cpp", "post_processor.cpp", "visual.cpp"
)

g++ -std=c++17 -O2 -g -fopenmp `
  -I "dependencies\include" `
  -I "headers" `
  -I "headers\explosionaxis" `
  -I "headers\planes" `
  $Sources `
  -L "dependencies\library" `
  -lglfw3 -lopengl32 -lgdi32 -lole32 -lcomctl32 -loleaut32 -luuid `
  -o explodedvolumes-mingw.exe
```

如果使用动态链接的 GLFW，应将匹配的 GLFW `.dll` 复制到生成的可执行文件旁边。

### MSVC

在 x64 Native Tools Command Prompt for Visual Studio 中运行，或使用其他已经配置好 `cl.exe` 的 shell：

```bat
set GLFW_LIB=C:\path\to\glfw\lib-vc2022

cl /std:c++17 /EHsc /O2 /openmp ^
  /I dependencies\include ^
  /I headers ^
  /I headers\explosionaxis ^
  /I headers\planes ^
  glad.c znzlib.c nifti1_io.c ^
  imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_impl_glfw.cpp ^
  imgui\imgui_impl_opengl3.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp ^
  explosionaxis\eigen_reflective_symmetry_detector.cpp ^
  explosionaxis\eigen_rotational_symmetry_detector.cpp ^
  explosionaxis\explosion_axis_strategy.cpp ^
  explosionaxis\mitra_reflective_symmetry_detector.cpp ^
  explosionaxis\mitra_rotational_symmetry_detector.cpp ^
  explosionaxis\pca_analyzer.cpp ^
  explosionaxis\pcl_reflective_symmetry_detector.cpp ^
  explosionaxis\pcl_rotational_symmetry_detector.cpp ^
  explosionaxis\vector_ops.cpp ^
  planes\cutting_planes.cpp planes\exploded_view.cpp planes\selecting_planes.cpp ^
  data.cpp main.cpp marching_cubes.cpp post_processor.cpp visual.cpp ^
  /Fe:explodedvolumes-msvc.exe ^
  /link /LIBPATH:%GLFW_LIB% glfw3.lib opengl32.lib gdi32.lib ole32.lib comctl32.lib oleaut32.lib uuid.lib user32.lib shell32.lib
```

如果使用动态链接的 GLFW，应将匹配的 GLFW `.dll` 复制到 `explodedvolumes-msvc.exe` 旁边。

---

## 8. macOS 编译

当前的编译入口是仓库根目录下的 `Makefile`。

安装 Homebrew 依赖：

```sh
brew install glfw glm eigen boost pcl vtk libomp nlohmann-json
```

准备 Makefile 期望的本地依赖结构：

```sh
mkdir -p dependencies/include dependencies/library
ln -sf "$(brew --prefix glfw)/lib/libglfw.3.dylib" dependencies/library/libglfw.3.4.dylib
ln -sfn "$(brew --prefix vtk)" dependencies/VTK
ln -sfn "$(brew --prefix glm)/include/glm" dependencies/include/glm
ln -sfn "$(brew --prefix eigen)/include/eigen3/Eigen" dependencies/include/Eigen
ln -sfn "$(brew --prefix boost)/include/boost" dependencies/include/boost
```

[第 6.2 节](#62-共享依赖目录结构)列出的项目本地头文件也应能在 `dependencies/include` 下被找到。

编译：

```sh
make
```

如果本地 PCL 版本与 Makefile 默认值不同：

```sh
make PCL_VERSION=1.15.1
```

对于 Intel macOS 或自定义 Homebrew 前缀：

```sh
make BREW_PREFIX=/usr/local
```

清理编译产物：

```sh
make clean
```

---

## 9. 运行程序

macOS：

```sh
make run
```

Windows MinGW：

```powershell
.\explodedvolumes-mingw.exe
```

Windows MSVC：

```bat
explodedvolumes-msvc.exe
```

程序启动后会打开文件对话框。请选择具有使用权限、并在适用情况下具有再发布权限的 `.nii` 体数据文件。公开发布时，不应包含私有或可识别身份的医学数据。

---

## 10. 交互说明

| 操作 | 控制方式 |
|---|---|
| 旋转相机 | 鼠标左键拖动 |
| 缩放 | 鼠标滚轮 |
| 调整相机距离 | `+` / `-` |
| 关闭程序 | `Esc` |
| 改变等值面阈值 | ImGui Marching Cubes 控制面板 |
| 开关爆炸视图 | ImGui Explosion View 控制面板 |
| 调整爆炸距离 | ImGui Explosion View 控制面板 |
| 显示或隐藏爆炸轴 | ImGui Explosion Axis Settings 面板 |

---

## 11. VS Code 配置说明

对于 macOS，可创建名为 `Build OpenGL` 的 VS Code build task，并使其运行：

```sh
make
```

launch configuration 可以使用：

```json
{
  "program": "${workspaceFolder}/app",
  "cwd": "${workspaceFolder}",
  "preLaunchTask": "Build OpenGL"
}
```

应使用 `${workspaceFolder}`，而不是 `${fileDirname}`。后者会随当前聚焦文件所在目录变化，可能导致编译路径或运行路径不一致。

---

## 12. 已知限制

- 仓库目前尚未提供跨平台 CMake 配置。
- 仅源代码发布的形式要求使用者在本地重新构造第三方头文件与平台相关库。
- 当前 Makefile 面向 macOS，并且在 PCL 或 VTK 路径变化时对版本较为敏感。
- 当前文件对话框主要面向 `.nii` 输入。若需便利地处理 `.nii.gz`，可能需要进一步工作。
- `imgui.ini` 可能由 Dear ImGui 生成或更新，用于保存 UI 布局。这是正常现象，并不表示输入体数据被修改。

---

## 13. 第三方组件与参考文献

在编译之前，应检查本地编译所使用的所有第三方组件的情况：

- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLM](https://github.com/g-truc/glm)
- [Eigen](https://eigen.tuxfamily.org/)
- [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs)
- [NIfTI C library](https://nifti.nimh.nih.gov/)
- [OpenMP](https://www.openmp.org/)
- [PCL](https://pointclouds.org/)
- [VTK](https://vtk.org/)

本项目的可视化概念遵循复杂曲面爆炸视图表示的一般思路，特别是参考了：

> Olga Karpenko, Wilmot Li, Niloy Mitra, and Maneesh Agrawala. **Exploded View Diagrams of Mathematical Surfaces.** *IEEE Transactions on Visualization and Computer Graphics*, 16(6), 2010, 1311–1318. DOI: `10.1109/TVCG.2010.151`.

作者谨向 M. Chamberland 与 H. van de Wetering 致以诚挚谢意，感谢二位在本项目开发过程中所提供的指导与宝贵反馈。