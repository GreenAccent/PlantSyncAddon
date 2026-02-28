cmake_minimum_required (VERSION 3.16)

# ---------------------------------------------------------------------------
# Compiler helpers
# ---------------------------------------------------------------------------

function (SetCompilerOptions target)
	target_compile_features (${target} PUBLIC cxx_std_17)
	target_compile_options (${target} PUBLIC "$<$<CONFIG:Debug>:-DDEBUG>")
	if (WIN32)
		target_compile_options (${target} PUBLIC /W3 /WX /wd4996 /Zc:wchar_t-)
	endif ()
endfunction ()

function (AddGSModuleIncludeFolder target folder)
	target_include_directories (${target} PUBLIC "${folder}")
endfunction ()

function (AddGSModuleLinkLibrary target library)
	if (WIN32)
		file (GLOB libs "${library}/Win/*.lib")
		foreach (lib ${libs})
			target_link_libraries (${target} ${lib})
		endforeach ()
	endif ()
endfunction ()

# ---------------------------------------------------------------------------
# Project settings
# ---------------------------------------------------------------------------

set_property (GLOBAL PROPERTY USE_FOLDERS ON)
set (CMAKE_SUPPRESS_REGENERATION 1)
set (CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)

if (WIN32)
	set (CMAKE_GENERATOR_TOOLSET v142)
	set (CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
endif ()

set (AC_API_DEVKIT_DIR "C:/Program Files/GRAPHISOFT/API Development Kit 28.4001" CACHE PATH "API DevKit directory.")
set (AC_ADDON_NAME "ClassSync" CACHE STRING "Add-On name.")
set (AC_ADDON_LANGUAGE "INT" CACHE STRING "Add-On language code.")

# ---------------------------------------------------------------------------
# Detect ArchiCAD version from DevKit headers
# ---------------------------------------------------------------------------

set (ACAPINC_FILE_LOCATION ${AC_API_DEVKIT_DIR}/Support/Inc/ACAPinc.h)
if (EXISTS ${ACAPINC_FILE_LOCATION})
	file (READ ${ACAPINC_FILE_LOCATION} ACAPIncContent)
	string (REGEX MATCHALL "#define[ \t]+ServerMainVers_([0-9][0-9])" VersionList ${ACAPIncContent})
	set (ARCHICAD_VERSION ${CMAKE_MATCH_1})
	message (STATUS "Detected Archicad version: ${ARCHICAD_VERSION}")
else ()
	message (FATAL_ERROR "Cannot find ACAPinc.h - check AC_API_DEVKIT_DIR: ${AC_API_DEVKIT_DIR}")
endif ()

# ---------------------------------------------------------------------------
# Platform definitions
# ---------------------------------------------------------------------------

if (WIN32)
	add_definitions (-DUNICODE -D_UNICODE -D_ITERATOR_DEBUG_LEVEL=0 -DGS_WIN)
endif ()
add_definitions (-DACExtension)

project (${AC_ADDON_NAME})

# ---------------------------------------------------------------------------
# Resource compilation
# ---------------------------------------------------------------------------

set (AddOnSourcesFolder ${CMAKE_CURRENT_LIST_DIR}/Src)
set (AddOnResourcesFolder ${CMAKE_CURRENT_LIST_DIR})
set (ResourceObjectsDir ${CMAKE_BINARY_DIR}/ResourceObjects)

file (GLOB AddOnImageFiles
	${AddOnResourcesFolder}/RFIX/Images/*.svg
)

if (WIN32)
	file (GLOB AddOnResourceFiles
		${AddOnResourcesFolder}/R${AC_ADDON_LANGUAGE}/*.grc
		${AddOnResourcesFolder}/RFIX/*.grc
		${AddOnResourcesFolder}/RFIX.win/*.rc2
	)
endif ()

source_group ("Images" FILES ${AddOnImageFiles})
source_group ("Resources" FILES ${AddOnResourceFiles})

get_filename_component (AddOnSourcesFolderAbsolute "${AddOnSourcesFolder}" ABSOLUTE)
get_filename_component (AddOnResourcesFolderAbsolute "${AddOnResourcesFolder}" ABSOLUTE)
get_filename_component (APIDevKitToolsFolderAbsolute "${AC_API_DEVKIT_DIR}/Support/Tools" ABSOLUTE)

add_custom_target (
	AddOnResources ALL
	DEPENDS "${ResourceObjectsDir}/AddOnResources.stamp"
	SOURCES ${AddOnResourceFiles} ${AddOnImageFiles}
)

if (WIN32)
	add_custom_command (
		OUTPUT "${ResourceObjectsDir}/AddOnResources.stamp"
		DEPENDS ${AddOnResourceFiles} ${AddOnImageFiles}
		COMMENT "Compiling resources..."
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ResourceObjectsDir}"
		COMMAND python "${APIDevKitToolsFolderAbsolute}/CompileResources.py" "${AC_ADDON_LANGUAGE}" "${AC_API_DEVKIT_DIR}" "${AddOnSourcesFolderAbsolute}" "${AddOnResourcesFolderAbsolute}" "${ResourceObjectsDir}" "${ResourceObjectsDir}/${AC_ADDON_NAME}.res"
		COMMAND ${CMAKE_COMMAND} -E touch "${ResourceObjectsDir}/AddOnResources.stamp"
	)
endif ()

# ---------------------------------------------------------------------------
# Add-On shared library
# ---------------------------------------------------------------------------

file (GLOB AddOnHeaderFiles ${AddOnSourcesFolder}/*.h ${AddOnSourcesFolder}/*.hpp)
file (GLOB AddOnSourceFiles ${AddOnSourcesFolder}/*.c ${AddOnSourcesFolder}/*.cpp)
file (GLOB AllCFiles ${AddOnSourcesFolder}/*.c)
set_source_files_properties (${AllCFiles} PROPERTIES LANGUAGE CXX)

set (AddOnFiles ${AddOnHeaderFiles} ${AddOnSourceFiles})
source_group ("Sources" FILES ${AddOnFiles})

if (WIN32)
	add_library (AddOn SHARED ${AddOnFiles})
else ()
	add_library (AddOn MODULE ${AddOnFiles})
endif ()

set_target_properties (AddOn PROPERTIES OUTPUT_NAME ${AC_ADDON_NAME})

if (WIN32)
	set_target_properties (AddOn PROPERTIES SUFFIX ".apx")
	target_link_options (AddOn PUBLIC "${ResourceObjectsDir}/${AC_ADDON_NAME}.res")
	target_link_options (AddOn PUBLIC /export:GetExportedFuncAddrs,@1 /export:SetImportedFuncAddrs,@2)
endif ()

target_include_directories (AddOn PUBLIC
	${AddOnSourcesFolder}
	${AC_API_DEVKIT_DIR}/Support/Inc
)

if (WIN32)
	target_link_libraries (AddOn "${AC_API_DEVKIT_DIR}/Support/Lib/ACAP_STAT.lib")
endif ()

SetCompilerOptions (AddOn)
add_dependencies (AddOn AddOnResources)

# ---------------------------------------------------------------------------
# Link all DevKit modules
# ---------------------------------------------------------------------------

get_filename_component (APIDevKitModulesDir "${AC_API_DEVKIT_DIR}/Support/Modules" ABSOLUTE)

file (GLOB GSIncludeFolders ${APIDevKitModulesDir}/*)
foreach (includeFolder ${GSIncludeFolders})
	AddGSModuleIncludeFolder (AddOn ${includeFolder})
endforeach ()

file (GLOB GSLinkLibFolders ${APIDevKitModulesDir}/*)
foreach (linkLibFolder ${GSLinkLibFolders})
	AddGSModuleLinkLibrary (AddOn ${linkLibFolder})
endforeach ()
