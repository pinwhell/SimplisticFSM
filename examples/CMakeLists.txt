add_executable(semaphore semaphore.cpp)
target_link_libraries(semaphore simplistic::fsm)

add_executable(lexer lexer.cpp)
target_link_libraries(lexer simplistic::fsm)