# Build the zep demo
IF (BUILD_DEMOS AND BUILD_IMGUI)

set(DEMO_ROOT ${ZEP_ROOT}/demos/demo_imgui)
set(DEMO_NAME ZepDemo)

project(ZepDemo
    VERSION 0.1.0.0
    LANGUAGES CXX
)

# Add project metadata
add_project_meta(META_FILES_TO_INCLUDE IS_IMGUI)

# Find required packages
find_package(tinyfiledialogs CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)

# Collect source files
set(CLIP_SOURCE
    ${DEMO_ROOT}/clip/clip.cpp
    ${DEMO_ROOT}/clip/image.cpp
)

if(WIN32)
    list(APPEND CLIP_SOURCE ${DEMO_ROOT}/clip/clip_win.cpp)
elseif(APPLE)
    list(APPEND CLIP_SOURCE ${DEMO_ROOT}/clip/clip_osx.mm)
else()
    list(APPEND CLIP_SOURCE ${DEMO_ROOT}/clip/clip_x11.cpp)
endif()

set(DEMO_SOURCE_IMGUI
    ${DEMO_ROOT}/janet/janet.c
    ${DEMO_ROOT}/janet_utils.cpp
    ${DEMO_ROOT}/main.cpp
    ${DEMO_ROOT}/dpi/dpi.cpp
    ${DEMO_ROOT}/dpi/dpi.h
    ${DEMO_ROOT}/CMakeLists.txt
    ${ZEP_ROOT}/include/zep/imgui/display_imgui.h
    ${ZEP_ROOT}/include/zep/imgui/editor_imgui.h
    ${META_FILES_TO_INCLUDE}
    ${CLIP_SOURCE}
)

# Create executable
add_executable(${DEMO_NAME} ${DEMO_SOURCE_IMGUI})

# Compile definitions
target_compile_definitions(${DEMO_NAME} PUBLIC -DZEP_USE_SDL)

# Include directories
target_include_directories(${DEMO_NAME}
    PRIVATE
        ${ZEP_ROOT}/include
        ${ZEP_ROOT}/demos
        ${DEMO_ROOT}/dpi
        ${DEMO_ROOT}/janet
        ${FREETYPE_INCLUDE_DIRS}
        ${SDL2_ROOT_DIR}
        ${CMAKE_BINARY_DIR}
)

# Link libraries
target_link_libraries(${DEMO_NAME}
    PRIVATE
        Zep
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        imgui::imgui
        unofficial::gl3w::gl3w
        tinyfiledialogs::tinyfiledialogs
        ${PLATFORM_LINKLIBS}
)

# Platform-specific linking
if(NOT WIN32)
    target_link_libraries(${DEMO_NAME} PRIVATE ${FREETYPE_LIBRARIES})
endif()

# Copy resources on Windows
if(WIN32)
    copy_existing_files(${PROJECT_NAME} "${RESOURCE_DEPLOY_FILES}" ${CMAKE_CURRENT_BINARY_DIR}/$(Configuration))
endif()

# Installation
install(TARGETS ${DEMO_NAME}
    EXPORT zep-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/imgui
    BUNDLE DESTINATION .   # For macOS
)

# Install font
install(FILES ${DEMO_ROOT}/res/Cousine-Regular.ttf
    DESTINATION ${CMAKE_INSTALL_BINDIR}/imgui
)

# Source grouping for IDE
source_group("Zep Demo" FILES ${DEMO_SOURCE_IMGUI})

ENDIF() # IMGUI