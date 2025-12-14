cmake_minimum_required(VERSION 3.16)

# Detect if we are building standalone or as a subproject
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ScrollBench LANGUAGES CXX)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Quick Concurrent Multimedia Gui QuickControls2 Widgets)
    qt_standard_project_setup()
endif()

# ScrollBench Test Application  
qt_add_executable(appScrollBench
    src/main_scrollbench.cpp
    src/ScrollBenchImageModel.cpp src/ScrollBenchImageModel.h
    src/FrameBudgetScheduler.cpp src/FrameBudgetScheduler.h
    src/TelemetryMonitor.cpp src/TelemetryMonitor.h
    # Reuse from main project
    ../src/AsyncImageProvider.cpp ../src/AsyncImageProvider.h
    ../src/TaskScheduler.cpp ../src/TaskScheduler.h
    ../src/VideoThumbnailer.cpp ../src/VideoThumbnailer.h
    ../src/DesktopHelper.cpp ../src/DesktopHelper.h
    ../src/SystemMonitor.cpp ../src/SystemMonitor.h
    ../src/LogManager.cpp ../src/LogManager.h
)

set_target_properties(appScrollBench PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/deploy")

qt_add_qml_module(appScrollBench
    URI ScrollBench
    VERSION 1.0
    QML_FILES
        qml/MainScrollBench.qml
        qml/GalleryViewScrollBench.qml
        qml/PhotoViewerScrollBench.qml
        qml/PerformanceOverlay.qml
        qml/ShareDialog.qml
        qml/ResizeEditor.qml
)

# --- LibRaw (copy from main project) ---
# Only add if not already defined by parent
if(NOT TARGET libraw_static)
    set(LIBRAW_PATH "../3rdparty/LibRaw")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIBRAW_PATH}/CMakeLists.txt")
        set(LIBRAW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(LIBRAW_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        set(LIBRAW_INSTALL OFF CACHE BOOL "" FORCE)
        add_subdirectory(${LIBRAW_PATH} libraw_build EXCLUDE_FROM_ALL)
    endif()
endif()

# --- FFmpeg (copy from main project) ---
set(FFMPEG_PATH "../3rdparty/ffmpeg")
set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/include")
set(FFMPEG_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/lib")
set(FFMPEG_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/bin")

# Link everything at once!
target_link_libraries(appScrollBench
    PRIVATE 
    Qt6::Quick Qt6::Concurrent Qt6::Multimedia Qt6::Gui Qt6::QuickControls2 Qt6::Widgets 
    psapi dxgi d3d11 pdh 
    libraw_static
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avformat.lib
    ${FFMPEG_LIB_DIR}/swscale.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
)

target_include_directories(appScrollBench PRIVATE 
    ../src 
    ${LIBRAW_PATH} 
    ${FFMPEG_INCLUDE_DIR}
)

# Copy FFmpeg DLLs post-build
file(GLOB FFMPEG_DLLS "${FFMPEG_BIN_DIR}/*.dll")
if (FFMPEG_DLLS)
    add_custom_command(TARGET appScrollBench POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FFMPEG_DLLS}
        $<TARGET_FILE_DIR:appScrollBench>
    )
endif()
