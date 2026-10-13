# The image engine: the effects and what they share. Standard C++ only, no Qt,
# so that a command line tool can grade images with it.
add_library(core-lib STATIC
    src/CameraMatch.cpp
    src/Color.cpp
    src/Display.cpp
    src/Effect.cpp
    src/Filters.cpp
    src/Geometry.cpp
    src/Luminance.cpp
    src/Parallel.cpp
    src/Pipeline.cpp
    src/Sampling.cpp
    src/Settings.cpp
    src/StandardPipeline.cpp
    src/effects/BlacksEffect.cpp
    src/effects/ColorMixerEffect.cpp
    src/effects/ColorNoiseEffect.cpp
    src/effects/ContrastEffect.cpp
    src/effects/DefringeEffect.cpp
    src/effects/DetailEffect.cpp
    src/effects/ExposureEffect.cpp
    src/effects/HighlightsEffect.cpp
    src/effects/LuminanceNoiseEffect.cpp
    src/effects/SaturationEffect.cpp
    src/effects/ShadowsEffect.cpp
    src/effects/SharpnessEffect.cpp
    src/effects/StructureEffect.cpp
    src/effects/ToneCompressionEffect.cpp
    src/effects/ToneCurveEffect.cpp
    src/effects/VibranceEffect.cpp
    src/effects/WhiteBalanceEffect.cpp)
add_library(stepit-grader::core ALIAS core-lib)
target_include_directories(core-lib PUBLIC include)
target_link_libraries(core-lib PUBLIC Threads::Threads)
stepit_grader_warnings(core-lib)

# The effects loop over every pixel, about ten times slower without the
# optimiser: optimised in a debug build too, so a program debugged with the
# engine still follows its sliders.
if(NOT MSVC)
    target_compile_options(core-lib PRIVATE $<$<CONFIG:Debug>:-O2>)
endif()
