cmake_minimum_required(VERSION 3.10)

project(QueueSimulations)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

function(update_git_submodules)
    message(STATUS "Checking if Git submodules need to be updated...")
    execute_process(
        COMMAND git submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR "Failed to initialize and update Git submodules")
    else()
        message(STATUS "Git submodules are up to date.")
    endif()
endfunction()

update_git_submodules()

add_subdirectory(common)
add_subdirectory(pdisk)

target_include_directories(common PUBLIC
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/arctic
)
