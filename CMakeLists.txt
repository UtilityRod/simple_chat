project(simple_chat)
cmake_minimum_required(VERSION 3.13.0)

add_executable(chat_main)
target_include_directories(chat_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_link_libraries(chat_main eloop_static networking_shared)
add_subdirectory(deps)
add_subdirectory(src)
