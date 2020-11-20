#include "backendapi.h"

#include <QDebug>
#include <QUuid>
#include <QtNetwork/QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QXmlStreamReader>
#include <QSslKey>
#include <QImageReader>
#include <QtEndian>
#include <QNetworkProxy>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkCookie>
#include <backend/nvhttp.h>

#define REQUEST_TIMEOUT_MS 5000

BackendAPI::BackendAPI(QString baseUrl,
                       QString sessionId,
                       QObject *parent)
    : QObject(parent),
    m_SessionId(sessionId)
{
    m_BaseUrl = baseUrl;

    // Never use a proxy server
    QNetworkProxy noProxy(QNetworkProxy::NoProxy);
    m_Nam.setProxy(noProxy);

    connect(&m_Nam, &QNetworkAccessManager::sslErrors, this, &BackendAPI::handleSslErrors);
}

QString BackendAPI::getBasicAuthHeader(QString username, QString password)
{
    QString concatenated = username + ":" + password;
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;

    return headerData;
}

bool BackendAPI::login(QString userName, QString password, QString &sessionId)
{
    QString answer;

    //return loginMock(userName, password, sessionId);

    try {
        QString postBody = QString("{\n\"email\": \"%1\",\n\"password\":\"%2\"\n}\n\n").arg(userName).arg(password);
        QMap<QString,QString> headers;
        QList<QNetworkCookie> cookies;
        //headers["content-type"] = "application/json";
        answer = openConnectionToString(m_BaseUrl,
                                        "api/v1/auth/login",
                                        nullptr,
                                        headers,
                                        REQUEST_TIMEOUT_MS,
                                        true,
                                        postBody.toUtf8(),
                                        &cookies
                                       );

        qDebug() << Q_FUNC_INFO << "Ack:" << answer;
        sessionId = "";

        QJsonDocument jsonDoc = QJsonDocument::fromJson(answer.toUtf8());
        QJsonObject msgObj = jsonDoc.object();

        for (auto c : cookies)
        {
            if (c.name() == "access_token")
            {
                sessionId = c.name() + "=" + c.value();
                qDebug() << Q_FUNC_INFO << "Cookie" << sessionId;
                break;
            }
        }
        if (sessionId != "")
        {
            return true;
        }else{
            return false;
        }
    } catch (...) {
        qWarning() << Q_FUNC_INFO << "Exception detected";
    }
    return false;
}

void BackendAPI::setSessionId(QString sessionId)
{
    m_SessionId = sessionId;
}

bool BackendAPI::getMyCredentials(QString &myId,
                                  QString &myCert,
                                  QString &myKey,
                                  QString &myServerIP,
                                  QString &myServerName,
                                  QString &myServerUuid,
                                  QString &myServerCert)
{
    //return getMyCredentialsMock(myId, myCert, myKey, myServerIP, myServerName, myServerUuid, myServerCert);

    QString answer;

    qDebug() << Q_FUNC_INFO << "getMyCreadentials";

    try {
        QMap<QString,QString> headers;

        headers["Cookie"] = m_SessionId;
        answer = openConnectionToString(m_BaseUrl,
                                        "api/v1/connections/myinfo",
                                        nullptr,
                                        headers,
                                        REQUEST_TIMEOUT_MS,
                                        false,
                                        QByteArray(),
                                        nullptr);

        qDebug() << Q_FUNC_INFO << "Ack:" << answer;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(answer.toUtf8());
        QJsonObject msgObj = jsonDoc.object();
        if (msgObj["status"] == "ok")
        {
            myId = msgObj["id"].toString();
            myCert = msgObj["cert"].toString();
            myCert = myCert.replace("\\n","\n");
            myCert = myCert.replace("\\\\","\\");
            myKey = msgObj["key"].toString();
            myKey = myKey.replace("\\n","\n");
            myKey = myKey.replace("\\\\","\\");
            myServerIP = msgObj["ip"].toString();
            myServerName = msgObj["serverName"].toString();
            myServerUuid = msgObj["serverUuid"].toString();
            myServerCert = msgObj["serverCert"].toString();
            myServerCert = myServerCert.replace("\\n","\n");
            myServerCert = myServerCert.replace("\\\\","\\");

            return true;
        }else{
            qWarning() << Q_FUNC_INFO << "Ack: status is not ok";
        }
    } catch (...) {
        qWarning() << Q_FUNC_INFO << "Exception detected";
    }
    return false;
}

bool BackendAPI::pushStats(QString &stats)
{
    QString answer;

    qDebug() << Q_FUNC_INFO << stats;

    return true;  // Mocked

    try {
        QMap<QString,QString> headers;
        headers["Cookie"] = m_SessionId;
        answer = openConnectionToString(m_BaseUrl,
                                        "api/v1/connections/kpi",
                                        nullptr,
                                        headers,
                                        REQUEST_TIMEOUT_MS,
                                        true,
                                        stats.toUtf8(),
                                        nullptr
                                       );

        qDebug() << Q_FUNC_INFO << "Ack:" << answer;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(answer.toUtf8());
        QJsonObject msgObj = jsonDoc.object();
        if (msgObj["status"] == "ok")
        {
            return true;
        }else{
            qWarning() << Q_FUNC_INFO << "Ack: status is not ok";
        }
    } catch (...) {
        qWarning() << Q_FUNC_INFO << "Exception detected";
    }
}

bool BackendAPI::getMyCredentialsMock(QString &myId,
                                      QString &myCert,
                                      QString &myKey,
                                      QString &myServerIP,
                                      QString &myServerName,
                                      QString &myServerUuid,
                                      QString &myServerCert)
{
    QString answer;

    QString id = "0123456789ABCDEF";
    QString cert = "-----BEGIN CERTIFICATE-----\nMIICvzCCAaegAwIBAgIBADANBgkqhkiG9w0BAQsFADAjMSEwHwYDVQQDDBhOVklE\nSUEgR2FtZVN0cmVhbSBDbGllbnQwHhcNMTkxMDAyMTI0ODQ5WhcNMzkwOTI3MTI0\nODQ5WjAjMSEwHwYDVQQDDBhOVklESUEgR2FtZVN0cmVhbSBDbGllbnQwggEiMA0G\nCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC51xwoOzjWyz6kBd8JhUBXXX80N77p\nnsouQut1si8BqbPHLW+8rZ1txVnlD4Q1aMMajaLjclAuYvYQk+t+gknSZrsrcMM6\nCW1ALx8rX5OuGqHOuzbr0QqsVp3Aps+wH1LrYmcoipOgXJGQmf9ThvHmx+sdrmi4\nxkb/1OHS3KguJAZR1jQbkjesbeWRdKn4Vo6mTZm99IK9EnOGzmNho5L1FvxVS8bd\nUjwwwScCCrVw08s2D8K7gpySpDAvg/I+LImQo99i+aP7eraeCMj1UY/oLIY8PqCA\nuzuaW6W/8TYFRKNZvhtAPbVkDdqXrY7mDQKAvhkVI+Sd6YOMtpGxzIDzAgMBAAEw\nDQYJKoZIhvcNAQELBQADggEBAKSbIVrEAbjIxD5mriFDk8BF8+KP9rFRGTOQVU8D\naQhnRxbE29IX0FdK8FxGnmiAS5lN297jz+5J6KdKbph025H2iSNECzd4LSNsnOiA\neCQNuByQQmMhOWXcJYPsYWC9nNwKVTr5P5pEps3ne2D6o5ubOW3jzFzpuBIbUmoP\nVFz4iqApf6wdOLV/NdkyJAB48VIDlvan8KUJNV90npKH0977QFNIpO3hItyWZNtn\nqpQ0yVgRh+/MimEnvuj2/6Ev9R7zIWgt54lUfwpnGyfkLVGerAao7kk3cgDSBCHs\nhcymujog/jkw/1tvpezfXzP3y7Fuvm5kltjni2mPXWveqK0=\n-----END CERTIFICATE-----\n";
    QString key = "-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC51xwoOzjWyz6k\nBd8JhUBXXX80N77pnsouQut1si8BqbPHLW+8rZ1txVnlD4Q1aMMajaLjclAuYvYQ\nk+t+gknSZrsrcMM6CW1ALx8rX5OuGqHOuzbr0QqsVp3Aps+wH1LrYmcoipOgXJGQ\nmf9ThvHmx+sdrmi4xkb/1OHS3KguJAZR1jQbkjesbeWRdKn4Vo6mTZm99IK9EnOG\nzmNho5L1FvxVS8bdUjwwwScCCrVw08s2D8K7gpySpDAvg/I+LImQo99i+aP7erae\nCMj1UY/oLIY8PqCAuzuaW6W/8TYFRKNZvhtAPbVkDdqXrY7mDQKAvhkVI+Sd6YOM\ntpGxzIDzAgMBAAECggEAT9RPjBikeeAksGC1RmmvEdhf5BZuM/y57NViP9SizJwR\nVeX2sZ4CmjzEONlJeYffB3EAH6PjPYnVGZnw9w8Qlwj6Ldbqheu2unODeCY+UfOu\nvhc9qF7LruwmJ7OAU6+g9uv2VDvy3lflT7BXNZIqJ3CJVt6srXK+3Padau7Ob8Ld\nvTZg0JSsHGm7bNvEX4dbObATTJs2VxthcMi3IlurmwIFh2db7OaTyvxdJl8DWsV8\nDSx1VyjXADxIvLUW5RPddopleBTaFrTyJRqxfLjGBKoHzLHrcU15z/6rYUhLHo6s\ne5xlzwgJ3TTLig52aMfbXXsF8yiQNPwS5ub8SjQSwQKBgQDtxqnQ43WBe5BWKihM\nsWXwQuLl+B/wfWDJdw/Ra5fSDaqxKE+J5XUoKearYHoRA/uM2wijPtzlgPKsM+6f\nN6NUfvenTNRbKC9gP8M88FMaVD+3upQ58927+q9QaefIlpYySZom9t/NkAJCR6C6\nVFnMp1Ky+W6mMpu/iQ78XzmTbQKBgQDIFW4GLx1AQbCaeVjdpbJSSNVza8gtZil5\npXCoaYtMl5E+pm4O8+mVpj4leQGtSMEMNURamRu6n72Cl23ct7mPw4rv00To++V7\nhhfyYZTgimUEtx6migULin/H6h5EHzlvcRr+S2xf0JDKJ81j0RRKiK8Yk1GxrJet\nufGclidJ3wKBgB6BVlgOVoz+JU5oqjLsr39blXCbnL1l2H0AYW8ktp2kUznXSh0O\n6zDz7zwdbIuyTxuLHliTQBPRr1CYeQzEPpggkfVMzhHD3hAjHhE7Y+4E0QfpUAr/\nVns/di6C59G8QBjDiJtnIN9mkmOefOhq2fp/nQSJK8D2zTSNiPan4OMhAoGBAKUL\naFKZkptqlG8YIgHTqKDPi6NGCT3Jw/SgT6ncRhfL/vea+bZD5S6YjzMB+iwik3uq\nhNPm8ESleAG9P9aNhvfb6UOjFnjJMKcQGbjKXbBK+MFG/HWL7FV0zaruqECxMQOI\nXSfet2rh9E2NP5NS6FYDIcw32W+iWwvnEjKEeawpAoGBAOags790zSWNpiBEOshs\nwPH+Pg+bwq3eaE9+/GOQE+OlZjfrLO4GaCYgd69TQUyBiZkPjpFga3pe540Nfu3g\nnuZLFVZBrA39M8nb0VNX6e7XWwNnAT5VzIT5ozF5yyxYt8HmQAf6mv+uFt+JPRHn\nMD9hw86OeliMKiRlvektIHsK\n-----END PRIVATE KEY-----\n";

    cert = "-----BEGIN CERTIFICATE-----\nMIIDETCCAfmgAwIBAgIBADANBgkqhkiG9w0BAQsFADAjMSEwHwYDVQQDDBhOVklE\nSUEgR2FtZVN0cmVhbSBDbGllbnQwHhcNMjAwODE0MjEyNzAwWhcNMzAwODEyMjEy\nNzAwWjAjMSEwHwYDVQQDDBhOVklESUEgR2FtZVN0cmVhbSBDbGllbnQwggEiMA0G\nCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC+gfiNB+7UeBctp91PQ5a5QJze5NQ1\niMvrdOLjL21vZu5KQai/FzJnEpIN1R4QuIAXrEDnYx0QctbUBQuheTRuqeAeG0//\n7CFwWFI32zJKTbLPcK9E3mWL/zzojGMx9DlCC7kstOgeruDm/6Pb2R1K4In0MgQU\n/pTMi6YZHafgJKtFfSy5XiYQe4ybl1Ela6sO0ImcFdvz6WD0AP0ZNX+o7nTXPLsJ\nEha31CsNGWRvUkzlEYBL42umfJgGwF/dWljj4fdJfQQkeZ9yC75SId/ZT3ZcRrOF\n8mxJjWpVdPzkDF+GYE/a/KIikHdcY4H9byhSyJqaPp3EPCmDf9woLPu9AgMBAAGj\nUDBOMB0GA1UdDgQWBBTPqHac7elbrlbdmgWH9jQ/w/871DAfBgNVHSMEGDAWgBTP\nqHac7elbrlbdmgWH9jQ/w/871DAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUA\nA4IBAQBIdZvHTaWJsabayiLCtLcdWkX7Fto/IGYfta2feIiAgD7xDy+MP7+y3puy\n5Icf3YwqBdryIOezNW5vqGwet3UQt4R3uKQC1ZuKnfrrlQGZzKTikbKaqvIc03Ec\nBsn1jc5dPpleFB+46NynbgWNC7giIqfWnmYKcK7Tb4ymnaPtUEDCnb7sYrsAwb3x\noI5UdP8Ve1piA5/YyAfzLwiRYYVPGGMc4EQfyLrwBMnbLCBPeFa4rIPpE8y7pU7z\ngcuGOyfIY5eicdcSD3cNwyIfB+NQGOdHkzQf5l69WLQSqpcIGO7BogKoHRlocMBj\njbStoZERo3IBKpjy3dYvJbCPASfk\n-----END CERTIFICATE-----\n";
    key = "-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQEAvoH4jQfu1HgXLafdT0OWuUCc3uTUNYjL63Ti4y9tb2buSkGo\nvxcyZxKSDdUeELiAF6xA52MdEHLW1AULoXk0bqngHhtP/+whcFhSN9sySk2yz3Cv\nRN5li/886IxjMfQ5Qgu5LLToHq7g5v+j29kdSuCJ9DIEFP6UzIumGR2n4CSrRX0s\nuV4mEHuMm5dRJWurDtCJnBXb8+lg9AD9GTV/qO501zy7CRIWt9QrDRlkb1JM5RGA\nS+NrpnyYBsBf3VpY4+H3SX0EJHmfcgu+UiHf2U92XEazhfJsSY1qVXT85AxfhmBP\n2vyiIpB3XGOB/W8oUsiamj6dxDwpg3/cKCz7vQIDAQABAoIBAGbFwOt8Oxh+DKsB\nov9uy/H2bGpGckDLIo9MSFYdMOFnWufOUhV8kyFNwGMb9JM6pUegEoeBTZ2hBkns\nyuB6fZGxrQkw3NCId2WfEAO4CYJSNkN4W+VAQPHsaNRfX+gEA5ugrW3zzkE3QHb9\ntE0W7JmHVoTZMqCM0oMiVyG3gOgGkeOuLOCeJazmCQYEUuHr2eTeBengHNAF3gTx\nxp7BKtk0PifwoEl/NLZRXhujOoQkLeNtpwv5Z50kHyxD6mL72Zan23BAGU198T6l\nulxxXkosC1md8ukG3fz7a4ImKTo+9jO9JnnWqarnsO5iIicmDSkHyDFjkj5pycNm\njRnknxUCgYEA3iTaZgY50vJYMmYmLzrSUkDasB4gO89NWl7i9cPz8Tp/tEmMj+on\nTCmk1UD3fRTptni992S7qiL3glxYnTIKtllP/x7pqCuRI4E312aZ3P9H6JwGh7Ad\nWZpN+9hwYlnrj7JamUx6PQh2/H9osiQHhSjp3KzwMKRZ7yH8KFArUwMCgYEA24rM\n9RB7wXfEcKIeAiJzFm4FEG3VfFfiWxARe8E0QCXfGTWCwJIk9JWvkxsGtpc0cjFc\nNo44UZaM+2wKDyep4XIxvOgOrmmJpT7XPXCMK1iGTQcMqc3tSyKgrgeldM35NzU4\niPfpzhNZNTD3PttHWv5U0RPos8uXww7FZSHx2j8CgYEAtDfRttN2NdzGEJ0ufUKL\nPo++2wKVw+/6IUa5egju7tU2pVzF3Dtqhi+Cuj9qiN9ee9qYwwvF89FKW0fv3Bes\n+SKb861wgi5WISfD1cw3J0MzY1KxOYN3TCoS+i4tfpzUrk9TkOqqgLlNTqtOlLcG\nItF+aLkkY1HyZD5+A3aGr2ECgYEAxxq+fpKQaglgknW0mWL81R66YQf7UWWy17te\n38l8xaCTEJ3DEzp3YYpPTS55jCbdXaL+akvZL8VF3s9A4DWsj9Ws3hHnHq6AMukm\n84Wa2cTTKBB4n5cu2bFx3+L3X1Nd7X/K4g1UcZyCjwb1dIAR2qhF68gOhuDhpflD\ndFCwsqkCgYBWCZgqLoUwDTmktzHafNv2HCfKwnzMVr5e5NiHF/677Tn0dwCI3aoh\nk1K8vem4DBg/X5H68AHhYR84xGy5p3a/fuqCOGJpc6zONjRW+Orb7VZKkBMRrjZg\n+8sKaMXt63Cik0R3G+QZK6IHxhtJsWlzt71Ore/xQnbC4KXpFCYunw==\n-----END RSA PRIVATE KEY-----\n";

    cert ="-----BEGIN CERTIFICATE-----\nMIIDETCCAfmgAwIBAgIBADANBgkqhkiG9w0BAQsFADAjMSEwHwYDVQQDDBhOVklE\nSUEgR2FtZVN0cmVhbSBDbGllbnQwHhcNMjAxMDAxMDk0MTUzWhcNNDAwOTI2MDk0\nMTUzWjAjMSEwHwYDVQQDDBhOVklESUEgR2FtZVN0cmVhbSBDbGllbnQwggEiMA0G\nCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDHun9ifPH0fKOBPHCGL6hMcRBODiXA\nhYOe7GOAmSfPV3XM79cYJYeFxBremKsHev6tArOcuFjm8zoOi/0S+5yWfFXwRxRw\nv3kUCdjw/8kbYgcnTIiY+a+muBbclUIyAdRsIcmuDKedta/x5tfIjHnYST2EdHGe\nvnWuqh0ux7IQif3qGiU17qr/lYJZQMO58zfaVKI4YHrFKFRc7wSF5CzM2h1C0PPb\nETKWsFNxenBjXV3dTwczBrz/nT1KIMxazECP+ph2sLMUA7ReWeTv4vf18xVAFbu9\nURHOSYtwiCppEHQ3Ee3Ru8hqFPr3wPNm9HHFwf2epJh7UY4gNthLbDynAgMBAAGj\nUDBOMB0GA1UdDgQWBBT4OlN5NXr3WNWk+cEY8KAUOMsKDDAfBgNVHSMEGDAWgBT4\nOlN5NXr3WNWk+cEY8KAUOMsKDDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUA\nA4IBAQBYOvyVBJMIboy9MorOfJSh17GKhiRQwdgmuqFCHvZc8zC0R1GGytDvw7G2\nZ+95rYpyEfrYxWIj16bsaQqDc5qXZpbVgGu1zMehwqWmRuWQT7EB9m3UhoC/vsBB\nN04HObZyFH2AqUHHaQ0wHNxZqiIpyVy/eF4fHWkDLVXMCj2i9HO5+CcRb9prxqsS\n7sx9fGW95TVuXieOY+fxpbbxRdGk/ADlDDHB+VLVtMCzyOseEIK/DP68eZ2Plmh7\nAKenTcBQGE4mUyAZ/zVr6kv19CSUWRbIdLqIIK1wXXjpEHPm3MguZtzX2gMgtRpG\n5jFp4u+x6J6e9EETJPN3RDcR3gHy\n-----END CERTIFICATE-----\n";
    key = "-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQEAx7p/Ynzx9HyjgTxwhi+oTHEQTg4lwIWDnuxjgJknz1d1zO/X\nGCWHhcQa3pirB3r+rQKznLhY5vM6Dov9EvuclnxV8EcUcL95FAnY8P/JG2IHJ0yI\nmPmvprgW3JVCMgHUbCHJrgynnbWv8ebXyIx52Ek9hHRxnr51rqodLseyEIn96hol\nNe6q/5WCWUDDufM32lSiOGB6xShUXO8EheQszNodQtDz2xEylrBTcXpwY11d3U8H\nMwa8/509SiDMWsxAj/qYdrCzFAO0Xlnk7+L39fMVQBW7vVERzkmLcIgqaRB0NxHt\n0bvIahT698DzZvRxxcH9nqSYe1GOIDbYS2w8pwIDAQABAoIBACd5o926ZjP1Qnez\nk2XiG3hoTFmxECMaAGazwALVwnmHhr/nk3KxUDTPCqR+Nid3VsipJQjPorqazhm7\n71wGIO/ZST8BPXf/UBX20ErpszzXRiStSr45c08XSWV1lqy9kdeBYhQlaieQlmLQ\nRkvb/+UQrLj2LtK6QBYyhywMyhPcaMAklzlW6bd856UYQiwsBYeaQ/ePozd6lSOv\nEzBrPFRKU3LXZzv/540EzCHfj6xDhGmDL+LM0XtcZ4O0CQ+zYbyKkABXlFPRie4r\nNE43Ufg3Z5nyA1QEbhonXOM3fmcB/W2/amgCWx6Bhl5MTFgkWpXaVRi/vZ7QPU9Q\nT/9KogkCgYEA6ylGZwztDTsZnNI3FZN8+tSVziPZ7oSbROJzZkqtW31GGml/75V4\nSbroXPIAAjfJ3nLvUtNaHV5fY5RQMlsBhekux+XW+mbjkDOKYHIJVPWJup0gBSlA\nrBkC8ysB4LKiQ5gXmhz7kxKvP+VbxdhJP3ICqzvwua3zVS0Vq4uliQsCgYEA2W1q\n1Fs0HJBayieZu6j/Z57rKsR4a9FI9ktNc9QJ9XfYAO2yImJFkx/1jg2Q5BWTSWaM\nL1gXQ2OKmHfJCWSZHQl9jytfMU5wpN/nPpQwyo7cgFbCJOYq4N1Et2hmaqEO3FVw\noTscDEt3lHT7w98sKRfngKaiRfVGG7VL1q+OtFUCgYEAnGrv8UufwdU1zCHnkepw\nj0cn/d3K7+Q0gzU9HSU++kPxhU/3UR1SzXgNZwVxDAiftd8tYSJEJpTHTi0wys3N\nzYy1GTHANP6LDze5Kz2vB1vrKL2qtsQhyFc6pxRMQ3GOjPd1e4UbsVAUUaP1zzt2\nPUVQkSVlkZwq5tT+PQS+LtECgYAqBWWSKRZHNvb0/vZNOHo0EpisqVAEtmv9fcw4\nSJCsKj9K3qWA07FSn+ibTa/lBO50Tydktwe39HMCyJPIBvM4k276zo0VQG2wz70n\ncQiE/VtMSng3HWzIDLuNHtY8YWg32SNtZbkj/ynbPUizMFyejIonaQ0PrDmifUVs\n9n0ECQKBgQCs0UdKFq4d3bnUWxIJKbwzPYeeIzbY/8oz4C+GBXF3drc2cwSUx5sV\nL8gZXqG37u7bO8i26CMraxKoFfoXnMaU9NW8ciaN0wjwg/QguVUPLH47UcXbJ+6G\nGU4SkvAaQNYZKZrz7nfbJEk2u5M/2T2yXfS2ZUxiW5nJEsHZOTXtOA==\n-----END RSA PRIVATE KEY-----\n";

    qDebug() << Q_FUNC_INFO << "getMyCredentialsMock" << m_SessionId;

    /* Following instructions are to generate a valid Certificate with OpenSSL Tool
     * The Certifica should contains a specific CN
     *
     * openssl genrsa -out user.key 2048
     * openssl req -new -x509 -sha256 -key user.key -out user.crt -subj "/CN=NVIDIA GameStream Client" -days 3650 -set_serial 0
     */

    try {
        if (true || m_SessionId == "access_token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJMdWRpY28iLCJzdWIiOiI1ZjNmYjQ4ODNjMmIyZjRlZjBiMGM5NzMiLCJpYXQiOjE2MDAyNjQxNjYsImV4cCI6MTYzMTgwMDE2Nn0.-SOJk0C0quUTu3SfkZTUKe2kZLJqq5sW-02VDFwCP1g") {
            answer = QString("{"
                     "\"status\":\"ok\","
                     "\"id\":\"%1\","
                     "\"cert\":\"%2\","
                     "\"key\":\"%3\","
                     "\"ip\":\"192.168.1.48\","
                     "\"serverName\":\"XiaoSunshine\","
                     "\"serverUuid\":\"D0CC273E-DDA8-532E-9F1E-83514117D963\","
                     "\"serverCert\":\"-----BEGIN CERTIFICATE-----\nMIICvzCCAaegAwIBAgIBADANBgkqhkiG9w0BAQsFADAjMSEwHwYDVQQDDBhTdW5z\naGluZSBHYW1lc3RyZWFtIEhvc3QwHhcNMjAwNjI1MTMwMTIwWhcNNDAwNjIwMTMw\nMTIwWjAjMSEwHwYDVQQDDBhTdW5zaGluZSBHYW1lc3RyZWFtIEhvc3QwggEiMA0G\nCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC3JLGY36rm5jmAgpuaWGg8pUZQgFMo\nq5ZXeiJ7+0Pkgd9lV3/25FZlE8h1dBiu8YGeq4BHR1P3S/3hYwlnDWdLpMKK3j/T\nUom6zVtOm+9ct6I1YKk+8YZT2ShbV5WzesfTBE5mbLCmrPqPHNYxMN+PfW0DVifR\nkXRyfWtc5+8antpAbaLod059K6lDxxz1d7gqge/ji4OdOQklfHhPeljrdcfPztmT\nvjzi8he5l0wpxN7RVLLFEn7h2vbTWRDWLskafoL1XbSbob3I9k6ztgrC+ORq3fZg\nSOrbliBgzeVea83SIFjHdmKji/0LTWFGo5rmyv9PUzeTxjnUVHvpI1GhAgMBAAEw\nDQYJKoZIhvcNAQELBQADggEBAGtzwwF27klbPVRcjxttgf8Pp19q9VtATnNEFopm\nsNLR0wiKJ0+tmGaNWzxe2ad+nTPpuAPnYoCTysN7qsQupFQa5PCThYfN8vqzORSQ\nZHXSxX4YehRVd1h9bsNTyeGWonYIkRh86mS5u+C3rYT3Ucxz0rUj45m6p0CxEVBh\nk3gkBGQLa20ccyeFB06G8nm1O6ApkaBYhh8Op2FEqaiV49U+a0EYfyU02dbhfzCO\noPIbt+5Wkjq0oJ7ECyrtvALGw5b1wsq/ETy4HaZaWRBsUj64S/zoIOrW6E4w92Ho\nydAmZOs4oeGJ+KaotbwOsFrIFRDyctUAchg+j4sRSe7UD5k=\n-----END CERTIFICATE-----\n\""
                     "}").arg(id).arg(cert).arg(key);
        }

        qDebug() << Q_FUNC_INFO << "Ack:" << answer;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(answer.toUtf8());
        QJsonObject msgObj = jsonDoc.object();
        qDebug() << Q_FUNC_INFO << msgObj.keys();
        if (msgObj["status"] == "ok")
        {
            myId = msgObj["id"].toString();
            myCert = msgObj["cert"].toString();
            myKey = msgObj["key"].toString();
            myServerIP = msgObj["ip"].toString();

            myServerName = msgObj["serverName"].toString();
            myServerUuid = msgObj["serverUuid"].toString();
            myServerCert = msgObj["serverCert"].toString();
            return true;
        }else{
            qWarning() << Q_FUNC_INFO << "Ack: status is not ok";
        }
    } catch (...) {
        qWarning() << Q_FUNC_INFO << "Exception detected";
    }
    return false;
}

bool BackendAPI::loginMock(QString userName, QString password, QString &sessionId)
{
    QString answer;

    qDebug() << Q_FUNC_INFO << "loginMock" << userName << " " << password;

    try {
        if (userName == "Xiao" && password == "mg2019") {
            sessionId = "access_token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJMdWRpY28iLCJzdWIiOiI1ZjNmYjQ4ODNjMmIyZjRlZjBiMGM5NzMiLCJpYXQiOjE2MDAyNjQxNjYsImV4cCI6MTYzMTgwMDE2Nn0.-SOJk0C0quUTu3SfkZTUKe2kZLJqq5sW-02VDFwCP1g";
        }else{
            sessionId = "";
            return false;
        }
        return true;
    } catch (...) {
        qWarning() << Q_FUNC_INFO << "Exception detected";
    }
    return false;
}

QString BackendAPI::openConnectionToString(QUrl baseUrl,
                                           QString command,
                                           QString arguments,
                                           QMap<QString,QString> extraHeaders,
                                           int timeoutMs,
                                           bool isAPost,
                                           const QByteArray & postBody,
                                           QList<QNetworkCookie> *cookies)
{
    QNetworkReply* reply = openConnection(baseUrl, command, arguments,
                                          extraHeaders, timeoutMs,
                                          isAPost,
                                          postBody);
    QString ret;

    QTextStream stream(reply);
    stream.setCodec("UTF-8");
    ret = stream.readAll();

   if (cookies != nullptr) {
       QVariant cookieVar = reply->header(QNetworkRequest::SetCookieHeader);
       if (cookieVar.isValid()) {
           QList<QNetworkCookie> rawCookies = cookieVar.value<QList<QNetworkCookie> >();
           for (QNetworkCookie c: rawCookies) {
               qDebug() << "Cookie" << c;
               cookies->push_back(c);
           }
       }
   }

    delete reply;
    return ret;
}

void BackendAPI::handleSslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    bool ignoreErrors = true;

    if (ignoreErrors) {
        reply->ignoreSslErrors(errors);
    }
}

QNetworkReply *BackendAPI::openConnection(QUrl baseUrl,
                                          QString command,
                                          QString arguments,
                                          QMap<QString,QString> extraHeaders,
                                          int timeoutMs,
                                          bool isAPost,
                                          const QByteArray & postBody)
{
    // Build a URL for the request
    QUrl url(baseUrl);
    url.setPath("/" + command);
    if (arguments != nullptr)
    {
        url.setQuery(arguments );
    }
    QNetworkRequest request(url);
    qDebug() << "REQUEST TO BACKEND:" << url <<  isAPost << postBody << extraHeaders ;

    for (auto i = extraHeaders.begin(); i != extraHeaders.end(); ++i)
    {
        qDebug() << "SET RAW HEADER:" << i.key() << ":" << i.value();
        request.setRawHeader(i.key().toLocal8Bit(), i.value().toLocal8Bit());
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) && !defined(QT_NO_BEARERMANAGEMENT)
    // HACK: Set network accessibility to work around QTBUG-80947.
    // Even though it was fixed in 5.14.2, it still breaks for users attempting to
    // directly connect their computers without a router using APIPA and in some cases
    // using OpenVPN with IPv6 enabled. https://github.com/moonlight-stream/moonlight-qt/issues/375
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    m_Nam.setNetworkAccessible(QNetworkAccessManager::Accessible);
    QT_WARNING_POP
#endif

    QNetworkReply* reply = 0;
    if (!isAPost)
    {
        reply = m_Nam.get(request);
    }else{
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        reply = m_Nam.post(request, postBody);
    }
    // Run the request with a timeout if requested
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), &loop, SLOT(quit()));
    if (timeoutMs) {
        QTimer::singleShot(timeoutMs, &loop, SLOT(quit()));
    }
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    // Abort the request if it timed out
    if (!reply->isFinished())
    {
        qWarning() << command << "reply is still ongoing, let abort it";
        reply->abort();
    }

    m_Nam.clearAccessCache();

    // Handle error
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << command << " request failed with error " << reply->error();

        if (reply->error() == QNetworkReply::SslHandshakeFailedError) {
            QtNetworkReplyException exception(QNetworkReply::SslHandshakeFailedError, "Ssl handshake failure");
            delete reply;
            throw exception;
        }
        else if (reply->error() == QNetworkReply::OperationCanceledError) {
            QtNetworkReplyException exception(QNetworkReply::TimeoutError, "Request timed out");
            delete reply;
            throw exception;
        }
        else {
            QtNetworkReplyException exception(reply->error(), reply->errorString());
            delete reply;
            throw exception;
        }
    }

    return reply;
}
