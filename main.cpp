
#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include "connection.h"

#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include "connection.h"
#include "login.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Connection c;
    bool test = c.createConnect();  // 1. Connect to DB first

    if(!test) {
        QMessageBox::critical(nullptr, QObject::tr("Database Status"),
                              QObject::tr("Connection failed."));
        return -1; // Exit if DB failed
    }
    Login login;
    // 2. Only create MainWindow after DB is open
    if (login.exec() == QDialog::Accepted) {
        // Si login réussi, afficher MainWindow
        MainWindow w;
        w.show();

        QMessageBox::information(nullptr, QObject::tr("Database Status"),
                                 QObject::tr("Connection successful."));

        return a.exec();
    }
}



















/*int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Création de la connexion
    Connection c;
    bool test = c.createconnect();

    if (!test) {
        QMessageBox::critical(nullptr, QObject::tr("Database is not open"),
                              QObject::tr("Connection failed.\nClick Cancel to exit."),
                              QMessageBox::Cancel);
        return -1; // Quitter si connexion échoue
    }

    // Connexion réussie
    QMessageBox::information(nullptr, QObject::tr("Database is open"),
                             QObject::tr("Connection successful."),
                             QMessageBox::Ok);

    // Créer et afficher la fenêtre principale
    MainWindow w;
    w.show();

    return a.exec();
}*/
