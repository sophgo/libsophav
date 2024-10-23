function(get_version_from_tag ver_var commit_hash_var commit_count_var branch_var commit_date_var)
    find_package(Git)
    set(ver "9.9.9")
    set(commit_hash "NA")
    set(commit_count "0")
    set(branch "xx")
    set(commit_date "NA")
    if (NOT Git_FOUND)
        message(WARNING "Git not found.")
    else()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD OUTPUT_VARIABLE commit_hash
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND stat -c "%u" ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE repo_owner
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND id -u
            OUTPUT_VARIABLE uid
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (NOT ${repo_owner} STREQUAL ${uid})
            set(cmd
                ${GIT_EXECUTABLE} config --global --add safe.directory
                ${CMAKE_CURRENT_SOURCE_DIR})
            string(REPLACE ";" " " cmd_str "${cmd}")
            message(WARNING "${cmd_str}")
            execute_process(COMMAND ${cmd})
        endif()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 OUTPUT_VARIABLE ver
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (NOT ver STREQUAL "")
            string(REGEX REPLACE "^v([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" ver "${ver}")
        endif()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count OUTPUT_VARIABLE commit_count
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE branch
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} show -s --format=%cd --date=format:"%Y-%m-%d %H:%M:%S" HEAD
            OUTPUT_VARIABLE commit_date
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    set(${ver_var} ${ver} PARENT_SCOPE)
    set(${commit_hash_var} ${commit_hash} PARENT_SCOPE)
    set(${commit_count_var} ${commit_count} PARENT_SCOPE)
    set(${branch_var} ${branch} PARENT_SCOPE)
    set(${commit_date_var} ${commit_date} PARENT_SCOPE)
    message(STATUS "Commit date: ${commit_date}")
endfunction()
