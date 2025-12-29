cmake_minimum_required(VERSION 3.13)
project(samples)

if(CMAKE_HOST_WIN32)
    set(WINDOWS 1)
elseif(CMAKE_HOST_APPLE)
    set(MACOS 1)
elseif(CMAKE_HOST_UNIX)
    set(LINUX 1)
endif()
cmake_policy(SET CMP0046 NEW)
########################
#  Target
set(PLATFORM_LIST pcie soc pice_arm64)
set(PLATFORM pcie CACHE STRING "platform: pcie soc pice_arm64")
set_property(CACHE PLATFORM PROPERTY STRINGS ${PLATFORM_LIST})

set(SAMPLE_TARGETS media_version)
set(component libsophon)
# add sub-modules
add_subdirectory(media_version)

# exclude from all, build alone after bmapi and bmcv
add_custom_target(samples DEPENDS ${SAMPLE_TARGETS})

install(TARGETS ${SAMPLE_TARGETS}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})