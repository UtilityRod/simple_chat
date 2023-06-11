set(
    SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/chat_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/client.c
)

target_sources(chat_main PUBLIC ${SOURCES}) 