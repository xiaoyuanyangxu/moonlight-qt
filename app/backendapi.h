#ifndef BACKENDAPI_H
#define BACKENDAPI_H


#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QObject>
#include <QSslCertificate>

class BackendAPI : public QObject
{
    Q_OBJECT
public:
    explicit BackendAPI(QString baseUrl,
                        QString sessionId,
                        QObject *parent = nullptr);

    static
    QString getBasicAuthHeader(QString username, QString password);

    bool login(QString userName, QString password, QString& sessionId);

    void setSessionId(QString sessionId);

    bool getMyCredentials(QString & myId,
                          QString & myCert,
                          QString &myKey,
                          QString &myServerIP,
                          QString &myServerName,
                          QString &myServerUuid,
                          QString &myServerCert);


    QString
    openConnectionToString(QUrl baseUrl,
                           QString command,
                           QString arguments, QMap<QString, QString> extraHeaders,
                           int timeoutMs, bool isAPost, const QByteArray &postBody, QList<QNetworkCookie> *cookies);

    QUrl m_BaseUrl;

private:
    bool getMyCredentialsMock(QString & myId,
                              QString & myCert,
                              QString &myKey,
                              QString &myServerIP,
                              QString &myServerName,
                              QString &myServerUuid,
                              QString &myServerCert);
    bool loginMock(QString userName, QString password, QString& sessionId);

private:
    void
    handleSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);

    QNetworkReply *openConnection(QUrl baseUrl,
                   QString command,
                   QString arguments, QMap<QString, QString> extraHeaders,
                   int timeoutMs, bool isAPost, const QByteArray &postBody);

    QString m_SessionId;
    QNetworkAccessManager m_Nam;

};

#endif // BACKENDAPI_H
