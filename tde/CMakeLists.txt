cmake_minimum_required(VERSION 3.16)
project(tde)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/osdrv/linux/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vglite/inc/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vglite/util/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/. SRC)

set(TDE_LIB_NAME tde CACHE INTERNAL "tde library name")

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vglite/lib/)
target_link_libraries(${VGLite_LIB_NAME})

add_library(${TDE_LIB_NAME} SHARED ${SRC})
add_library(${TDE_LIB_NAME}-static STATIC ${SRC})
# sync static lib name with dynamic lib name
set_target_properties(${TDE_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${TDE_LIB_NAME})

# set(LIBRARY_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(TDE_path ${CMAKE_CURRENT_SOURCE_DIR})
set(component libsophon)
set(sample_name sample_tde)
set(HEADERS inc/tde.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/include)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/sample/. SRC)

# link_directories(${TDE_path}/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/lib/${PLATFORM})

add_executable(${sample_name} ${SRC})
target_link_libraries(${sample_name} ${VGLite_LIB_NAME}-static ${TDE_LIB_NAME}-static bmlib dl m pthread atomic)

# INSTALL(TARGETS ${sample_name} RUNTIME DESTINATION ${TDE_path}/bin)

# INSTALL(TARGETS ${TDE_LIB_NAME} ${TDE_LIB_NAME}-static RUNTIME DESTINATION ${TDE_path}/lib/)

install(TARGETS ${sample_name}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${TDE_LIB_NAME} 
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${TDE_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)
    
install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)