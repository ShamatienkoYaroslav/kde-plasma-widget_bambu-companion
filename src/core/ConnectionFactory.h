#pragma once

class QObject;
class PrinterConnection;
struct PrinterProfile;

class ConnectionFactory
{
public:
    static PrinterConnection *create(const PrinterProfile &profile, QObject *parent = nullptr);
};
