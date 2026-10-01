cmake_minimum_required(VERSION 3.24)
project(audio-visualizer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.5.1
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG 0.11.25
  GIT_SHALLOW TRUE
)
FetchContent_GetProperties(miniaudio)
if(NOT miniaudio_POPULATED)
  FetchContent_Populate(miniaudio)
endif()

add_library(miniaudio STATIC ${miniaudio_SOURCE_DIR}/miniaudio.c)
target_include_directories(miniaudio PUBLIC ${miniaudio_SOURCE_DIR})
if(MSVC)
  target_compile_options(miniaudio PRIVATE /W0 /bigobj)
else()
  target_compile_options(miniaudio PRIVATE -w)
endif()

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)
if(MSVC)
  target_compile_options(imgui PRIVATE /W0)
else()
  target_compile_options(imgui PRIVATE -w)
endif()

add_executable(audio-visualizer
  app.manifest
  src/main.cpp
  src/gl/GlExt.cpp
  src/gl/Shader.cpp
  src/gl/Batch.cpp
  src/gl/ScreenQuad.cpp
  src/audio/AudioEngine.cpp
  src/analysis/Fft.cpp
  src/analysis/Analyzer.cpp
  src/viz/Palette.cpp
  src/viz/Trail.cpp
  src/viz/BuiltinModes.cpp
  src/viz/SpectrumBars.cpp
  src/viz/RadialRing.cpp
  src/viz/Oscilloscope.cpp
  src/viz/Spectrogram.cpp
  src/viz/Terrain.cpp
  src/viz/Particles.cpp
  src/viz/Lissajous.cpp
  src/viz/Kaleidoscope.cpp
  src/viz/Nebula.cpp
  src/viz/Skyline.cpp
  src/viz/Radar.cpp
  src/viz/Ripples.cpp
  src/viz/StereoField.cpp
  src/viz/Helix.cpp
  src/viz/VisualizationSet.cpp
  src/ui/Controls.cpp
  src/ui/FileDialog.cpp
)

target_include_directories(audio-visualizer PRIVATE src)
target_link_libraries(audio-visualizer PRIVATE glfw imgui miniaudio opengl32 ole32 uuid shell32)

if(MSVC)
  target_compile_options(audio-visualizer PRIVATE /W4 /permissive- /utf-8)
  target_compile_definitions(audio-visualizer PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
  set_target_properties(audio-visualizer PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
  )
endif()

add_custom_command(TARGET audio-visualizer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders
          $<TARGET_FILE_DIR:audio-visualizer>/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/visualizations
          $<TARGET_FILE_DIR:audio-visualizer>/visualizations
  COMMENT "Copy shaders and visualizations next to the executable"
)
