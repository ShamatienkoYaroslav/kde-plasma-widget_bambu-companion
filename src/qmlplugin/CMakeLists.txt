# A plain static library holding the model classes, so tests can link against
# a normal STATIC target (a MODULE-type QML plugin target cannot itself be
# linked into another target).
add_library(bambucompanioncore STATIC
    PrinterListModel.h
    PrinterListModel.cpp
)

target_include_directories(bambucompanioncore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bambucompanioncore PUBLIC
    Qt::Quick
    KF6::CoreAddons
)

ecm_add_qml_module(bambucompanionplugin
    URI "io.github.shamatienkoyaroslav.bambucompanion"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
)

# QML_ELEMENT registration requires the sources to be compiled directly as
# part of the qml-module target, so they are compiled again here (once into
# bambucompanioncore for tests, once into the plugin module below).
target_sources(bambucompanionplugin PRIVATE
    PrinterListModel.h
    PrinterListModel.cpp
)

target_link_libraries(bambucompanionplugin PRIVATE
    Qt::Quick
    KF6::CoreAddons
)

ecm_finalize_qml_module(bambucompanionplugin DESTINATION ${KDE_INSTALL_QMLDIR})
