TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    persistent/json.cpp \
    persistent/json_node.cpp \
    persistent/json_node_object.cpp \
    persistent/json_node_string.cpp \
    persistent/json_node_number.cpp \
    persistent/json_node_array.cpp \
    persistent/json_node_bool.cpp \
    persistent/json_node_null.cpp \
    persistent/json_parse.cpp

HEADERS += \
    persistent/json.h \
    persistent/map.h \
    persistent/json_node.h \
    persistent/json_type.h \
    persistent/json_node_object.h \
    persistent/json_node_string.h \
    persistent/json_node_number.h \
    persistent/json_node_array.h \
    persistent/json_node_bool.h \
    persistent/json_node_null.h \
    persistent/json_parse.h
