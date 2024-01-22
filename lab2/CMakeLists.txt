# 
# Generally starts every CMakeLists.txt file.  Setting Cmake, C and C++ versions
#
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#
# The pick build tools are a liuttle odd but make for a better cmake file below.
# generally cross-compile cmake systems that I've used int he past use a tools file 
# as part of the command line.. (.e.g cmake -DCMAKE_TOOLCHAIN_FILE="../armgcc.cmake" ) 
# but the cmake foolery in this included file sets up most of the environment.
#
include(../../pico/pico-sdk/external/pico_sdk_import.cmake)

#
# Setting up $FREERTOS_SRC_DIR to point to the FreeRTOS distrobution that you have on your build machine.
# I personally have 10 or 12 Downrev versions of FreeRTOS installed on my workstation so that I never have to 
# monkey with updating older projects.
#
set(FREERTOS_SRC_DIR ../../FreeRTOSv202212.01/FreeRTOS/Source)

#
# Set the cmake 'Project'.  This line sets up the cmake target and does an implicent define of PROJECT_NAME
# that I use below to not have to put the 'lab2' project on every definition.  
#
project(lab2 C CXX ASM)

#
# a way to make sure that you have a support library version at or greater than a needed library.
#
if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# 
# Initialize the SDK
#
pico_sdk_init()

#
# I'm not terriably happy with the default options that the Pico folks choose...  I prefer to set warnings as errors then 
# disable specific warnings as theyu have done below.  When you perform the make you can add the command line option 
# VERBOSE=1 and the generated makefile will dump the full command line for each compile and show all the warnings..  There 
# are several that we would fix if this was in my workplace where warniongs count as compile errors.
#
add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )


#
# This builds a freertos library rather than just adding all the FreeRTOS modules to the project build..
# adding libraries to your project can both add complexity as well as clairity but as projects get bigger 
# they become more helpful.
#
add_library(freertos
        ${FREERTOS_SRC_DIR}/event_groups.c
        ${FREERTOS_SRC_DIR}/list.c
        ${FREERTOS_SRC_DIR}/queue.c
        ${FREERTOS_SRC_DIR}/stream_buffer.c
        ${FREERTOS_SRC_DIR}/tasks.c
        ${FREERTOS_SRC_DIR}/timers.c
        ${FREERTOS_SRC_DIR}/portable/MemMang/heap_3.c
        ${FREERTOS_SRC_DIR}/portable/GCC/ARM_CM0/port.c
)

#
# add any specialk include paths that are unique to the FreeRTOS library build.
#
target_include_directories(freertos PUBLIC
        .
        ${FREERTOS_SRC_DIR}/include
        ${FREERTOS_SRC_DIR}/portable/GCC/ARM_CM0
)        

#
# Add the long list of .c files that we have for lab2.
# As we add files for later consideration thys will get added like lab2.c below.
#
add_executable(${PROJECT_NAME}
        lab2.c
        )


#
# pull in common dependencies, This is where cmake adds the pico and freertos libraries to ourt project
#
target_link_libraries(${PROJECT_NAME} pico_stdlib freertos)


# 
# tell the pico library that you will be using usb serial and not an actual uart on the 
# processor
#
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)


# 
# create map/bin/hex file etc.  These are generally extra files that are useful in addition to the .elf file.
#
pico_add_extra_outputs(${PROJECT_NAME})

#
# Use the ARM tools to dump sizes of the major segments in the file.  As your projects get large this is 
# something yoiu want to keep track of.
#
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND arm-none-eabi-size ${PROJECT_NAME}.elf)

# Define the source and destination files
set(source_file "${PROJECT_NAME}.uf2")
set(destination_file "/media/$ENV{USER}/RPI-RP2")

# 
# Add a custom command to copy the file
#
# Until you are using a debugger the pico is programmed by copying a .uf2 file to the faux
# usb drive manufactured by the Pico bootloader.  The code below will allow the command line 
# `make flash` to be used.  Be sure to boot the Pico into it's faux-drive mode before running this 
# command
#
add_custom_command(
    OUTPUT ${destination_file}
    COMMAND ${CMAKE_COMMAND} -E copy ${source_file} ${destination_file}
    DEPENDS ${source_file} 
    COMMENT "Copying file: ${source_file} to ${destination_file}"
)

# Add a custom target that depends on the custom command
add_custom_target(flash DEPENDS ${destination_file})