#ifndef STATSSINGLETON_H
#define STATSSINGLETON_H

#include <QObject>
#include "streaming/video/decoder.h"

class StatsSingleton: public QObject
{
    Q_OBJECT
private:
    StatsSingleton();

    static StatsSingleton* instance;

public:
    static StatsSingleton* getInstance();

    void initialize(QString baseUrl, QString sessionId);

    bool intereded();

    void logStats(VIDEO_STATS &stats);

private:
    QString m_baseUrl;
    QString m_sessionId;
};

#endif // STATSSINGLETON_H
