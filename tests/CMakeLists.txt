moderna_add_test(
  ${PROJECT_NAME}_test_file
  TARGETS 
    ${CMAKE_CURRENT_LIST_DIR}/file.cc
  LIBRARIES 
    moderna::io 
    moderna::generic
  COMPILE_DEFINITIONS 
    READ_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/read.txt" 
    WRITE_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/write.txt"
    READ_CSV_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/read.csv"
    WRITE_CSV_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/write.csv"
  SANITIZERS all
)

moderna_add_test(
  ${PROJECT_NAME}_test_sock
  TARGETS ${CMAKE_CURRENT_LIST_DIR}/sock.cc
  LIBRARIES
    moderna::io 
    moderna::generic
  SANITIZERS all
)

moderna_add_test(
  ${PROJECT_NAME}_test_pipe 
  TARGETS 
    ${CMAKE_CURRENT_LIST_DIR}/pipe.cc
  LIBRARIES
    moderna::io 
    moderna::generic
  SANITIZERS all
)