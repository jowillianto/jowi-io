cmake_minimum_required(VERSION 3.28)
project (moderna_io)
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED true)
set (CMAKE_CXX_SCAN_FOR_MODULES true)

option (MODERNA_IO_BUILD_TESTS "Build tests" OFF)

if (NOT moderna::generic)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/moderna-generic)
endif()

file(
  GLOB ${PROJECT_NAME}_src 
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/src/reader_writer/*.cc
)
if (WIN32)
  file (GLOB ${PROJECT_NAME}_src_sys_call ${CMAKE_CURRENT_LIST_DIR}/src/win32/*.cc)
elseif(UNIX)
  file (GLOB ${PROJECT_NAME}_src_sys_call ${CMAKE_CURRENT_LIST_DIR}/src/unix/*.cc)
endif()

add_library(${PROJECT_NAME})
add_library(moderna::io ALIAS ${PROJECT_NAME})
target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET CXX_MODULES FILES ${${PROJECT_NAME}_src} ${${PROJECT_NAME}_src_sys_call}
)
target_link_libraries(${PROJECT_NAME} 
  PRIVATE 
    moderna::generic
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

if (MODERNA_IO_BUILD_TESTS)
  include (CTest)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/test-lib)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif()

if (MODERNA_INSTALL)
  include(GNUInstallDirs)
  set (MODERNA_COMPONENT_NAME "io")
  set_property(
    TARGET ${PROJECT_NAME} 
    PROPERTY EXPORT_NAME ${MODERNA_COMPONENT_NAME}
  )
  install (
    TARGETS ${PROJECT_NAME}
    EXPORT moderna
    FILE_SET 
      CXX_MODULES 
      DESTINATION ${CMAKE_INSTALL_PREFIX}/cxx_modules/moderna/${MODERNA_COMPONENT_NAME}
      COMPONENT ${MODERNA_COMPONENT_NAME}_file_set
    CXX_MODULES_BMI 
      COMPONENT ${MODERNA_COMPONENT_NAME}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/cxx_modules/moderna/${MODERNA_COMPONENT_NAME}
  )
  if (NOT MODERNA_GLOBAL_INSTALL)
    install (
      EXPORT moderna
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/moderna
      FILE moderna-config.cmake
      NAMESPACE moderna::
      CXX_MODULES_DIRECTORY cxx_modules/moderna
    )
  endif()
endif()