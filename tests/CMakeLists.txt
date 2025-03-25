moderna_add_test(
  test_file ${CMAKE_CURRENT_LIST_DIR}/file.cc
  LIBRARIES moderna_io
  COMPILE_DEFINITIONS 
    READ_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/read.txt" 
    WRITE_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/write.txt"
    READ_CSV_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/read.csv"
    WRITE_CSV_FILE="${CMAKE_CURRENT_LIST_DIR}/../assets/write.csv"
  SANITIZERS all
)

moderna_add_test(
  test_socket ${CMAKE_CURRENT_LIST_DIR}/socket.cc
  LIBRARIES moderna_io
  SANITIZERS all
)

moderna_add_test(
  test_pipe ${CMAKE_CURRENT_LIST_DIR}/pipe.cc
  LIBRARIES moderna_io
  SANITIZERS all
)