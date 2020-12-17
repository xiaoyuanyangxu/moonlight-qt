#include "backendapi.h"
#include "loginlauncher.h"
#include "loginlauncher.h"

#include "backend/computermanager.h"
#include "backend/computerseeker.h"
#include "streaming/session.h"
#include "backend/nvcomputer.h"
#include "statssingleton.h"

#include <QThread>
#include <QThreadPool>
#include <QCoreApplication>
#include <QTimer>

#define COMPUTER_SEEK_TIMEOUT 20000
#define LOGIN_TIMEOUT 10000

namespace CliLoginLauncher
{
enum State {
    StateInit,
    StatePerformLogin,
    StateGettingCreadentials,
    StateSeekComputer,
    StateComputerConnected,
    StateLoginFailure,
};

class Event
{
public:
    enum Type {
        Login,
        ChangePassword,
        ResetMachine,
        GetCurrentStatus,
        GetMyCredentials,
        SeekComputer,
        AppQuitCompleted,
        AppQuitRequested,
        ComputerFound,
        ComputerUpdated,
        Executed,
        Timedout,
    };

    Event(Type type)
        : type(type), computerManager(nullptr), computer(nullptr) {}

    Type type;
    QString userName;
    QString password;

    QString oldPassword;
    QString newPassword;

    QSslCertificate serverCert;
    QString sessionId;

    QString myId;
    QString myCredentials;
    QString myPrivateKey;

    bool loginOk;
    QString loginData;

    ComputerManager *computerManager;
    QString computerIp;
    QString machineId;
    int index;

    NvComputer *computer;
    QString errorMessage;
};



#define SESSION_COOKIE "SESSION_COOKIE"
#define SESSION_USERNAME "SESSION_USERNAME"

class LoginLauncherPrivate
{
    Q_DECLARE_PUBLIC(LoginLauncher)

public:
    LoginLauncherPrivate(LoginLauncher *q) : q_ptr(q) {}

    void handleEvent(Event event)
    {
        qDebug() << Q_FUNC_INFO << "Event: " << event.type;
        Q_Q(LoginLauncher);

        switch (event.type) {
        case Event::Login:
            {
                m_State = StatePerformLogin;
                m_sessionId = "";

                LoginTask* loginTask = new LoginTask(m_LoginComputerName,
                                                     event.serverCert,
                                                     event.userName,
                                                     event.password);

                q->connect(loginTask, &LoginTask::taskCompleted,
                           q, &LoginLauncher::onLoginFinished);

                QThreadPool::globalInstance()->start(loginTask);


                emit q->performingLogin();
            }
            break;

        case Event::GetMyCredentials:
            {
                m_State = StateGettingCreadentials;
                m_sessionId = event.sessionId;

                GetCredentialTask* pTask = new GetCredentialTask(m_LoginComputerName,
                                                                 event.sessionId);

                q->connect(pTask, &GetCredentialTask::taskCompleted,
                        q, &LoginLauncher::onGetMyCredentialsFinished);

                QThreadPool::globalInstance()->start(pTask);


                emit q->performingGetMyCreadentials();
            }
            break;

        case Event::ChangePassword:
            {
                ChangePasswordTask* pTask = new ChangePasswordTask(m_LoginComputerName,
                                                                  event.sessionId,
                                                                  event.userName,
                                                                  event.oldPassword,
                                                                  event.newPassword);

                q->connect(pTask, &ChangePasswordTask::taskCompleted,
                        q, &LoginLauncher::onChangePasswordFinished);

                QThreadPool::globalInstance()->start(pTask);


                emit q->performingChangePassword();
            }
            break;
        case Event::ResetMachine:
            {
                ResetMachineTask* pTask = new ResetMachineTask(m_LoginComputerName,
                                                                  event.sessionId,
                                                                  event.machineId);

                q->connect(pTask, &ResetMachineTask::taskCompleted,
                        q, &LoginLauncher::onResetMachineFinished);

                QThreadPool::globalInstance()->start(pTask);


                emit q->performingResetMachine();
            }
            break;

        case Event::GetCurrentStatus:
            {
                MachineCurrentStatusTask* pTask = new MachineCurrentStatusTask(m_LoginComputerName,
                                                                  event.sessionId,
                                                                  event.machineId);

                q->connect(pTask, &MachineCurrentStatusTask::taskCompleted,
                        q, &LoginLauncher::onGetMachineStatusFinished);

                QThreadPool::globalInstance()->start(pTask);


                emit q->performingGetMachineStatus();
            }
            break;

        case Event::SeekComputer:
            {
                if (m_State != StateSeekComputer)
                {
                    m_State = StateSeekComputer;
                    m_ComputerManager = event.computerManager;

                    if (!IdentityManager::get()->setCertificated(event.myCredentials,
                                                            event.myPrivateKey,
                                                            event.myId))
                    {
                        qDebug() << Q_FUNC_INFO << "This credentials are not valid";
                        emit q->searchingComputer();
                        q->onComputerFound(0);
                        return;
                    }
                    m_ComputerManager->deleteAllHosts();
                    m_ComputerManager->addComputer(event.computer);

                    qDebug()<< Q_FUNC_INFO << "Seek computer:" << m_ComputerManager << "IP:" << event.computerIp;

                    m_ComputerSeeker = new ComputerSeeker(m_ComputerManager, event.computerIp, q);
                    q->connect(m_ComputerSeeker, &ComputerSeeker::computerFound,
                               q, &LoginLauncher::onComputerFound);
                    q->connect(m_ComputerSeeker, &ComputerSeeker::errorTimeout,
                               q, &LoginLauncher::onTimeout);
                    m_ComputerSeeker->start(COMPUTER_SEEK_TIMEOUT);

                    q->connect(m_ComputerManager, &ComputerManager::computerStateChanged,
                               q, &LoginLauncher::onComputerUpdated);
                    q->connect(m_ComputerManager, &ComputerManager::quitAppCompleted,
                               q, &LoginLauncher::onQuitAppCompleted);

                    emit q->searchingComputer();
                }
            }
            break;
        case Event::ComputerUpdated:
            if (m_State == StateSeekComputer) {

                QString appName = getFirstAppName(event.computer);
                Session* session = 0;
                if (!appName.isEmpty())
                {
                    m_State = StateComputerConnected;
                    NvApp app = event.computer->appList.at(0);

                    session = new Session(event.computer, app);

                    emit q->computerReady(event.computer->pairState == NvComputer::PS_PAIRED,
                                          event.index,
                                          0,
                                          appName,
                                          session);
                }

                qDebug() << Q_FUNC_INFO << event.computer->name << " app:" << appName;
            }
            break;

        case Event::Timedout:
            qDebug() << "Timeout" << m_State;
            if (m_State == StatePerformLogin) {
                m_State = StateLoginFailure;
                q->onLoginFinished(false, QString("Failed to login"));
            }
            if (m_State == StatePerformLogin) {
                m_State = StateLoginFailure;
                q->onGetMyCredentialsFinished(false,"", "","", "","","","");
            }
            if (m_State == StateSeekComputer) {
                m_State = StateLoginFailure;

                qDebug() << "Emit computerReady with error";

                emit q->computerReady(false,
                                      0,
                                      0,
                                      "",
                                      nullptr);
            }
            break;
        }
    }

    QString getFirstAppName(NvComputer* computer) const
    {
        if (computer->appList.length()>0)
        {
            qDebug() << Q_FUNC_INFO << computer->appList[0].name;
            return computer->appList[0].name;
        }
        return "";
    }

    LoginLauncher *q_ptr;
    QString m_LoginComputerName;
    QString m_sessionId;
    QString m_ComputerName;
    QString m_AppName;
    StreamingPreferences *m_Preferences;
    ComputerManager *m_ComputerManager;
    ComputerSeeker *m_ComputerSeeker;
    NvComputer *m_Computer;
    State m_State;
    QTimer *m_TimeoutTimer;

};

LoginLauncher::LoginLauncher(QString loginComputer, QObject *parent)
    : QObject(parent),
      m_DPtr(new LoginLauncherPrivate(this))
{
    Q_D(LoginLauncher);
    d->m_LoginComputerName = loginComputer;
    d->m_ComputerName = "";
    d->m_AppName = "";
    d->m_State = StateInit;
    //d->m_TimeoutTimer = new QTimer(this);
    //d->m_TimeoutTimer->setSingleShot(true);
    //connect(d->m_TimeoutTimer, &QTimer::timeout,
   //         this, &LoginLauncher::onTimeout);
}

LoginLauncher::~LoginLauncher()
{

}

void LoginLauncher::getMyCredentials(QString sessionId)
{
    qDebug() << Q_FUNC_INFO << sessionId;
    Q_D(LoginLauncher);
    Event event(Event::GetMyCredentials);
    event.sessionId = sessionId;
    d->handleEvent(event);
}

void LoginLauncher::login(QString username, QString password)
{
    Q_D(LoginLauncher);
    Event event(Event::Login);
    event.userName = username;
    event.password = password;

    QSettings settings;
    settings.setValue(SESSION_USERNAME, username);

    d->handleEvent(event);
}

void LoginLauncher::logout()
{
    QSettings settings;
    settings.setValue(SESSION_COOKIE, "");
}

void LoginLauncher::changePassword(QString userName, QString oldPassword, QString newPassword)
{
    Q_D(LoginLauncher);
    Event event(Event::ChangePassword);
    event.sessionId = d->m_sessionId;
    event.userName = userName;
    event.oldPassword = oldPassword;
    event.newPassword = newPassword;

    d->handleEvent(event);
}

void LoginLauncher::resetMachine(QString machineId)
{
    Q_D(LoginLauncher);
    Event event(Event::ResetMachine);
    event.sessionId = d->m_sessionId;
    event.machineId = machineId;

    d->handleEvent(event);
}

void LoginLauncher::getMachineStatus(QString machineId)
{
    Q_D(LoginLauncher);
    Event event(Event::GetCurrentStatus);
    event.sessionId = d->m_sessionId;
    event.machineId = machineId;

    d->handleEvent(event);
}

void LoginLauncher::seekComputer(ComputerManager *manager,
                                 QString myId,
                                 QString myCred,
                                 QString myKey,
                                 QString computerIp,
                                 QString name, QString uuid, QString cert)
{

    qDebug() << Q_FUNC_INFO << manager;

    m_computerManager = manager;

    Q_D(LoginLauncher);
    Event event(Event::SeekComputer);
    event.computerManager = manager;
    event.computerIp = computerIp;
    event.myId = myId;
    event.myCredentials = myCred;
    event.myPrivateKey = myKey;
    NvComputer *computer = new NvComputer(name,
                                          computerIp,
                                          uuid,
                                          cert);
    event.computer = computer;



    d->handleEvent(event);
}

void LoginLauncher::quitRunningApp()
{
    Q_D(LoginLauncher);
    Event event(Event::AppQuitRequested);
    d->handleEvent(event);
}

bool LoginLauncher::isExecuted() const
{
    Q_D(const LoginLauncher);
    return d->m_State != StateInit;
}

bool LoginLauncher::isLoginSuccess() const
{
    Q_D(const LoginLauncher);
    return d->m_sessionId.isEmpty() == false;
}

QString LoginLauncher::getCachedSessionCookie()
{
    QSettings settings;
    return settings.value(SESSION_COOKIE).toString();
}

QString LoginLauncher::getLastUsername()
{
    QSettings settings;
    return settings.value(SESSION_USERNAME).toString();
}

void LoginLauncher::onLoginFinished(bool ok, QString data)
{
    Q_D(LoginLauncher);
    if (ok) {
        QSettings settings;
        settings.setValue(SESSION_COOKIE, data);
        d->m_sessionId = data;

        qDebug() << Q_FUNC_INFO << "Cookie: " << data;
    }
    emit logginDone(ok, data);
}

void LoginLauncher::onChangePasswordFinished(bool ok, QString msg)
{
    Q_D(LoginLauncher);
    qDebug() << Q_FUNC_INFO<< ok << msg;
    emit changePasswordDone(ok, msg);
}

void LoginLauncher::onResetMachineFinished(bool ok, QString msg)
{
    Q_D(LoginLauncher);
    qDebug() << Q_FUNC_INFO<< ok << msg;
    emit resetMachineDone(ok, msg);
}

void LoginLauncher::onGetMachineStatusFinished(bool ok, int status, QString desc)
{
    Q_D(LoginLauncher);
    qDebug() << Q_FUNC_INFO<< ok << status << desc;
    emit getMachineStatusDone(ok, status, desc);
}

void LoginLauncher::onGetMyCredentialsFinished(bool ok,
                                               QString myId,
                                               QString myCred,
                                               QString myKey,
                                               QString myServerIp,
                                               QString myServerName,
                                               QString myServerUuid,
                                               QString myServerCert)
{
    Q_D(LoginLauncher);
    if (ok)
    {
        StatsSingleton::getInstance()->initialize(m_DPtr->m_LoginComputerName, d->m_sessionId);
    }else{
        d->m_sessionId = "";
    }
    emit myCredentialsDone(ok, myId, myCred, myKey, myServerIp,
                           myServerName, myServerUuid, myServerCert);
}

void LoginLauncher::onComputerFound(NvComputer *computer)
{
    if (computer)
    {
        int index = -1;
        for (auto i: m_computerManager->getComputers())
        {
            index ++;
            if (computer == i) break;

        }
        qDebug()<< Q_FUNC_INFO << "computer found"<< computer->name << " index:" << index << " apps:"
                << computer->appList.size() << " status:"
                << computer->state << " paired: "
                << computer->pairState << "/" << NvComputer::PS_PAIRED;
        if (computer->pairState == NvComputer::PS_PAIRED)
        {
            emit searchingComputerDone(true, computer->name);
        }else{
            emit searchingComputerDone(false, "it is not paired");
        }
    }else{
        emit searchingComputerDone(false, "didn't get the computer");
    }
}

void LoginLauncher::onComputerUpdated(NvComputer *computer)
{

    qDebug()<< Q_FUNC_INFO << "computer updated"
            << computer->name << "apps: "
            << computer->appList.size() << " status:"
            << computer->state << " paired: "
            << computer->pairState << "/" << NvComputer::PS_PAIRED;

    int index = -1;
    for (auto i: m_computerManager->getComputers())
    {
        index ++;
        if (computer == i) break;

    }
    if (computer->appList.size() > 1)
    {
        for(int i=0; i<computer->appList.size() ; i++)
        {
            if (computer->appList[i].name == "Steam BigPicture")
            {
                computer->appList.remove(i);
                break;
            }else{
                qDebug() << Q_FUNC_INFO << "APP: "<< computer->appList[i].name;
            }
        }
    }
    Q_D(LoginLauncher);
    Event event(Event::ComputerUpdated);
    event.computer = computer;
    event.index = index;

    d->handleEvent(event);

    //emit computerReady(computer->pairState == NvComputer::PS_PAIRED);

    return;

    /*Q_D(LoginLauncher);
    Event event(Event::ComputerUpdated);
    event.computer = computer;
    d->handleEvent(event); */
}

void LoginLauncher::onTimeout()
{
    Q_D(LoginLauncher);
    Event event(Event::Timedout);
    d->handleEvent(event);
}

void LoginLauncher::onQuitAppCompleted(QVariant error)
{
    Q_D(LoginLauncher);
    Event event(Event::AppQuitCompleted);
    event.errorMessage = error.toString();
    d->handleEvent(event);
}

} // namespace
