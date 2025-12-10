#ifndef EQUIPEMENT_H
#define EQUIPEMENT_H

#include <QString>
#include <QDate>
#include <QByteArray>
#include <QSqlQueryModel>

class Equipement
{
private:
    QString id_equipement;  // Gardez QString si vos IDs sont alphanumériques
    QString categorie;
    QString etat;
    QString fournisseur;
    QDate date_acquisition;
    QString lieu;
    // QByteArray image; // SUPPRIMEZ cette ligne - pas dans la nouvelle table

public:
    Equipement();
    Equipement(QString id, QString categorie, QString etat,
               QString fournisseur, QDate date_acquisition,
               QString lieu);  // 6 paramètres seulement

    bool ajouter();
    bool modifier(QString id_equipement, QString categorie, QString etat,
                  QString fournisseur, QDate date_acquisition,
                  QString lieu);  // 6 paramètres seulement

    bool supprimer(QString id);

    QSqlQueryModel* afficher();
    QSqlQueryModel* rechercher(QString valeur);
    QSqlQueryModel* trier(QString critere);
};

#endif // EQUIPEMENT_H
