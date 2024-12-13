cmake_minimum_required(VERSION 3.15)
project(step)
set(CMAKE_CXX_STANDARD 17)

add_library(
  ${PROJECT_NAME} # library name
    ${PROJECT_SOURCE_DIR}/src/step.cpp #sources...
    ${PROJECT_SOURCE_DIR}/include/step.h #just for vscode intellisence
)
#passing -Iinclude/ to compiler
#PUBLIC will be added to all dependent targets
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include/) 
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/) 

add_library(
  Timer
     ${PROJECT_SOURCE_DIR}/src/Timer.cpp
     ${PROJECT_SOURCE_DIR}/include/Timer.h 
)
target_include_directories(Timer PUBLIC ${PROJECT_SOURCE_DIR}/include/)

add_library(
  cmesh4
     ${PROJECT_SOURCE_DIR}/src/cmesh4.cpp
     ${PROJECT_SOURCE_DIR}/include/cmesh4.h
)
target_include_directories(cmesh4 PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_include_directories(cmesh4 PUBLIC ${PROJECT_SOURCE_DIR}/external/)


add_library(
    stl INTERFACE
    ${PROJECT_SOURCE_DIR}/include/stl.h
)
target_include_directories(stl INTERFACE ${PROJECT_SOURCE_DIR}/include/)

option(COMPILE_EXAMPLES "Compile examples" OFF)
if (${COMPILE_EXAMPLES})
  add_executable(
    nurbss_stdout
      examples/nurbss_stdout.cpp
  )
  target_link_libraries(nurbss_stdout PRIVATE ${PROJECT_NAME} Timer)
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/)

  add_executable(
    nurbss_tofile
      examples/nurbss_tofile.cpp
  )
  target_link_libraries(nurbss_tofile ${PROJECT_NAME} Timer)
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/)

  add_executable(
    argsplit
      examples/argsplit.cpp
  )
  target_link_libraries(argsplit ${PROJECT_NAME} Timer)
  #target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/external/)

  add_executable(
    stlmesh
      examples/stl.cpp
  )
  target_link_libraries(stlmesh stl Timer)

  add_executable(
    cmesh
      examples/cmesh.cpp
  )
  target_link_libraries(cmesh cmesh4 Timer)

endif()

