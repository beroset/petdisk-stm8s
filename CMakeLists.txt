cmake_minimum_required(VERSION 3.31)
project(petdisk-stm8 
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "Commodore PET to microSD card interface")

SET(CMAKE_C_FLAGS "-mstm8")
option(DEVBOARD "Define this if using a NUCLEO-8S207K8 board" OFF)
option(WITH_DOC "Create project documentation" OFF)
if (DEVBOARD)
    set(STLINKVER "stlinkv21" CACHE STRING "ST-Link version")
    set_property(CACHE STLINKVER PROPERTY STRINGS stlink stlinkv2 stlinkv21 stlinkv3 espstlink)
    set(PROCESSOR "stm8s207k8")
else()
    set(STLINKVER "stlinkv2" CACHE STRING "ST-Link version")
    set_property(CACHE STLINKVER PROPERTY STRINGS stlink stlinkv2 stlinkv21 stlinkv3 espstlink)
    set(PROCESSOR "stm8s105k6")
endif()

add_subdirectory(src)
if (WITH_DOC)
    add_subdirectory(doc)
endif()

add_custom_target(flash
  COMMAND stm8flash -c ${STLINKVER} -p ${PROCESSOR} -w "$<TARGET_FILE_DIR:main>/$<TARGET_FILE_BASE_NAME:main>.ihx"
  DEPENDS main
  COMMENT "Flashing $<TARGET_FILE_BASE_NAME:main>.ihx"
)
