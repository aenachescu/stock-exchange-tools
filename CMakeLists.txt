cmake_minimum_required(VERSION 3.27)

project(StockExchangeTools LANGUAGES CXX)

find_package(CURL REQUIRED)

set(CMAKE_CXX_COMPILER "clang++-15")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(bvb_scraper_tool
    src/bvb_scraper_tool.cpp
    src/bvb_scraper.cpp)

target_include_directories(bvb_scraper_tool PUBLIC
    include
    expected/include/tl)

set_target_properties(bvb_scraper_tool PROPERTIES COMPILE_FLAGS "-std=c++2b")

target_link_libraries(bvb_scraper_tool ${CURL_LIBRARIES})
