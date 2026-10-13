# The reusable widgets: the panel of sliders that edits the settings of a
# pipeline, its sections and sliders, and the image view. They know nothing of
# the editor: Qt Widgets and the engine only, so a program of its own can take
# them with the engine.
set(CMAKE_AUTOMOC ON)

add_library(widgets-lib STATIC
    include/widgets/AdjustmentsPanel.h
    include/widgets/CollapsibleSection.h
    include/widgets/CurveView.h
    include/widgets/HistogramUtils.h
    include/widgets/HistogramView.h
    include/widgets/ImageView.h
    include/widgets/ParameterSlider.h
    include/widgets/SectionUtils.h
    include/widgets/SliderStyle.h
    src/AdjustmentsPanel.cpp
    src/CollapsibleSection.cpp
    src/CurveView.cpp
    src/HistogramUtils.cpp
    src/HistogramView.cpp
    src/ImageView.cpp
    src/ParameterSlider.cpp
    src/SectionUtils.cpp
    src/SliderStyle.cpp)
add_library(stepit-grader::widgets ALIAS widgets-lib)
target_include_directories(widgets-lib PUBLIC include)
target_link_libraries(widgets-lib PUBLIC core-lib Qt6::Widgets)
stepit_grader_warnings(widgets-lib)
