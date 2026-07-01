set(SRCS
  main.c
  timer.c
  spi.c
  uart.c
  diskio.c
  ../fatfs/source/ff.c
  ../fatfs/source/ffunicode.c
)

add_executable(main ${SRCS})
target_include_directories(main PRIVATE ../fatfs/source)

target_compile_options(main PRIVATE --opt-code-size --std=c23)
if (DEVBOARD)
    target_compile_definitions(main PRIVATE DEVBOARD=1)
endif()

