cmake_minimum_required(VERSION 3.16)

project(serigraph VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.80 REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
endif()

find_package(Eigen3 3.3 REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- 3. Find Coin-OR Libraries via PkgConfig ---
find_package(PkgConfig REQUIRED)

pkg_check_modules(COIN_DEPS REQUIRED 
    cbc 
    clp 
    osi-clp 
    osi
    coinutils
    IMPORTED_TARGET
)
add_executable(serigraph
    src/main.cpp
    src/main_window.cpp
    src/serigraph_widget.cpp
    src/color_lut.cpp
    src/third-party/mixbox.cpp
    src/palette_widget.cpp
)

target_link_libraries(
    serigraph PRIVATE 
    Qt6::Widgets  
    Eigen3::Eigen
    PkgConfig::COIN_DEPS
)

set_target_properties(serigraph PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)