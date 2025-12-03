cmake_minimum_required(VERSION 3.16)

project(SamsungGallery LANGUAGES CXX)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick Concurrent Multimedia Gui QuickControls2)

qt_standard_project_setup()

# --- Main Application (Stable) ---
# --- Main Application (Stable) ---
qt_add_executable(appSamsungGallery
    src/main.cpp
    src/ImageModel.cpp src/ImageModel.h
    src/AlbumModel.cpp src/AlbumModel.h
    src/SettingsHelper.cpp src/SettingsHelper.h
    src/SystemMonitor.cpp src/SystemMonitor.h
    src/AsyncImageProvider.cpp src/AsyncImageProvider.h
    src/GroupedProxyModel.cpp src/GroupedProxyModel.h
)

qt_add_qml_module(appSamsungGallery
    URI SamsungGallery
    VERSION 1.0
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

target_link_libraries(appSamsungGallery
    PRIVATE Qt6::Quick Qt6::Concurrent Qt6::Multimedia Qt6::Gui Qt6::QuickControls2 psapi dxgi d3d11
)

# --- Test Application (Semantic Zoom) ---
qt_add_executable(appSamsungGalleryTest
    src/main_test.cpp
    src/ImageModel.cpp src/ImageModel.h
    src/AlbumModel.cpp src/AlbumModel.h
    src/SettingsHelper.cpp src/SettingsHelper.h
    src/SystemMonitor.cpp src/SystemMonitor.h
    src/AsyncImageProvider.cpp src/AsyncImageProvider.h
    src/GroupedProxyModel.cpp src/GroupedProxyModel.h
)

qt_add_qml_module(appSamsungGalleryTest
    URI SamsungGalleryTest
    VERSION 1.0
    QML_FILES
        resources/qml/MainSemantic.qml
        resources/qml/GalleryViewSemantic.qml
        resources/qml/GalleryViewTiles.qml
        resources/qml/DateScrubber.qml
        resources/qml/PhotoViewer.qml
        resources/qml/AlbumsView.qml
        resources/qml/BottomBar.qml
        resources/qml/StatsOverlay.qml
        resources/qml/UsageGraph.qml
)

target_link_libraries(appSamsungGalleryTest
    PRIVATE Qt6::Quick Qt6::Concurrent Qt6::Multimedia Qt6::Gui Qt6::QuickControls2 psapi dxgi d3d11
)

enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(tst_imagemodel tests/tst_imagemodel.cpp src/ImageModel.cpp src/ImageModel.h)
target_link_libraries(tst_imagemodel PRIVATE Qt6::Core Qt6::Test Qt6::Concurrent Qt6::Gui Qt6::Quick)
add_test(NAME tst_imagemodel COMMAND tst_imagemodel)
