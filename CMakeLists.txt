cmake_minimum_required(VERSION 3.31)
project(petdisk-stm8 C)

SET(CMAKE_C_FLAGS "-mstm8")

add_subdirectory(src)

add_custom_target(flash
  COMMAND stm8flash -c stlinkv21 -p stm8s207k8 -w "$<TARGET_FILE_DIR:main>/$<TARGET_FILE_BASE_NAME:main>.ihx"
  DEPENDS main
  COMMENT "Flashing $<TARGET_FILE_BASE_NAME:main>.ihx"
)
