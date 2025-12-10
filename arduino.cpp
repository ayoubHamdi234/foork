#include "arduino.h"
#include <QDebug>

Arduino::Arduino(QObject *parent) : QObject(parent), serial(new QSerialPort(this))
{
}

Arduino::~Arduino()
{
    if (serial->isOpen()) serial->close();
}

QString Arduino::detectArduinoPort()
{
    return "COM7";   // Port de ton Arduino
}

bool Arduino::connectArduino()
{
    QString portName = detectArduinoPort();
    if (portName.isEmpty()) {
        qDebug() << "[QT] Aucun port Arduino detecte.";
        return false;
    }

    serial->setPortName(portName);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!serial->open(QIODevice::ReadWrite)) {
        qDebug() << "[QT] Impossible d'ouvrir le port" << portName << ":" << serial->errorString();
        return false;
    }

    connect(serial, &QSerialPort::readyRead, this, &Arduino::readSerial);
    qDebug() << "[QT] Arduino connecté sur" << portName;
    return true;
}

void Arduino::readSerial()
{
    // lire ligne par ligne (Arduino envoie Serial.println -> newline)
    while (serial->canReadLine()) {
        QByteArray raw = serial->readLine();
        QString line = QString::fromUtf8(raw).trimmed();
        qDebug() << "[QT] Recu d'Arduino:" << line;

        if (line.startsWith("ID:")) {
            processIDString(line);
        } else {
            qDebug() << "[QT] Message ignoré (format inattendu):" << line;
        }
    }
}

void Arduino::processIDString(const QString &raw)
{
    // raw = "ID:xxxx"
    QString payload = raw;
    payload.remove(0, 3); // remove "ID:"
    payload = payload.trimmed();

    bool ok;
    int idInt = payload.toInt(&ok);
    if (!ok) {
        qDebug() << "[QT] ID non numerique recu:" << payload;
        // renvoyer NOK (élève inexistant / format invalide)
        sendToArduino("NOK");
        emit idProcessed(payload, false);
        return;
    }

    // ----- 1) verifier existence et recuperer nom/prenom -----
    QSqlQuery q;
    q.prepare("SELECT NOM, PRENOM FROM ELEVES WHERE ID_ELEVE = :id");
    q.bindValue(":id", idInt);

    if (!q.exec()) {
        qDebug() << "[QT] Erreur SQL (verif existence):" << q.lastError().text();
        sendToArduino("NOK");
        emit idProcessed(QString::number(idInt), false);
        return;
    }

    if (!q.next()) {
        qDebug() << "[QT] Eleve inexistant:" << idInt;
        sendToArduino("NOK");
        emit idProcessed(QString::number(idInt), false);
        return;
    }

    QString nom = q.value(0).toString().trimmed();
    QString prenom = q.value(1).toString().trimmed();

    // ----- 2) verifier s'il a un cours aujourd'hui -----
    QSqlQuery qc;
    // Oracle: compare TRUNC(DATE_COURS) = TRUNC(SYSDATE)
    // Use a DB-side function for date equality. For portability we'll use TRUNC if Oracle.
    QString sqlCheck = "SELECT COUNT(*) FROM COURS WHERE ID_ELEVE = :id AND TRUNC(DATE_COURS) = TRUNC(SYSDATE)";
    qc.prepare(sqlCheck);
    qc.bindValue(":id", idInt);

    if (!qc.exec()) {
        qDebug() << "[QT] Erreur SQL (check cours):" << qc.lastError().text();
        sendToArduino("NOK");
        emit idProcessed(QString::number(idInt), false);
        return;
    }

    int cnt = 0;
    if (qc.next()) cnt = qc.value(0).toInt();

    if (cnt > 0) {
        // acces autorise
        QString out = QString("OK:%1:%2").arg(escapeField(nom)).arg(escapeField(prenom));
        sendToArduino(out);
        qDebug() << "[QT] Acces autorise ->" << out;
        emit idProcessed(QString::number(idInt), true);
    } else {
        // existe mais pas de cours aujourd'hui
        QString out = QString("REF:%1:%2").arg(escapeField(nom)).arg(escapeField(prenom));
        sendToArduino(out);
        qDebug() << "[QT] Pas de cours aujourd'hui ->" << out;
        emit idProcessed(QString::number(idInt), false);
    }
}

QString Arduino::escapeField(const QString &s)
{
    // retire ':' et '\n' pour éviter casser le protocole
    QString r = s;
    r.replace(':', ' ');
    r.replace('\n', ' ');
    r.replace('\r', ' ');
    return r;
}

void Arduino::sendToArduino(const QString &msg)
{
    if (!serial->isOpen()) {
        qDebug() << "[QT] sendToArduino: port non ouvert";
        return;
    }
    QByteArray out = msg.toUtf8();
    if (!out.endsWith("\n")) out.append('\n');
    serial->write(out);
    serial->flush();
    qDebug() << "[QT] Envoye a Arduino:" << msg;
}
