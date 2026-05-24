# ExplodedVolumes

**ExplodedVolumes** is a C++17/OpenGL application for interactive visualization of volumetric data. It loads NIfTI volume data, extracts an iso-surface with Marching Cubes, computes or selects an explosion axis, constructs cutting planes, separates the resulting components, and renders the result in both normal and exploded-view modes.

The project is designed as a source-only archive and as a course project of TU Eindhoven. Third-party headers and platform-specific binary libraries are intentionally not bundled. Before compiling, each user must reconstruct the local dependency folder using libraries that match their operating system, CPU architecture, and compiler toolchain.

---

## Table of Contents

- [1. Project Context](#1-project-context)
- [2. Figure Placeholders](#2-figure-placeholders)
- [3. Functional Overview](#3-functional-overview)
- [4. Methodological Pipeline](#4-methodological-pipeline)
- [5. Source Tree](#5-source-tree)
- [6. Dependency Model](#6-dependency-model)
  - [6.1 Source-Only Distribution Policy](#61-source-only-distribution-policy)
  - [6.2 Shared Dependency Layout](#62-shared-dependency-layout)
  - [6.3 Platform-Specific Binary Rule](#63-platform-specific-binary-rule)
- [7. Windows Build](#7-windows-build)
  - [7.1 Windows Dependency Preparation](#71-windows-dependency-preparation)
  - [7.2 Windows with MinGW-w64](#72-windows-with-mingw-w64)
  - [7.3 Windows with MSVC](#73-windows-with-msvc)
- [8. macOS Build](#8-macos-build)
  - [8.1 macOS Dependency Preparation](#81-macos-dependency-preparation)
  - [8.2 macOS Makefile Build](#82-macos-makefile-build)
  - [8.3 macOS Direct Build Fallback](#83-macos-direct-build-fallback)
  - [8.4 Optional Extended macOS Build](#84-optional-extended-macos-build)
- [9. Running the Application](#9-running-the-application)
- [10. Interaction Guide](#10-interaction-guide)
- [11. VS Code Configuration Notes](#11-vs-code-configuration-notes)
- [12. Release Packaging](#12-release-packaging)
- [13. Troubleshooting](#13-troubleshooting)
- [14. Known Limitations](#14-known-limitations)
- [15. Third-Party Components and References](#15-third-party-components-and-references)

---

## 1. Project Context

Many volumetric data sets contain internal structures that cannot be adequately understood from the exterior iso-surface alone. An exploded view addresses this limitation by cutting a surface into a sequence of components and translating those components along a chosen direction. The resulting visualization exposes hidden geometry while preserving the spatial relationship between the separated pieces.

In this implementation, the input is a scientific or medical volume stored in NIfTI format. The program extracts a triangulated iso-surface, estimates candidate explosion directions, creates cutting planes, separates the mesh into components, and renders the output in an interactive OpenGL viewer. The rendering style is intentionally edge-enhanced to improve surface readability.

---

## 2. Figure Placeholders

The following locations are reserved for figures. Replace the placeholder paths with screenshots or diagrams before final submission or publication.

### Figure 1. Overall result teaser

Suggested content: a side-by-side comparison of the original iso-surface, cutting-plane configuration, and final exploded view.

<!--
![Figure 1. Original and exploded views of the same volume.](docs/images/figure1_result_teaser.png)
-->

### Figure 2. Computational pipeline

Suggested content: NIfTI volume → Marching Cubes → explosion-axis estimation → cutting-plane generation → surface segmentation → exploded rendering.

<!--
![Figure 2. Computational pipeline of ExplodedVolumes.](docs/images/figure2_pipeline.png)
-->

### Figure 3. User interface and interaction

Suggested content: a screenshot of the OpenGL viewport and the ImGui control panels.

<!--
![Figure 3. User interface with iso-level, axis, and explosion controls.](docs/images/figure3_interface.png)
-->

### Figure 4. Dependency reconstruction model

Suggested content: a small diagram showing the source-only repository, the local dependency folder, and the separate Windows/macOS binary libraries.

<!--
![Figure 4. Source-only distribution and platform-specific dependency reconstruction.](docs/images/figure4_dependency_model.png)
-->

---

## 3. Functional Overview

The current implementation provides the following functionality:

- loading `.nii` NIfTI volume files through a native file dialog;
- extracting triangulated iso-surfaces with Marching Cubes;
- rendering normal and exploded views in an OpenGL 3.3 Core profile context;
- estimating or selecting explosion axes using PCA-, symmetry-, and combined-strategy code paths;
- constructing cutting planes and separating the mesh into exploded components;
- interactively adjusting the iso-level and explosion distance;
- visualizing the explosion axis;
- rotating and zooming the camera;
- applying a framebuffer-based post-processing pass for edge-enhanced rendering;
- exposing runtime controls through Dear ImGui.

---

## 4. Methodological Pipeline

The implementation can be read as the following data-flow pipeline:

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

The active rendering loop is located in `main.cpp`. Rendering and user-interface construction are implemented mainly in `visual.cpp`, while post-processing is handled by `post_processor.cpp`. Cutting-plane and exploded-view logic is implemented under `planes/`, and explosion-axis estimation is implemented under `explosionaxis/`.

---

## 5. Source Tree

A source-only copy of the project has the following approximate structure:

```text
.
├── main.cpp                         # Program entry point and active render loop
├── data.cpp                         # NIfTI loading and volume utilities
├── marching_cubes.cpp               # Marching Cubes iso-surface extraction
├── visual.cpp                       # GLFW/OpenGL/ImGui rendering and UI
├── post_processor.cpp               # FBO-based post-processing pass
├── glad.c                           # GLAD OpenGL loader implementation
├── nifti1_io.c                      # NIfTI reader implementation
├── znzlib.c                         # NIfTI low-level file I/O helper
├── file_dialog.h                    # Wrapper around portable-file-dialogs
├── headers/
│   ├── data.h
│   ├── marching_cubes.h
│   ├── visual.h
│   ├── post_processor.h
│   ├── explosionaxis/               # Explosion-axis strategy headers
│   └── planes/                      # Cutting-plane and exploded-view headers
├── explosionaxis/                   # Explosion-axis strategy implementations
├── planes/                          # Cutting-plane, selection, and exploded-view code
├── imgui/                           # Dear ImGui implementation files used by the project
└── dependencies/                    # Local dependency folder; reconstructed by the user
```

The `dependencies/` directory is normally not committed. It should be created locally according to the Windows or macOS instructions below.

Input volume data is not required to be stored in the repository. For public releases, include only volume files that are public, licensed for redistribution, and properly de-identified where applicable.

---

## 6. Dependency Model

### 6.1 Source-Only Distribution Policy

This project is distributed as source code without third-party binary dependencies. This avoids shipping platform-specific libraries that may be incompatible with a different compiler, operating system, or CPU architecture.

In practice, this means:

- do not expect a pre-filled dependency folder in a clean source archive;
- reconstruct the dependency folder locally;
- use Windows libraries only for Windows builds;
- use macOS libraries only for macOS builds;
- do not mix MinGW, MSVC, and macOS binary artifacts.

### 6.2 Shared Dependency Layout

The build commands in this README assume the following local layout. Equivalent paths can be used, but the include and library flags must then be adjusted accordingly.

```text
dependencies/
├── include/
│   ├── GLFW/                        # GLFW headers
│   ├── glad/                        # glad.h
│   ├── KHR/                         # khrplatform.h
│   ├── glm/                         # GLM headers
│   ├── Eigen/                       # Eigen headers, or use an external Eigen include path
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

The following components are used by the application:

| Dependency | Role | Typical handling |
|---|---|---|
| C++17 compiler | Compiles the application | GCC/MinGW, MSVC, or Clang |
| OpenGL 3.3 | Rendering backend | Provided by platform and graphics driver |
| GLFW | Window, OpenGL context, input | Header + platform-specific library |
| GLAD | OpenGL function loader | Compile `glad.c`; provide matching `glad/` and `KHR/` headers |
| Dear ImGui | Runtime GUI | Compile the bundled ImGui `.cpp` files; provide matching headers |
| GLM | Graphics mathematics | Header-only |
| Eigen | Linear algebra for axis estimation | Header-only |
| NIfTI C files | Volume loading | Compile `nifti1_io.c` and `znzlib.c`; provide matching headers |
| portable-file-dialogs | Native file dialog | Single header |
| OpenMP | Parallel CPU loops | Compiler flag and runtime library |
| zlib | Optional compressed NIfTI support | Link when `HAVE_ZLIB` is enabled or required by the local NIfTI configuration |
| PCL / VTK / Boost | Optional extended macOS variants | Only needed if a local build script or branch explicitly enables them |

### 6.3 Platform-Specific Binary Rule

Compiled libraries must match the target platform and compiler ABI.

| File type | Typical context |
|---|---|
| `.a` built by MinGW | Windows MinGW-w64 or compatible GCC-style toolchain |
| `.lib` built for Visual Studio | Windows MSVC |
| `.dll` | Windows runtime library; must match the import library used at link time |
| `.dylib` | macOS dynamic library |
| `.framework` | macOS system or framework dependency |

For example, MSVC should not link against a MinGW `.a` file, and a macOS `.dylib` cannot be used by a Windows build. The Windows and macOS build procedures are therefore documented separately.

---

## 7. Windows Build

This section is only for Windows. Do not use macOS `.dylib` files, Homebrew paths, or macOS framework flags in this build.

### 7.1 Windows Dependency Preparation

Create the local dependency directories:

```powershell
mkdir dependencies
mkdir dependencies\include
mkdir dependencies\library
```

Then obtain the dependencies from their upstream sources or from package managers appropriate for your compiler. The important point is that the headers and libraries must belong to the same toolchain family.

Recommended sources:

- GLFW: download a Windows binary release matching your compiler, or build GLFW from source with CMake.
- GLAD: generate an OpenGL 3.3 Core profile loader and copy `glad/glad.h` and `KHR/khrplatform.h`. The generated headers should match the existing `glad.c`; if they do not, regenerate or replace `glad.c` as well.
- GLM: copy the `glm/` include directory or install it through a package manager.
- Eigen: copy the directory that directly contains the `Eigen/` folder, or install it through a package manager.
- Dear ImGui: use headers from the same ImGui version as the `.cpp` files in the `imgui/` directory. If the version is uncertain, replace both the headers and the `.cpp` files with a consistent upstream release.
- NIfTI: provide headers matching `nifti1_io.c` and `znzlib.c`.
- portable-file-dialogs: copy the single header into `dependencies/include/`.
- OpenMP: install or use the OpenMP runtime associated with the selected compiler.

A minimal reconstructed dependency folder should contain at least the headers shown in [Section 6.2](#62-shared-dependency-layout), plus a platform-specific GLFW library.

### 7.2 Windows with MinGW-w64

Assumptions:

- a 64-bit MinGW-w64 compiler is available as `g++`;
- the dependency folder has been reconstructed locally;
- the GLFW library in `dependencies/library` was built for MinGW;
- the command is executed from the project root.

PowerShell example:

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

If dynamic GLFW is used, copy the matching GLFW `.dll` next to the executable. If static GLFW is used successfully, a separate GLFW DLL may not be required.

If `nifti1_io.c` or `znzlib.c` is compiled with zlib support enabled, also add the correct zlib include path and link library for your MinGW environment.

### 7.3 Windows with MSVC

MSVC should be linked against libraries built for Visual Studio, not MinGW libraries. Obtain a Visual Studio-compatible GLFW package or build GLFW from source with CMake using the same Visual Studio toolset as the application.

Run the following from an **x64 Native Tools Command Prompt for VS** or an equivalent terminal where `cl.exe` is available:

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

If a dynamic GLFW library is used, copy the matching GLFW DLL next to `explodedvolumes-msvc.exe`. If zlib support is enabled for the NIfTI I/O layer, add the matching MSVC zlib import/static library to the link step.

---

## 8. macOS Build

This section is only for macOS. Do not use Windows `.a`, `.lib`, or `.dll` files here. The macOS build normally uses Clang, Homebrew libraries, macOS frameworks, and `.dylib` files.

### 8.1 macOS Dependency Preparation

Install the base dependency set:

```sh
brew install glfw glm eigen libomp pkg-config
```

Apple Silicon Homebrew normally uses `/opt/homebrew`, while Intel macOS commonly uses `/usr/local`. Prefer `brew --prefix` or Makefile variables rather than hard-coding the prefix.

Create local include and library folders:

```sh
mkdir -p dependencies/include dependencies/library
```

Link or copy the Homebrew headers into the local dependency folder:

```sh
ln -sfn "$(brew --prefix glm)/include/glm" dependencies/include/glm
ln -sfn "$(brew --prefix eigen)/include/eigen3/Eigen" dependencies/include/Eigen
ln -sfn "$(brew --prefix glfw)/include/GLFW" dependencies/include/GLFW
```

For dynamic GLFW linking through a local folder:

```sh
ln -sf "$(brew --prefix glfw)/lib/libglfw.3.dylib" dependencies/library/libglfw.3.dylib
```

The project-local headers listed in [Section 6.2](#62-shared-dependency-layout) must still be provided. In particular, a source-only archive may not include the companion headers for GLAD, Dear ImGui, NIfTI, or portable-file-dialogs. Copy those headers from matching upstream releases or from the same versions used to create the source files already present in the repository.

### 8.2 macOS Makefile Build

Some macOS variants of the project provide a Makefile. If a Makefile is present, the recommended workflow is:

```sh
make
```

If the Makefile exposes a PCL version variable and the local PCL version differs from the default, set it explicitly:

```sh
make PCL_VERSION=1.15.1
```

For Intel macOS or a custom Homebrew prefix:

```sh
make BREW_PREFIX=/usr/local
```

To remove build outputs:

```sh
make clean
```

To build and run the application:

```sh
make run
```

If `omp.h` or `-lomp` is not found, add Homebrew `libomp` paths to the Makefile:

```text
-I$(brew --prefix libomp)/include
-L$(brew --prefix libomp)/lib
```

### 8.3 macOS Direct Build Fallback

If the source copy does not contain a Makefile, or if the build needs to be reproduced explicitly, use a direct Clang command.

```sh
BREW_PREFIX="$(brew --prefix)"
LIBOMP_PREFIX="$(brew --prefix libomp)"

SOURCES="
  glad.c znzlib.c nifti1_io.c
  imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_impl_glfw.cpp
  imgui/imgui_impl_opengl3.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
  explosionaxis/eigen_reflective_symmetry_detector.cpp
  explosionaxis/eigen_rotational_symmetry_detector.cpp
  explosionaxis/explosion_axis_strategy.cpp
  explosionaxis/mitra_reflective_symmetry_detector.cpp
  explosionaxis/mitra_rotational_symmetry_detector.cpp
  explosionaxis/pca_analyzer.cpp
  explosionaxis/pcl_reflective_symmetry_detector.cpp
  explosionaxis/pcl_rotational_symmetry_detector.cpp
  explosionaxis/vector_ops.cpp
  planes/cutting_planes.cpp planes/exploded_view.cpp planes/selecting_planes.cpp
  data.cpp main.cpp marching_cubes.cpp post_processor.cpp visual.cpp
"

clang++ -std=c++17 -O2 -g \
  -Xpreprocessor -fopenmp \
  -I dependencies/include \
  -I headers -I headers/explosionaxis -I headers/planes \
  -I "$LIBOMP_PREFIX/include" \
  $SOURCES \
  $(pkg-config --cflags --libs glfw3) \
  -L "$LIBOMP_PREFIX/lib" -lomp \
  -framework OpenGL \
  -framework Cocoa \
  -framework IOKit \
  -framework CoreVideo \
  -framework CoreFoundation \
  -o explodedvolumes
```

If the NIfTI I/O layer is compiled with zlib support enabled, add `-lz` to the command.

### 8.4 Optional Extended macOS Build

Some local macOS branches or Makefiles may include an extended geometry stack using PCL, VTK, Boost, and additional OpenMP settings. Install these only if the local build script or source branch actually requires them:

```sh
brew install boost pcl vtk nlohmann-json
```

Then update versioned include and library paths in the Makefile or direct command. Homebrew package versions change over time, so avoid hard-coding versioned paths unless the exact version is known.

---

## 9. Running the Application

After building, run the executable generated for the relevant platform:

```sh
# macOS Makefile build
make run
```

```sh
# macOS direct build
./explodedvolumes
```

```powershell
# Windows MinGW
.\explodedvolumes-mingw.exe
```

```bat
REM Windows MSVC
explodedvolumes-msvc.exe
```

The application opens a file dialog. Select a `.nii` volume file for which you have permission to use and, if applicable, redistribute. Public releases should not include private or identifiable medical data.

---

## 10. Interaction Guide

Typical interaction controls are:

| Action | Control |
|---|---|
| Rotate camera | Left mouse drag |
| Zoom | Mouse wheel |
| Adjust camera distance | `+` / `-` |
| Close application | `Esc` |
| Change iso-level | ImGui Marching Cubes control panel |
| Toggle exploded view | ImGui Explosion View control panel |
| Adjust explosion distance | ImGui Explosion View control panel |
| Show or hide explosion axis | ImGui Explosion Axis Settings panel |

---

## 11. VS Code Configuration Notes

A VS Code build task should run the appropriate platform-specific build command from this README.

For macOS Makefile builds, a launch configuration can use:

```json
{
  "program": "${workspaceFolder}/app",
  "cwd": "${workspaceFolder}",
  "preLaunchTask": "Build OpenGL"
}
```

The corresponding task should run:

```sh
make
```

For Windows MinGW builds, set `program` to the generated `.exe`:

```json
{
  "program": "${workspaceFolder}/explodedvolumes-mingw.exe",
  "cwd": "${workspaceFolder}",
  "preLaunchTask": "Build OpenGL Windows"
}
```

Use `${workspaceFolder}` rather than `${fileDirname}`. The latter changes depending on which file is currently focused in the editor and can lead to inconsistent build or runtime paths.

---

## 12. Release Packaging

For a source release, include the source tree and this README. Do not include locally reconstructed third-party dependency folders unless their licenses explicitly permit redistribution and the package is clearly marked for one platform and compiler toolchain.

A minimal source-only release can be created as follows:

```sh
mkdir -p dist
zip -r dist/ExplodedVolumes-source.zip . \
  -x "dependencies/*" \
  -x "app" \
  -x "*.exe" \
  -x "*.dll" \
  -x "*.dylib" \
  -x "*.lib" \
  -x "*.a" \
  -x "*.o" \
  -x "*.dSYM/*" \
  -x "imgui.ini"
```

For a macOS binary release using a Makefile build:

```sh
make clean
make
mkdir -p dist/ExplodedVolumes
cp app dist/ExplodedVolumes/
cp README.md dist/ExplodedVolumes/

# Include sample data only if it is public, licensed, and de-identified.
# cp path/to/public_sample_volume.nii dist/ExplodedVolumes/

tar -czf dist/ExplodedVolumes-macos-arm64.tar.gz -C dist ExplodedVolumes
```

For a Windows binary release, create a separate archive for the exact compiler toolchain used. For example, do not combine a MinGW build and an MSVC build in a single ambiguous binary package.

If a binary is distributed, document:

- operating system and version tested;
- CPU architecture;
- compiler and version;
- whether GLFW and OpenMP are statically or dynamically linked;
- any runtime libraries that must be placed next to the executable.

---

## 13. Troubleshooting

### `fatal error: glad/glad.h: No such file or directory`

The include path does not contain GLAD headers. Ensure the following files exist and match the compiled `glad.c`:

```text
dependencies/include/glad/glad.h
dependencies/include/KHR/khrplatform.h
```

### `fatal error: imgui.h: No such file or directory`

The source archive includes ImGui implementation files, but a clean source-only archive may not include all ImGui headers. Copy the matching ImGui headers into `dependencies/include/`, or replace the ImGui source and headers together with a consistent upstream version.

### `fatal error: nifti1_io.h: No such file or directory`

Copy the NIfTI headers into `dependencies/include/`, or add the directory containing them to the include path:

```text
nifti1.h
nifti1_io.h
nifti1_io_version.h
znzlib.h
znzlib_version.h
```

### `fatal error: Eigen/Dense: No such file or directory`

Add the Eigen root directory to the include path. The correct directory is the one that directly contains the `Eigen/` folder.

Examples:

```text
-I dependencies/include
-I /opt/homebrew/include/eigen3
-I /usr/include/eigen3
```

### `fatal error: glm/glm.hpp: No such file or directory`

Install GLM or copy the `glm/` folder into a directory that is already on the include path.

Examples:

```text
dependencies/include/glm/glm.hpp
/opt/homebrew/include/glm/glm.hpp
```

### GLFW link errors on Windows

Check that the GLFW library matches the compiler:

- MinGW normally uses a MinGW-built `.a` library.
- MSVC normally uses a Visual Studio-built `.lib` library.

Also link the required Windows system libraries, commonly including:

```text
opengl32 gdi32 ole32 comctl32 oleaut32 uuid user32 shell32
```

### OpenMP errors

Use the appropriate compiler flag and runtime:

| Compiler/platform | Typical OpenMP flag |
|---|---|
| GCC / MinGW | `-fopenmp` |
| MSVC | `/openmp` |
| Apple Clang with Homebrew libomp | `-Xpreprocessor -fopenmp -I $(brew --prefix libomp)/include -L $(brew --prefix libomp)/lib -lomp` |

If OpenMP is not needed, a serial build is possible, but direct includes such as `#include <omp.h>` must then be guarded or replaced.

### macOS cannot find Homebrew packages

Check the actual Homebrew prefix:

```sh
brew --prefix
brew --prefix glfw
brew --prefix libomp
```

Then update the Makefile variables or direct build paths accordingly.

### `make: *** No targets specified and no makefile found`

The Makefile-based instructions apply only to source variants that contain a Makefile. If the local copy does not include one, use the direct build fallback in [Section 8.3](#83-macos-direct-build-fallback).

### Linux file dialog note

Linux is not the primary target of this README. If compiling on Linux, `portable-file-dialogs` typically requires an external dialog backend such as Zenity or KDialog at runtime.

---

## 14. Known Limitations

- The repository does not yet provide a single cross-platform CMake configuration. Windows, macOS Makefile, and direct compiler commands are documented separately.
- The source-only distribution requires users to reconstruct third-party headers and platform-specific libraries locally.
- The macOS extended build can be version-sensitive when PCL and VTK paths contain version numbers.
- The current file dialog focuses on `.nii` input. Additional work may be needed for convenient `.nii.gz` handling.
- `imgui.ini` may be generated or updated by Dear ImGui to store UI layout. This is normal and does not indicate that input volume files have been modified.
- Some legacy or experimental class names still contain `pcl` even when the active implementation relies on Eigen/manual geometry computations rather than external PCL calls.

---

## 15. Third-Party Components and References

Before redistribution, check the licenses of all third-party components used in the local build:

- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLM](https://github.com/g-truc/glm)
- [Eigen](https://eigen.tuxfamily.org/)
- [portable-file-dialogs](https://github.com/samhocevar/portable-file-dialogs)
- [NIfTI C library](https://nifti.nimh.nih.gov/)
- [OpenMP](https://www.openmp.org/)
- [PCL](https://pointclouds.org/), if an extended geometry build is used
- [VTK](https://vtk.org/), if an extended geometry build is used

The visualization concept follows the general idea of exploded-view representations of complex surfaces, especially:

> Olga Karpenko, Wilmot Li, Niloy Mitra, and Maneesh Agrawala. **Exploded View Diagrams of Mathematical Surfaces.** *IEEE Transactions on Visualization and Computer Graphics*, 16(6), 2010, 1311–1318. DOI: `10.1109/TVCG.2010.151`.
