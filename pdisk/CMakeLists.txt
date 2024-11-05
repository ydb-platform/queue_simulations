cmake_minimum_required(VERSION 3.10)

project(pdisk)

# a hack
set(DATA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/common/data")
set(DATA_DEST_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}")
file(COPY ${DATA_SOURCE_DIR} DESTINATION ${DATA_DEST_DIR})

add_executable(${PROJECT_NAME} pdisk.cpp)
target_link_libraries(${PROJECT_NAME} common)
