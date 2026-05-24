CXX ?= /usr/bin/clang++
APP := app

ARCH ?= arm64
PCL_VERSION ?= 1.15.0
BREW_PREFIX ?= /opt/homebrew

CXXFLAGS := \
	-std=c++17 \
	-fdiagnostics-color=always \
	-Wall \
	-g \
	-arch $(ARCH) \
	-Iheaders \
	-Iheaders/explosionaxis \
	-Iheaders/planes \
	-Idependencies/include \
	-Idependencies/include/libomp \
	-Idependencies/nlohmann/include \
	-Idependencies/VTK/include/vtk-9.4 \
	-I$(BREW_PREFIX)/include/pcl-1.15 \
	-I$(BREW_PREFIX)/Cellar/pcl/$(PCL_VERSION)/include/pcl-1.15

LDFLAGS := \
	-Ldependencies/library \
	-Ldependencies/library/libomp \
	-L$(BREW_PREFIX)/lib \
	-L$(BREW_PREFIX)/Cellar/pcl/$(PCL_VERSION)/lib \
	-Ldependencies/VTK/lib \
	-Wl,-rpath,dependencies/VTK/lib

SOURCES := \
	$(wildcard explosionaxis/*.cpp) \
	$(wildcard planes/*.cpp) \
	main.cpp \
	data.cpp \
	marching_cubes.cpp \
	visual.cpp \
	post_processor.cpp \
	glad.c \
	nifti1_io.c \
	znzlib.c \
	imgui/imgui.cpp \
	imgui/imgui_draw.cpp \
	imgui/imgui_tables.cpp \
	imgui/imgui_widgets.cpp \
	imgui/imgui_impl_glfw.cpp \
	imgui/imgui_impl_opengl3.cpp

PCL_LIBS := \
	-lpcl_common \
	-lpcl_features \
	-lpcl_filters \
	-lpcl_io \
	-lpcl_kdtree \
	-lpcl_keypoints \
	-lpcl_octree \
	-lpcl_registration \
	-lpcl_sample_consensus \
	-lpcl_search \
	-lpcl_segmentation \
	-lpcl_surface

BOOST_LIBS := \
	-lboost_system \
	-lboost_filesystem \
	-lboost_thread \
	-lboost_date_time \
	-lboost_iostreams \
	-lboost_serialization \
	-lboost_chrono \
	-lboost_atomic \
	-lboost_regex

VTK_LIBS := \
	-lvtkCommonCore-9.4 \
	-lvtkCommonDataModel-9.4 \
	-lvtkCommonExecutionModel-9.4 \
	-lvtkCommonMath-9.4 \
	-lvtkCommonTransforms-9.4 \
	-lvtksys-9.4 \
	-lvtkFiltersCore-9.4 \
	-lvtkFiltersGeneral-9.4 \
	-lvtkFiltersSources-9.4 \
	-lvtkFiltersGeometry-9.4 \
	-lvtkFiltersModeling-9.4 \
	-lvtkInteractionStyle-9.4 \
	-lvtkRenderingCore-9.4 \
	-lvtkRenderingOpenGL2-9.4 \
	-lvtkRenderingFreeType-9.4 \
	-lvtkIOCore-9.4 \
	-lvtkIOImage-9.4 \
	-lvtkCommonSystem-9.4 \
	-lvtkRenderingUI-9.4 \
	-lvtkRenderingLOD-9.4 \
	-lvtkRenderingAnnotation-9.4 \
	-lvtkIOExport-9.4 \
	-lvtkIOXML-9.4

LIBS := \
	dependencies/library/libglfw.3.4.dylib \
	$(PCL_LIBS) \
	$(BOOST_LIBS) \
	$(VTK_LIBS) \
	-lomp \
	-lz \
	-framework OpenGL \
	-framework Cocoa \
	-framework IOKit \
	-framework CoreVideo \
	-framework CoreFoundation

.PHONY: all clean run

all: $(APP)

$(APP): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) $(LIBS) -o $(APP) -Wno-deprecated

run: $(APP)
	./$(APP)

clean:
	rm -rf $(APP) $(APP).dSYM
