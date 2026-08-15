#pragma once

#include <QByteArray>

class BambuCommandBuilder
{
public:
    // Requests a full status report; publish once per connection (Bambu
    // printers stream delta reports afterward, so polling this is unnecessary
    // and discouraged by the community-documented protocol).
    static QByteArray pushAll();
};
