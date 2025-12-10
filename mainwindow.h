#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QMainWindow>
#include <QTableWidgetItem>
#include "employe.h"
#include "login.h"
#include "cours.h"
#include "qtextedit.h"
#include "vehicule.h"
#include"examen.h"
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QMap>
#include <QByteArray>
#include "arduino.h"


struct SupplierInfo {
    QString name;
    QString address;
    QString phone;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onIdProcessed(const QString &id, bool granted); // slot pour logs/UI

    // Fonctions employés
    void ajouterEmploye();
    void supprimerEmploye();
    void allerPageModification();
    void modifierEmploye();
    void remplirChampsDepuisTable_emp(int row, int column);
    void trierEmployes(int index);
    void afficherEmployes();
    bool verifChamps();
    void viderChamps();
    void exporterPDF_Employes();
    void rechercherEmploye();
    void afficherStatistiquePoste();
    void on_affich_2_clicked();

    void on_lineEdit_telephone_2_textChanged(const QString &text);

    // Fonctions messagerie
    void on_btnMessagerie_clicked();
    void on_btnEnvoyerMessage_clicked();
    void refreshMessagerie();
    void connecterEmploye();
    void onLogoutClicked();

    void on_btnAjouter_clicked();
    void on_supprimer_clicked();
    void on_btnStats_clicked();
    void on_pushButton_rechercher_clicked();

     void on_tableWidgetExamen_itemClicked(QTableWidgetItem *item);
     void on_btnModifier_clicked();
     void afficherExamensDansTable();
     void on_btnTrier_clicked();
     void on_btnQR_clicked();


     /* void on_btnShowPopup_clicked();*/
      void insertData();
      void clearFields();
       void filterCoursTable();
      void loadDataToTable();
      void on_pushButton_88_clicked();
      void on_pushButton_38_clicked();
      void on_pushButton_117_clicked();
      void on_pushButton_140_clicked();
      void loadComboBoxes();
      void modifierCoursDepuisTable();

      void on_btnShowPopup_clicked();
      void showFreeSlotsPopup(const QDate &selectedDate);
      void on_btnShowCalendar_clicked(); // your push button


      void on_valider_clicked();

      void on_boutonViderFormulaireVoiture_clicked();
      void on_boutonSupprimerVoiture_clicked();
      void on_boutonModifierVoiture_clicked();
      void on_boutonChercherVoiture_clicked();
      void on_listeVoiture_cellClicked(int row, int column);
      void on_label_103_clicked();
      void on_label_143_clicked();
      void on_boutonExporterPdf_clicked();
      void on_Analyse_clicked();







      void on_supprimer_2_clicked();
      void on_ajouter_clicked();
      void on_modifier_clicked();
      void on_annuler_clicked();
      void on_rechercher_clicked();
      void afficherStatistiques();
      void remplirChampsDepuisTable(const QModelIndex &index);
      void trierTableau();
      void on_exporter_clicked();
      void on_historique_clicked();



      void on_pushButton_Ajouter_Equip_clicked();
      void on_pushButton_Modifier_Equip_clicked();
       void afficherEquipements();
      void on_pushButton_Supprimer_Equip_clicked();
      void on_pushButton_Rechercher_Equip_clicked();
      void on_cb_tri_eq_currentIndexChanged(int index);
      void on_pushButton_Annuler_Equip_clicked();
      void on_pushButton_exporterpdf_clicked();
      void on_pushButton_Ajouter_Equip_2_clicked();
      void on_pushButton_Localiser_Equip_clicked();
      void wheelEvent(QWheelEvent *event) override;
      void on_t_eq_itemClicked(QTableWidgetItem *item);




private:
    Ui::MainWindow *ui;
    Examen E;
    Arduino *arduino; // pointeur géré par MainWindow

    QByteArray imageEquipementData;

    void actualiserTableau();
    void on_pushButton_exporterPDF_clicked();


    void chargerTableEquipements(const QString &sql);
    void rafraichirTableEquipements();
    void clearFormEquipement();
    void afficherStatistiquesEquipements();
    QGraphicsScene *m_mapScene;
    QMap<QGraphicsItem*, SupplierInfo> m_suppliersMap;


    void afficherVehicules();
    void afficherVehicules(QList<Vehicule> vehicules);
    void viderFormulaireVehicule();
    void remplirFormulaireVehicule(Vehicule v);
    void afficherStatistiquesVehicules();
    void afficherAnalyseVehicules();
    void verifierKilometrageLimite(int kilometrage, int kilometrageLimite, const QString &matricule);
    void showMaintenanceAlerts();
      void showCalendarWithCourses(); // helper function

    QString obtenirMotDePasseEmploye(const QString& nom, const QString& prenom);
    void chargerEmployeDepuisBase(int idEmploye);
    QString obtenirMotDePasseActuel(int idEmploye);
    QString genererMessageProfessionnel(const QString& type, const QString& prenom, const QString& nom);

    // === VARIABLES ET MÉTHODES MESSAGERIE ===
    QComboBox *comboDestinataires;
    QTextEdit *displayMessagerie;
    QLineEdit *inputMessage;
    QPushButton *btnEnvoyerMsg;
    QTimer *msgRefreshTimer;

    int currentEmployeId;
    QString currentEmployeNom;

    // Méthodes messagerie
    void setupMessagerieInTab();
    void afficherFormulaireConnexion();
    void chargerEmployesDestinataires();
    void chargerMessages();
    void onDestinataireChange();
    void chargerMessagesDiscussion(int idDestinataire);
    void afficherMessagesWhatsApp(const QList<QJsonObject>& messages, const QString& nomDestinataire);
    void afficherAucunMessageDiscussion(const QString& nomDestinataire);
    void ajouterMessageEnTempsReel(const QString &message, bool estMoi);

    QString selectedMatricule;
    //QTextToSpeech *speech;

 Cours cours;

};
#endif // MAINWINDOW_H
