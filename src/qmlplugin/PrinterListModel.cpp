#include "PrinterListModel.h"

PrinterListModel::PrinterListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Placeholder data for Phase 0 (proves the plugin<->applet wiring works).
    // Replaced by PrinterRegistry-backed data in Phase 1.
    m_printers = {
        {QStringLiteral("Living Room X1C"), QStringLiteral("Idle")},
        {QStringLiteral("Garage P1S"), QStringLiteral("Printing")},
        {QStringLiteral("Workshop A1"), QStringLiteral("Offline")},
    };
}

int PrinterListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_printers.size();
}

QVariant PrinterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_printers.size()) {
        return {};
    }

    const PrinterEntry &entry = m_printers.at(index.row());
    switch (role) {
    case NameRole:
        return entry.name;
    case StatusRole:
        return entry.status;
    default:
        return {};
    }
}

QHash<int, QByteArray> PrinterListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {StatusRole, "status"},
    };
}
