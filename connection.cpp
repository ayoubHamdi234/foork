#include "connection.h"
#include <QDebug>

Connection::Connection() {}

bool Connection::createConnect()
{
    // Création de la connexion ODBC
    db = QSqlDatabase::addDatabase("QODBC");

    db.setDatabaseName("sdrive");

    db.setUserName("smartdrive");
    db.setPassword("etudiant2A");

    qDebug() << "Trying to open database with name:" << db.databaseName();

    if (!db.open()) {
        QMessageBox::critical(nullptr, "Database Connection",
                              "Connection failed: " + db.lastError().text());
        qDebug() << "Database drivers available:" << QSqlDatabase::drivers();
        return false;
    }

    QMessageBox::information(nullptr, "Database Connection",
                             "Connection successful!");
    return true;
}

void Connection::closeConnect()
{
    if (db.isOpen())
        db.close();
}
