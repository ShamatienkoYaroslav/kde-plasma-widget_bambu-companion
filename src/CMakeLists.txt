# A plain static library holding all business-logic classes, so both the QML
# plugin and the test suite can link against a normal STATIC target (a
# MODULE-type QML plugin target cannot itself be linked into another target).
add_library(bambucompanioncore STATIC
    core/PrinterProfile.h
    core/PrinterProfile.cpp
    core/PrinterStatus.h
    core/PrinterStatus.cpp
    core/PrinterConnection.h
    core/PrinterCommand.h
    core/ConnectionFactory.h
    core/ConnectionFactory.cpp
    core/PrinterRegistry.h
    core/PrinterRegistry.cpp
    security/SecretStore.h
    security/SecretStore.cpp
    security/CertificateProbe.h
    security/CertificateProbe.cpp
    security/CertificateTrustStore.h
    security/CertificateTrustStore.cpp
    protocol/BambuReportParser.h
    protocol/BambuReportParser.cpp
    protocol/BambuCommandBuilder.h
    protocol/BambuCommandBuilder.cpp
    transport/lan/MqttClient.h
    transport/lan/MqttClient.cpp
    transport/lan/LanPrinterConnection.h
    transport/lan/LanPrinterConnection.cpp
    transport/lan/PendingCommandTracker.h
    transport/lan/PendingCommandTracker.cpp
)

target_include_directories(bambucompanioncore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Linked into the bambucompanionplugin MODULE (shared object) below, which
# requires position-independent code.
set_target_properties(bambucompanioncore PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(bambucompanioncore PUBLIC
    Qt::Core
    Qt::Network
    KF6::CoreAddons
    KF6::Wallet
    KF6::ConfigCore
    PkgConfig::MOSQUITTO
)

add_subdirectory(qmlplugin)
