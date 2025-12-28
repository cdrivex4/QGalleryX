# ============================================================================
# SINGLE EXECUTABLE BUILD - SCROLLBENCH PORTABLE
# ============================================================================
# This is a SKELETON build configuration that pulls ScrollBench sources
# and builds them with static linking for maximum portability.
# 
# This creates a third binary alongside:
#   1. appSamsungGallery.exe (Main app)
#   2. appScrollBench.exe (Test app - dynamic linking)
#   3. ScrollBenchPortable.exe (This - static linking)
# ============================================================================

# NOTE: This file is designed to be included by the parent CMakeLists.txt
# It reuses all sources from test_scrollbench/ with different build settings

# ============================================================================
# Build Configuration for Static Linking
# ============================================================================

option(STATIC_RUNTIME "Link C++ runtime statically" ON)
option(MINIMAL_BUILD "Exclude video support for smaller binary" OFF)
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)  # Disabled: MinGW 13 has LTO bugs
option(STRIP_SYMBOLS "Strip debug symbols for smaller size" ON)

# Optimization flags for size
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O1 /GL /Gy /Gw")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
    if(STATIC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
    endif()
else()
    # MinGW: Basic optimization without LTO (has bugs in MinGW 13)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    if(STATIC_RUNTIME)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    endif()
endif()

# LTO disabled due to MinGW compiler bugs
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# ============================================================================
# Source Files - PULLED FROM SCROLLBENCH (No duplication)
# ============================================================================

set(PORTABLE_SOURCES
    # ScrollBench sources
    ../test_scrollbench/src/main_scrollbench.cpp
    ../test_scrollbench/src/ScrollBenchImageModel.cpp
    ../test_scrollbench/src/ScrollBenchImageModel.h
    ../test_scrollbench/src/FileTypeRouter.cpp
    ../test_scrollbench/src/FileTypeRouter.h
    ../test_scrollbench/src/FrameBudgetScheduler.cpp
    ../test_scrollbench/src/FrameBudgetScheduler.h
    ../test_scrollbench/src/TelemetryMonitor.cpp
    ../test_scrollbench/src/TelemetryMonitor.h
    
    # Shared from main project
    ../src/AsyncImageProvider.cpp ../src/AsyncImageProvider.h
    ../src/TaskScheduler.cpp ../src/TaskScheduler.h
    ../src/DesktopHelper.cpp ../src/DesktopHelper.h
    ../src/SystemMonitor.cpp ../src/SystemMonitor.h
    ../src/LogManager.cpp ../src/LogManager.h
    ../src/SettingsHelper.cpp ../src/SettingsHelper.h
    ../src/GroupedProxyModel.cpp ../src/GroupedProxyModel.h
    ../src/AlbumModel.cpp ../src/AlbumModel.h
    ../src/ImageProcessor.cpp ../src/ImageProcessor.h
    ../src/HardwareAccelerationManager.cpp ../src/HardwareAccelerationManager.h
)

if(NOT MINIMAL_BUILD)
    list(APPEND PORTABLE_SOURCES ../src/VideoThumbnailer.cpp ../src/VideoThumbnailer.h)
endif()

# ============================================================================
# Create Executable
# ============================================================================

qt_add_executable(ScrollBenchPortable WIN32 ${PORTABLE_SOURCES})

# Output to single_exe directory for clarity
set_target_properties(ScrollBenchPortable PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# ============================================================================
# QML Resources - PULLED FROM SCROLLBENCH
# ============================================================================

# Define QML files with absolute paths
set(SCROLLBENCH_QML_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/MainScrollBench.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/GalleryViewScrollBench.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/GalleryViewSemanticScrollBench.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/PhotoViewerScrollBench.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/AlbumsViewScrollBench.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/PerformanceOverlay.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/DateScrubber.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/UsageGraph.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/ShareDialog.qml
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/ResizeEditor.qml
)

# Set aliases for each QML file (required for absolute paths in Qt resources)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/MainScrollBench.qml
    PROPERTIES QT_RESOURCE_ALIAS MainScrollBench.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/GalleryViewScrollBench.qml
    PROPERTIES QT_RESOURCE_ALIAS GalleryViewScrollBench.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/GalleryViewSemanticScrollBench.qml
    PROPERTIES QT_RESOURCE_ALIAS GalleryViewSemanticScrollBench.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/PhotoViewerScrollBench.qml
    PROPERTIES QT_RESOURCE_ALIAS PhotoViewerScrollBench.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/AlbumsViewScrollBench.qml
    PROPERTIES QT_RESOURCE_ALIAS AlbumsViewScrollBench.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/PerformanceOverlay.qml
    PROPERTIES QT_RESOURCE_ALIAS PerformanceOverlay.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/DateScrubber.qml
    PROPERTIES QT_RESOURCE_ALIAS DateScrubber.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/UsageGraph.qml
    PROPERTIES QT_RESOURCE_ALIAS UsageGraph.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/ShareDialog.qml
    PROPERTIES QT_RESOURCE_ALIAS ShareDialog.qml)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/../test_scrollbench/qml/ResizeEditor.qml
    PROPERTIES QT_RESOURCE_ALIAS ResizeEditor.qml)

qt_add_qml_module(ScrollBenchPortable
    URI ScrollBenchPortable
    VERSION 1.0
    QML_FILES ${SCROLLBENCH_QML_FILES}
    RESOURCE_PREFIX /
    NO_CACHEGEN  # Disable QML caching to avoid path issues with external files
)

# ============================================================================
# Link Libraries (Same as ScrollBench but static)
# ============================================================================

target_link_libraries(ScrollBenchPortable PRIVATE
    Qt6::Quick Qt6::Concurrent Qt6::Gui Qt6::QuickControls2 Qt6::Widgets
    psapi dxgi d3d11 pdh
    libraw_static
)

if(NOT MINIMAL_BUILD)
    target_link_libraries(ScrollBenchPortable PRIVATE Qt6::Multimedia)
    
    # FFmpeg libraries (reuse parent configuration)
    set(FFMPEG_LIB_DIR "${CMAKE_SOURCE_DIR}/3rdparty/ffmpeg/lib")
    target_link_libraries(ScrollBenchPortable PRIVATE
        ${FFMPEG_LIB_DIR}/avcodec.lib
        ${FFMPEG_LIB_DIR}/avformat.lib
        ${FFMPEG_LIB_DIR}/swscale.lib
        ${FFMPEG_LIB_DIR}/avutil.lib
    )
endif()

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(ScrollBenchPortable PRIVATE
    ../src
    ../test_scrollbench/src
    ../3rdparty/LibRaw
    ../3rdparty/ffmpeg/include
)

# ============================================================================
# Post-Build
# ============================================================================

if(STRIP_SYMBOLS AND NOT MSVC)
    add_custom_command(TARGET ScrollBenchPortable POST_BUILD
        COMMAND strip $<TARGET_FILE:ScrollBenchPortable>
        COMMENT "Stripping debug symbols from portable build..."
    )
endif()

message(STATUS ">>> Single EXE Portable build configured")
message(STATUS "    Output: single_exe/bin/ScrollBenchPortable.exe")
message(STATUS "    Static Runtime: ${STATIC_RUNTIME}")
message(STATUS "    Minimal Build: ${MINIMAL_BUILD}")
