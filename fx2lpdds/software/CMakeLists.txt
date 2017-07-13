# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
	project (fx2lpddsfm)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

find_path(LIBUSB_INCLUDE_DIR
	NAMES qlibusb.h
	PATH_SUFFIXES "qinclude" "qlibusb" "qlibusb-1.0")

find_library(USB_LIB usb-1.0)
if(NOT USB_LIB)
	  message(FATAL_ERROR "usb-1.0 library not found")
endif()

add_executable (fx2lpddsfm fx2lpddscomm.c fx2lpddsfm.c metricprefix.c modulators.c readers.c)
add_executable (sweep sweep.c)
add_executable (stereo_sine_wahwah stereo_sine_wahwah.c)

TARGET_LINK_LIBRARIES(fx2lpddsfm usb-1.0 m)
TARGET_LINK_LIBRARIES(sweep m)
TARGET_LINK_LIBRARIES(stereo_sine_wahwah m)

