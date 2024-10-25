cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  optift
  VERSION 0.0.1
  LANGUAGES CXX)

add_executable(optift src/main.cpp src/partitioner.cpp src/cost_model.cpp src/input.cpp)
target_include_directories(optift PRIVATE include)

# For formatting
find_package(fmt CONFIG REQUIRED)
target_link_libraries(optift PRIVATE fmt::fmt)
# For font subsetting
find_package(harfbuzz CONFIG REQUIRED)
target_link_libraries(optift PRIVATE harfbuzz::harfbuzz-subset)
# For JSON parsing
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(optift PRIVATE nlohmann_json::nlohmann_json)
# For easy parallelism
find_package(TBB CONFIG REQUIRED)
target_link_libraries(optift PRIVATE TBB::tbb TBB::tbbmalloc)
# For progress bars
find_package(indicators CONFIG REQUIRED)
target_link_libraries(optift PRIVATE indicators::indicators)
# For ranges (mostly a few good features from C++23 backported to C++20)
find_package(range-v3 CONFIG REQUIRED)
target_link_libraries(optift PRIVATE range-v3::meta range-v3::concepts range-v3::range-v3)
# For logging
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(optift PRIVATE spdlog::spdlog)
# For argument parsing
find_package(argparse CONFIG REQUIRED)
target_link_libraries(optift PRIVATE argparse::argparse)
# For plotting
# find_package(Matplot++ CONFIG REQUIRED)
# target_link_libraries(optift PRIVATE Matplot++::matplot)

# find_package(Microsoft.GSL CONFIG REQUIRED)
# target_link_libraries(optift PRIVATE Microsoft.GSL::GSL)

find_package(PkgConfig)
pkg_check_modules(libwoff2enc REQUIRED IMPORTED_TARGET GLOBAL libwoff2enc>=1.0)
pkg_check_modules(icu-uc REQUIRED IMPORTED_TARGET GLOBAL icu-uc>=74.2)
pkg_check_modules(zlib-ng REQUIRED IMPORTED_TARGET GLOBAL zlib-ng>=2.1.5)
# For WOFF2 encoding
target_link_libraries(optift PRIVATE PkgConfig::libwoff2enc)
# For dealing with Unicode
target_link_libraries(optift PRIVATE PkgConfig::icu-uc)
# For compression
target_link_libraries(optift PRIVATE PkgConfig::zlib-ng)
