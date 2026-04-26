set(SRCS
  main.c
  timer1ms.c
  display.c
)

add_executable(main ${SRCS})

target_compile_options(main PRIVATE --opt-code-size --std=c23)

