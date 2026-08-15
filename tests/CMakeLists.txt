find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
include(ECMAddTests)

# PrinterListModel/PrinterController are QML_SINGLETON types that must be
# compiled directly into the QML module target (see
# src/qmlplugin/CMakeLists.txt) and can't be linked from there since it's a
# MODULE target; compile a private copy of them directly into this test
# executable instead, same as the module does.
ecm_add_test(
    printerlistmodeltest.cpp
    ${CMAKE_SOURCE_DIR}/src/qmlplugin/PrinterListModel.cpp
    ${CMAKE_SOURCE_DIR}/src/qmlplugin/PrinterController.cpp
    TEST_NAME printerlistmodeltest
    LINK_LIBRARIES Qt::Test Qt::Qml bambucompanioncore
)

ecm_add_test(bambureportparsertest.cpp
    TEST_NAME bambureportparsertest
    LINK_LIBRARIES Qt::Test bambucompanioncore
)
target_compile_definitions(bambureportparsertest PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

ecm_add_test(certificatetruststoretest.cpp
    TEST_NAME certificatetruststoretest
    LINK_LIBRARIES Qt::Test bambucompanioncore
)

ecm_add_test(printerregistrytest.cpp
    TEST_NAME printerregistrytest
    LINK_LIBRARIES Qt::Test bambucompanioncore
)
