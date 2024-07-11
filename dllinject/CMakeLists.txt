add_executable(dllinject inject.cpp consumer.cpp)

set(SOURCE_FILES dllheap.cpp producer.cpp)
add_library(dllheap SHARED ${SOURCE_FILES})
target_link_libraries(dllheap PRIVATE lib_detours)
