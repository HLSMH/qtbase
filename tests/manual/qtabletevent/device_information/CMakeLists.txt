# Generated from device_information.pro.

#####################################################################
## device_information Binary:
#####################################################################

add_qt_manual_test(device_information
    GUI
    SOURCES
        main.cpp
        tabletwidget.cpp tabletwidget.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
