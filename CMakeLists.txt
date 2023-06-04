project(simple_chat)
cmake_minimum_required(VERSION 3.13.0)

add_executable(chat_main)
target_include_directories(chat_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
add_subdirectory(deps)
add_subdirectory(src)