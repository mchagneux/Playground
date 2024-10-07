# Example Audio Plugin CMakeLists.txt

# To get started on a new plugin, copy this entire folder (containing this file and C++ sources) to
# a convenient location, and then start making modifications.

# The first line of any CMake project should be a call to `cmake_minimum_required`, which checks
# that the installed CMake will be able to understand the following CMakeLists, and ensures that
# CMake's behaviour is compatible with the named version. This is a standard CMake command, so more
# information can be found in the CMake docs.

cmake_minimum_required(VERSION 3.22)

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.


# option(NNITEMPLATE_WITH_INSTALL "Add install targets" ON)

project(Playground VERSION 0.0.1)


if(APPLE)
set(FORMATS_TO_BUILD AU VST3 Standalone)
else()
set(FORMATS_TO_BUILD VST3 Standalone)
endif()

set (TARGET_NAME ${PROJECT_NAME})

# Sets the minimum macOS version
if (APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum version of the target platform" FORCE) 
	if(CMAKE_OSX_DEPLOYMENT_TARGET)
		message("The minimum macOS version is set to " $CACHE{CMAKE_OSX_DEPLOYMENT_TARGET}.)
	endif()
endif ()

# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED True)

# if(NNITEMPLATE_WITH_INSTALL)
#     set(ANIRA_WITH_INSTALL ON)
# endif()

# ==================================================<============================
# Add the anira library
# ==============================================================================

# Either add the anira library as a git submodule and use add_subdirectory
# add_subdirectory(3rd_party/anira)


include(3rd_party/cmajor/tools/scripts/cmake_warning_flags)


# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

if(NOT CMAJ_VERSION)
    # Determine the version - use the latest tag for the checkout out repo
    set(CMAJ_VERSION "1.0")
    find_package (Git)

    if (GIT_FOUND)
        execute_process (
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/cmajor"
            RESULT_VARIABLE GIT_RESULT
            OUTPUT_VARIABLE GIT_STDOUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if (GIT_RESULT EQUAL 0)
            set(CMAJ_VERSION ${GIT_STDOUT})
        endif()
    endif()

    message ("Setting CMAJ_VERSION to ${CMAJ_VERSION}")
endif()


set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)

add_subdirectory(3rd_party/JUCE juce)                    # If you've put JUCE in a subdirectory called JUCE

Include (FetchContent)


# FetchContent_Declare (melatonin_perfetto
#   GIT_REPOSITORY https://github.com/sudara/melatonin_perfetto.git
#   GIT_TAG origin/main)

# FetchContent_MakeAvailable (melatonin_perfetto)



FetchContent_Declare (melatonin_inspector
  GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
  GIT_TAG origin/main
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)
FetchContent_MakeAvailable (melatonin_inspector)



add_subdirectory(assets)

# Add the path to the anira library as cmake prefix path and find the package
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/anira-0.1.2-lib)
find_package(anira REQUIRED)



juce_add_plugin(${TARGET_NAME}
    PRODUCT_NAME "Playground"
    BUNDLE_ID ""
    COMPANY_NAME "RM_ESTALI"
    COMPANY_COPYRIGHT ""
    PLUGIN_CODE Playground
    FORMATS ${FORMATS_TO_BUILD}
    IS_SYNTH TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
    NEEDS_MIDI_INPUT TRUE
    VST3_AUTO_MANIFEST FALSE
    NEEDS_MIDI_OUTPUT FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
)        # The name of the final executable, which can differ from the target name

juce_generate_juce_header(${TARGET_NAME})



set(GUITARLSTM_MODELS_PATH_TENSORFLOW "${CMAKE_CURRENT_LIST_DIR}/3rd_party/GuitarLSTM/tensorflow-version/models/")
set(GUITARLSTM_MODELS_PATH_PYTORCH "${CMAKE_CURRENT_LIST_DIR}/3rd_party/GuitarLSTM/pytorch-version/models/")

# target_compile_definitions(${TARGET_NAME}
#     PUBLIC
#     GUITARLSTM_MODELS_PATH_TENSORFLOW="${GUITARLSTM_MODELS_PATH_TENSORFLOW}"
#     GUITARLSTM_MODELS_PATH_PYTORCH="${GUITARLSTM_MODELS_PATH_PYTORCH}"
# )

# # Set the model path as compile definition for the build interface
# set(GUITARLSTM_MODELS_PATH_TENSORFLOW "${CMAKE_CURRENT_LIST_DIR}/3rd_party/GuitarLSTM/tensorflow-version/models/")
# set(GUITARLSTM_MODELS_PATH_PYTORCH "${CMAKE_CURRENT_LIST_DIR}/3rd_party/GuitarLSTM/pytorch-version/models/")


# add_compile_definitions(
#         GUITARLSTM_MODELS_PATH_TENSORFLOW="${GUITARLSTM_MODELS_PATH_TENSORFLOW}"
#         GUITARLSTM_MODELS_PATH_PYTORCH="${GUITARLSTM_MODELS_PATH_PYTORCH}"
#         STEERABLENAFX_MODELS_PATH_TENSORFLOW="${STEERABLENAFX_MODELS_PATH_TENSORFLOW}"
#         STEERABLENAFX_MODELS_PATH_PYTORCH="${STEERABLENAFX_MODELS_PATH_PYTORCH}"
#         STATEFULLSTM_MODELS_PATH_TENSORFLOW="${STATEFULLSTM_MODELS_PATH_TENSORFLOW}"
#         STATEFULLSTM_MODELS_PATH_PYTORCH="${STATEFULLSTM_MODELS_PATH_PYTORCH}"
# )
# add_compile_definitions(MODEL_TO_USE=2) # GuitarLSTM

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
target_compile_options(${TARGET_NAME} PRIVATE ${CMAJ_WARNING_FLAGS})

target_compile_definitions(${TARGET_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        GUITARLSTM_MODELS_PATH_TENSORFLOW="${GUITARLSTM_MODELS_PATH_TENSORFLOW}"
        GUITARLSTM_MODELS_PATH_PYTORCH="${GUITARLSTM_MODELS_PATH_PYTORCH}"
        STEERABLENAFX_MODELS_PATH_TENSORFLOW="${STEERABLENAFX_MODELS_PATH_TENSORFLOW}"
        STEERABLENAFX_MODELS_PATH_PYTORCH="${STEERABLENAFX_MODELS_PATH_PYTORCH}"
        STATEFULLSTM_MODELS_PATH_TENSORFLOW="${STATEFULLSTM_MODELS_PATH_TENSORFLOW}"
        STATEFULLSTM_MODELS_PATH_PYTORCH="${STATEFULLSTM_MODELS_PATH_PYTORCH}"
        MODEL_TO_USE=2
        # add_compile_definitions(MODEL_TO_USE=1) # steerable-nafx
# GuitarLSTM
# add_compile_definitions(MODEL_TO_USE=3) # stateful-LSTM
            # Backend-specific definitions
        $<$<BOOL:${BACKEND_LIBTORCH}>:USE_LIBTORCH>
        $<$<BOOL:${BACKEND_ONNXRUNTIME}>:USE_ONNXRUNTIME>
        $<$<BOOL:${BACKEND_TFLITE}>:USE_TFLITE>
    )
    

add_compile_definitions (
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:CMAJ_ENABLE_ALLOCATION_CHECKER=1>
    CMAJ_ENABLE_WEBVIEW_DEV_TOOLS=1
)


# Add all source files to file list
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/source/*.h)


target_sources(${TARGET_NAME} PRIVATE ${SOURCES})# ${RNBO_SOURCES})

# Add include directories for all folders in the source
file(GLOB_RECURSE SOURCE_DIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/source/*)
list(APPEND SOURCE_DIRS ${CMAKE_CURRENT_LIST_DIR}/source)

foreach (DIR ${SOURCE_DIRS})
    if (IS_DIRECTORY ${DIR})
        target_include_directories(${TARGET_NAME} PRIVATE ${DIR})
    endif ()
endforeach ()

# Make the folder structure visible in IDEs like Xcode
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "source" FILES ${SOURCES})

# target_include_directories(${TARGET_NAME} PRIVATE 3rd_party/cmajor/include/cmajor)
# target_include_directories(${TARGET_NAME} PRIVATE 3rd_party/cmajor/include/choc)



if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules (GTK3 REQUIRED gtk+-3.0 IMPORTED_TARGET)
    pkg_check_modules (WEBKIT2 REQUIRED webkit2gtk-4.0 IMPORTED_TARGET)
    target_include_directories(${TARGET_NAME} PUBLIC ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
    target_link_libraries (${TARGET_NAME} PUBLIC ${GTK3_LIBRARIES} ${WEBKIT2_LIBRARIES})
    # add_compile_definitions (JUCE_USE_CUSTOM_PLUGIN_STANDALONE_ENTRYPOINT=1)
endif ()


# Make the folder structure visible in the IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "source" FILES ${SOURCES})

# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.

add_subdirectory(3rd_party/cmajor/modules cmajor)

MAKE_CMAJ_LIBRARY (
    LIBRARY_NAME cmajor_lib
    INCLUDE_PLAYBACK
    ENABLE_PERFORMER_LLVM
)


# Add the path to the anira library as cmake prefix path and find the package
# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/3rd_party/anira_lib")
# find_package(anira REQUIRED)



target_link_libraries(${TARGET_NAME}
    PRIVATE
        # BinaryData
        juce::juce_dsp
        anira::anira
        cmajor_lib
        juce::juce_audio_utils
        # Melatonin::Perfetto
        melatonin_inspector

)

file(GLOB_RECURSE INFERENCE_ENGINE_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/anira-0.1.2-lib/lib/*.dll")
list(APPEND NECESSARY_DLLS ${INFERENCE_ENGINE_DLLS})

foreach(FORMAT IN LISTS FORMATS_TO_BUILD)
    string(CONCAT CONCATENATED_TARGET ${TARGET_NAME}_${FORMAT})
    add_custom_command(TARGET ${CONCATENATED_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${NECESSARY_DLLS}
            $<TARGET_FILE_DIR:${TARGET_NAME}>/${FORMAT})
endforeach()
# endif (MSVC)


# foreach(FORMAT IN LISTS FORMATS_TO_BUILD)
#     string(CONCAT PLUGIN_TARGET ${TARGET_NAME}_${FORMAT})
#     message(STATUS "${PLUGIN_TARGET}")
#     message(STATUS "${ANIRA_SHARED_LIBS_WIN}")

#     add_custom_command(TARGET ${PLUGIN_TARGET}
#             POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             ${ANIRA_SHARED_LIBS_WIN}
#             $<TARGET_FILE_DIR:${TARGET_NAME}>/${FORMAT})
# endforeach()
