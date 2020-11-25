#include "statssingleton.h"

#include <QThread>
#include <QThreadPool>
#include <QCoreApplication>
#include <QTimer>
#include <QSettings>
#include <openssl/rand.h>
#include <QDateTime>

StatsSingleton* StatsSingleton::instance = 0;

#define DEVICEID "deviceid"
#define REPORTPERIOD_SEC 5

StatsSingleton::StatsSingleton()
    :m_pushingStats(false)
{
    QSettings settings;


    // Load the unique ID from settings
    m_deviceId = settings.value(DEVICEID).toString();
    if (!m_deviceId.isEmpty()) {
        qInfo() << "Loaded device ID from settings:" << m_deviceId;
    }
    else {
        // Generate a new unique ID in base 16
        uint64_t uid;
        RAND_bytes(reinterpret_cast<unsigned char*>(&uid), sizeof(uid));
        m_deviceId = QString::number(uid, 16);
        qInfo() << "Generated new device ID:" << m_deviceId;
        settings.setValue(DEVICEID, m_deviceId);
    }

    m_sessionId = QDateTime::currentDateTime().toString(Qt::ISODate);

    m_deviceType = "UNKNOWN";
    #ifdef Q_OS_WIN
    m_deviceType = "WINDOWS";
    #endif
    #ifdef Q_OS_UNIX
    m_deviceType = "UNIX";
    #endif
    #ifdef Q_OS_MACOS
    m_deviceType = "MACOS";
    #endif

    m_lastStatsPush = QDateTime::currentDateTime();

}

StatsSingleton *StatsSingleton::getInstance()
{
    if (instance == 0) {
        instance = new StatsSingleton();
    }
    return instance;
}

void StatsSingleton::initialize(QString baseUrl, QString sessionCookie)
{
    qDebug() << Q_FUNC_INFO << baseUrl << " cookie:" << sessionCookie;
    m_baseUrl   = baseUrl;
    m_sessionCookie = sessionCookie;
    m_subSessionId = QDateTime::currentDateTime().toString(Qt::ISODate);
}

bool StatsSingleton::intereded()
{
    return !m_baseUrl.isEmpty();
}

void StatsSingleton::logStats(VIDEO_STATS &stats)
{

    qDebug() << Q_FUNC_INFO << m_baseUrl << m_pushingStats;

    if (m_baseUrl.isEmpty()) return;

    if (m_pushingStats)
    {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsDiff = m_lastStatsPush.secsTo(now);

    if (secondsDiff < REPORTPERIOD_SEC)
    {
        return;
    }
    m_lastStatsPush = now;

    QString clientId ="UNKNOWN";
    QString deviceId = m_deviceId;
    QString deviceType = m_deviceType;
    QString sessionId = m_sessionId;
    QString subSessionId = m_subSessionId;
    float fps=0, netFps=0, decodeFps=0, renderFps=0;
    float videoDataRate=0, averageDataRate=0;
    float networkDropRate=0, averageDropRate=0;
    float averageReceivedTime=0, averageDecodeTime=0, averageQueueTime=0, averageRenderTime=0;
    float jitter=0;
    float controlData=0;

    QString json;

    if (stats.receivedFps > 0) {
        fps = stats.totalFps;
        netFps = stats.receivedFps;
        decodeFps = stats.decodedFps;
        renderFps = stats.renderedFps;
        videoDataRate = stats.videoDataRate;
    }


    if (stats.renderedFrames != 0) {

        networkDropRate = (float)stats.networkDroppedFrames / stats.totalFrames;
        jitter = (float)stats.pacerDroppedFrames / stats.decodedFrames;
        averageDecodeTime = (float)stats.totalDecodeTime / stats.decodedFrames;
        averageRenderTime = (float)stats.totalRenderTime / stats.renderedFrames;
        averageQueueTime =  (float)stats.totalPacerTime / stats.renderedFrames;
     }


    json = QString("{"
                   "\"clientID\": \"%1\","
                   "\"deviceID\":\"%2\","
                   "\"deviceType\":\"%3\","
                   "\"sessionID\":\"%4\","
                   "\"subSessionID\":\"%5\","
                   "\"fps\":%6,"
                   "\"net_fps\":%7,"
                   "\"decode_fps\":%8,"
                   "\"video_data_rate\":%9,"
                   "\"render_fps\":%10,"
                   "\"network_drop_rate\":%11,"
                   "\"average_received_time\":%12,"
                   "\"jitter\":%13,"
                   "\"average_decode_time\":%14,"
                   "\"average_queue_time\":%15,"
                   "\"average_render_time\":%16,"
                   "\"average_drop_rate\":%17,"
                   "\"average_data_rate\":%18,"
                   "\"control_data\":%19}")
                .arg(clientId).arg(deviceId).arg(deviceType)
                .arg(sessionId).arg(subSessionId)
                .arg(fps).arg(netFps).arg(decodeFps)
                .arg(videoDataRate).arg(renderFps).arg(networkDropRate)
                .arg(averageReceivedTime).arg(jitter)
                .arg(averageDecodeTime).arg(averageQueueTime).arg(averageRenderTime)
                .arg(averageDropRate).arg(averageDataRate).arg(controlData);


    StatsPushTask* task = new StatsPushTask(m_baseUrl,
                                            m_sessionCookie,
                                            json);

    connect(task, &StatsPushTask::taskCompleted,
             this, &StatsSingleton::onStatsPushFinished);

    m_pushingStats = true;

    QThreadPool::globalInstance()->start(task);
}

void StatsSingleton::onStatsPushFinished(bool ok)
{
    if (!ok) {
        qDebug() << Q_FUNC_INFO << " error in pushing stats to backend";
    }
    m_pushingStats = false;
}






