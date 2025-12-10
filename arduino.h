#ifndef ARDUINO_H
#define ARDUINO_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

class Arduino : public QObject
{
    Q_OBJECT
public:
    explicit Arduino(QObject *parent = nullptr);
    ~Arduino();

    // detecte + ouvre le port (retourne true si ok)
    bool connectArduino();

    // envoie une chaîne complète vers l'Arduino (ajoute '\n')
    void sendToArduino(const QString &msg);

signals:
    // pour logging / interface : id et si accès autorisé (true=OK)
    void idProcessed(const QString &id, bool granted);

private slots:
    // appelé automatiquement par readyRead()
    void readSerial();

private:
    QSerialPort *serial;
    bool arduinoAvailable();

    QString detectArduinoPort();
    void processIDString(const QString &raw); // traite "ID:xxx"
    QString escapeField(const QString &s); // optionnel, nettoyage des champs avant envoi
};

#endif // ARDUINO_H
