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

project(Playground VERSION 0.0.1)

set (TARGET_NAME ${PROJECT_NAME})

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



# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or

set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)

add_subdirectory(3rd_party/JUCE juce)                    # If you've put JUCE in a subdirectory called JUCE

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(...)

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (${TARGET_NAME} here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.


# Include (FetchContent)
# FetchContent_Declare (melatonin_inspector
#   GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
#   GIT_TAG origin/main
#   SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)
# FetchContent_MakeAvailable (melatonin_inspector)

juce_add_plugin(${TARGET_NAME}
    PRODUCT_NAME "Playground"
    BUNDLE_ID ""
    COMPANY_NAME "RM_Estali"
    COMPANY_COPYRIGHT ""
    PLUGIN_CODE Cmaj
    FORMATS Standalone VST3
    IS_SYNTH FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE        # Should the plugin be installed to a default location after building?
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
)        # The name of the final executable, which can differ from the target name

# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

juce_generate_juce_header(${TARGET_NAME})

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
target_compile_options(${TARGET_NAME} PRIVATE ${CMAJ_WARNING_FLAGS})

target_compile_definitions(${TARGET_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0)


add_compile_definitions (
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:CMAJ_ENABLE_ALLOCATION_CHECKER=1>
    CMAJ_ENABLE_WEBVIEW_DEV_TOOLS=1
)

# Add all source files to file list
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/source/*.h)


target_sources(${TARGET_NAME} PRIVATE ${SOURCES})# ${RNBO_SOURCES})


# Add include directories for all folders in the source
file(GLOB_RECURSE source_dirs LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/source/*)

foreach (dir ${source_dirs})
    if (IS_DIRECTORY ${dir})
        target_include_directories(${TARGET_NAME} PRIVATE ${dir})
    endif ()
endforeach ()

# target_include_directories(${TARGET_NAME} PRIVATE 3rd_party/cmajor/include/cmajor)
# target_include_directories(${TARGET_NAME} PRIVATE 3rd_party/cmajor/include/choc)



if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules (GTK3 REQUIRED gtk+-3.0 IMPORTED_TARGET)
    pkg_check_modules (WEBKIT2 REQUIRED webkit2gtk-4.0 IMPORTED_TARGET)
    target_include_directories(${TARGET_NAME} PUBLIC ${GTK3_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
    target_link_libraries (${TARGET_NAME} PUBLIC ${GTK3_LIBRARIES} ${WEBKIT2_LIBRARIES})
    # add_compile_definitions (JUCE_USE_CUSTOM_PLUGIN_STANDALONE_ENTsRYPOINT=1)

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


target_link_libraries(${TARGET_NAME}
    PRIVATE
        cmajor_lib
        juce::juce_audio_utils
)

