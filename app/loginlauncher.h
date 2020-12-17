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

    explicit LoginTask(QString baseUrl,
              QSslCertificate serverCert,
              QString username,
              QString password)
        : m_baseUrl(baseUrl),
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
        BackendAPI backend(m_baseUrl,"");
        QString sessionId;
        QString errorMsg;

        bool ok = backend.login(m_userName, m_password, sessionId, errorMsg);

        emit taskCompleted(ok, ok?sessionId:errorMsg);

    }
    QString m_baseUrl;
    QString m_userName;
    QString m_password;
};


class ChangePasswordTask: public QObject, public QRunnable
{
    Q_OBJECT

public:

    explicit ChangePasswordTask(  QString baseUrl,
                                  QString sessionId,
                                  QString userName,
                                  QString oldPassword,
                                  QString newPassword)
        : m_baseUrl(baseUrl),
          m_sessionId(sessionId),
          m_userName(userName),
          m_oldPassword(oldPassword),
          m_newPassword(newPassword)
    {

    }

    ~ChangePasswordTask(){}

signals:
    void taskCompleted(bool ok, QString msg);

private:
    void run()
    {
        BackendAPI backend(m_baseUrl,m_sessionId);
        QString msg;
        bool ok = backend.changePassword(m_userName, m_oldPassword, m_newPassword, msg);
        emit taskCompleted(ok, msg);
    }
    QString m_baseUrl;
    QString m_sessionId;
    QString m_userName;
    QString m_oldPassword;
    QString m_newPassword;
};

class ResetMachineTask: public QObject, public QRunnable
{
    Q_OBJECT

public:

    explicit ResetMachineTask(    QString baseUrl,
                                  QString sessionId,
                                  QString machineId)
        : m_baseUrl(baseUrl),
          m_sessionId(sessionId),
          m_machineId(machineId)
    {

    }

    ~ResetMachineTask(){}

signals:
    void taskCompleted(bool ok, QString msg);

private:
    void run()
    {
        BackendAPI backend(m_baseUrl,m_sessionId);
        QString errorMsg;
        bool ok = backend.resetMachine(m_machineId, errorMsg);
        emit taskCompleted(ok, errorMsg);
    }
    QString m_baseUrl;
    QString m_sessionId;
    QString m_machineId;
};

class MachineCurrentStatusTask: public QObject, public QRunnable
{
    Q_OBJECT

public:

    explicit MachineCurrentStatusTask( QString baseUrl,
                                  QString sessionId,
                                  QString machineId)
        : m_baseUrl(baseUrl),
          m_sessionId(sessionId),
          m_machineId(machineId)
    {

    }

    ~MachineCurrentStatusTask(){}

signals:
    void taskCompleted(bool ok, int status, QString description);

private:
    void run()
    {
        BackendAPI backend(m_baseUrl,m_sessionId);
        QString desc;
        int status;
        bool ok = backend.getMachineStatus(m_machineId, status, desc);
        emit taskCompleted(ok, status, desc);
    }
    QString m_baseUrl;
    QString m_sessionId;
    QString m_machineId;
};



class GetCredentialTask: public QObject, public QRunnable
{
    Q_OBJECT

public:
    GetCredentialTask(QString baseUrl,
              QString sessionId)
        : m_baseUrl(baseUrl),
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
                       QString myServerCert,
                       QString errorMsg);

private:
    void run()
    {
        qDebug() << Q_FUNC_INFO << "Run thread GetCredentialTask";
        BackendAPI backend(m_baseUrl,
                           m_sessionId);
        QString myId, myCert, myKey, myServerIP, myServerName, myServerUuid, myServerCert;
        QString errorMsg;

        bool ok = backend.getMyCredentials(myId,
                                           myCert,
                                           myKey,
                                           myServerIP,
                                           myServerName,
                                           myServerUuid,
                                           myServerCert,
                                           errorMsg);

        emit taskCompleted(ok, myId, myCert, myKey, myServerIP,
                           myServerName, myServerUuid, myServerCert,
                           errorMsg);

    }
    QString m_baseUrl;
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
    Q_INVOKABLE void changePassword(QString username, QString oldPassword, QString newPassword);
    Q_INVOKABLE void resetMachine(QString machineId);
    Q_INVOKABLE void getMachineStatus(QString machineId);


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
    Q_INVOKABLE bool isLoginSuccess() const;

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
    void changePasswordDone(bool ok, QString msg);
    void resetMachineDone(bool ok, QString msg);
    void getMachineStatusDone(bool ok, int status, QString desc);

    void performingLogin();
    void performingGetMyCreadentials();
    void performingChangePassword();
    void performingResetMachine();
    void performingGetMachineStatus();

    void searchingComputer();
    void searchingComputerDone(bool ok, QString data);

    void computerReady(bool enabled, int index, int appIndex, QString appName, Session* session);

public slots:
    void onLoginFinished(bool ok, QString data);
    void onChangePasswordFinished(bool ok, QString msg);
    void onResetMachineFinished(bool ok, QString msg);
    void onGetMyCredentialsFinished(bool ok,
                                    QString myId, QString myCred, QString myKey,
                                    QString myServerIp, QString myServerName,
                                    QString myServerUuid, QString myServerCert);
    void onComputerFound(NvComputer *computer);
    void onComputerUpdated(NvComputer *computer);
    void onTimeout();
    void onQuitAppCompleted(QVariant error);
    void onGetMachineStatusFinished(bool ok, int status, QString desc);

private:
    QScopedPointer<LoginLauncherPrivate> m_DPtr;
    ComputerManager* m_computerManager;
};

}

#endif // LOGINLAUNCHER_H
