
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qa)
add_executable(qa_core_mutex_count qa_mutex_count.cpp)
target_link_libraries(qa_core_mutex_count stdc++ refbox-core)

# qa_core_mutex_sync
add_executable(qa_core_mutex_sync qa_mutex_sync.cpp)
target_link_libraries(qa_core_mutex_sync stdc++ refbox-core)

# qa_core_waitcond
add_executable(qa_core_waitcond qa_waitcond.cpp)
target_link_libraries(qa_core_waitcond stdc++ refbox-core)

# qa_core_waitcond_serialize
add_executable(qa_core_waitcond_serialize qa_waitcond_serialize.cpp)
target_link_libraries(qa_core_waitcond_serialize stdc++ refbox-core)

# qa_core_rwlock
add_executable(qa_core_rwlock qa_rwlock.cpp)
target_link_libraries(qa_core_rwlock stdc++ refbox-core)

# qa_core_barrier
add_executable(qa_core_barrier qa_barrier.cpp)
target_link_libraries(qa_core_barrier stdc++ refbox-core)

# qa_core_exception
add_executable(qa_core_exception qa_exception.cpp)
target_link_libraries(qa_core_exception stdc++ refbox-core)
