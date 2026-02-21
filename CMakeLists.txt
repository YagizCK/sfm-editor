cmake_minimum_required(VERSION 3.10)

project(sfm-editor VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# glad
file(GLOB_RECURSE GLAD_SOURCES external/glad/src/*.c)
add_library(glad STATIC ${GLAD_SOURCES})
target_include_directories(glad PUBLIC external/glad/include)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/GLFW)

# imgui
set(IMGUI_DIR external/imgui)
file(GLOB_RECURSE IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/*.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC
    glad
    glfw
)

# glm
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE external/glm)

# ImGuizmo
set(IMGUIZMO_DIR external/ImGuizmo)
file(GLOB_RECURSE IMGUIZMO_SOURCES ${IMGUIZMO_DIR}/*.cpp)
add_library(imguizmo STATIC ${IMGUIZMO_SOURCES})
target_include_directories(imguizmo PUBLIC ${IMGUIZMO_DIR})
target_link_libraries(imguizmo PUBLIC
    imgui
    glm
)

# stb_image
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE external/stb)

file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.h
    src/*.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    imgui
    glm
    imguizmo
    stb
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32
        gdi32
        user32
        shell32
    )
endif()

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
endif()

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(TARGET_DIR "${CMAKE_BINARY_DIR}")
else()
    set(TARGET_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

file(MAKE_DIRECTORY "${TARGET_DIR}")

if(NOT EXISTS "${TARGET_DIR}/assets")
    message(STATUS "Creating asset junction: ${TARGET_DIR}/assets")

    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/res/assets" SRC_PATH)
    file(TO_NATIVE_PATH "${TARGET_DIR}/assets" DST_PATH)

    execute_process(
        COMMAND cmd /c mklink /J "${DST_PATH}" "${SRC_PATH}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        RESULT_VARIABLE link_result
        ERROR_VARIABLE link_error
    )

    if(NOT link_result EQUAL 0)
        message(WARNING "Failed to create link! Error: ${link_error}")
    endif()
endif()
