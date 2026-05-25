# ExplodedVolumes

[English](README.md) | [中文](README.zh.md) | [Nederlands](README.nl.md)

![Introduction](images/Screenshot_20240130_115627.png)

**ExplodedVolumes** is een C++17/OpenGL-toepassing voor de interactieve visualisatie van volumetrische gegevens. Het programma laadt NIfTI-volumes, extraheert een iso-oppervlak met Marching Cubes, berekent of selecteert een explosie-as, construeert snijvlakken, scheidt de resulterende componenten en rendert het resultaat in zowel een normale weergave als een exploded-viewweergave.

Het project wordt verspreid als broncodearchief en is ontwikkeld in de context van een cursusproject aan de TU Eindhoven. Externe headers en platformspecifieke binaire bibliotheken zijn bewust niet meegeleverd. Voor het compileren dient iedere gebruiker lokaal een afhankelijkhedenmap op te bouwen met bibliotheken die passen bij het besturingssysteem, de CPU-architectuur en de gebruikte compiler-toolchain.

---

## Inhoudsopgave

- [1. Projectcontext](#1-projectcontext)
- [2. Resultaten](#2-resultaten)
- [3. Functioneel overzicht](#3-functioneel-overzicht)
- [4. Methodologische pijplijn](#4-methodologische-pijplijn)
- [5. Bronstructuur](#5-bronstructuur)
- [6. Afhankelijkhedenmodel](#6-afhankelijkhedenmodel)
  - [6.1 Beleid voor broncode-only distributie](#61-beleid-voor-broncode-only-distributie)
  - [6.2 Gedeelde mapstructuur voor afhankelijkheden](#62-gedeelde-mapstructuur-voor-afhankelijkheden)
  - [6.3 Regel voor platformspecifieke binaire bestanden](#63-regel-voor-platformspecifieke-binaire-bestanden)
- [7. Bouwen op Windows](#7-bouwen-op-windows)
- [8. Bouwen op macOS](#8-bouwen-op-macos)
- [9. De toepassing uitvoeren](#9-de-toepassing-uitvoeren)
- [10. Interactie](#10-interactie)
- [11. Opmerkingen over VS Code-configuratie](#11-opmerkingen-over-vs-code-configuratie)
- [12. Bekende beperkingen](#12-bekende-beperkingen)
- [13. Externe componenten en referenties](#13-externe-componenten-en-referenties)

---

## 1. Projectcontext

Veel volumetrische datasets bevatten interne structuren die niet voldoende kunnen worden begrepen op basis van het uitwendige iso-oppervlak alleen. Een exploded view behandelt deze beperking door een oppervlak in een reeks componenten te snijden en deze componenten langs een gekozen richting te verplaatsen. De resulterende visualisatie maakt verborgen geometrie zichtbaar, terwijl de ruimtelijke relatie tussen de gescheiden delen behouden blijft.

In deze implementatie is de invoer een wetenschappelijk of medisch volume in NIfTI-formaat. Het programma extraheert een getrianguleerd iso-oppervlak, schat kandidaat-richtingen voor de explosie-as, maakt snijvlakken, verdeelt de mesh in componenten en toont het resultaat in een interactieve OpenGL-viewer. De renderstijl versterkt randen om de leesbaarheid van oppervlakken te verbeteren.

---

## 2. Resultaten

### Hoofdinterface

![Main Interface](images/Iguana_1.png)

*Hoofdvenster met het Marching Cubes-bedieningspaneel, instellingen voor de explosie-as en bediening voor de exploded view.*

### Exploded view

| Normale weergave | Exploded view |
|:---:|:---:|
| ![Iguana Normal](images/Iguana_1.png) | ![Iguana Exploded](images/Iguana_2.png) |
| ![chris_t1 Normal](images/chris_t1_1.png) | ![chris_t1 Exploded](images/chris_t1_2.png) |
| ![CT_Abdo Normal](images/CT_Abdo_1.png) | ![CT_Abdo Exploded](images/CT_Abdo_2.png) |
| ![CT_Philips Normal](images/CT_Philips_1.png) | ![CT_Philips Exploded](images/CT_Philips_2.png) |

*Vergelijking tussen normale rendering links en exploded-viewrendering rechts voor meerdere datasets.*

---

## 3. Functioneel overzicht

De huidige implementatie biedt de volgende functionaliteit:

- laden van `.nii`-bestanden met NIfTI-volumegegevens via een native bestandsdialoog;
- extraheer van getrianguleerde iso-oppervlakken met Marching Cubes;
- renderen van normale weergaven en exploded views in een OpenGL 3.3 Core Profile-context;
- schatten of selecteren van explosie-assen via PCA-, symmetrie- en gecombineerde strategieën;
- construeren van snijvlakken en scheiden van de mesh in exploded-viewcomponenten;
- interactief aanpassen van iso-niveau en explosieafstand;
- visualiseren van de explosie-as;
- roteren en zoomen van de camera;
- toepassen van een framebuffer-gebaseerde nabewerkingsstap voor randversterkte rendering;
- aanbieden van runtimebediening via Dear ImGui.

---

## 4. Methodologische pijplijn

De implementatie kan als de volgende gegevensstroom worden gelezen:

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

De actieve renderlus bevindt zich in `main.cpp`. Rendering en gebruikersinterface worden hoofdzakelijk geïmplementeerd in `visual.cpp`, terwijl nabewerking wordt afgehandeld door `post_processor.cpp`. De logica voor snijvlakken en exploded views bevindt zich onder `planes/`; de schatting van de explosie-as bevindt zich onder `explosionaxis/`.

---

## 5. Bronstructuur

Een broncode-only kopie van het project heeft bij benadering de volgende structuur:

```text
.
├── main.cpp                         # Ingangspunt van het programma en actieve renderlus
├── data.cpp                         # NIfTI-laden en volumefuncties
├── marching_cubes.cpp               # Marching Cubes-iso-oppervlakextractie
├── visual.cpp                       # GLFW/OpenGL/ImGui-rendering en UI
├── post_processor.cpp               # FBO-gebaseerde nabewerking
├── glad.c                           # Implementatie van de GLAD OpenGL-loader
├── nifti1_io.c                      # Implementatie van de NIfTI-lezer
├── znzlib.c                         # Hulpcode voor laag-niveau NIfTI-bestands-I/O
├── file_dialog.h                    # Wrapper rond portable-file-dialogs
├── headers/
│   ├── data.h
│   ├── marching_cubes.h
│   ├── visual.h
│   ├── post_processor.h
│   ├── explosionaxis/               # Headers voor strategieën voor explosie-assen
│   └── planes/                      # Headers voor snijvlakken en exploded views
├── explosionaxis/                   # Implementaties van strategieën voor explosie-assen
├── planes/                          # Code voor snijvlakken, selectie en exploded views
├── imgui/                           # Door het project gebruikte Dear ImGui-implementatiebestanden
└── dependencies/                    # Lokale afhankelijkhedenmap; door de gebruiker opgebouwd
```

De map `dependencies/` wordt normaliter niet gecommit. Deze map dient lokaal te worden aangemaakt volgens de platformspecifieke bouwinstructies hieronder.

---

## 6. Afhankelijkhedenmodel

### 6.1 Beleid voor broncode-only distributie

Dit project wordt als broncode verspreid, zonder externe binaire afhankelijkheden. Dit voorkomt dat platformspecifieke bibliotheken worden meegeleverd die mogelijk niet compatibel zijn met een andere compiler, een ander besturingssysteem of een andere CPU-architectuur.

In de praktijk betekent dit:

- een schone broncodekopie bevat geen vooraf ingevulde afhankelijkhedenmap;
- de afhankelijkhedenmap moet lokaal worden opgebouwd;
- Windows-builds gebruiken uitsluitend Windows-bibliotheken;
- macOS-builds gebruiken uitsluitend macOS-bibliotheken;
- binaire artefacten van verschillende besturingssystemen, CPU-architecturen of compiler-toolchains mogen niet worden gemengd.

### 6.2 Gedeelde mapstructuur voor afhankelijkheden

De bouwcommando's in dit README gaan uit van de volgende lokale structuur. Gelijkwaardige paden kunnen worden gebruikt, mits de include- en library-vlaggen overeenkomstig worden aangepast.

```text
dependencies/
├── include/
│   ├── GLFW/                        # GLFW-headers
│   ├── glad/                        # glad.h
│   ├── KHR/                         # khrplatform.h
│   ├── glm/                         # GLM-headers
│   ├── Eigen/                       # Eigen-headers, of een extern Eigen-include-pad
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

De toepassing gebruikt de volgende componenten:

| Afhankelijkheid | Rol | Gebruikelijke verwerking |
|---|---|---|
| C++17 compiler | Compileren van de toepassing | Clang, MinGW-w64 of MSVC |
| OpenGL 3.3 | Renderingbackend | Aangeboden door platform en grafische driver |
| GLFW | Venster, OpenGL-context en invoer | Header + platformspecifieke bibliotheek |
| GLAD | OpenGL-functielader | Compileer `glad.c`; lever overeenkomende `glad/`- en `KHR/`-headers |
| Dear ImGui | Runtime-GUI | Compileer de gebruikte ImGui `.cpp`-bestanden; lever overeenkomende headers |
| GLM | Grafische wiskunde | Header-only |
| Eigen | Lineaire algebra voor asschatting | Header-only |
| NIfTI C files | Laden van volumes | Compileer `nifti1_io.c` en `znzlib.c`; lever overeenkomende headers |
| portable-file-dialogs | Native bestandsdialoog | Eén header |
| OpenMP | Parallelle CPU-lussen | Compilervlag en runtimebibliotheek |
| zlib | Optionele ondersteuning voor gecomprimeerde NIfTI-bestanden | Link wanneer `HAVE_ZLIB` is ingeschakeld of vereist is door de lokale NIfTI-configuratie |
| PCL / VTK / Boost | Geometrie- en renderondersteuning gebruikt door de huidige Makefile | Installeren met Homebrew op macOS |

### 6.3 Regel voor platformspecifieke binaire bestanden

Gecompileerde bibliotheken moeten overeenkomen met het doelplatform en de ABI van de compiler.

| Bestandstype | Gebruikelijke context |
|---|---|
| `.a` | Windows MinGW-w64 of compatibele GCC-achtige toolchain |
| `.lib` | Windows MSVC |
| `.dll` | Windows runtimebibliotheek |
| `.dylib` | macOS dynamische bibliotheek |
| `.framework` | macOS systeem- of frameworkafhankelijkheid |

MSVC dient bijvoorbeeld niet tegen MinGW `.a`-bestanden te linken, en macOS `.dylib`-bestanden kunnen niet worden gebruikt in een Windows-build. Apple Silicon-builds vereisen arm64-compatibele afhankelijkheden; Intel macOS-builds vereisen x86_64-compatibele afhankelijkheden.

---

## 7. Bouwen op Windows

Windows-builds moeten uitsluitend Windows-compatibele headers en bibliotheken gebruiken. Hergebruik geen macOS `.dylib`-bestanden, Homebrew-paden of macOS-frameworkvlaggen.

Maak de lokale afhankelijkhedenmappen aan:

```powershell
mkdir dependencies
mkdir dependencies\include
mkdir dependencies\library
```

Plaats de in [sectie 6.2](#62-gedeelde-mapstructuur-voor-afhankelijkheden) genoemde headers in `dependencies\include`, en plaats de bijbehorende Windows GLFW/OpenMP-runtimebibliotheken in `dependencies\library`.

### MinGW-w64

Voer vanuit de projectroot in PowerShell uit:

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

Indien de GLFW-build dynamisch is, moet de overeenkomende GLFW `.dll` naast het gegenereerde uitvoerbare bestand worden geplaatst.

### MSVC

Voer uit vanuit een x64 Native Tools Command Prompt for Visual Studio, of vanuit een andere shell waarin `cl.exe` is geconfigureerd:

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

Indien de GLFW-build dynamisch is, moet de overeenkomende GLFW `.dll` naast `explodedvolumes-msvc.exe` worden geplaatst.

---

## 8. Bouwen op macOS

Het huidige ingangspunt voor de build is de `Makefile` in de repositoryroot.

Installeer de Homebrew-afhankelijkheden:

```sh
brew install glfw glm eigen boost pcl vtk libomp nlohmann-json
```

Bereid de lokale afhankelijkhedenstructuur voor die door de Makefile wordt verwacht:

```sh
mkdir -p dependencies/include dependencies/library
ln -sf "$(brew --prefix glfw)/lib/libglfw.3.dylib" dependencies/library/libglfw.3.4.dylib
ln -sfn "$(brew --prefix vtk)" dependencies/VTK
ln -sfn "$(brew --prefix glm)/include/glm" dependencies/include/glm
ln -sfn "$(brew --prefix eigen)/include/eigen3/Eigen" dependencies/include/Eigen
ln -sfn "$(brew --prefix boost)/include/boost" dependencies/include/boost
```

De projectlokale headers uit [sectie 6.2](#62-gedeelde-mapstructuur-voor-afhankelijkheden) moeten eveneens beschikbaar zijn onder `dependencies/include`.

Bouwen:

```sh
make
```

Wanneer de lokale PCL-versie afwijkt van de standaardwaarde in de Makefile:

```sh
make PCL_VERSION=1.15.1
```

Voor Intel macOS of een aangepaste Homebrew-prefix:

```sh
make BREW_PREFIX=/usr/local
```

Build-uitvoer opruimen:

```sh
make clean
```

---

## 9. De toepassing uitvoeren

macOS:

```sh
make run
```

Windows MinGW:

```powershell
.\explodedvolumes-mingw.exe
```

Windows MSVC:

```bat
explodedvolumes-msvc.exe
```

De toepassing opent een bestandsdialoog. Selecteer een `.nii`-volumebestand waarvoor gebruiksrechten bestaan en, indien relevant, ook herdistributierechten. Publieke releases mogen geen private of identificeerbare medische gegevens bevatten.

---

## 10. Interactie

| Handeling | Bediening |
|---|---|
| Camera roteren | Linkermuisknop slepen |
| Zoomen | Muiswiel |
| Camera-afstand aanpassen | `+` / `-` |
| Toepassing sluiten | `Esc` |
| Iso-niveau wijzigen | ImGui Marching Cubes-bedieningspaneel |
| Exploded view in- of uitschakelen | ImGui Explosion View-bedieningspaneel |
| Explosieafstand aanpassen | ImGui Explosion View-bedieningspaneel |
| Explosie-as tonen of verbergen | ImGui Explosion Axis Settings-paneel |

---

## 11. Opmerkingen over VS Code-configuratie

Voor macOS kan een VS Code build task met de naam `Build OpenGL` worden aangemaakt die het volgende uitvoert:

```sh
make
```

Een launchconfiguratie kan het volgende gebruiken:

```json
{
  "program": "${workspaceFolder}/app",
  "cwd": "${workspaceFolder}",
  "preLaunchTask": "Build OpenGL"
}
```

Gebruik `${workspaceFolder}` in plaats van `${fileDirname}`. De laatste waarde verandert afhankelijk van het bestand dat op dat moment in de editor actief is, en kan daardoor leiden tot inconsistente build- of runtimepaden.

---

## 12. Bekende beperkingen

- De repository bevat nog geen platformonafhankelijke CMake-configuratie.
- De broncode-only distributie vereist dat gebruikers externe headers en platformspecifieke bibliotheken lokaal opnieuw opbouwen.
- De huidige Makefile is op macOS gericht en is gevoelig voor wijzigingen in PCL- of VTK-paden.
- De huidige bestandsdialoog richt zich op `.nii`-invoer. Voor gebruiksvriendelijke ondersteuning van `.nii.gz` kan aanvullend werk nodig zijn.
- `imgui.ini` kan door Dear ImGui worden gegenereerd of bijgewerkt om de UI-indeling op te slaan. Dit is normaal en betekent niet dat invoervolumebestanden zijn gewijzigd.

---

## 13. Externe componenten en referenties

Controleer vóór het compileren of alle componenten van derden die in de lokale build worden gebruikt, beschikbaar zijn:

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

Het visualisatieconcept volgt het algemene idee van exploded-viewrepresentaties van complexe oppervlakken, met name:

> Olga Karpenko, Wilmot Li, Niloy Mitra, and Maneesh Agrawala. **Exploded View Diagrams of Mathematical Surfaces.** *IEEE Transactions on Visualization and Computer Graphics*, 16(6), 2010, 1311–1318. DOI: `10.1109/TVCG.2010.151`.

De auteurs danken M. Chamberland en H. van de Wetering voor hun begeleiding en waardevolle terugkoppeling tijdens de ontwikkeling van dit project.