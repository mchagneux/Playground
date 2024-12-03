cmake_minimum_required(VERSION 3.22)

project(Playground VERSION 0.0.1)


set (TARGET_NAME ${PROJECT_NAME})
Include (FetchContent)


add_subdirectory(3rd_party/JUCE juce)                    
# add_subdirectory(3rd_party/anira)
add_subdirectory(assets)

set(FORMATS_TO_BUILD Standalone VST3)

juce_add_plugin(${TARGET_NAME}
    PRODUCT_NAME "Playground"
    BUNDLE_ID ""
    COMPANY_NAME "RM_ESTALI"
    COMPANY_COPYRIGHT ""
    PLUGIN_CODE PLGD
    FORMATS ${FORMATS_TO_BUILD}
    IS_SYNTH TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE       
    NEEDS_MIDI_INPUT TRUE
    VST3_AUTO_MANIFEST FALSE
    NEEDS_MIDI_OUTPUT FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
)       

juce_generate_juce_header(${TARGET_NAME})
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/anira-1.0.0)
find_package(anira REQUIRED)
target_include_directories(${TARGET_NAME} PUBLIC ${anira_INCLUDE_DIRS})


# add_library(cmaj IMPORTED)

# foreach (DIR ${CMAKE_CURRENT_LIST_DIR}/3rd_party/cmaj/include)
#     if (IS_DIRECTORY ${DIR})
#         target_include_directories(${TARGET_NAME} PUBLIC ${DIR})
#     endif ()
# endforeach ()

target_compile_definitions(${TARGET_NAME}
    PUBLIC
        JUCE_USE_CURL=0    
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        # RAVE_MODELS_PATH_PYTORCH="${RAVE_MODELS_PATH_PYTORCH}"
)
    
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/source/*.h)

target_sources(${TARGET_NAME} PRIVATE ${SOURCES})

file(GLOB_RECURSE SOURCE_DIRS LIST_DIRECTORIES true ${CMAKE_CURRENT_LIST_DIR}/source/*)
list(APPEND SOURCE_DIRS ${CMAKE_CURRENT_LIST_DIR}/source)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "source" FILES ${SOURCES})

foreach (DIR ${SOURCE_DIRS})
    if (IS_DIRECTORY ${DIR})
        target_include_directories(${TARGET_NAME} PRIVATE ${DIR})
    endif ()
endforeach ()

# add_library(cmaj_lib STATIC IMPORTED)

# set_target_properties(cmaj_lib PROPERTIES
#     IMPORTED_LOCATION  "${CMAKE_CURRENT_LIST_DIR}/3rd_party/cmaj/lib/cmaj.lib"
# )


if(NOT CMAJ_VERSION)
    # Determine the version - use the latest tag for the checkout out repo
    set(CMAJ_VERSION "1.0")
    find_package (Git)

    if (GIT_FOUND)
        execute_process (
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
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


add_subdirectory(3rd_party/cmajor/modules cmajor)

MAKE_CMAJ_LIBRARY (
    LIBRARY_NAME cmaj_lib
    INCLUDE_PLAYBACK
    ENABLE_PERFORMER_LLVM
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        # BinaryData
        juce::juce_dsp
        anira::anira
        cmaj_lib
        juce::juce_audio_utils
)

file(GLOB_RECURSE INFERENCE_ENGINE_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/anira-1.0.0/lib/*.dll")
list(APPEND NECESSARY_DLLS ${INFERENCE_ENGINE_DLLS})

foreach(FORMAT IN LISTS FORMATS_TO_BUILD)
    string(CONCAT CONCATENATED_TARGET ${TARGET_NAME}_${FORMAT})
    add_custom_command(TARGET ${CONCATENATED_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${NECESSARY_DLLS}
            $<TARGET_FILE_DIR:${TARGET_NAME}>/${FORMAT})
endforeach()    
