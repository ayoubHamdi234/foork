#ifndef BADGE_H
#define BADGE_H

#include <QObject>
#include <QSerialPort>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

class Badge : public QObject
{
    Q_OBJECT
public:
    explicit Badge(QObject *parent = nullptr);
    ~Badge();

    bool connectArduino();                   // connect to Arduino
    void sendToArduino(const QString &msg);  // send a command

signals:
    void badgeProcessed(const QString &uid, bool granted,
                        const QString &nom, const QString &prenom);

private slots:
    void readSerial();                       // called when data received from Arduino

private:
    QSerialPort *serial;

    QString detectArduinoPort();             // choose port like COM3
    void processUID(const QString &uid);     // database check
};

#endif // BADGE_H
