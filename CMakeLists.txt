cmake_minimum_required(VERSION 3.16)

project(SamsungGallery LANGUAGES CXX)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick Concurrent Multimedia Gui QuickControls2 Widgets)

qt_standard_project_setup()

# --- Main Application (Stable) ---
qt_add_executable(appSamsungGallery
    src/main.cpp
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
    src/HardwareAccelerationManager.cpp src/HardwareAccelerationManager.h
    src/FrameBudgetScheduler.cpp src/FrameBudgetScheduler.h
    src/FastVolumeScanner.cpp src/FastVolumeScanner.h
)

qt_add_qml_module(appSamsungGallery
    URI SamsungGallery
    VERSION 2.0
    QML_FILES
        resources/qml/Main.qml
        resources/qml/GalleryView.qml
        resources/qml/GalleryViewSemantic.qml
        resources/qml/GalleryViewTiles.qml
        resources/qml/DateScrubber.qml
        resources/qml/PhotoViewer.qml
        resources/qml/AlbumsView.qml
        resources/qml/AlbumCard.qml
        resources/qml/BottomBar.qml
        resources/qml/StatsOverlay.qml
        resources/qml/UsageGraph.qml
        resources/qml/BuildInfo.qml
        resources/qml/WatermarkOverlay.qml
)

# --- LibRaw ---
set(LIBRAW_PATH "3rdparty/LibRaw")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${LIBRAW_PATH}/CMakeLists.txt")
    # Disable LibRaw examples and demos to speed up build
    set(LIBRAW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(LIBRAW_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
    set(LIBRAW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${LIBRAW_PATH})
endif()

# --- ScrollBench (Test App) ---
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_scrollbench/CMakeLists.txt")
    add_subdirectory(test_scrollbench)
endif()

# --- Single EXE Portable Build (Third Target) ---
# Builds a single, portable executable of ScrollBench with static linking
# for maximum compatibility across Windows 7-11 (x86 and x64)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/single_exe/CMakeLists.txt")
    option(BUILD_SINGLE_EXE "Build single portable executable" OFF)
    if(BUILD_SINGLE_EXE)
        add_subdirectory(single_exe)
    endif()
endif()

# --- FFmpeg ---
set(FFMPEG_PATH "3rdparty/ffmpeg")
set(FFMPEG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/include")
set(FFMPEG_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/lib")
set(FFMPEG_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${FFMPEG_PATH}/bin")

target_link_libraries(appSamsungGallery
    PRIVATE Qt6::Quick Qt6::Concurrent Qt6::Multimedia Qt6::Gui Qt6::QuickControls2 Qt6::Widgets psapi dxgi d3d11 pdh libraw_static
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avformat.lib
    ${FFMPEG_LIB_DIR}/swscale.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
)
target_include_directories(appSamsungGallery PRIVATE ${LIBRAW_PATH} ${FFMPEG_INCLUDE_DIR})

# Copy FFmpeg DLLs post-build
file(GLOB FFMPEG_DLLS "${FFMPEG_BIN_DIR}/*.dll")
if (FFMPEG_DLLS)
    add_custom_command(TARGET appSamsungGallery POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FFMPEG_DLLS}
        $<TARGET_FILE_DIR:appSamsungGallery>
    )
endif()

enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_imagemodel tests/tst_imagemodel.cpp 
    src/ImageModel.cpp src/ImageModel.h 
    src/AsyncImageProvider.cpp src/AsyncImageProvider.h
    src/DesktopHelper.cpp src/DesktopHelper.h
    src/SystemMonitor.cpp src/SystemMonitor.h
    src/SettingsHelper.cpp src/SettingsHelper.h
    src/TaskScheduler.cpp src/TaskScheduler.h 
    src/LogManager.cpp src/LogManager.h
    src/VideoThumbnailer.cpp src/VideoThumbnailer.h
    src/HardwareAccelerationManager.cpp src/HardwareAccelerationManager.h
    src/FrameBudgetScheduler.cpp src/FrameBudgetScheduler.h
    src/FastVolumeScanner.cpp src/FastVolumeScanner.h
)
target_link_libraries(tst_imagemodel PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent Qt6::Gui Qt6::Quick Qt6::Widgets libraw_static psapi pdh d3d11 dxgi
    ${FFMPEG_LIB_DIR}/avcodec.lib
    ${FFMPEG_LIB_DIR}/avformat.lib
    ${FFMPEG_LIB_DIR}/swscale.lib
    ${FFMPEG_LIB_DIR}/avutil.lib
)
target_include_directories(tst_imagemodel PRIVATE ${LIBRAW_PATH} ${FFMPEG_INCLUDE_DIR})
add_test(NAME tst_imagemodel COMMAND tst_imagemodel)

add_executable(tst_scheduler tests/tst_scheduler.cpp src/TaskScheduler.cpp src/TaskScheduler.h src/LogManager.cpp src/LogManager.h)
target_link_libraries(tst_scheduler PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent)
add_test(NAME tst_scheduler COMMAND tst_scheduler)

add_executable(tst_mft tests/tst_mft.cpp src/FastVolumeScanner.cpp src/FastVolumeScanner.h)
target_link_libraries(tst_mft PRIVATE Qt6::Core)

