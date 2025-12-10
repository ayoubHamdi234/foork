#include "badge.h"
#include <QSerialPortInfo>

Badge::Badge(QObject *parent) :
    QObject(parent),
    serial(new QSerialPort(this))
{}

Badge::~Badge()
{
    if (serial->isOpen())
        serial->close();
}

QString Badge::detectArduinoPort()
{
    // SIMPLE VERSION: force the COM port
    return "COM7";

    // If you want auto-detect later, I can add it for you.
}

bool Badge::connectArduino()
{
    QString portName = detectArduinoPort();
    if (portName.isEmpty()) {
        qDebug() << "[QT] No Arduino port detected.";
        return false;
    }

    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        qDebug() << "[QT] Cannot open" << portName << ":" << serial->errorString();
        return false;
    }

    connect(serial, &QSerialPort::readyRead, this, &Badge::readSerial);

    qDebug() << "[QT] Arduino connected on" << portName;

    qDebug() << "Trying to open port:" << portName;

    return true;
}

void Badge::readSerial()
{
    while (serial->canReadLine()) {
        QString line = QString::fromUtf8(serial->readLine()).trimmed();
        qDebug() << "[QT] Received from Arduino:" << line;

        if (line.startsWith("ID:")) {
            QString uid = line.mid(3).trimmed();
            processUID(uid);
        }
    }
}

void Badge::processUID(const QString &uid)
{
    // ===== Query Oracle EMPLOYES =====
    QSqlQuery q;
    q.prepare("SELECT NOM, PRENOM FROM EMPLOYES WHERE RFID_UID = :uid");
    q.bindValue(":uid", uid);

    if (!q.exec()) {
        qDebug() << "[QT] SQL Error:" << q.lastError().text();
        sendToArduino("NOK");
        emit badgeProcessed(uid, false, "", "");
        return;
    }

    if (!q.next()) {
        qDebug() << "[QT] Access denied for UID:" << uid;
        sendToArduino("NOK");
        emit badgeProcessed(uid, false, "", "");
        return;
    }

    // ===== Access Granted =====
    QString nom = q.value(0).toString();
    QString prenom = q.value(1).toString();

    QString msg = QString("OK:%1:%2").arg(nom).arg(prenom);
    sendToArduino(msg);

    qDebug() << "[QT] Access granted to" << nom << prenom;

    emit badgeProcessed(uid, true, nom, prenom);
}

void Badge::sendToArduino(const QString &msg)
{
    if (!serial->isOpen()) {
        qDebug() << "[QT] Port not open. Cannot send to Arduino.";
        return;
    }

    QByteArray out = msg.toUtf8();
    if (!out.endsWith('\n'))
        out.append('\n');

    serial->write(out);
    serial->flush();

    qDebug() << "[QT] Sent to Arduino:" << msg;
}
