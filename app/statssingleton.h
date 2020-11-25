#ifndef STATSSINGLETON_H
#define STATSSINGLETON_H

#include <QObject>
#include <QRunnable>
#include <QSslCertificate>
#include <QVariant>
#include "streaming/video/decoder.h"
#include "backendapi.h"


class StatsPushTask: public QObject, public QRunnable
{
    Q_OBJECT

public:
    StatsPushTask(QString baseUrl,
                  QString sessionId,
                  QString stats)
        : m_baseUrl(baseUrl),
          m_sessionId(sessionId),
          m_stats(stats)
    {

    }
    ~StatsPushTask(){

    }

signals:
    void taskCompleted(bool ok);

private:
    void run()
    {
        qDebug() << Q_FUNC_INFO << "Run thread StatsPushTask" << m_baseUrl << " cookie:" << m_sessionId;
        BackendAPI backend(m_baseUrl,
                           m_sessionId);

        bool ok = backend.pushStats(m_stats);

        emit taskCompleted(ok);

    }
    QString m_baseUrl;
    QString m_sessionId;
    QString m_stats;
};


class StatsSingleton: public QObject
{
    Q_OBJECT
private:
    StatsSingleton();

    static StatsSingleton* instance;

public:
    static StatsSingleton* getInstance();

    void initialize(QString baseUrl, QString sessionCookie);

    bool intereded();

    void logStats(VIDEO_STATS &stats);

public slots:
    void onStatsPushFinished(bool ok);

private:
    QString m_baseUrl;
    QString m_sessionCookie;
    QString m_sessionId;
    QString m_subSessionId;
    QString m_deviceId;
    QString m_deviceType;
    QDateTime m_lastStatsPush;

    bool m_pushingStats;
};

#endif // STATSSINGLETON_H
