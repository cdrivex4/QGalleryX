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
        resources/qml/BottomBar.qml
        resources/qml/StatsOverlay.qml
        resources/qml/UsageGraph.qml
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
    src/TaskScheduler.cpp src/TaskScheduler.h 
    src/LogManager.cpp src/LogManager.h
)
target_link_libraries(tst_imagemodel PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent Qt6::Gui Qt6::Quick libraw_static)
target_include_directories(tst_imagemodel PRIVATE ${LIBRAW_PATH})
add_test(NAME tst_imagemodel COMMAND tst_imagemodel)

add_executable(tst_scheduler tests/tst_scheduler.cpp src/TaskScheduler.cpp src/TaskScheduler.h src/LogManager.cpp src/LogManager.h)
target_link_libraries(tst_scheduler PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent)
add_test(NAME tst_scheduler COMMAND tst_scheduler)
