#ifndef VEHICULE_H
#define VEHICULE_H

#include <QString>
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>

class Vehicule
{
private:
    QString matricule;
    QString marque;
    QString modele;
    int kilometrage;
    int kilometrageLimite;
    QDate dateMiseService;
    QString etat;
    QString type;
    QDate dernierEntretien;
    QString frequenceEntretien;

public:
    // Constructors
    Vehicule();
    Vehicule(QString matricule, QString marque, QString modele, int kilometrage,
             int kilometrageLimite, QDate dateMiseService, QString etat, QString type,
             QDate dernierEntretien, QString frequenceEntretien);

    // Getters
    QString getMatricule() const { return matricule; }
    QString getMarque() const { return marque; }
    QString getModele() const { return modele; }
    int getKilometrage() const { return kilometrage; }
    int getKilometrageLimite() const { return kilometrageLimite; }
    QDate getDateMiseService() const { return dateMiseService; }
    QString getEtat() const { return etat; }
    QString getType() const { return type; }
    QDate getDernierEntretien() const { return dernierEntretien; }
    QString getFrequenceEntretien() const { return frequenceEntretien; }

    // Setters
    void setMatricule(QString m) { matricule = m; }
    void setMarque(QString m) { marque = m; }
    void setModele(QString m) { modele = m; }
    void setKilometrage(int k) { kilometrage = k; }
    void setKilometrageLimite(int k) { kilometrageLimite = k; }
    void setDateMiseService(QDate d) { dateMiseService = d; }
    void setEtat(QString e) { etat = e; }
    void setType(QString t) { type = t; }
    void setDernierEntretien(QDate d) { dernierEntretien = d; }
    void setFrequenceEntretien(QString f) { frequenceEntretien = f; }

    // CRUD Operations
    bool ajouter();
    static QList<Vehicule> afficher();
    bool modifier();
    bool supprimer();
    static Vehicule getByMatricule(QString matricule);

    // Search functionality
    static QList<Vehicule> rechercher(QString critere);

    // Sorting functionality
    static QList<Vehicule> afficherTrie(QString critere, bool ascending = true);
};

#endif // VEHICULE_H
