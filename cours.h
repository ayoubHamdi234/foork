#ifndef COURS_H
#define COURS_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QTableWidget>
#include <QPrinter>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>

class Cours
{
private:
    int id;
    QString date;
    QString heureDebut;
    QString heureFin;
    QString type;
    QString circuit;
    double tarif;
    int id_eleve;
    int idEmploye;
    QString matricule;
    QChartView *chartView = nullptr;

public:
    // Constructeurs
    Cours() {}
    Cours(int, QString, QString, QString, QString, QString, double,int, int, QString);

    // Getters
    int getId() const { return id; }
    QString getDate() const { return date; }
    QString getHeureDebut() const { return heureDebut; }
    QString getHeureFin() const { return heureFin; }
    QString getType() const { return type; }
    QString getCircuit() const { return circuit; }
    double getTarif() const { return tarif; }
    int getIdEleve() const { return id_eleve; }
    int getIdEmploye() const { return idEmploye; }
    QString getMatricule() const { return matricule; }

    // Setters
    void setId(int id) { this->id = id; }
    void setDate(QString d) { date = d; }
    void setHeureDebut(QString hd) { heureDebut = hd; }
    void setHeureFin(QString hf) { heureFin = hf; }
    void setType(QString t) { type = t; }
    void setCircuit(QString c) { circuit = c; }
    void setTarif(double t) { tarif = t; }
    void setIdEleve(int idElv) { id_eleve = idElv; }
    void setIdEmploye(int idEmp) { idEmploye = idEmp; }
    void setMatricule(QString mat) { matricule = mat; }

    // Fonctionnalités de base (CRUD)
    bool ajouter();
    QSqlQueryModel* afficher();
    bool supprimer(int);
    bool modifier();

    // Autres fonctionnalités
    void chargerDansTable(QTableWidget *table);
    bool exporterPDF(QTableWidget *table, QWidget *parent);

    QChartView* creerStatistiques(QWidget *parent = nullptr); // same as old
    void refreshStatistiques(); // added for live update
};

#endif // COURS_H
