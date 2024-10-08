cmake_minimum_required(VERSION 3.27)

project(StockExchangeTools LANGUAGES CXX)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
    GIT_TAG v5.0.0
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
    FetchContent_Populate(ftxui)
    add_subdirectory(${ftxui_SOURCE_DIR} ${ftxui_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(CMAKE_CXX_COMPILER "clang++-17")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE on)

#
# bvb_scraper_tool build
#
add_executable(bvb_scraper_tool
    src/bvb_scraper_tool.cpp
    src/bvb_scraper.cpp
    src/html_parser.cpp
    src/curl_utils.cpp
    src/string_utils.cpp
    src/cli_utils.cpp
    src/chrono_utils.cpp
)
target_include_directories(bvb_scraper_tool PUBLIC
    include
    expected/include/tl
    magic_enum/include)
set_target_properties(bvb_scraper_tool PROPERTIES COMPILE_FLAGS
    "-std=c++23 -Wall -Werror")
target_link_libraries(bvb_scraper_tool ${CURL_LIBRARIES})

#
# index_investing_tool build
#
add_executable(index_investing_tool
    src/index_investing/main.cpp
    src/index_investing/config.cpp
    src/index_investing/index_replication.cpp
    src/index_investing/terminal_ui.cpp
    src/string_utils.cpp
    src/cli_utils.cpp
    src/websocket_connection.cpp
    src/tradeville.cpp
    src/bvb_scraper.cpp
    src/html_parser.cpp
    src/curl_utils.cpp
    src/chrono_utils.cpp
)
target_include_directories(index_investing_tool PUBLIC
    include
    expected/include/tl
    magic_enum/include
)
set_target_properties(index_investing_tool PROPERTIES COMPILE_FLAGS
    "-std=c++23 -Wall -Werror")
target_link_libraries(index_investing_tool PRIVATE
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
    ftxui::screen
    ftxui::dom
    ftxui::component
)

#
# unit tests build
#
add_executable(set_unit_tests
    test/html_parser_test.cpp
    test/bvb_scraper_test.cpp
    src/html_parser.cpp
    src/bvb_scraper.cpp
    src/curl_utils.cpp
    src/string_utils.cpp
    src/chrono_utils.cpp
)
target_include_directories(set_unit_tests PUBLIC
    include
    expected/include/tl
    magic_enum/include)
set_target_properties(set_unit_tests PROPERTIES COMPILE_FLAGS
    "-std=c++23 -Wall -Werror")
target_link_libraries(set_unit_tests ${CURL_LIBRARIES} gtest gtest_main pthread)
