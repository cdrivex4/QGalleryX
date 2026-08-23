cmake_minimum_required(VERSION 3.16)

# Detect if we are building standalone or as a subproject
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ScrollBench LANGUAGES CXX)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Quick Concurrent Multimedia Gui QuickControls2 Widgets)
    qt_standard_project_setup()
endif()

# QGalleryXBench Application (Direct Clone of QGalleryX + SIMD & Benchmark Engine)
qt_add_executable(QGalleryXBench
    src/main_scrollbench.cpp
    src/BC1Engine.cpp src/BC1Engine.h
    src/BenchmarkRunner.cpp src/BenchmarkRunner.h
    src/FastImageItem.cpp src/FastImageItem.h
    src/ImageModel.cpp src/ImageModel.h
    src/AlbumModel.cpp src/AlbumModel.h
    src/SettingsHelper.cpp src/SettingsHelper.h
    src/SystemMonitor.cpp src/SystemMonitor.h
    src/AsyncImageProvider.cpp src/AsyncImageProvider.h
    src/GroupedProxyModel.cpp src/GroupedProxyModel.h
    src/DesktopHelper.cpp src/DesktopHelper.h
    src/VideoThumbnailer.cpp src/VideoThumbnailer.h
    src/LogManager.cpp src/LogManager.h
    src/TaskScheduler.cpp src/TaskScheduler.h
    src/RingBufferDispatcher.cpp src/RingBufferDispatcher.h
    src/ViewportGovernor.cpp src/ViewportGovernor.h
    src/ImageProcessor.cpp src/ImageProcessor.h
    src/FileCacheManager.cpp src/FileCacheManager.h
    src/PassiveReadLatencyGuard.cpp src/PassiveReadLatencyGuard.h
    src/FastVolumeScanner.cpp src/FastVolumeScanner.h
)

set_target_properties(QGalleryXBench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy")

# appScrollBenchNet - Network Diagnostic Watchdog
add_executable(QGalleryXBenchNet src/main_net_watchdog.cpp)
set_target_properties(QGalleryXBenchNet PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy")
target_link_libraries(QGalleryXBenchNet PRIVATE kernel32 user32)

qt_add_qml_module(QGalleryXBench
    URI ScrollBench
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/GalleryView.qml
        qml/GalleryViewSemantic.qml
        qml/GalleryViewTiles.qml
        qml/DateScrubber.qml
        qml/PhotoViewer.qml
        qml/AlbumsView.qml
        qml/BottomBar.qml
        qml/StatsOverlay.qml
        qml/UsageGraph.qml
        qml/SelectionActionBar.qml
        qml/ShareDialog.qml
        qml/ResizeEditor.qml
        qml/ToastOverlay.qml
        qml/StyledButton.qml
        qml/MediaIcon.qml
        qml/SvgIcon.qml
        qml/PerformanceOverlay.qml
        qml/BenchmarkRunner.qml
)

# --- LibRaw ---
if(NOT TARGET libraw_static)
    set(LIBRAW_PATH "../3rdparty/LibRaw")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIBRAW_PATH}/CMakeLists.txt")
        set(LIBRAW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(LIBRAW_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        set(LIBRAW_INSTALL OFF CACHE BOOL "" FORCE)
        add_subdirectory(${LIBRAW_PATH} libraw_build EXCLUDE_FROM_ALL)
    endif()
endif()

# --- FFmpeg ---
set(FFMPEG_PATH "../3rdparty/ffmpeg")
set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/include")
set(FFMPEG_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/lib")
set(FFMPEG_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/bin")

target_link_libraries(QGalleryXBench
    PRIVATE 
    Qt6::Quick Qt6::Concurrent Qt6::Multimedia Qt6::Gui Qt6::QuickControls2 Qt6::Widgets 
    psapi dxgi d3d11 pdh 
    libraw_static
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avformat.lib
    ${FFMPEG_LIB_DIR}/swscale.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
)

target_include_directories(QGalleryXBench PRIVATE 
    src
    ${LIBRAW_PATH} 
    ${FFMPEG_INCLUDE_DIR}
)

# Copy FFmpeg DLLs post-build
file(GLOB FFMPEG_DLLS "${FFMPEG_BIN_DIR}/*.dll")
if (FFMPEG_DLLS)
    add_custom_command(TARGET QGalleryXBench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FFMPEG_DLLS}
        $<TARGET_FILE_DIR:QGalleryXBench>
    )
endif()
