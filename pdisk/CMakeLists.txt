cmake_minimum_required(VERSION 3.10)

project(pdisk)

# a hack
set(DATA_SOURCE_DIR "${CMAKE_SOURCE_DIR}/common/data")
set(DATA_DEST_DIR "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/data")

# Copy only the necessary data files, excluding .md and .js
file(GLOB DATA_FILES "${DATA_SOURCE_DIR}/*")
foreach(FILE ${DATA_FILES})
    get_filename_component(EXT ${FILE} EXT)
    if(NOT ${EXT} MATCHES "\\.md$" AND NOT ${EXT} MATCHES "\\.js$")
        file(COPY ${FILE} DESTINATION ${DATA_DEST_DIR})
    endif()
endforeach()

add_executable(${PROJECT_NAME} pdisk.cpp)
target_link_libraries(${PROJECT_NAME} common)
