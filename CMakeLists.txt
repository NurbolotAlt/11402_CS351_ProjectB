cmake_minimum_required(VERSION 3.10)
project(csvdb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(csvdb
    src/main.cpp
    src/csv_parser.cpp
    src/table.cpp
    src/query.cpp
)
