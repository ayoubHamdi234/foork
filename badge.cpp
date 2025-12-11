#include "badge.h"
#include <QSerialPortInfo>

Badge::Badge(QObject *parent)
    : QObject(parent),
    serial(new QSerialPort(this))
{
}

Badge::~Badge()
{
    if (serial->isOpen())
        serial->close();
}

/* ================== PORT ARDUINO ================== */
QString Badge::detectArduinoPort()
{
    // ✅ Fixe (simple et fiable)
    return "COM7";
}

/* ================== CONNEXION ================== */
bool Badge::connectArduino()
{
    QString portName = detectArduinoPort();

    if (portName.isEmpty()) {
        qDebug() << "[QT] Aucun port Arduino détecté.";
        return false;
    }

    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        qDebug() << "[QT] Impossible d’ouvrir" << portName
                 << ":" << serial->errorString();
        return false;
    }

    connect(serial, &QSerialPort::readyRead,
            this, &Badge::readSerial);

    qDebug() << "[QT] Arduino connecté sur" << portName;
    return true;
}

/* ================== LECTURE SERIE ================== */
void Badge::readSerial()
{
    while (serial->canReadLine()) {
        QString line = QString::fromUtf8(serial->readLine()).trimmed();
        qDebug() << "[QT] Reçu Arduino:" << line;

        // Format attendu : ID:123456
        if (line.startsWith("ID:")) {
            QString uid = line.mid(3).trimmed();
            processUID(uid);
        } else {
            qDebug() << "[QT] Message ignoré (format inconnu)";
        }
    }
}

/* ================== TRAITEMENT UID ================== */
void Badge::processUID(const QString &uid)
{
    QSqlQuery q;
    q.prepare(
        "SELECT NOM, PRENOM "
        "FROM EMPLOYES "
        "WHERE RFID_UID = :uid"
        );
    q.bindValue(":uid", uid);

    if (!q.exec()) {
        qDebug() << "[QT] Erreur SQL:" << q.lastError().text();
        sendToArduino("NOK");
        emit badgeProcessed(uid, false, "", "");
        return;
    }

    if (!q.next()) {
        qDebug() << "[QT] Accès refusé UID:" << uid;
        sendToArduino("NOK");
        emit badgeProcessed(uid, false, "", "");
        return;
    }

    QString nom = cleanField(q.value(0).toString());
    QString prenom = cleanField(q.value(1).toString());

    QString reply = QString("OK:%1:%2").arg(nom).arg(prenom);
    sendToArduino(reply);

    qDebug() << "[QT] Accès autorisé:" << nom << prenom;
    emit badgeProcessed(uid, true, nom, prenom);
}

/* ================== ENVOI ARDUINO ================== */
void Badge::sendToArduino(const QString &msg)
{
    if (!serial->isOpen()) {
        qDebug() << "[QT] Port série non ouvert";
        return;
    }

    QByteArray out = msg.toUtf8();
    if (!out.endsWith('\n'))
        out.append('\n');

    serial->write(out);
    serial->flush();

    qDebug() << "[QT] Envoyé Arduino:" << msg;
}

/* ================== NETTOYAGE TEXTE ================== */
QString Badge::cleanField(const QString &s)
{
    QString r = s;
    r.replace(':', ' ');
    r.replace('\n', ' ');
    r.replace('\r', ' ');
    return r.trimmed();
}
