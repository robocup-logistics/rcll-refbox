include_directories(qa)
add_subdirectory(qa)

add_library(refbox-core SHARED
    utils/refcount.cpp
    threading/wait_condition.cpp
    threading/thread_list.cpp
    threading/recursive_mutex.cpp
    threading/spinlock.cpp
    threading/mutex_locker.cpp
    threading/barrier.cpp
    threading/thread_finalizer.cpp
    threading/thread_notification_listener.cpp
    threading/interruptible_barrier.cpp
    threading/thread.cpp
    threading/scoped_rwlock.cpp
    threading/thread_collector.cpp
    threading/refc_rwlock.cpp
    threading/thread_initializer.cpp
    threading/read_write_lock.cpp
    threading/thread_initializer.h
    threading/mutex.cpp
    exception.cpp
    exceptions/software.cpp
    exceptions/system.cpp
    plugin.cpp)

target_link_libraries(refbox-core pthread stdc++ m mps_comm)

if(NOT APPLE)
  target_link_libraries(refbox-core rt)
endif()


if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  find_path(EXECINFO_INCLUDE_DIR execinfo.h)
  if(EXECINFO_INCLUDE_DIR)
    target_link_libraries(refbox-core execinfo)
    target_compile_definitions(refbox-core PRIVATE HAVE_EXECINFO)
  else()
    message(WARNING "LLSF RefBox Core lib built without backtrace support (FreeBSD: devel/libexecinfo not installed)")
  endif()
else()
  target_compile_definitions(refbox-core PRIVATE HAVE_EXECINFO)
endif()

