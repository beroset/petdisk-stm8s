set(SRCS
  main.c
)

add_executable(main ${SRCS})

target_compile_options(main PRIVATE --opt-code-size --std=c23)

