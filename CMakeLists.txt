cmake_minimum_required(VERSION 3.20)
project(power_quality_analyser C)

set(CMAKE_C_STANDARD 99)

add_executable(power_analyser
    main.c
    waveform.c
    io.c
)

# Link the maths library (needed for sqrt())
target_link_libraries(power_analyser m)
