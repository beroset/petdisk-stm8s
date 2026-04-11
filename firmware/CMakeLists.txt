cmake_minimum_required(VERSION 3.20)

# ── Platform-independent C library ───────────────────────────────────────────
add_library(petdisk_lib STATIC
    src/ieee488.c
    src/sdcard.c
    src/cbmdos.c
    src/petdisk.c
)
target_include_directories(petdisk_lib PUBLIC include)
target_compile_options(petdisk_lib PRIVATE -Wall -Wextra -Wpedantic)

# ── Linux simulation executable ───────────────────────────────────────────────
add_executable(petdisk_sim
    src/main.cpp
    src/hal_linux.cpp
)
target_include_directories(petdisk_sim PRIVATE include)
target_link_libraries(petdisk_sim PRIVATE petdisk_lib)
target_compile_options(petdisk_sim PRIVATE -Wall -Wextra)

# ── Tests ─────────────────────────────────────────────────────────────────────
add_subdirectory(tests)
