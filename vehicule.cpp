#include "vehicule.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// Default constructor
Vehicule::Vehicule()
{
    this->kilometrage = 0;
    this->kilometrageLimite = 0;
}

// Parameterized constructor
Vehicule::Vehicule(QString matricule, QString marque, QString modele, int kilometrage,
                   int kilometrageLimite, QDate dateMiseService, QString etat, QString type,
                   QDate dernierEntretien, QString frequenceEntretien)
{
    this->matricule = matricule;
    this->marque = marque;
    this->modele = modele;
    this->kilometrage = kilometrage;
    this->kilometrageLimite = kilometrageLimite;
    this->dateMiseService = dateMiseService;
    this->etat = etat;
    this->type = type;
    this->dernierEntretien = dernierEntretien;
    this->frequenceEntretien = frequenceEntretien;
}

// Add a vehicle to the database
bool Vehicule::ajouter()
{
    QSqlQuery query;

    query.prepare("INSERT INTO VEHICULES (MATRICULE, MARQUE, MODELE, KILOMETRAGE, "
                  "KILOMETRAGE_LIMITE, DATE_MISE_SERVICE, ETAT, TYPE, DERNIER_ENTRETIEN, FREQUENCE_ENTRETIEN) "
                  "VALUES (:matricule, :marque, :modele, :kilometrage, "
                  ":kilometrageLimite, :dateMiseService, :etat, :type, :dernierEntretien, :frequenceEntretien)");

    query.bindValue(":matricule", matricule);
    query.bindValue(":marque", marque);
    query.bindValue(":modele", modele);
    query.bindValue(":kilometrage", kilometrage);
    query.bindValue(":kilometrageLimite", kilometrageLimite);
    query.bindValue(":dateMiseService", dateMiseService);
    query.bindValue(":etat", etat);
    query.bindValue(":type", type);
    query.bindValue(":dernierEntretien", dernierEntretien);
    query.bindValue(":frequenceEntretien", frequenceEntretien);

    if (query.exec()) {
        qDebug() << "Vehicle added successfully!";
        return true;
    } else {
        qDebug() << "Error adding vehicle:" << query.lastError().text();
        return false;
    }
}

// Retrieve all vehicles from the database
QList<Vehicule> Vehicule::afficher()
{
    QList<Vehicule> vehicules;
    QSqlQuery query;

    query.prepare("SELECT * FROM VEHICULES");

    if (query.exec()) {
        while (query.next()) {
            Vehicule v;
            v.setMatricule(query.value("MATRICULE").toString());
            v.setMarque(query.value("MARQUE").toString());
            v.setModele(query.value("MODELE").toString());
            v.setKilometrage(query.value("KILOMETRAGE").toInt());
            v.setKilometrageLimite(query.value("KILOMETRAGE_LIMITE").toInt());
            v.setDateMiseService(query.value("DATE_MISE_SERVICE").toDate());
            v.setEtat(query.value("ETAT").toString());
            v.setType(query.value("TYPE").toString());
            v.setDernierEntretien(query.value("DERNIER_ENTRETIEN").toDate());
            v.setFrequenceEntretien(query.value("FREQUENCE_ENTRETIEN").toString());

            vehicules.append(v);
        }
        qDebug() << "Retrieved" << vehicules.size() << "vehicles";
    } else {
        qDebug() << "Error retrieving vehicles:" << query.lastError().text();
    }

    return vehicules;
}

// Update a vehicle in the database
bool Vehicule::modifier()
{
    QSqlQuery query;

    query.prepare("UPDATE VEHICULES SET MARQUE = :marque, MODELE = :modele, "
                  "KILOMETRAGE = :kilometrage, KILOMETRAGE_LIMITE = :kilometrageLimite, "
                  "DATE_MISE_SERVICE = :dateMiseService, "
                  "ETAT = :etat, TYPE = :type, DERNIER_ENTRETIEN = :dernierEntretien, "
                  "FREQUENCE_ENTRETIEN = :frequenceEntretien "
                  "WHERE MATRICULE = :matricule");

    query.bindValue(":matricule", matricule);
    query.bindValue(":marque", marque);
    query.bindValue(":modele", modele);
    query.bindValue(":kilometrage", kilometrage);
    query.bindValue(":kilometrageLimite", kilometrageLimite);
    query.bindValue(":dateMiseService", dateMiseService);
    query.bindValue(":etat", etat);
    query.bindValue(":type", type);
    query.bindValue(":dernierEntretien", dernierEntretien);
    query.bindValue(":frequenceEntretien", frequenceEntretien);

    if (query.exec()) {
        qDebug() << "Vehicle updated successfully!";
        return true;
    } else {
        qDebug() << "Error updating vehicle:" << query.lastError().text();
        return false;
    }
}

// Delete a vehicle from the database
bool Vehicule::supprimer()
{
    QSqlQuery query;

    query.prepare("DELETE FROM VEHICULES WHERE MATRICULE = :matricule");
    query.bindValue(":matricule", matricule);

    if (query.exec()) {
        qDebug() << "Vehicle deleted successfully!";
        return true;
    } else {
        qDebug() << "Error deleting vehicle:" << query.lastError().text();
        return false;
    }
}

// Get a vehicle by its matricule
Vehicule Vehicule::getByMatricule(QString matricule)
{
    QSqlQuery query;
    Vehicule v;

    query.prepare("SELECT * FROM VEHICULES WHERE MATRICULE = :matricule");
    query.bindValue(":matricule", matricule);

    if (query.exec() && query.next()) {
        v.setMatricule(query.value("MATRICULE").toString());
        v.setMarque(query.value("MARQUE").toString());
        v.setModele(query.value("MODELE").toString());
        v.setKilometrage(query.value("KILOMETRAGE").toInt());
        v.setKilometrageLimite(query.value("KILOMETRAGE_LIMITE").toInt());
        v.setDateMiseService(query.value("DATE_MISE_SERVICE").toDate());
        v.setEtat(query.value("ETAT").toString());
        v.setType(query.value("TYPE").toString());
        v.setDernierEntretien(query.value("DERNIER_ENTRETIEN").toDate());
        v.setFrequenceEntretien(query.value("FREQUENCE_ENTRETIEN").toString());
    } else {
        qDebug() << "Error retrieving vehicle:" << query.lastError().text();
    }

    return v;
}

// Search vehicles by criteria (marque, modele, or matricule)
QList<Vehicule> Vehicule::rechercher(QString critere)
{
    QList<Vehicule> vehicules;
    QSqlQuery query;

    query.prepare("SELECT * FROM VEHICULES WHERE "
                  "UPPER(MARQUE) LIKE UPPER(:critere) OR "
                  "UPPER(MODELE) LIKE UPPER(:critere) OR "
                  "UPPER(MATRICULE) LIKE UPPER(:critere) OR "
                  "UPPER(ETAT) LIKE UPPER(:critere) "
                  "ORDER BY MATRICULE");

    query.bindValue(":critere", "%" + critere + "%");

    if (query.exec()) {
        while (query.next()) {
            Vehicule v;
            v.setMatricule(query.value("MATRICULE").toString());
            v.setMarque(query.value("MARQUE").toString());
            v.setModele(query.value("MODELE").toString());
            v.setKilometrage(query.value("KILOMETRAGE").toInt());
            v.setKilometrageLimite(query.value("KILOMETRAGE_LIMITE").toInt());
            v.setDateMiseService(query.value("DATE_MISE_SERVICE").toDate());
            v.setEtat(query.value("ETAT").toString());
            v.setType(query.value("TYPE").toString());
            v.setDernierEntretien(query.value("DERNIER_ENTRETIEN").toDate());
            v.setFrequenceEntretien(query.value("FREQUENCE_ENTRETIEN").toString());

            vehicules.append(v);
        }
    } else {
        qDebug() << "Error searching vehicles:" << query.lastError().text();
    }

    return vehicules;
}

// Retrieve and sort vehicles by specified criteria
QList<Vehicule> Vehicule::afficherTrie(QString critere, bool ascending)
{
    QList<Vehicule> vehicules;
    QSqlQuery query;

    QString sqlQuery = "SELECT * FROM VEHICULES";
    
    if (critere == "MATRICULE") {
        sqlQuery += ascending ? " ORDER BY MATRICULE ASC" : " ORDER BY MATRICULE DESC";
    }

    if (query.exec(sqlQuery)) {
        while (query.next()) {
            Vehicule v;
            v.setMatricule(query.value("MATRICULE").toString());
            v.setMarque(query.value("MARQUE").toString());
            v.setModele(query.value("MODELE").toString());
            v.setKilometrage(query.value("KILOMETRAGE").toInt());
            v.setKilometrageLimite(query.value("KILOMETRAGE_LIMITE").toInt());
            v.setDateMiseService(query.value("DATE_MISE_SERVICE").toDate());
            v.setEtat(query.value("ETAT").toString());
            v.setType(query.value("TYPE").toString());
            v.setDernierEntretien(query.value("DERNIER_ENTRETIEN").toDate());
            v.setFrequenceEntretien(query.value("FREQUENCE_ENTRETIEN").toString());

            vehicules.append(v);
        }
        qDebug() << "Retrieved and sorted" << vehicules.size() << "vehicles by" << critere;
    } else {
        qDebug() << "Error retrieving sorted vehicles:" << query.lastError().text();
    }

    return vehicules;
}
