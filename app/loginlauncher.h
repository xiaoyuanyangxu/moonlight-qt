#ifndef LOGINLAUNCHER_H
#define LOGINLAUNCHER_H

#include "backendapi.h"
#include "backend/nvcomputer.h"

#include <QObject>
#include <QRunnable>
#include <QSslCertificate>
#include <QVariant>

class ComputerManager;
class NvComputer;
class Session;
class StreamingPreferences;

namespace CliLoginLauncher
{
class Event;
class LoginLauncherPrivate;

class LoginTask: public QObject, public QRunnable
{
    Q_OBJECT

public:

    explicit LoginTask(QString address,
              QSslCertificate serverCert,
              QString username,
              QString password)
        : m_address(address),
          m_serverCert(serverCert),
          m_userName(username),
          m_password(password)
    {

    }

    ~LoginTask(){}

signals:
    void taskCompleted(bool ok, QString data);

private:
    void run()
    {
        BackendAPI backend(m_address,
                           "",
                           m_serverCert);
        QString sessionId;
        bool ok = backend.login(m_userName, m_password, sessionId);

        emit taskCompleted(ok, sessionId);

    }
    QString m_address;
    QSslCertificate m_serverCert;
    QString m_userName;
    QString m_password;
};

class GetCredentialTask: public QObject, public QRunnable
{
    Q_OBJECT

public:
    GetCredentialTask(QString address,
              QSslCertificate serverCert,
              QString sessionId)
        : m_address(address),
          m_serverCert(serverCert),
          m_sessionId(sessionId)
    {

    }
    ~GetCredentialTask(){}

signals:
    void taskCompleted(bool ok,
                       QString myid,
                       QString myCert,
                       QString myKey,
                       QString myServerIP,
                       QString myServerName,
                       QString myServerUuid,
                       QString myServerCert);

private:
    void run()
    {
        qDebug() << Q_FUNC_INFO << "Run thread GetCredentialTask";
        BackendAPI backend(m_address,
                           m_sessionId,
                           m_serverCert);
        QString myId, myCert, myKey, myServerIP, myServerName, myServerUuid, myServerCert;

        bool ok = backend.getMyCredentials(myId,
                                           myCert,
                                           myKey,
                                           myServerIP,
                                           myServerName,
                                           myServerUuid,
                                           myServerCert);

        emit taskCompleted(ok, myId, myCert, myKey, myServerIP,
                           myServerName, myServerUuid, myServerCert);

    }
    QString m_address;
    QSslCertificate m_serverCert;
    QString m_sessionId;
};


class LoginLauncher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE_D(m_DPtr, LoginLauncher)

public:
    explicit LoginLauncher(QString loginComputer, QObject *parent = nullptr);

    ~LoginLauncher();

    Q_INVOKABLE void getMyCredentials(QString sessionId);
    Q_INVOKABLE void login(QString username, QString password);
    Q_INVOKABLE void logout();

    Q_INVOKABLE void seekComputer(ComputerManager *manager,
                                  QString myId,
                                  QString myCred,
                                  QString myKey,
                                  QString computerIp,
                                  QString name,
                                  QString uuid,
                                  QString cert);
    Q_INVOKABLE void quitRunningApp();
    Q_INVOKABLE bool isExecuted() const;
    Q_INVOKABLE QString getCachedSessionCookie();
    Q_INVOKABLE QString getLastUsername();


signals:
    void myCredentialsDone(bool ok,
                           QString myId, QString myCred, QString myKey,
                           QString myServerIp,
                           QString myServerName,
                           QString myServerUuid,
                           QString myServerCert);
    void logginDone(bool ok, QString data);
    void performingLogin();
    void performingGetMyCreadentials();

    void searchingComputer();
    void searchingComputerDone(bool ok, QString data);

    void computerReady(bool enabled, int index);

public slots:
    void onLoginFinished(bool ok, QString data);
    void onGetMyCredentialsFinished(bool ok,
                                    QString myId, QString myCred, QString myKey,
                                    QString myServerIp, QString myServerName,
                                    QString myServerUuid, QString myServerCert);
    void onComputerFound(NvComputer *computer);
    void onComputerUpdated(NvComputer *computer);
    void onTimeout();
    void onQuitAppCompleted(QVariant error);

private:
    QScopedPointer<LoginLauncherPrivate> m_DPtr;
    ComputerManager* m_computerManager;
};

}

#endif // LOGINLAUNCHER_H
