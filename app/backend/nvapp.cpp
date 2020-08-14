#include "nvapp.h"

#include <qdebug.h>

#define SER_APPNAME "name"
#define SER_APPID "id"
#define SER_APPHDR "hdr"
#define SER_APPCOLLECTOR "appcollector"

NvApp::NvApp(QSettings& settings)
{

    name = settings.value(SER_APPNAME).toString();
    id = settings.value(SER_APPID).toInt();
    hdrSupported = settings.value(SER_APPHDR).toBool();
    isAppCollectorGame = settings.value(SER_APPCOLLECTOR).toBool();

     qDebug()<< Q_FUNC_INFO
             << "name: "<< name
             << "id:"<<id
             << "hdrSupported:"<< hdrSupported
             << "isAppCollectorGame:" << isAppCollectorGame;
}

void NvApp::serialize(QSettings& settings) const
{
    settings.setValue(SER_APPNAME, name);
    settings.setValue(SER_APPID, id);
    settings.setValue(SER_APPHDR, hdrSupported);
    settings.setValue(SER_APPCOLLECTOR, isAppCollectorGame);
}
