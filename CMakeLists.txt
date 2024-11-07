cmake_minimum_required(VERSION 3.8)
project(stp_parser)
set(CMAKE_CXX_STANDARD 17)

add_library(
  ${PROJECT_NAME} # library name
    ${PROJECT_SOURCE_DIR}/src/stp_parser.cpp #sources...
    ${PROJECT_SOURCE_DIR}/include/stp_parser.hpp #just for vscode intellisence
)

#link_libraries(${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/)
add_library(stepcore SHARED IMPORTED)
set_property(TARGET stepcore PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libstepcore.dylib")

add_library(exppp SHARED IMPORTED)
set_property(TARGET exppp PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libexppp.dylib")

add_library(express SHARED IMPORTED)
set_property(TARGET express PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libexpress.dylib")

add_library(sdai_IFC4 SHARED IMPORTED)
set_property(TARGET sdai_IFC4 PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libsdai_IFC4.dylib")

add_library(stepdai SHARED IMPORTED)
set_property(TARGET stepdai PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libstepdai.dylib")

add_library(stepeditor SHARED IMPORTED)
set_property(TARGET stepeditor PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libstepeditor.dylib")

add_library(steplazyfile SHARED IMPORTED)
set_property(TARGET steplazyfile PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libsteplazyfile.dylib")

add_library(steputils SHARED IMPORTED)
set_property(TARGET steputils PROPERTY IMPORTED_LOCATION
    "${PROJECT_SOURCE_DIR}/external/stepcode/build/lib/libsteputils.dylib")

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stepcode/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stepcode/build/include/)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stepcode/src/test/)

#passing -Iinclude/ to compiler
#PUBLIC will be added to all dependent targets
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/)

#include_directories(${PROJECT_SOURCE_DIR}/external/stepcode)

option(compile_examples ON)
if (${compile_examples})
  add_executable(
    parse_and_print
      examples/example_print.cpp
  )
  target_link_libraries(parse_and_print ${PROJECT_NAME}
      stepcore exppp express stepdai stepeditor steplazyfile steputils)
  target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/)

  #add_executable(
  #  parse_and_div
  #    examples/example_div_individual.cpp
  #)
  #target_link_libraries(parse_and_div ${PROJECT_NAME})
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/)

  #add_executable(
  #  treg
  #    examples/treg.cpp
  #)
  #target_link_libraries(treg ${PROJECT_NAME})
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stepcode/include/)
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stepcode/build/include/)
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/stepcode/src/test/)
  target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/)
  #target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/external/)
endif()

