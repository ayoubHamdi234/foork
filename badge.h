#ifndef BADGE_H
#define BADGE_H

#include <QObject>
#include <QSerialPort>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QRegularExpression>

class Badge : public QObject
{
    Q_OBJECT
public:
    explicit Badge(QObject *parent = nullptr);
    ~Badge();

    bool connectArduino();                  // Connexion série
    void sendToArduino(const QString &msg); // Envoi vers Arduino

signals:
    // uid, accès autorisé ?, nom, prénom
    void badgeProcessed(const QString &uid,
                        bool granted,
                        const QString &nom,
                        const QString &prenom);

private slots:
    void readSerial();                      // Lecture série

private:
    QSerialPort *serial;

    QString detectArduinoPort();            // COM7
    void processUID(const QString &uid);    // Vérification DB
    QString cleanField(const QString &s);   // Sécurise le protocole
    QString normalizeUID(const QString &line); // Nettoyage UID reçu
};

#endif // BADGE_H
