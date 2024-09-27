cmake_minimum_required(VERSION 3.10)

project(Sockets)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
)

add_library(socket socket.cpp addr_list.cpp)

add_library(encrypt encrypt_extra.cpp)

add_executable(serverM serverM.cpp)
target_link_libraries(serverM socket encrypt)

add_executable(client client.cpp)
target_link_libraries(client socket encrypt)

add_library(backend backend.cpp)
target_link_libraries(backend socket)

add_executable(serverS serverS.cpp)
target_link_libraries(serverS backend)

add_executable(serverD serverD.cpp)
target_link_libraries(serverD backend)

add_executable(serverU serverU.cpp)
target_link_libraries(serverU backend)

add_executable(encrypt_tester encrypt_tester.cpp)
target_link_libraries(encrypt_tester encrypt)

add_executable(encrypt_userinfo encrypt_userinfo.cpp)
target_link_libraries(encrypt_userinfo encrypt)
