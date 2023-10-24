cmake_minimum_required(VERSION 3.27)

project(StockExchangeTools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER "clang++-15")
set(CMAKE_VERBOSE_MAKEFILE on)

add_executable(bvb_scraper_tool src/bvb_scraper_tool.cpp)
target_include_directories(bvb_scraper_tool PUBLIC include expected/include/tl)
