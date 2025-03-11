cmake_minimum_required(VERSION 3.28)
project (moderna_io)
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED true)
set (CMAKE_CXX_SCAN_FOR_MODULES true)

option (MODERNA_IO_BUILD_TESTS "Build tests" OFF)

if (NOT TARGET ${PROJECT_NAME})
  file(
    GLOB ${PROJECT_NAME}_src 
      ${CMAKE_CURRENT_LIST_DIR}/src/*.cc
      ${CMAKE_CURRENT_LIST_DIR}/src/file_descriptor/*.cc
    )
  if (WIN32)
    file (GLOB ${PROJECT_NAME}_src_sys_call ${CMAKE_CURRENT_LIST_DIR}/src/win32/*.cc)
  elseif(UNIX)
    file (GLOB ${PROJECT_NAME}_src_sys_call ${CMAKE_CURRENT_LIST_DIR}/src/unix/*.cc)
  endif()
  add_library(${PROJECT_NAME})
  target_sources(${PROJECT_NAME}
    PUBLIC
      FILE_SET CXX_MODULES FILES ${${PROJECT_NAME}_src} ${${PROJECT_NAME}_src_sys_call}
  )
endif()

if (MODERNA_IO_BUILD_TESTS)
  include (CTest)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/test-lib)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif()