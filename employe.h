#ifndef EMPLOYE_H
#define EMPLOYE_H

#include <QString>
#include <QDate>
#include <QSqlQueryModel>
#include <QTableWidget>
#include <QMap>
#include <QList>
#include <QPair>
#include <QJsonObject>

class Employe
{
private:
    int id;
    QString nom;
    QString prenom;
    QString genre;
    QString cin;
    QString email;
    QDate dateNaissance;
    QString adresse;
    QString telephone;
    QString poste;
    QDate dateEmbauche;
    QString mot_de_passe;
    QString messages;
    QString question;
    QString reponse;
    QString pin;

public:
    // Constructeurs
    Employe();
    Employe(QString n, QString p, QString g, QString c, QString e, QDate dn,
            QString a, QString t, QString po, QDate de, QString mdp,
            QString q = "", QString r = "", QString pin_code = "", int i = 0);

    // ================= GETTERS =================
    int getId() const { return id; }
    QString getNom() const { return nom; }
    QString getPrenom() const { return prenom; }
    QString getGenre() const { return genre; }
    QString getCin() const { return cin; }
    QString getEmail() const { return email; }
    QDate getDateNaissance() const { return dateNaissance; }
    QString getAdresse() const { return adresse; }
    QString getTelephone() const { return telephone; }
    QString getPoste() const { return poste; }
    QDate getDateEmbauche() const { return dateEmbauche; }
    QString getMotDePasse() const { return mot_de_passe; }
    QString getQuestion() const { return question; }
    QString getReponse() const { return reponse; }
    QString getPin() const { return pin; }

    bool ajouter();
    bool modifier();
    bool supprimer(int id);
    QSqlQueryModel* afficher();

    // Méthodes d'interface
    void remplirTableWidget(QTableWidget* tableWidget);
    void trierEtRemplirTable(QTableWidget* tableWidget, int index);
    void rechercherEtRemplirTable(QTableWidget* tableWidget, const QString& texteRecherche, int typeRecherche);
    bool exporterPDF_Employes(QTableWidget* tableWidget, const QString& fileName = "");
    QMap<QString, int> getStatistiquePoste();

    // Méthodes de validation
    static bool validerTelephone(const QString& telephone, QString& messageErreur);
    static bool validerEmail(const QString& email, QString& messageErreur);
    static bool validerCIN(const QString& cin, QString& messageErreur);

    // Méthodes SMS
    static bool envoyerSMS(const QString& to, const QString& message);

    // Méthodes d'authentification
    static int authentifierEmploye(const QString& nom, const QString& motDePasse);
    static QString getNomEmploye(int idEmploye);
    static QList<QPair<int, QString>> getListeEmployes(int idExclu = -1);

    // Méthodes de messagerie
    static bool envoyerMessage(int idExpediteur, const QString& nomExpediteur,
                               int idDestinataire, const QString& message);
    static QList<QJsonObject> getMessagesPourEmploye(int idEmploye);
    static QList<QJsonObject> getMessagesDiscussion(int idEmploye1, int idEmploye2);
    static void marquerMessagesLus(int idEmploye);
    static int getNombreMessagesNonLus(int idEmploye);

private:
    static bool testerConnexionInternet();
    static QString getTelephoneFromId(int idEmploye);
};

#endif // EMPLOYE_H
