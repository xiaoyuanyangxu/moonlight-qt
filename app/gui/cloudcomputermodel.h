#ifndef CLOUDCOMPUTERMODEL_H
#define CLOUDCOMPUTERMODEL_H


#include <QAbstractListModel>


typedef struct {
    QString userId;
    QString userCert;
    QString userKey;
    QString serverIp;
    QString serverName;
    QString serverUuid;
    QString serverCert;
} CloudComputerData;

class CloudComputerModel : public QAbstractListModel
{
    Q_OBJECT

    enum Roles
    {
        ServerNameRole = Qt::UserRole,
        ServerIpRole,
        ServerUuidRole,
        ServerCertRole,
        UserIdRole,
        UserCredRole,
        UserKeyRole
    };

public:
    explicit CloudComputerModel(QObject* object = nullptr);

    // Must be called before any QAbstractListModel functions
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void addComputer(QString userId,
                                 QString userCred,
                                 QString userKey,
                                 QString serverIp,
                                 QString serverName,
                                 QString serverUuid,
                                 QString serverCert);


    QVariant data(const QModelIndex &index, int role) const override;

    int rowCount(const QModelIndex &parent) const override;

    virtual QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void resetComputer(int computerIndex);

signals:
    void resetCompleted(QVariant error);

private slots:
    void handleResetCompleted(QString error);

private:
    QVector<CloudComputerData> m_Computers;
};
#endif // CLOUDCOMPUTERMODEL_H
