# Generated from qdbuscpp2xml.pro.

#####################################################################
## tst_qdbuscpp2xml Test:
#####################################################################

add_qt_test(tst_qdbuscpp2xml
    SOURCES
        test1.h
        tst_qdbuscpp2xml.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)

# Resources:
set(qdbuscpp2xml_resource_files
    "test1.h"
)

add_qt_resource(tst_qdbuscpp2xml "qdbuscpp2xml"
    PREFIX
        "/tst_qdbuscpp2xml/"
    FILES
        ${qdbuscpp2xml_resource_files}
)

