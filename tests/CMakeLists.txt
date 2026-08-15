find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
include(ECMAddTests)

ecm_add_test(printerlistmodeltest.cpp
    TEST_NAME printerlistmodeltest
    LINK_LIBRARIES Qt::Test bambucompanioncore
)
