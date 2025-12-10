#ifndef ELEVE_H
#define ELEVE_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QDate>
#include <QSqlError>
#include <QDebug>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QtCharts/QChartView>


class eleve
{
    int id_eleve;
    QString nom;
    QString prenom;
    QDate dateNaissance;
    QString telephone;
    QString adresse;
    QString email;

public:
    // Constructeurs / Destructeur
    eleve();
    eleve(int, QString, QString, QDate, QString, QString, QString);
    ~eleve();

    // Getters
    int getIdEleve() const { return id_eleve; }
    QString getNom() const { return nom; }
    QString getPrenom() const { return prenom; }
    QDate getDateNaissance() const { return dateNaissance; }
    QString getTelephone() const { return telephone; }
    QString getAdresse() const { return adresse; }
    QString getEmail() const { return email; }

    // Setters
    void setIdEleve(int id) { id_eleve = id; }
    void setNom(QString n) { nom = n; }
    void setPrenom(QString p) { prenom = p; }
    void setDateNaissance(QDate d) { dateNaissance = d; }
    void setTelephone(QString t) { telephone = t; }
    void setAdresse(QString a) { adresse = a; }
    void setEmail(QString e) { email = e; }

    //  CRUD
    bool ajouter();
    QSqlQueryModel *afficher();
    bool supprimer(int id);
    bool modifier();


    QSqlQueryModel* rechercher(int id, QString nom);
    QChartView* genererStatistiquesAgeVille();
    QSqlQueryModel* trier(QString critere);

    bool exporterPDF(QAbstractItemModel *model, const QString &filePath);
    void envoyerMail(QString to, QString sujet,
                     QString nom, QString prenom, QString dateNaissance,
                     QString telephone, QString adresse);
    void enregistreristorique(QString champ, QString ancienne, QString nouvelle, QString mouvement);







};

#endif
