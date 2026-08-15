#include "PrinterListModel.h"

#include "../core/PrinterRegistry.h"

PrinterListModel::PrinterListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    reload();

    auto &registry = PrinterRegistry::instance();
    connect(&registry, &PrinterRegistry::printerAdded, this, [this](const QUuid &) {
        reload();
    });
    connect(&registry, &PrinterRegistry::printerRemoved, this, [this](const QUuid &) {
        reload();
    });
    connect(&registry, &PrinterRegistry::printerStatusChanged, this, [this](const QUuid &id) {
        const int row = rowForId(id);
        if (row >= 0) {
            const QModelIndex idx = index(row, 0);
            Q_EMIT dataChanged(idx, idx);
        }
    });
    connect(&registry, &PrinterRegistry::printerConnectionStateChanged, this, [this](const QUuid &id) {
        const int row = rowForId(id);
        if (row >= 0) {
            const QModelIndex idx = index(row, 0);
            Q_EMIT dataChanged(idx, idx);
        }
    });
}

void PrinterListModel::reload()
{
    beginResetModel();
    m_order.clear();
    const auto printers = PrinterRegistry::instance().printers();
    for (const PrinterProfile &profile : printers) {
        m_order.append(profile.id);
    }
    endResetModel();
}

int PrinterListModel::rowForId(const QUuid &id) const
{
    return m_order.indexOf(id);
}

int PrinterListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_order.size();
}

QVariant PrinterListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_order.size()) {
        return {};
    }

    const QUuid id = m_order.at(index.row());
    auto &registry = PrinterRegistry::instance();
    const PrinterProfile profile = registry.profile(id);
    const PrinterStatus status = registry.status(id);

    switch (role) {
    case IdRole:
        return profile.id.toString();
    case NameRole:
        return profile.name;
    case StateRole:
        return PrinterStatus::stateToString(status.state);
    case ProgressRole:
        return status.progressPercent ? *status.progressPercent : -1;
    case NozzleTempRole:
        return status.nozzleTemp ? QVariant(*status.nozzleTemp) : QVariant();
    case BedTempRole:
        return status.bedTemp ? QVariant(*status.bedTemp) : QVariant();
    case ConnectionStateRole:
        return static_cast<int>(registry.connectionState(id));
    default:
        return {};
    }
}

QHash<int, QByteArray> PrinterListModel::roleNames() const
{
    return {
        {IdRole, "printerId"},
        {NameRole, "name"},
        {StateRole, "status"},
        {ProgressRole, "progress"},
        {NozzleTempRole, "nozzleTemp"},
        {BedTempRole, "bedTemp"},
        {ConnectionStateRole, "connectionState"},
    };
}

QVariantMap PrinterListModel::printerDetails(const QString &printerId) const
{
    auto &registry = PrinterRegistry::instance();
    const QUuid id(printerId);
    const PrinterProfile profile = registry.profile(id);
    const PrinterStatus status = registry.status(id);

    QVariantMap details;
    details[QStringLiteral("name")] = profile.name;
    details[QStringLiteral("host")] = profile.host;
    details[QStringLiteral("serial")] = profile.serial;
    details[QStringLiteral("status")] = PrinterStatus::stateToString(status.state);
    details[QStringLiteral("connectionState")] = static_cast<int>(registry.connectionState(id));
    details[QStringLiteral("progress")] = status.progressPercent ? *status.progressPercent : -1;
    details[QStringLiteral("currentLayer")] = status.currentLayer ? *status.currentLayer : -1;
    details[QStringLiteral("totalLayers")] = status.totalLayers ? *status.totalLayers : -1;
    details[QStringLiteral("nozzleTemp")] = status.nozzleTemp ? QVariant(*status.nozzleTemp) : QVariant();
    details[QStringLiteral("nozzleTargetTemp")] = status.nozzleTargetTemp ? QVariant(*status.nozzleTargetTemp) : QVariant();
    details[QStringLiteral("bedTemp")] = status.bedTemp ? QVariant(*status.bedTemp) : QVariant();
    details[QStringLiteral("bedTargetTemp")] = status.bedTargetTemp ? QVariant(*status.bedTargetTemp) : QVariant();
    details[QStringLiteral("chamberTemp")] = status.chamberTemp ? QVariant(*status.chamberTemp) : QVariant();
    details[QStringLiteral("coolingFanPercent")] = status.coolingFanPercent ? *status.coolingFanPercent : -1;
    details[QStringLiteral("auxFanPercent")] = status.auxFanPercent ? *status.auxFanPercent : -1;
    details[QStringLiteral("chamberFanPercent")] = status.chamberFanPercent ? *status.chamberFanPercent : -1;
    details[QStringLiteral("speedProfile")] = status.speedProfile ? *status.speedProfile : QString();
    details[QStringLiteral("wifiSignalDbm")] = status.wifiSignalDbm ? *status.wifiSignalDbm : 0;
    details[QStringLiteral("remainingMinutes")] = status.remainingMinutes ? *status.remainingMinutes : -1;
    return details;
}
