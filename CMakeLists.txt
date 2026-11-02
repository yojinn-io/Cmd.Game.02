cmake_minimum_required(VERSION 3.15)
project(CmdGame02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CmdGame02 main.cpp)

# ソースと実行時の文字コードを UTF-8 に強制する
if(MSVC)
    target_compile_options(CmdGame02 PRIVATE /utf-8)
else()
    target_compile_options(CmdGame02 PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
