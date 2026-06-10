set(SRCS
  main.c
  timer.c
  display.c
  spi.c
  uart.c
  diskio.c
  ../pff/source/pff.c
)

add_executable(main ${SRCS})
target_include_directories(main PRIVATE ../pff/source)

target_compile_options(main PRIVATE --opt-code-size --std=c23)
if (DEVBOARD)
    target_compile_definitions(main PRIVATE DEVBOARD=1)
endif()

