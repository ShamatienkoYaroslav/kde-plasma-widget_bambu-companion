ecm_add_qml_module(bambucompanionplugin
    URI "io.github.shamatienkoyaroslav.bambucompanion"
    VERSION 1.0
    GENERATE_PLUGIN_SOURCE
)

# QML_ELEMENT/QML_SINGLETON registration requires these sources to be compiled
# directly as part of the qml-module target (qmltyperegistrar only scans a
# target's own sources) — they are not part of bambucompanioncore, which this
# target links below for the business logic they depend on (PrinterRegistry
# etc.). tests/CMakeLists.txt compiles its own separate copy of these two
# files directly into the test executable that needs them, since a MODULE
# target like this one can't be linked into another target.
target_sources(bambucompanionplugin PRIVATE
    PrinterListModel.h
    PrinterListModel.cpp
    PrinterController.h
    PrinterController.cpp
    CloudAccountController.h
    CloudAccountController.cpp
)

target_link_libraries(bambucompanionplugin PRIVATE
    Qt::Quick
    bambucompanioncore
)

ecm_finalize_qml_module(bambucompanionplugin DESTINATION ${KDE_INSTALL_QMLDIR})
