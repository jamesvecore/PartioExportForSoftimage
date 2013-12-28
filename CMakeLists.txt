cmake_minimum_required (VERSION 2.8)

project (PartioExportForSoftimage)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMake/Modules)

find_package (Softimage REQUIRED)

message (STATUS "---configuring (PartioExportForSoftimage) plugin---")

set (PARTIO_INCLUDE_DIR
	""
	CACHE
	PATH
	"Partio include directory"
)

set (PARTIO_LIB_DIR 
	""
	CACHE
	PATH
	"Partio library directory"
)

find_package(ZLIB)

set (INCLUDE_DIRS
	${PARTIO_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
)

set (LINK_DIRS
	${PARTIO_LIB_DIR}
)

include_directories (${INCLUDE_DIRS})

link_directories (${LINK_DIRS})

set (SOURCES
	PartioExportForSoftimage.cpp
)

set (HEADERS
)

set (LINK_LIBS
	partio
	${ZLIB_LIBRARY}
)

add_softimage_plugin (PartioExportForSoftimage ${SOURCES} ${HEADERS})

target_link_libraries (PartioExportForSoftimage ${LINK_LIBS})