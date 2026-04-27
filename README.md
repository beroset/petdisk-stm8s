# petdisk-stm8s
Interfacing the Commodore PET to a microSD card via the IEEE-488 bus using the STM8S microcontroller.

## Prerequisites
You must have `CMake`, the `sdcc` compiler (version 4.4 or newer) and [`stm8flash`](https://github.com/vdudouyt/stm8flash) to flash the result into the target hardware.

## Building the firmware
The code uses CMake to create the build system.  On the Fedora Linux machine I use, here's the toolchain file:

```
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER /usr/bin/sdcc-sdcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

Note that for this system, the `sdcc-sdcc` file is a shell script:
```
#!/usr/bin/sh
PATH=/usr/libexec/sdcc:/usr/libexec/sdcc/x86_64-redhat-linux-gnu/12.1.0:$PATH
/usr/libexec/sdcc/sdcc "$@"
```

If you use another distro, you may need to explicitly set the path something like this.

Here's the command line to set it up:
```
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=~/toolchain-sdcc.cmake
```

Then to build and flash in one step:
```
cmake --build build -t flash
```
