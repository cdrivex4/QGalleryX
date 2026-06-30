cmake_minimum_required(VERSION 3.16)

# Project information
project(LMStudioTest LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt 6.4 (same as main project)
find_package(Qt6 6.9.3 REQUIRED COMPONENTS Quick Gui Test)

# Qt standard setup
qt_standard_project_setup()

# Create executable for Hello World application
qt_add_executable(HelloWorld
    src/main.cpp
)

# Add QML module
qt_add_qml_module(HelloWorld
    URI LMStudioTest
    VERSION 1.0
    QML_FILES
        src/main.qml
)

# Link Qt libraries
target_link_libraries(HelloWorld
    PRIVATE
        Qt6::Quick
        Qt6::Gui
        Qt6::Qml
)

# Create test executable
add_executable(tst_helloworld
    tests/tst_helloworld.cpp
)

# Link Qt Test libraries
target_link_libraries(tst_helloworld
    PRIVATE
        Qt6::Test
        Qt6::Quick
        Qt6::Gui
        Qt6::Qml
)

# Enable testing
enable_testing()

# Add test to CTest
add_test(NAME tst_helloworld COMMAND tst_helloworld)

# Set working directory for tests
set_tests_properties(tst_helloworld
    PROPERTIES
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Install targets (optional)
install(TARGETS HelloWorld
    RUNTIME DESTINATION bin
)

install(TARGETS tst_helloworld
    RUNTIME DESTINATION bin
)

# Print configuration information
message(STATUS "Project: LMStudioTest")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt Version: ${Qt6_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")