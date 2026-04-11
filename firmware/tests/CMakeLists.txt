cmake_minimum_required(VERSION 3.20)

# ── Catch2 (amalgamated) ──────────────────────────────────────────────────────
add_library(Catch2 STATIC catch_amalgamated.cpp)
target_include_directories(Catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Suppress warnings in third-party Catch2 code; suppress built-in main so we
# can provide our own via test_main.cpp.
target_compile_options(Catch2 PRIVATE -w)
target_compile_definitions(Catch2 PUBLIC CATCH_AMALGAMATED_CUSTOM_MAIN)

# ── Test executable ───────────────────────────────────────────────────────────
add_executable(petdisk_tests
    test_main.cpp
    test_ieee488.cpp
    test_sdcard.cpp
    test_cbmdos.cpp
    ../src/hal_linux.cpp
)
target_link_libraries(petdisk_tests PRIVATE petdisk_lib Catch2)
target_include_directories(petdisk_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # for mock_hal.hpp and catch headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_options(petdisk_tests PRIVATE -Wall -Wextra)

# ── CTest integration ─────────────────────────────────────────────────────────
enable_testing()
add_test(NAME petdisk_tests COMMAND petdisk_tests)
