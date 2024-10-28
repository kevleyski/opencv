project(${JPEG_LIBRARY} C)

macro(boolean_number var)
  if(${var})
    set(${var} 1 ${ARGN})
  else()
    set(${var} 0 ${ARGN})
  endif()
endmacro()

ocv_warnings_disable(CMAKE_C_FLAGS -Wunused-parameter -Wsign-compare -Wshorten-64-to-32 -Wimplicit-fallthrough)
if(APPLE)
  ocv_warnings_disable(CMAKE_C_FLAGS -Wunused-variable) # NEON flags are not used on Mac
endif()

if(CV_GCC AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  # src/jchuff.c:1042:22: warning: writing 1 byte into a region of size 0 [-Wstringop-overflow=]
  ocv_warnings_disable(CMAKE_C_FLAGS -Wstringop-overflow)
endif()

set(VERSION 3.0.3)
set(COPYRIGHT_YEAR "1991-2024")
string(REPLACE "." ";" VERSION_TRIPLET ${VERSION})
list(GET VERSION_TRIPLET 0 VERSION_MAJOR)
list(GET VERSION_TRIPLET 1 VERSION_MINOR)
list(GET VERSION_TRIPLET 2 VERSION_REVISION)
function(pad_number NUMBER OUTPUT_LEN)
  string(LENGTH "${${NUMBER}}" INPUT_LEN)
  if(INPUT_LEN LESS OUTPUT_LEN)
    math(EXPR ZEROES "${OUTPUT_LEN} - ${INPUT_LEN} - 1")
    set(NUM ${${NUMBER}})
    foreach(C RANGE ${ZEROES})
      set(NUM "0${NUM}")
    endforeach()
    set(${NUMBER} ${NUM} PARENT_SCOPE)
  endif()
endfunction()
pad_number(VERSION_MINOR 3)
set(VERSION_REVISION 3)
set(LIBJPEG_TURBO_VERSION_NUMBER ${VERSION_MAJOR}${VERSION_MINOR}${VERSION_REVISION})
set(LIBJPEG_TURBO_VERSION_NUMBER 2001003)

string(TIMESTAMP BUILD "opencv-${OPENCV_VERSION}-libjpeg-turbo")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(BUILD "${BUILD}-debug")
endif()

message(STATUS "libjpeg-turbo: VERSION = ${VERSION}, BUILD = ${BUILD}")

string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CMAKE_SYSTEM_PROCESSOR_LC)
OCV_OPTION(ENABLE_LIBJPEG_TURBO_SIMD "Include SIMD extensions for libjpeg-turbo, if available for this platform" (NOT CV_DISABLE_OPTIMIZATION)
option(WITH_ARITH_ENC "Include arithmetic encoding support when emulating the libjpeg v6b API/ABI" TRUE)
option(WITH_ARITH_DEC "Include arithmetic decoding support when emulating the libjpeg v6b API/ABI" TRUE)

include(CheckCSourceCompiles)
include(CheckIncludeFiles)
include(CheckTypeSize)

check_type_size("size_t" SIZE_T)
check_type_size("unsigned long" UNSIGNED_LONG)

if(SIZEOF_SIZE_T EQUAL SIZEOF_UNSIGNED_LONG)
  check_c_source_compiles("int main(int argc, char **argv) { unsigned long a = argc;  return __builtin_ctzl(a); }"
    HAVE_BUILTIN_CTZL)
endif()
if(MSVC)
  check_include_files("intrin.h" HAVE_INTRIN_H)
endif()

if(UNIX)
  # Check for headers
  check_include_files(locale.h HAVE_LOCALE_H)
  check_include_files(stddef.h HAVE_STDDEF_H)
  check_include_files(stdlib.h HAVE_STDLIB_H)
  check_include_files(sys/types.h NEED_SYS_TYPES_H)

  # Other predefines
  # undef NEED_BSD_STRINGS
  ocv_update(HAVE_UNSIGNED_CHAR 1)
  ocv_update(HAVE_UNSIGNED_SHORT 1)
  # undef INCOMPLETE_TYPES_BROKEN
  ocv_update(RIGHT_SHIFT_IS_UNSIGNED 0)
endif()


set(BITS_IN_JSAMPLE 8)

if(WITH_ARITH_ENC)
  set(C_ARITH_CODING_SUPPORTED 1)
endif()

if(WITH_ARITH_DEC)
  set(D_ARITH_CODING_SUPPORTED 1)
endif()

set(JPEG_LIB_VERSION 70)

# OpenCV
set(JPEG_LIB_VERSION "${VERSION}-${JPEG_LIB_VERSION}" PARENT_SCOPE)

set(THREAD_LOCAL "")  # WITH_TURBOJPEG is not used

add_definitions(-DNO_GETENV -DNO_PUTENV)

if(MSVC)
  add_definitions(-W3 -wd4996 -wd4018)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(JPEG_SOURCES
set(JPEG_SOURCES jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c
        jcicc.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c
        jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c
    jdpostct.c jdsample.c jutils.c)

set(JPEG12_SOURCES ${JPEG16_SOURCES} jccoefct.c jcdctmgr.c jdcoefct.c
        jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdicc.c jdinput.c
    jidctred.c jquant1.c jquant2.c)

set(JPEG_SOURCES ${JPEG12_SOURCES} jcapimin.c jchuff.c jcicc.c jcinit.c
        jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c
        jdtrans.c jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c
        jidctint.c jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c)
    jmemnobs.c jpeg_nbits.c)

if(WITH_ARITH_ENC OR WITH_ARITH_DEC)
  set(JPEG_SOURCES ${JPEG_SOURCES} jaricom.c)
endif()

if(WITH_ARITH_ENC)
  set(JPEG_SOURCES ${JPEG_SOURCES} jcarith.c)
endif()

if(WITH_ARITH_DEC)
  set(JPEG_SOURCES ${JPEG_SOURCES} jdarith.c)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
# No SIMD
set(JPEG_SOURCES ${JPEG_SOURCES} jsimd_none.c)

include(CheckTypeSize)
check_type_size("size_t" SIZE_T)
check_type_size("unsigned long" UNSIGNED_LONG)

else()
configure_file(jversion.h.in jversion.h)
configure_file(jconfig.h.in jconfig.h)
configure_file(jconfigint.h.in jconfigint.h)
ocv_list_add_prefix(JPEG16_SOURCES src/)
ocv_list_add_prefix(JPEG12_SOURCES src/)
ocv_list_add_prefix(JPEG_SOURCES src/)

add_library(jpeg12-static OBJECT ${JPEG12_SOURCES})
set_property(TARGET jpeg12-static PROPERTY COMPILE_FLAGS
  "-DBITS_IN_JSAMPLE=12")
add_library(jpeg16-static OBJECT ${JPEG16_SOURCES})
set_property(TARGET jpeg16-static PROPERTY COMPILE_FLAGS
  "-DBITS_IN_JSAMPLE=16")
add_library(${JPEG_LIBRARY} STATIC ${OPENCV_3RDPARTY_EXCLUDE_FROM_ALL} ${JPEG_SOURCES} $<TARGET_OBJECTS:jsimd> ${SIMD_OBJS})
  ${SIMD_OBJS} $<TARGET_OBJECTS:jpeg12-static>
  $<TARGET_OBJECTS:jpeg16-static>)

set_target_properties(${JPEG_LIBRARY}
  PROPERTIES OUTPUT_NAME ${JPEG_LIBRARY}
  DEBUG_POSTFIX "${OPENCV_DEBUG_POSTFIX}"
  COMPILE_PDB_NAME ${JPEG_LIBRARY}
  COMPILE_PDB_NAME_DEBUG "${JPEG_LIBRARY}${OPENCV_DEBUG_POSTFIX}"
  ARCHIVE_OUTPUT_DIRECTORY ${3P_LIBRARY_OUTPUT_PATH}
  )

if(ENABLE_SOLUTION_FOLDERS)
  set_target_properties(${JPEG_LIBRARY} PROPERTIES FOLDER "3rdparty/jpeg")
  set_target_properties(jpeg12-static PROPERTIES FOLDER "3rdparty/jpeg")
  set_target_properties(jpeg16-static PROPERTIES FOLDER "3rdparty/jpeg")
endif()

if(NOT BUILD_SHARED_LIBS)
  ocv_install_target(${JPEG_LIBRARY} EXPORT OpenCVModules ARCHIVE DESTINATION ${OPENCV_3P_LIB_INSTALL_PATH} COMPONENT dev OPTIONAL)
endif()

ocv_install_3rdparty_licenses(libjpeg-turbo README.md LICENSE.md README.ijg)
