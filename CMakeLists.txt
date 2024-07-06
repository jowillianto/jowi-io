cmake_minimum_required(VERSION 3.28)
project (moderna_io)
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED true)
set (CMAKE_CXX_SCAN_FOR_MODULES true)


if (NOT moderna_variant_error)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/variant_error)
endif()

file(GLOB ${PROJECT_NAME}_src ${CMAKE_CURRENT_LIST_DIR}/src/*.ccm)
add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET CXX_MODULES FILES ${${PROJECT_NAME}_src}
)
target_link_libraries(${PROJECT_NAME} PUBLIC moderna_variant_error)

if (
  EXISTS ${CMAKE_CURRENT_LIST_DIR}/../test-lib
  AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/../file-lock
  AND PROJECT_IS_TOP_LEVEL
)
  include(CTest)
  if (NOT moderna_test_lib)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../test-lib test-lib)
  endif()
  if (NOT moderna_file_lock)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../file-lock file-lock)
  endif()
  moderna_add_test(
    ${PROJECT_NAME}_tests
    ${CMAKE_CURRENT_LIST_DIR}/tests/tests.cpp
    LIBRARIES ${PROJECT_NAME} moderna_test_lib moderna_file_lock
    COMPILE_DEFINITIONS 
      READ_FILE="${CMAKE_CURRENT_LIST_DIR}/assets/read.txt" 
      WRITE_FILE="${CMAKE_CURRENT_LIST_DIR}/assets/write.txt"
    SANITIZERS all
  )
else()
  message (${PROJECT_NAME} : "Clone Moderna Library to Test")
endif()