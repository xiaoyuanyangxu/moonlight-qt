#include "cloudcomputermodel.h"

#include <QReadWriteLock>
#include <QDebug>

CloudComputerModel::CloudComputerModel(QObject* object)
{

}

void CloudComputerModel::initialize()
{

}

void CloudComputerModel::addComputer(QString userId, QString userCred, QString userKey, QString serverIp, QString serverName, QString serverUuid, QString serverCert)
{
    qDebug() << Q_FUNC_INFO << userId << " IP:" << serverIp << " Name:"<< serverName;
    m_Computers.append({
                           userId,
                           userCred,
                           userKey,
                           serverIp,
                           serverName,
                           serverUuid,
                           serverCert
                       });
}

QHash<int, QByteArray> CloudComputerModel::roleNames() const
{
    QHash<int, QByteArray> names;

    names[ServerNameRole] = "name";
    names[ServerIpRole] = "ip";
    names[ServerUuidRole] = "uuid";
    names[ServerCertRole] = "cert";
    names[UserIdRole] = "user_id";
    names[UserCredRole] = "user_cred";
    names[UserKeyRole] = "user_key";

    return names;
}

QVariant CloudComputerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Q_ASSERT(index.row() < m_Computers.count());

    switch (role) {
    case ServerNameRole:
        return m_Computers[index.row()].serverName;
    case ServerIpRole:
        return m_Computers[index.row()].serverIp;
    case ServerUuidRole:
        return m_Computers[index.row()].serverUuid;
    case ServerCertRole:
        return m_Computers[index.row()].serverCert;
    case UserIdRole:
        return m_Computers[index.row()].userId;
    case UserCredRole:
        return m_Computers[index.row()].userCert;
    case UserKeyRole:
        return m_Computers[index.row()].userKey;
    default:
        return QVariant();
    }
}

int CloudComputerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_Computers.count();
}

void CloudComputerModel::resetComputer(int computerIndex)
{

}

void CloudComputerModel::handleResetCompleted(QString error)
{

}
