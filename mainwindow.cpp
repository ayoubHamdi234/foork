#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include "examen.h"
#include "eleve.h"
#include "equipement.h"
#include <QDebug>
#include <QEvent>
#include <algorithm>
#include <QtCharts>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QPdfWriter>
#include <QPainter>
#include <QFileDialog>
#include <QDateTime>
#include <QFileDialog>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableFormat>
#include <QTextCharFormat>
#include <QDateTime>
#include <QMessageBox>
#include <QColor>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QIntValidator>
#include "cours.h"
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QChart>
#include <QtCharts/QPieSlice>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include "vehicule.h"
#include <QDir>
#include "arduino.h"
#include "badge.h"

Badge *badge;









static QPointF coordsFromLieu(const QString &lieuRaw, const QSize &mapSize)
{
    const double w = mapSize.width();
    const double h = mapSize.height();

    QString lieu = lieuRaw.toLower().trimmed();

    if (lieu.contains("tunis"))
        return QPointF(0.55 * w, 0.25 * h);
    else if (lieu.contains("ariana"))
        return QPointF(0.52 * w, 0.22 * h);
    else if (lieu.contains("ben arous"))
        return QPointF(0.58 * w, 0.30 * h);
    else if (lieu.contains("manouba"))
        return QPointF(0.50 * w, 0.26 * h);
    else if (lieu.contains("nabeul"))
        return QPointF(0.70 * w, 0.25 * h);
    else if (lieu.contains("sousse"))
        return QPointF(0.65 * w, 0.45 * h);
    else if (lieu.contains("sfax"))
        return QPointF(0.65 * w, 0.65 * h);
    else
        return QPointF(0.5 * w, 0.5 * h); // centre par défaut
}










// Histogramme simple sur 4x4x4 = 64 bins
static QVector<double> computeHistogram(const QImage &origImage)
{
    QImage img = origImage.convertToFormat(QImage::Format_RGB32);

    const int binsPerChannel = 4;
    const int totalBins = binsPerChannel * binsPerChannel * binsPerChannel;

    QVector<double> hist(totalBins);
    hist.fill(0.0);

    int w = img.width();
    int h = img.height();
    if (w == 0 || h == 0)
        return hist;

    for (int y = 0; y < h; ++y) {
        const QRgb *row = reinterpret_cast<const QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QColor c(row[x]);
            int rBin = c.red()   * binsPerChannel / 256;
            int gBin = c.green() * binsPerChannel / 256;
            int bBin = c.blue()  * binsPerChannel / 256;

            int idx = rBin * binsPerChannel * binsPerChannel
                      + gBin * binsPerChannel
                      + bBin;

            if (idx >= 0 && idx < totalBins)
                hist[idx] += 1.0;
        }
    }

    double total = double(w) * double(h);
    if (total > 0.0) {
        for (int i = 0; i < totalBins; ++i)
            hist[i] /= total;   // normalisation
    }

    return hist;
}

// Distance entre deux histogrammes (L1)
static double histogramDistance(const QVector<double> &a, const QVector<double> &b)
{
    int n = qMin(a.size(), b.size());
    double sum = 0.0;
    for (int i = 0; i < n; ++i)
        sum += qAbs(a[i] - b[i]);
    return sum;
}

// Classifieur très simple : plus proche voisin entre "équipement" et "autre"
// Version stricte : mieux vaut REFUSER une image douteuse que d'accepter du non-équipement.
static bool isEquipmentImage(const QString &candidatePath)
{
    // 1) Charger l'image candidate (celle que l'utilisateur/prof choisit)
    QImage candidate(candidatePath);
    if (candidate.isNull()) {
        qDebug() << "Image candidate invalide :" << candidatePath;
        return false; // par sécurité, on REFUSE
    }

    QVector<double> candHist = computeHistogram(candidate);

    // 2) Jeux d’apprentissage (images du dataset), chargés une seule fois
    static bool initialized = false;
    static QList<QVector<double>> equipHists;
    static QList<QVector<double>> otherHists;

    if (!initialized) {
        initialized = true;

        QString base = QCoreApplication::applicationDirPath();

        // On part de .../build/Desktop_Qt_6_7_3_MinGW_64_bit-Debug/debug
        QDir dir(base);
        dir.cdUp();  // debug -> Desktop_Qt_6_7_3_MinGW_64_bit-Debug
        dir.cdUp();  // Desktop_Qt... -> build
        dir.cdUp();  // build -> 2a24-smart-driving-school (dossier du projet)

        base = dir.absolutePath();

        // Dossiers dataset à la RACINE du projet
        QString equipDirPath = base + "/dataset/equipement";
        QString otherDirPath = base + "/dataset/other";



        QDir equipDir(equipDirPath);
        QDir otherDir(otherDirPath);

        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp";

        // --- Charger TOUTES les images d'équipement ---
        for (const QString &file : equipDir.entryList(filters, QDir::Files)) {
            QString p = equipDir.absoluteFilePath(file);
            QImage img(p);
            if (!img.isNull()) {
                equipHists.append(computeHistogram(img));
                qDebug() << "Image équipement chargée pour IA:" << p;
            } else {
                qDebug() << "Impossible de charger image équipement:" << p;
            }
        }

        // --- Charger TOUTES les images 'other' ---
        for (const QString &file : otherDir.entryList(filters, QDir::Files)) {
            QString p = otherDir.absoluteFilePath(file);
            QImage img(p);
            if (!img.isNull()) {
                otherHists.append(computeHistogram(img));
                qDebug() << "Image 'autre' chargée pour IA:" << p;
            } else {
                qDebug() << "Impossible de charger image 'autre':" << p;
            }
        }

        qDebug() << "NB images équipement =" << equipHists.size()
                 << ", NB images other =" << otherHists.size();
    }

    // Si rien n’a été chargé → REFUSER (système IA non disponible)
    if (equipHists.isEmpty() || otherHists.isEmpty()) {
        qDebug() << "ERREUR IA: pas assez d'images d'entraînement, on REFUSE.";
        return false;
    }

    // 3) Comparer l'image candidate aux exemples
    double bestEquip = 1e9;
    for (const auto &h : equipHists) {
        double d = histogramDistance(candHist, h);
        if (d < bestEquip) bestEquip = d;
    }

    double bestOther = 1e9;
    for (const auto &h : otherHists) {
        double d = histogramDistance(candHist, h);
        if (d < bestOther) bestOther = d;
    }

    qDebug() << "[IA] distance equip =" << bestEquip
             << ", distance autre =" << bestOther;

    // 4) Décision STRICTE
    //
    // diff > 0  -> plus proche des équipements que des autres
    double diff = bestOther - bestEquip;

    // Plus ces seuils sont élevés, plus l'IA est stricte
    double minDiff      = 0.08;  // différence MIN entre les deux distances
    double maxEquipDist = 0.35;  // distance MAX pour considérer "ça ressemble vraiment à un équipement"

    // On accepte SEULEMENT si :
    // 1) l'image est nettement plus proche des équipements que des autres
    // 2) ET la distance aux équipements est suffisamment petite
    bool isEquip = (diff > minDiff) && (bestEquip < maxEquipDist);

    qDebug() << "[IA] diff =" << diff
             << ", minDiff =" << minDiff
             << ", maxEquipDist =" << maxEquipDist
             << ", Décision finale IA -> isEquip =" << isEquip;

    return isEquip;
}


























MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);



    badge = new Badge(this);

    if (badge->connectArduino()) {
        qDebug() << "Arduino ready.";
    } else {
        qDebug() << "Arduino NOT connected!";
    }


















    arduino = new Arduino(this);

        // Assure-toi que ta QSqlDatabase est déjà ouverte AVANT d'appeler connectArduino()
        // (tu as dit que la DB est déjà connectée dans ton app)
        if (!arduino->connectArduino()) {
            qDebug() << "Impossible de connecter Arduino";
        } else {
            qDebug() << "Arduino connecté";
        }

        connect(arduino, &Arduino::idProcessed, this, &MainWindow::onIdProcessed);


    ui->graphicsView_map->setVisible(false);



    // ==========================
    //     INITIALISATION CARTE
    // ==========================

    m_mapScene = new QGraphicsScene(this);
    ui->graphicsView_map->setScene(m_mapScene);

    // Charger l'image
    QPixmap mapPixmap(":/images/images/tunis_map.png");


    if (mapPixmap.isNull()) {
        QMessageBox::critical(this, "Erreur", "Impossible de charger la carte !");
    } else {
        m_mapScene->addPixmap(mapPixmap);
        m_mapScene->setSceneRect(mapPixmap.rect());

    }
    QGraphicsRectItem *btnClose = new QGraphicsRectItem(0, 0, 28, 28);
    btnClose->setBrush(QColor("#D9534F"));   // rouge
    btnClose->setPen(Qt::NoPen);

    // Texte "X"
    QGraphicsTextItem *txt = new QGraphicsTextItem("X", btnClose);
    txt->setDefaultTextColor(Qt::white);
    txt->setFont(QFont("Arial", 16, QFont::Bold));
    txt->setPos(7, -1); // centrer le X

    // Rendre cliquable
    btnClose->setFlag(QGraphicsItem::ItemIsSelectable, true);
    btnClose->setCursor(Qt::PointingHandCursor);

    // Ajouter dans la scène
    m_mapScene->addItem(btnClose);

    // position en haut à droite de la carte
    btnClose->setPos(mapPixmap.width() - 40, 10);
    // Déplacement de la carte
    ui->graphicsView_map->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView_map->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView_map->setRenderHint(QPainter::SmoothPixmapTransform);

    // Fonction pour ajouter un fournisseur
    auto addSupplier = [this](const QPointF &pos, const SupplierInfo &info) {
        QGraphicsEllipseItem *marker = m_mapScene->addEllipse(
            pos.x() - 7, pos.y() - 7,
            14, 14,
            QPen(Qt::red, 2),
            QBrush(Qt::red)
            );
        marker->setFlag(QGraphicsItem::ItemIsSelectable, true);
        m_suppliersMap.insert(marker, info);
    };

    // Fournisseurs (coordonnées approximatives sur ta carte)
    addSupplier(QPointF(930, 210), {"Librairie Tunis",
                                    "Adresse : Tunis Centre",
                                    "Tél : 71 222 333"});

    addSupplier(QPointF(980, 180), {"MyTek",
                                    "Adresse : Charguia 2",
                                    "Tél : 71 123 456"});

    addSupplier(QPointF(910, 240), {"RT Mobilia",
                                    "Adresse : Centre-Ville",
                                    "Tél : 71 987 654"});

    addSupplier(QPointF(1000, 420), {"Librairie Zaghouan",
                                     "Adresse : Zaghouan",
                                     "Tél : 72 556 778"});

    // Quand on clique sur un fournisseur
    // ===== Bouton X sur la carte =====




    // Quand on clique sur un fournisseur OU sur X
    connect(m_mapScene, &QGraphicsScene::selectionChanged, this, [this, btnClose]() {
        auto selected = m_mapScene->selectedItems();
        if (selected.isEmpty()) return;

        QGraphicsItem *item = selected.first();

        // 🔥 Clique sur le bouton X → fermer la carte
        if (item == btnClose) {
            ui->graphicsView_map->setVisible(false);
            return;
        }

        // 🔥 Sinon → clique fournisseur
        if (!m_suppliersMap.contains(item)) return;

        SupplierInfo info = m_suppliersMap.value(item);
        QString msg = info.address + "\n" + info.phone;

        QMessageBox::information(this, info.name, msg);
    });



    this->setStyleSheet("QLabel, QLineEdit, QDateEdit, QComboBox, QPushButton { color: black; background-color: white; } "
                        "QTableWidget, QTableView { color: black; background-color: white; gridline-color: #d0d0d0; } "
                        "QHeaderView::section { color: black; background-color: #f0f0f0; } "
                        "QMessageBox { background-color: white; color: black; }");
    connect(ui->cb_tri_eq, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_cb_tri_eq_currentIndexChanged);
    ui->le_id_eq->setValidator(new QIntValidator(0, 999999, this));
    rafraichirTableEquipements();

    afficherExamensDansTable();
    afficherVehicules();
    
    // Initialize text-to-speech with French locale
    /*speech = new QTextToSpeech(this);
    
    // Set French voice
    QVector<QVoice> voices = speech->availableVoices();
    for (const QVoice &voice : voices) {
        if (voice.name().contains("French", Qt::CaseInsensitive) || 
            voice.name().contains("Français", Qt::CaseInsensitive) ||
            voice.locale().name().startsWith("fr")) {
            speech->setVoice(voice);
            qDebug() << "French voice selected:" << voice.name();
            break;
        }
    }
    
    // Set French locale
    speech->setLocale(QLocale(QLocale::French, QLocale::France));*/

    // Show maintenance alerts immediately on startup/login so every user is notified
    showMaintenanceAlerts();

    connect(ui->tableWidgetExamen, &QTableWidget::itemClicked,
            this, &MainWindow::on_tableWidgetExamen_itemClicked);
    connect(ui->pushButton_exporterPDF, &QPushButton::clicked,
            this, &MainWindow::on_pushButton_exporterPDF_clicked);
    ui->selecttri_2->addItem("Aucun tri");    // index 0
    ui->selecttri_2->addItem("Nom");          // index 1
    ui->selecttri_2->addItem("Prénom");       // index 2
    ui->selecttri_2->addItem("ID");           // index 3
    ui->selecttri_2->addItem("Nom(DESC)");    // index 4
    ui->selecttri_2->addItem("Prénom(DESC)"); // index 5
    ui->selecttri_2->addItem("ID(DESC)");     // index 6
    connect(ui->pushButton_ajout_2, &QPushButton::clicked, this, &MainWindow::ajouterEmploye);
    connect(ui->supp_2, &QPushButton::clicked, this, &MainWindow::supprimerEmploye);
    connect(ui->modif_3, &QPushButton::clicked, this, &MainWindow::allerPageModification);
    connect(ui->modif_4, &QPushButton::clicked, this, &MainWindow::modifierEmploye);
    connect(ui->table_emp_2, &QTableWidget::cellClicked, this, [this](int row, int column) {
        this->remplirChampsDepuisTable_emp(row, column);
    });

    connect(ui->selecttri_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::trierEmployes);
    connect(ui->affich_2, &QPushButton::clicked, this, &MainWindow::afficherEmployes);
    connect(ui->PDF_Emp, &QPushButton::clicked, this, &MainWindow::exporterPDF_Employes);

    // Détecter le changement de texte pour la recherche dynamique
    connect(ui->recherche_emp, &QLineEdit::textChanged, this, &MainWindow::rechercherEmploye);

    // Détecter le changement de type de recherche dans la comboBox
    connect(ui->rech_emp, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::rechercherEmploye);
    connect(ui->statt, &QPushButton::clicked, this, [=](){
        int index = 2; // numéro de l'onglet vers lequel naviguer (0 = premier)
        ui->tabWidget_48->setCurrentIndex(index);

        // Optionnel : appeler la fonction de statistiques si c'est l'onglet 3
        afficherStatistiquePoste();
    });
    connect(ui->lineEdit_telephone_2, &QLineEdit::textChanged,
            this, &MainWindow::on_lineEdit_telephone_2_textChanged);

    // ✅ CORRECTION: Initialisations sans afficher le formulaire automatiquement
    currentEmployeId = -1; // Pas connecté au début
    currentEmployeNom = "";
    setupMessagerieInTab();
    // ✅ CORRECTION SUPPRIMÉE: afficherFormulaireConnexion(); // Ne plus appeler automatiquement

    // Connecter le bouton messagerie
    connect(ui->btnChat, &QPushButton::clicked, this, &MainWindow::connecterEmploye);

    // Assure le bon format et l'interaction
    ui->labelLogout->setText("<a href='logout'>Déconnexion</a>");
    ui->labelLogout->setTextFormat(Qt::RichText);
    ui->labelLogout->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelLogout->setOpenExternalLinks(false);


    // Connecter le signal linkActivated au slot
    connect(ui->labelLogout, &QLabel::linkActivated, this, &MainWindow::onLogoutClicked);

    afficherEmployes();



    eleve e;
    QSqlQueryModel *model = e.afficher();
    if (model != nullptr) {
        ui->tab->setModel(model);
        ui->tab->resizeColumnsToContents();
        ui->tab->setStyleSheet("background-color: white;");
    }
    afficherStatistiques();
    connect(ui->tab, &QTableView::clicked, this, &MainWindow::remplirChampsDepuisTable);
    ui->tri_2->setCurrentIndex(-1);

    connect(ui->tri_2, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::trierTableau);
    connect(ui->pushButton_exporterPDF, &QPushButton::clicked, this, &MainWindow::on_exporter_clicked);
    connect(ui->rech, &QLineEdit::textChanged, this, [=](const QString &text){
        if (text.isEmpty())
        {
            ui->tab->setModel(model);
            ui->tab->resizeColumnsToContents();
        }
    });






    loadComboBoxes();
    QPixmap image(":/images/logo.png");
    ui->label_6->setPixmap(image);
    ui->label_6->setScaledContents(true);


    QPixmap dash(":/images/dash.png");
    ui->label_156->setPixmap(dash);
    ui->label_156->setScaledContents(true);



    QPixmap eleves(":/images/eleves.png");
    ui->label_157->setPixmap(eleves);
    ui->label_157->setScaledContents(true);

    QPixmap eleves2(":/images/eleves.png");
    ui->label_168->setPixmap(eleves2);
    ui->label_168->setScaledContents(true);

    QIcon sch(":/images/schedule.png");
    ui->btnShowCalendar->setIcon(sch);
    ui->btnShowCalendar->setIconSize(QSize(35, 35));
    QPixmap employe(":/images/employe.png");
    ui->label_158->setPixmap(employe);
    ui->label_158->setScaledContents(true);

    QPixmap employe2(":/images/employe.png");
    ui->label_171->setPixmap(employe2);
    ui->label_171->setScaledContents(true);



    QPixmap veh(":/images/veh.png");
    ui->label_159->setPixmap(veh);
    ui->label_159->setScaledContents(true);

    QPixmap veh2(":/images/veh.png");
    ui->label_170->setPixmap(veh2);
    ui->label_170->setScaledContents(true);


    QPixmap courss(":/images/cours.png");
    ui->label_160->setPixmap(courss);
    ui->label_160->setScaledContents(true);



    QPixmap exam(":/images/exam.png");
    ui->label_161->setPixmap(exam);
    ui->label_161->setScaledContents(true);



    QPixmap equip(":/images/equip.png");
    ui->label_162->setPixmap(equip);
    ui->label_162->setScaledContents(true);


    QPixmap param(":/images/parametres.png");
    ui->label_163->setPixmap(param);
    ui->label_163->setScaledContents(true);

    QPixmap aide(":/images/help.png");
    ui->label_164->setPixmap(aide);
    ui->label_164->setScaledContents(true);



    QPixmap decon(":/images/deconnexion.png");
    ui->label_165->setPixmap(decon);
    ui->label_165->setScaledContents(true);






    QIcon dele(":/images/delete.png");
    ui->pushButton_88->setIcon(dele);
    ui->pushButton_88->setIconSize(QSize(35, 35));



    QIcon dele1(":/images/delete.png");
    ui->supprimer_2->setIcon(dele1);
    ui->supprimer_2->setIconSize(QSize(35, 35));

    QIcon dele2(":/images/delete.png");
    ui->pushButton_Supprimer_Equip->setIcon(dele2);
    ui->pushButton_Supprimer_Equip->setIconSize(QSize(35, 35));



    QIcon dele3(":/images/delete.png");
    ui->boutonSupprimerVoiture->setIcon(dele3);
    ui->boutonSupprimerVoiture->setIconSize(QSize(35, 35));


    QIcon dele4(":/images/delete.png");
    ui->pushButton_149->setIcon(dele4);
    ui->pushButton_149->setIconSize(QSize(35, 35));

    QIcon valid(":/images/valid.png");
    ui->ajouter->setIcon(valid);
    ui->ajouter->setIconSize(QSize(35, 35));


    QIcon rech(":/images/rech.png");
    ui->rechercher->setIcon(rech);
    ui->rechercher->setIconSize(QSize(25, 25));

    QPixmap tri(":/images/tri.png");
    ui->label_36->setPixmap(tri);
    ui->label_36->setScaledContents(true);


    QIcon modif(":/images/modif.png");
    ui->modifier->setIcon(modif);
    ui->modifier->setIconSize(QSize(25, 25));


    QIcon annuler(":/images/annuler.png");
    ui->annuler->setIcon(annuler);
    ui->annuler->setIconSize(QSize(25, 25));



    QIcon pdf(":/images/pdf.png");
    ui->exporter->setIcon(pdf);
    ui->exporter->setIconSize(QSize(25, 25));





    QIcon pdf1(":/images/pdf.png");
    ui->pushButton_24->setIcon(pdf1);
    ui->pushButton_24->setIconSize(QSize(25, 25));

    QIcon modif1(":/images/modif.png");
    ui->modif_3->setIcon(modif1);
    ui->modif_3->setIconSize(QSize(25, 25));

    QIcon modif6(":/images/modif.png");
    ui->modif_4->setIcon(modif1);
    ui->modif_4->setIconSize(QSize(25, 25));


    QIcon modif2(":/images/modif.png");
    ui->boutonModifierVoiture->setIcon(modif2);
    ui->boutonModifierVoiture->setIconSize(QSize(25, 25));


    QIcon modif3(":/images/modif.png");
    ui->pushButton_Modifier_Equip->setIcon(modif3);
    ui->pushButton_Modifier_Equip->setIconSize(QSize(25, 25));



    QIcon modif4(":/images/modif.png");
    ui->btnModifier->setIcon(modif4);
    ui->btnModifier->setIconSize(QSize(25, 25));



    QIcon modif5(":/images/modif.png");
    ui->pushButton_Modifier_Equip->setIcon(modif5);
    ui->pushButton_Modifier_Equip->setIconSize(QSize(25, 25));






    QIcon valid1(":/images/valid.png");
    ui->pushButton_Ajouter_Equip->setIcon(valid1);
    ui->pushButton_Ajouter_Equip->setIconSize(QSize(35, 35));



    QIcon valid2(":/images/valid.png");
    ui->valider->setIcon(valid2);
    ui->valider->setIconSize(QSize(35, 35));






    QIcon valid3(":/images/valid.png");
    ui->pushButton_Ajouter_Equip->setIcon(valid3);
    ui->pushButton_Ajouter_Equip->setIconSize(QSize(35, 35));



    QIcon valid4(":/images/valid.png");
    ui->btnAjouter->setIcon(valid4);
    ui->btnAjouter->setIconSize(QSize(35, 35));


    QIcon valid5(":/images/valid.png");
    ui->pushButton_105->setIcon(valid5);
    ui->pushButton_105->setIconSize(QSize(35, 35));









    QIcon annuler1(":/images/annuler.png");
    ui->pushButton_130->setIcon(annuler1);
    ui->pushButton_130->setIconSize(QSize(25, 25));


    QIcon annuler2(":/images/annuler.png");
    ui->boutonViderFormulaireVoiture->setIcon(annuler2);
    ui->boutonViderFormulaireVoiture->setIconSize(QSize(25, 25));



    QIcon annuler3(":/images/annuler.png");
    ui->pushButton_24->setIcon(annuler3);
    ui->pushButton_24->setIconSize(QSize(25, 25));



    QIcon annuler4(":/images/annuler.png");
    ui->supprimer->setIcon(annuler4);
    ui->supprimer->setIconSize(QSize(25, 25));



    QIcon annuler5(":/images/annuler.png");
    ui->pushButton_Annuler_Equip->setIcon(annuler5);
    ui->pushButton_Annuler_Equip->setIconSize(QSize(25, 25));





    QIcon rech2(":/images/rech.png");
    ui->pushButton_Rechercher_Equip->setIcon(rech2);
    ui->pushButton_Rechercher_Equip->setIconSize(QSize(25, 25));




    QPixmap tri1(":/images/tri.png");
    ui->label_168->setPixmap(tri1);
    ui->label_168->setScaledContents(true);

    QIcon tri2(":/images/tri.png");
    ui->label_103->setIcon(tri2);
    ui->label_103->setIconSize(QSize(20, 20));


    QPixmap tri3(":/images/tri.png");
    ui->label_92->setPixmap(tri3);
    ui->label_92->setScaledContents(true);


    QIcon modiff5(":/images/modif.png");
    ui->pushButton_38->setIcon(modiff5);
    ui->pushButton_38->setIconSize(QSize(25, 25));


    QIcon annuller5(":/images/annuler.png");
    ui->pushButton_106->setIcon(annuller5);
    ui->pushButton_106->setIconSize(QSize(25, 25));





    QIcon chatbot(":/images/chatbot.png");
    ui->pushButton_28->setIcon(chatbot);
    ui->pushButton_28->setIconSize(QSize(35, 35));

    QIcon rech3(":/images/rech.png");
    ui->boutonChercherVoiture->setIcon(rech3);
    ui->boutonChercherVoiture->setIconSize(QSize(25, 25));

    QAction *searchIcon = ui->lineEdit_59->addAction(
        QIcon(":/images/rech.png"),
        QLineEdit::LeadingPosition   // icon on the left side
        );

    ui->lineEdit_59->setPlaceholderText("Rechercher...");

    QIcon rech4(":/images/rech.png");
    ui->pushButton_Rechercher_Equip->setIcon(rech4);
    ui->pushButton_Rechercher_Equip->setIconSize(QSize(25, 25));

    QIcon rech5(":/images/rech.png");
    ui->pushButton_rechercher->setIcon(rech5);
    ui->pushButton_rechercher->setIconSize(QSize(25, 25));

    QIcon rech9(":/images/tri.png");
    ui->btnTrier->setIcon(rech9);
    ui->btnTrier->setIconSize(QSize(20, 20));

    QIcon pdf55(":/images/pdf.png");
    ui->pushButton_117->setIcon(pdf55);
    ui->pushButton_117->setIconSize(QSize(25, 25));



    QIcon pdf2(":/images/pdf.png");
    ui->boutonExporterPdf->setIcon(pdf2);
    ui->boutonExporterPdf->setIconSize(QSize(25, 25));

    QIcon pdf3(":/images/pdf.png");
    ui->pushButton_exporterpdf->setIcon(pdf3);
    ui->pushButton_exporterpdf->setIconSize(QSize(25, 25));



    QIcon pdf4(":/images/pdf.png");
    ui->pushButton_exporterPDF->setIcon(pdf4);
    ui->pushButton_exporterPDF->setIconSize(QSize(25, 25));


    QIcon pdf5(":/images/pdf.png");
    ui->pushButton_exporterpdf->setIcon(pdf5);
    ui->pushButton_exporterpdf->setIconSize(QSize(25, 25));
    QIcon tr(":/images/tri.png");
    ui->pushButton_140->setIcon(tr);
    ui->pushButton_140->setIconSize(QSize(15, 15));




    // Example: switch page when "Home" button clicked
    connect(ui->btndashboard, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(0);
    });

    connect(ui->btnpage1, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(1);
    });
    connect(ui->btnpage2, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(2);
    });
    connect(ui->btnveh, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(3);
    });
    connect(ui->btncours, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(4);
    });
    connect(ui->btnexamen, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(5);
    });
    connect(ui->btnequip, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(6);
    });
    connect(ui->btnModifier, &QPushButton::clicked, this, &MainWindow::on_btnModifier_clicked);
    connect(ui->btnStats, &QPushButton::clicked, this, &MainWindow::on_btnStats_clicked);



    // Load dynamic chart from database using Cours class

    QChartView *chartView = cours.creerStatistiques(this);

    // Replace old chart logic with layout injection
    if (ui->widget_49->layout()) {
        QLayoutItem *item;
        while ((item = ui->widget_49->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete ui->widget_49->layout();
    }

    QVBoxLayout *layout = new QVBoxLayout(ui->widget_49);
    layout->addWidget(chartView);
    ui->widget_49->setLayout(layout);

    // Button connections
    connect(ui->pushButton_105, &QPushButton::clicked, this, &MainWindow::insertData);
    connect(ui->pushButton_106, &QPushButton::clicked, this, &MainWindow::clearFields);
    connect(ui->pushButton_88, &QPushButton::clicked, this, &MainWindow::on_pushButton_88_clicked);
    connect(ui->pushButton_38, &QPushButton::clicked, this, &MainWindow::on_pushButton_38_clicked);
    connect(ui->pushButton_117, &QPushButton::clicked, this, &MainWindow::on_pushButton_117_clicked);
    connect(ui->pushBu, &QPushButton::clicked, this, &MainWindow::on_pushButton_140_clicked);

    loadDataToTable(); // initial load
    connect(ui->btndashboard, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(0);  // Show Home page
    });

    connect(ui->btnpage1, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(1);  // Show Students page
    });
    connect(ui->btnpage2, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(2);  // Show Students page
    });
    connect(ui->btnveh, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(3);  // Show Students page
    });
    connect(ui->btncours, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(4);  // Show Students page
    });
    connect(ui->btnexamen, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(5);  // Show Students page
    });
    connect(ui->btnequip, &QPushButton::clicked, [=]() {
        ui->stackedWidget->setCurrentIndex(6);  // Show Students page
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::onLogoutClicked()
{
    qDebug() << "onLogoutClicked called"; // vérifier si le slot est appelé

    QMessageBox::StandardButton reply =
        QMessageBox::question(this,
                              tr("Déconnexion"),
                              tr("Voulez-vous vraiment vous déconnecter ?"),
                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        this->close();
        // Ouvrir Login (si tu veux garder le login modal, utilise exec())
        Login *login = new Login();
        login->show();
    } else {
        // Annuler : ne rien faire
    }
}

void MainWindow::onIdProcessed(const QString &id, bool granted)
{
    QString info = QString("ID traité: %1  Accès: %2").arg(id).arg(granted ? "O" : "N");
    qDebug() << info;
    // si tu as un QLabel (ui->labelStatus) tu peux faire :
    // ui->labelStatus->setText(info);
}

// ==================== AJOUTER EMPLOYÉ ====================
void MainWindow::ajouterEmploye()
{
    if (!verifChamps()) return;

    QString nom = ui->lineEdit_nom_2->text();
    QString prenom = ui->lineEdit_prenom_2->text();
    QString genre = ui->comboBox_genre_2->currentText();
    QString cin = ui->lineEdit_cin_2->text();
    QString email = ui->lineEdit_email_2->text();
    QDate dateNaissance = ui->dateEdit_naissance_2->date();
    QString adresse = ui->lineEdit_adresse_2->text();
    QString telephone = ui->lineEdit_telephone_2->text();
    QString poste = ui->comboBox_poste_2->currentText();
    QDate dateEmbauche = ui->dateEdit_embauche_2->date();

    // ✅ Nouveaux champs
    QString question = ui->comboBoxQuestion->currentText(); // ComboBox pour questions secrètes
    QString reponse = ui->lineEditReponse->text();           // Réponse de l’employé
    QString pin = ui->lineEditPIN->text();                   // PIN 4 chiffres

    // ✅ CIN comme mot de passe par défaut (ou autre logique)
    QString mot_de_passe = cin;

    Employe e(nom, prenom, genre, cin, email, dateNaissance, adresse, telephone,
              poste, dateEmbauche, mot_de_passe, question, reponse, pin);

    if (e.ajouter()) {
        qDebug() << "✅ Employé ajouté, envoi SMS de bienvenue...";


        afficherEmployes();
        viderChamps();

        QMessageBox::information(this, "Succès",
                                 "Employé ajouté avec succès !\n\n" +
                                     prenom + " " + nom + "\n" +
                                     "Poste: " + poste + "\n\n" +
                                     "Un SMS de bienvenue a été envoyé.");
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de l'ajout de l'employé.");
    }
}

// ==================== SMS AUTOMATIQUE À L'AJOUT ====================

// =========================== AFFICHAGE DES EMPLOYÉS ===========================
void MainWindow::afficherEmployes()
{
    Employe e;
    e.remplirTableWidget(ui->table_emp_2); // Utilise la nouvelle méthode
}

// =========================== VIDER LES CHAMPS ===========================
void MainWindow::viderChamps()
{
    ui->lineEdit_nom_2->clear();
    ui->lineEdit_prenom_2->clear();
    ui->comboBox_genre_2->setCurrentIndex(0);
    ui->lineEdit_cin_2->clear();
    ui->lineEdit_email_2->clear();
    ui->dateEdit_naissance_2->setDate(QDate::currentDate());
    ui->lineEdit_adresse_2->clear();
    ui->lineEdit_telephone_2->clear();
    ui->comboBox_poste_2->setCurrentIndex(0);
    ui->dateEdit_embauche_2->setDate(QDate::currentDate());
}

// =========================== MODIFICATION D'UN EMPLOYÉ ===========================
void MainWindow::allerPageModification()
{
    int row = ui->table_emp_2->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Modification", "⚠️ Sélectionnez un employé !");
        return;
    }

    // Récupérer l'ID de l'employé sélectionné
    int idEmploye = ui->table_emp_2->item(row, 0)->text().toInt();

    // Charger les données directement depuis la base pour avoir les bonnes dates
    chargerEmployeDepuisBase(idEmploye);

    ui->tabWidget_48->setCurrentIndex(1);
}

// ==================== CHARGER EMPLOYÉ DEPUIS LA BASE ====================
void MainWindow::chargerEmployeDepuisBase(int idEmploye)
{
    QSqlQuery query;
    query.prepare("SELECT nom, prenom, genre, cin, email, datenaissance, adresse, telephone, poste, dateembauche, mot_de_passe "
                  "FROM Employes WHERE id_employe = :id");
    query.bindValue(":id", idEmploye);

    if (query.exec() && query.next()) {
        // Remplir les champs avec les données exactes de la base
        ui->lineEdit_nom_2->setText(query.value("nom").toString());
        ui->lineEdit_prenom_2->setText(query.value("prenom").toString());
        ui->comboBox_genre_2->setCurrentText(query.value("genre").toString());
        ui->lineEdit_cin_2->setText(query.value("cin").toString());
        ui->lineEdit_email_2->setText(query.value("email").toString());

        // ✅ Dates exactes depuis la base
        ui->dateEdit_naissance_2->setDate(query.value("datenaissance").toDate());
        ui->dateEdit_embauche_2->setDate(query.value("dateembauche").toDate());

        ui->lineEdit_adresse_2->setText(query.value("adresse").toString());
        ui->lineEdit_telephone_2->setText(query.value("telephone").toString());
        ui->comboBox_poste_2->setCurrentText(query.value("poste").toString());

        qDebug() << "✅ Données chargées depuis la base pour ID:" << idEmploye;
    } else {
        QMessageBox::critical(this, "Erreur", "Impossible de charger les données de l'employé.");
    }
}

void MainWindow::modifierEmploye()
{
    if (!verifChamps()) return; // Vérifie que tous les champs sont remplis

    // Récupération des valeurs depuis les widgets
    QString nom = ui->lineEdit_nom_2->text();
    QString prenom = ui->lineEdit_prenom_2->text();
    QString genre = ui->comboBox_genre_2->currentText();
    QString cin = ui->lineEdit_cin_2->text();
    QString email = ui->lineEdit_email_2->text();
    QDate dateNaissance = ui->dateEdit_naissance_2->date();
    QString adresse = ui->lineEdit_adresse_2->text();
    QString telephone = ui->lineEdit_telephone_2->text();
    QString poste = ui->comboBox_poste_2->currentText();
    QDate dateEmbauche = ui->dateEdit_embauche_2->date();
    QString mot_de_passe = ui->lineEdit_cin_2->text(); // exemple : CIN comme mot de passe

    // Nouveaux champs pour la récupération de mot de passe
    QString question = ui->comboBoxQuestion->currentText();
    QString reponse = ui->lineEditReponse->text();
    QString pin = ui->lineEditPIN->text();

    int id = ui->lineEdit_cin_2->text().toInt(); // L'id de l'employé à modifier

    // Création de l'objet Employe avec le constructeur correct
    Employe e(nom, prenom, genre, cin, email, dateNaissance,
              adresse, telephone, poste, dateEmbauche, mot_de_passe,
              question, reponse, pin, id);

    // Préparer la requête SQL de modification
    QSqlQuery query;
    query.prepare("UPDATE Employes SET "
                  "nom = :nom, prenom = :prenom, genre = :genre, cin = :cin, "
                  "email = :email, datenaissance = :datenaissance, adresse = :adresse, "
                  "telephone = :telephone, poste = :poste, dateembauche = :dateembauche, "
                  "mot_de_passe = :mot_de_passe, question = :question, reponse = :reponse, a_pin = :pin "
                  "WHERE id = :id");

    query.bindValue(":nom", e.getNom());
    query.bindValue(":prenom", e.getPrenom());
    query.bindValue(":genre", e.getGenre());
    query.bindValue(":cin", e.getCin());
    query.bindValue(":email", e.getEmail());
    query.bindValue(":datenaissance", e.getDateNaissance());
    query.bindValue(":adresse", e.getAdresse());
    query.bindValue(":telephone", e.getTelephone());
    query.bindValue(":poste", e.getPoste());
    query.bindValue(":dateembauche", e.getDateEmbauche());
    query.bindValue(":mot_de_passe", e.getMotDePasse());
    query.bindValue(":question", e.getQuestion());
    query.bindValue(":reponse", e.getReponse());
    query.bindValue(":pin", e.getPin());
    query.bindValue(":id", e.getId());

    if (query.exec()) {
        QMessageBox::information(this, "Succès", "✅ Employé modifié avec succès !");
        afficherEmployes();   // Rafraîchir la table
        viderChamps();        // Vider les champs du formulaire
    } else {
        QMessageBox::critical(this, "Erreur", "❌ Erreur lors de la modification : " + query.lastError().text());
    }
}

// ==================== OBTENIR MOT DE PASSE ACTUEL ====================
QString MainWindow::obtenirMotDePasseActuel(int idEmploye)
{
    QSqlQuery query;
    query.prepare("SELECT mot_de_passe FROM Employes WHERE id_employe = :id");
    query.bindValue(":id", idEmploye);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "1234"; // Fallback
}

void MainWindow::remplirChampsDepuisTable_emp(int row, int column) {
    if (row < 0) return;

    ui->lineEdit_nom_2->setText(ui->table_emp_2->item(row, 1)->text());
    ui->lineEdit_prenom_2->setText(ui->table_emp_2->item(row, 2)->text());
    ui->comboBox_genre_2->setCurrentText(ui->table_emp_2->item(row, 3)->text());
    ui->lineEdit_cin_2->setText(ui->table_emp_2->item(row, 4)->text());
    ui->lineEdit_email_2->setText(ui->table_emp_2->item(row, 5)->text());
    ui->lineEdit_adresse_2->setText(ui->table_emp_2->item(row, 7)->text());
    ui->lineEdit_telephone_2->setText(ui->table_emp_2->item(row, 8)->text());
    ui->comboBox_poste_2->setCurrentText(ui->table_emp_2->item(row, 9)->text());

    // ✅ Gestion robuste des dates
    QString dateNaissanceStr = ui->table_emp_2->item(row, 6)->text();
    QString dateEmbaucheStr = ui->table_emp_2->item(row, 10)->text();

    // Essayer plusieurs formats de date
    QDate dateNaissance = QDate::fromString(dateNaissanceStr, "yyyy-MM-dd");
    if (!dateNaissance.isValid()) {
        dateNaissance = QDate::fromString(dateNaissanceStr, "dd/MM/yyyy");
    }
    if (!dateNaissance.isValid()) {
        dateNaissance = QDate::fromString(dateNaissanceStr, Qt::ISODate);
    }

    QDate dateEmbauche = QDate::fromString(dateEmbaucheStr, "yyyy-MM-dd");
    if (!dateEmbauche.isValid()) {
        dateEmbauche = QDate::fromString(dateEmbaucheStr, "dd/MM/yyyy");
    }
    if (!dateEmbauche.isValid()) {
        dateEmbauche = QDate::fromString(dateEmbaucheStr, Qt::ISODate);
    }

    if (dateNaissance.isValid()) {
        ui->dateEdit_naissance_2->setDate(dateNaissance);
    }
    if (dateEmbauche.isValid()) {
        ui->dateEdit_embauche_2->setDate(dateEmbauche);
    }
}

// ==================== SUPPRIMER EMPLOYÉ ====================
void MainWindow::supprimerEmploye()
{
    int row = ui->table_emp_2->currentRow();
    if(row < 0) {
        QMessageBox::warning(this, "Suppression", "⚠️ Sélectionnez un employé !");
        return;
    }

    int id = ui->table_emp_2->item(row, 0)->text().toInt();
    Employe e;

    if(e.supprimer(id)) {
        QMessageBox::information(this, "Succès", "✅ Employé supprimé !");
        afficherEmployes();
    } else {
        QMessageBox::critical(this, "Erreur", "❌ Échec de la suppression !");
    }
}

// ==================== VÉRIFICATION DES CHAMPS ====================
bool MainWindow::verifChamps()
{
    bool ok = true;
    QString message = "";
    QString styleError = "border: 2px solid red; background-color: #FFE6E6;";
    QString styleNormal = "border: 1px solid #ccc; background-color: white;";

    // Récupération des valeurs
    QString nom = ui->lineEdit_nom_2->text().trimmed();
    QString prenom = ui->lineEdit_prenom_2->text().trimmed();
    QString cin = ui->lineEdit_cin_2->text().trimmed();
    QString email = ui->lineEdit_email_2->text().trimmed();
    QString telephone = ui->lineEdit_telephone_2->text().trimmed();
    QString adresse = ui->lineEdit_adresse_2->text().trimmed();
    QString poste = ui->comboBox_poste_2->currentText();
    QString genre = ui->comboBox_genre_2->currentText();
    QDate dateNaissance = ui->dateEdit_naissance_2->date();
    QDate dateEmbauche = ui->dateEdit_embauche_2->date();

    // Réinitialisation du style
    ui->lineEdit_nom_2->setStyleSheet(styleNormal);
    ui->lineEdit_prenom_2->setStyleSheet(styleNormal);
    ui->lineEdit_cin_2->setStyleSheet(styleNormal);
    ui->lineEdit_email_2->setStyleSheet(styleNormal);
    ui->lineEdit_telephone_2->setStyleSheet(styleNormal);
    ui->lineEdit_adresse_2->setStyleSheet(styleNormal);
    ui->comboBox_poste_2->setStyleSheet(styleNormal);
    ui->comboBox_genre_2->setStyleSheet(styleNormal);
    ui->dateEdit_naissance_2->setStyleSheet(styleNormal);
    ui->dateEdit_embauche_2->setStyleSheet(styleNormal);

    // ==========================
    // VALIDATION PROFESSIONNELLE
    // ==========================

    // 🔹 Validation du NOM
    if (nom.isEmpty()) {
        ui->lineEdit_nom_2->setStyleSheet(styleError);
        message += "❌ Le nom est obligatoire.\n";
        ok = false;
    } else if (nom.length() < 2) {
        ui->lineEdit_nom_2->setStyleSheet(styleError);
        message += "❌ Le nom doit contenir au moins 2 caractères.\n";
        ok = false;
    } else if (!nom.contains(QRegularExpression("^[A-Za-zÀ-ÿ\\s\\-']+$"))) {
        ui->lineEdit_nom_2->setStyleSheet(styleError);
        message += "❌ Le nom ne peut contenir que des lettres, espaces, traits d'union et apostrophes.\n";
        ok = false;
    }

    // 🔹 Validation du PRÉNOM
    if (prenom.isEmpty()) {
        ui->lineEdit_prenom_2->setStyleSheet(styleError);
        message += "❌ Le prénom est obligatoire.\n";
        ok = false;
    } else if (prenom.length() < 2) {
        ui->lineEdit_prenom_2->setStyleSheet(styleError);
        message += "❌ Le prénom doit contenir au moins 2 caractères.\n";
        ok = false;
    } else if (!prenom.contains(QRegularExpression("^[A-Za-zÀ-ÿ\\s\\-']+$"))) {
        ui->lineEdit_prenom_2->setStyleSheet(styleError);
        message += "❌ Le prénom ne peut contenir que des lettres, espaces, traits d'union et apostrophes.\n";
        ok = false;
    }

    // 🔹 Validation du CIN
    if (cin.isEmpty()) {
        ui->lineEdit_cin_2->setStyleSheet(styleError);
        message += "❌ Le CIN est obligatoire.\n";
        ok = false;
    } else if (!cin.contains(QRegularExpression("^[0-9]{8}$"))) {
        ui->lineEdit_cin_2->setStyleSheet(styleError);
        message += "❌ Le CIN doit contenir exactement 8 chiffres.\n";
        ok = false;
    }

    // 🔹 Validation de l'EMAIL
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (email.isEmpty()) {
        ui->lineEdit_email_2->setStyleSheet(styleError);
        message += "❌ L'email est obligatoire.\n";
        ok = false;
    } else if (!emailRegex.match(email).hasMatch()) {
        ui->lineEdit_email_2->setStyleSheet(styleError);
        message += "❌ Format d'email invalide. Exemple: exemple@domaine.com\n";
        ok = false;
    }

    // 🔹 Validation du TÉLÉPHONE (Format International +216)
    QRegularExpression phoneRegex("^(\\+216|00216)?[24579][0-9]{7}$");
    QString telClean = telephone;

    // Nettoyer le format
    telClean = telClean.replace(" ", "").replace("-", "").replace(".", "");

    if (telephone.isEmpty()) {
        ui->lineEdit_telephone_2->setStyleSheet(styleError);
        message += "❌ Le numéro de téléphone est obligatoire.\n";
        ok = false;
    } else if (!phoneRegex.match(telClean).hasMatch()) {
        ui->lineEdit_telephone_2->setStyleSheet(styleError);
        message += "❌ Format de téléphone invalide.\n";
        message += "📞 Formats acceptés:\n";
        message += "   • +216XXXXXXXX\n";
        message += "   • 00216XXXXXXXX\n";
        message += "   • XXXXXXXXX (8 chiffres)\n";
        message += "   Le premier chiffre doit être 2, 4, 5, 7 ou 9\n";
        ok = false;
    } else {
        // Formater automatiquement le téléphone
        QString telFormatted;
        if (telClean.startsWith("+216")) {
            telFormatted = telClean;
        } else if (telClean.startsWith("00216")) {
            telFormatted = "+" + telClean.mid(2);
        } else if (telClean.length() == 8) {
            telFormatted = "+216" + telClean;
        }
        ui->lineEdit_telephone_2->setText(telFormatted);
    }

    // 🔹 Validation du GENRE
    if (ui->comboBox_genre_2->currentIndex() == 0) {
        ui->comboBox_genre_2->setStyleSheet(styleError);
        message += "❌ Veuillez sélectionner un genre.\n";
        ok = false;
    }

    // 🔹 Validation du POSTE
    if (ui->comboBox_poste_2->currentIndex() == 0) {
        ui->comboBox_poste_2->setStyleSheet(styleError);
        message += "❌ Veuillez sélectionner un poste.\n";
        ok = false;
    }

    // 🔹 Validation DATE DE NAISSANCE
    QDate today = QDate::currentDate();
    int age = dateNaissance.daysTo(today) / 365;

    if (dateNaissance >= today) {
        ui->dateEdit_naissance_2->setStyleSheet(styleError);
        message += "❌ La date de naissance doit être antérieure à aujourd'hui.\n";
        ok = false;
    } else if (age < 18) {
        ui->dateEdit_naissance_2->setStyleSheet(styleError);
        message += "❌ L'employé doit avoir au moins 18 ans.\n";
        ok = false;
    } else if (age > 70) {
        ui->dateEdit_naissance_2->setStyleSheet(styleError);
        message += "❌ L'âge de l'employé ne peut pas dépasser 70 ans.\n";
        ok = false;
    }

    // 🔹 Validation DATE D'EMBAUCHE
    if (dateEmbauche < dateNaissance.addYears(18)) {
        ui->dateEdit_embauche_2->setStyleSheet(styleError);
        message += "❌ La date d'embauche ne peut pas être avant l'âge de 18 ans.\n";
        ok = false;
    } else if (dateEmbauche > today.addDays(30)) {
        ui->dateEdit_embauche_2->setStyleSheet(styleError);
        message += "❌ La date d'embauche ne peut pas être plus de 30 jours dans le futur.\n";
        ok = false;
    }

    // 🔹 Validation ADRESSE
    if (adresse.isEmpty()) {
        ui->lineEdit_adresse_2->setStyleSheet(styleError);
        message += "❌ L'adresse est obligatoire.\n";
        ok = false;
    } else if (adresse.length() < 10) {
        ui->lineEdit_adresse_2->setStyleSheet(styleError);
        message += "❌ L'adresse doit contenir au moins 10 caractères.\n";
        ok = false;
    }

    // 🔹 Vérification de doublons (CIN et Email)
    if (ok) {
        QSqlQuery checkQuery;

        // Vérifier CIN unique
        checkQuery.prepare("SELECT COUNT(*) FROM Employes WHERE cin = :cin AND id_employe != :id");
        checkQuery.bindValue(":cin", cin);
        checkQuery.bindValue(":id", ui->table_emp_2->currentRow() >= 0 ?
                                        ui->table_emp_2->item(ui->table_emp_2->currentRow(), 0)->text().toInt() : 0);

        if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            ui->lineEdit_cin_2->setStyleSheet(styleError);
            message += "❌ Ce CIN est déjà utilisé par un autre employé.\n";
            ok = false;
        }

        // Vérifier Email unique
        checkQuery.prepare("SELECT COUNT(*) FROM Employes WHERE email = :email AND id_employe != :id");
        checkQuery.bindValue(":email", email);
        checkQuery.bindValue(":id", ui->table_emp_2->currentRow() >= 0 ?
                                        ui->table_emp_2->item(ui->table_emp_2->currentRow(), 0)->text().toInt() : 0);

        if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
            ui->lineEdit_email_2->setStyleSheet(styleError);
            message += "❌ Cet email est déjà utilisé par un autre employé.\n";
            ok = false;
        }
    }

    // Affichage des messages d'erreur
    if (!ok) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Validation des champs");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Veuillez corriger les erreurs suivantes :");
        msgBox.setDetailedText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    } else {
        // Style de succès pour tous les champs
        QString styleSuccess = "border: 2px solid #28a745; background-color: #F0FFF0;";
        ui->lineEdit_nom_2->setStyleSheet(styleSuccess);
        ui->lineEdit_prenom_2->setStyleSheet(styleSuccess);
        ui->lineEdit_cin_2->setStyleSheet(styleSuccess);
        ui->lineEdit_email_2->setStyleSheet(styleSuccess);
        ui->lineEdit_telephone_2->setStyleSheet(styleSuccess);
        ui->lineEdit_adresse_2->setStyleSheet(styleSuccess);
        ui->comboBox_poste_2->setStyleSheet(styleSuccess);
        ui->comboBox_genre_2->setStyleSheet(styleSuccess);
        ui->dateEdit_naissance_2->setStyleSheet(styleSuccess);
        ui->dateEdit_embauche_2->setStyleSheet(styleSuccess);
    }

    return ok;
}

// ==================== FORMATAGE AUTOMATIQUE DU TÉLÉPHONE ====================
void MainWindow::on_lineEdit_telephone_2_textChanged(const QString &text)
{
    // Nettoyer le texte (supprimer tout sauf les chiffres et +)
    QString cleanText = text;
    cleanText = cleanText.replace(" ", "").replace("-", "").replace(".", "").replace("(", "").replace(")", "");

    // Si le texte commence par +216, formater avec des espaces
    if (cleanText.startsWith("+216") && cleanText.length() == 12) {
        QString formatted = "+216 " + cleanText.mid(4, 2) + " " + cleanText.mid(6, 3) + " " + cleanText.mid(9, 3);
        if (text != formatted) {
            ui->lineEdit_telephone_2->setText(formatted);
        }
    }
    // Si c'est un numéro local (8 chiffres), formater
    else if (cleanText.length() == 8 && cleanText.contains(QRegularExpression("^[24579][0-9]{7}$"))) {
        QString formatted = "+216 " + cleanText.left(2) + " " + cleanText.mid(2, 3) + " " + cleanText.mid(5, 3);
        if (text != formatted) {
            ui->lineEdit_telephone_2->setText(formatted);
        }
    }
}

// ==================== TRIER EMPLOYÉS ====================
void MainWindow::trierEmployes(int index)
{
    Employe e;
    e.trierEtRemplirTable(ui->table_emp_2, index); // Utilise la nouvelle méthode
}

// ==================== RECHERCHER EMPLOYÉ ====================
void MainWindow::rechercherEmploye()
{
    QString texteRecherche = ui->recherche_emp->text().trimmed();
    int typeRecherche = ui->rech_emp->currentIndex();

    Employe e;
    e.rechercherEtRemplirTable(ui->table_emp_2, texteRecherche, typeRecherche); // Utilise la nouvelle méthode
}

// ==================== EXPORTER PDF ====================
void MainWindow::exporterPDF_Employes()
{
    Employe e;
    if (e.exporterPDF_Employes(ui->table_emp_2)) {
        QMessageBox::information(this, "Succès", "PDF exporté avec succès !");
        ui->tabWidget_48->setCurrentIndex(0);
    }
}

// ==================== STATISTIQUES ====================
void MainWindow::afficherStatistiquePoste()
{
    Employe e;
    QMap<QString, int> stats = e.getStatistiquePoste();
    QMap<QString, int> compteurPoste;
    int rowCount = ui->table_emp_2->rowCount();
    int colPoste = 9; // colonne du poste

    for(int r = 0; r < rowCount; ++r) {
        if(ui->table_emp_2->isRowHidden(r)) continue;
        QTableWidgetItem* item = ui->table_emp_2->item(r, colPoste);
        if(!item) continue;
        QString poste = item->text();
        compteurPoste[poste]++;
    }

    // Préparer la série de barres
    QBarSet *set = new QBarSet("Nombre d'employés");
    QStringList categories;
    for(auto it = compteurPoste.begin(); it != compteurPoste.end(); ++it) {
        *set << it.value();
        categories << it.key();
    }

    QBarSeries *series = new QBarSeries();
    series->append(set);

    // Créer le graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Statistique des employés par poste");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Axe X
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Axe Y
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, std::max(1, *std::max_element(compteurPoste.begin(), compteurPoste.end())));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Afficher dans la page 3
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Supprimer l'ancien widget s'il existe
    QLayout *layout = ui->tabWidget_48->widget(2)->layout();
    if(layout) {
        QLayoutItem *item;
        while((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    } else {
        layout = new QVBoxLayout(ui->tabWidget_48->widget(2));
        ui->tabWidget_48->widget(2)->setLayout(layout);
    }

    layout->addWidget(chartView);
}

// ==================== AFFICHER DÉTAILS EMPLOYÉ ====================
void MainWindow::on_affich_2_clicked()
{
    // Vérifier qu'une ligne est sélectionnée
    QList<QTableWidgetSelectionRange> selection = ui->table_emp_2->selectedRanges();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "Sélection", "Veuillez sélectionner un employé dans le tableau.");
        return;
    }

    int row = selection.first().topRow();

    // Récupération des valeurs (adapte les index selon tes colonnes)
    auto getText = [&](int col) -> QString {
        QTableWidgetItem *item = ui->table_emp_2->item(row, col);
        return item ? item->text() : "";
    };

    QString id       = getText(0);
    QString nom      = getText(1);
    QString prenom   = getText(2);
    QString genre    = getText(3);
    QString cinEmp   = getText(4);
    QString email    = getText(5);
    QString tel      = getText(6);
    QString adresse  = getText(7);
    QString poste    = getText(9);   // 👉 poste en colonne 9

    // HTML avec style "grand et professionnel"
    QString html;
    html += R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<style>
    body {
        font-family: Arial, sans-serif;
        background: #f4f6f7;
    }
    .container {
        width: 80%;
        margin: 20px auto;
        text-align: left;
    }
    .title {
        text-align: center;
        font-size: 26px;
        font-weight: bold;
        color: #2c3e50;
        margin-bottom: 20px;
        text-transform: uppercase;
        letter-spacing: 1px;
    }
    .card {
        background: #ffffff;
        border-radius: 10px;
        padding: 20px 30px;
        box-shadow: 0 4px 10px rgba(0,0,0,0.1);
    }
    .emp-name {
        font-size: 22px;
        font-weight: bold;
        color: #34495e;
        margin-bottom: 10px;
    }
    .emp-id {
        font-size: 14px;
        color: #7f8c8d;
        margin-bottom: 20px;
    }
    .section-title {
        font-size: 16px;
        font-weight: bold;
        color: #2c3e50;
        margin-top: 15px;
        margin-bottom: 8px;
        border-left: 4px solid #3498db;
        padding-left: 8px;
    }
    .info-row {
        margin-bottom: 6px;
        font-size: 14px;
    }
    .label {
        font-weight: bold;
        color: #2c3e50;
        display: inline-block;
        width: 120px;
    }
    .value {
        color: #34495e;
    }
</style>
</head>
<body>
<div class="container">
    <div class="title">Détail de l'employé</div>
    <div class="card">
)";

    // Nom + prénom en grand
    html += QString(R"(
        <div class="emp-name">%1 %2</div>
        <div class="emp-id">ID Employé : %3</div>
)").arg(nom, prenom, id);

    // Section Informations générales
    html += R"(
        <div class="section-title">Informations générales</div>
)";
    html += QString(R"(
        <div class="info-row"><span class="label">Genre :</span><span class="value">%1</span></div>
        <div class="info-row"><span class="label">CIN :</span><span class="value">%2</span></div>
        <div class="info-row"><span class="label">Poste :</span><span class="value">%3</span></div>
)").arg(genre, cinEmp, poste);

    // Section Contact
    html += R"(
        <div class="section-title">Contact</div>
)";
    html += QString(R"(
        <div class="info-row"><span class="label">Email :</span><span class="value">%1</span></div>
        <div class="info-row"><span class="label">Téléphone :</span><span class="value">%2</span></div>
        <div class="info-row"><span class="label">Adresse :</span><span class="value">%3</span></div>
)").arg(email, tel, adresse);

    html += R"(
    </div> <!-- card -->
</div> <!-- container -->
</body>
</html>
)";

    // Afficher dans le QTextBrowser
    ui->textBrowser_detail->setHtml(html);

    // Aller sur l'onglet des détails (si besoin)
    ui->tabWidget_48->setCurrentIndex(4); // adapte si ce n'est pas l'index 3
}


// ==================== FORMULAIRE DE CONNEXION ====================
void MainWindow::afficherFormulaireConnexion()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("🔐 Connexion Employé");
    dialog->setFixedSize(300, 200);

    QVBoxLayout *layout = new QVBoxLayout(dialog);

    QLabel *labelTitre = new QLabel("CONNEXION CHAT");
    labelTitre->setAlignment(Qt::AlignCenter);
    labelTitre->setStyleSheet("font-weight: bold; font-size: 16px; color: #2c3e50;");

    QLabel *labelNom = new QLabel("Nom:");
    QLineEdit *editNom = new QLineEdit();
    editNom->setPlaceholderText("Entrez votre nom");

    QLabel *labelMdp = new QLabel("Mot de passe:");
    QLineEdit *editMdp = new QLineEdit();
    editMdp->setPlaceholderText("Entrez votre mot de passe");
    editMdp->setEchoMode(QLineEdit::Password);

    QPushButton *btnConnexion = new QPushButton("Se connecter");
    btnConnexion->setStyleSheet(
        "QPushButton {"
        "    background: #3498db;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 10px;"
        "    padding: 10px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover { background: #2980b9; }"
        );

    layout->addWidget(labelTitre);
    layout->addWidget(labelNom);
    layout->addWidget(editNom);
    layout->addWidget(labelMdp);
    layout->addWidget(editMdp);
    layout->addWidget(btnConnexion);

    // Connexion du bouton
    connect(btnConnexion, &QPushButton::clicked, this, [=]() {
        QString nom = editNom->text().trimmed();
        QString mdp = editMdp->text().trimmed();

        if (nom.isEmpty() || mdp.isEmpty()) {
            QMessageBox::warning(dialog, "Erreur", "Veuillez remplir tous les champs!");
            return;
        }

        int idEmploye = Employe::authentifierEmploye(nom, mdp);
        if (idEmploye != -1) {
            currentEmployeId = idEmploye;
            currentEmployeNom = Employe::getNomEmploye(idEmploye);

            QMessageBox::information(dialog, "Succès",
                                     QString("Bienvenue %1!").arg(currentEmployeNom));

            dialog->accept();
            chargerEmployesDestinataires();
            chargerMessages();
            msgRefreshTimer->start(5000);

        } else {
            QMessageBox::critical(dialog, "Erreur",
                                  "Nom ou mot de passe incorrect!");
        }
    });

    dialog->exec();
}

// ==================== MESSAGERIE ====================
void MainWindow::setupMessagerieInTab()
{
    QWidget *ongletMessagerie = ui->tabWidget_48->widget(3);
    ongletMessagerie->setStyleSheet("background: #FFFFFF;");

    QVBoxLayout *layoutMessagerie = new QVBoxLayout(ongletMessagerie);
    layoutMessagerie->setContentsMargins(0, 0, 0, 0);
    layoutMessagerie->setSpacing(0);

    // === EN-TÊTE AVEC SÉLECTION DE DESTINATAIRE ===
    QWidget *headerWidget = new QWidget();
    headerWidget->setFixedHeight(70);
    headerWidget->setStyleSheet(
        "QWidget {"
        "    background: #FFFFFF;"
        "    border-bottom: 1px solid #E0E0E0;"
        "}"
        );

    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(15, 5, 15, 5);

    // Première ligne : Titre et boutons
    QHBoxLayout *topHeaderLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel("💬 Messagerie");
    titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #000000;"
        "    font-weight: bold;"
        "    font-size: 18px;"
        "}"
        );

    QPushButton *btnRefresh = new QPushButton("🔄");
    QPushButton *btnSearch = new QPushButton("🔍");

    QString btnActionStyle =
        "QPushButton {"
        "    background: transparent;"
        "    border: none;"
        "    font-size: 16px;"
        "    color: #666666;"
        "    padding: 5px;"
        "}"
        "QPushButton:hover {"
        "    background: #F0F0F0;"
        "    border-radius: 15px;"
        "}";

    btnRefresh->setStyleSheet(btnActionStyle);
    btnSearch->setStyleSheet(btnActionStyle);
    btnRefresh->setFixedSize(30, 30);
    btnSearch->setFixedSize(30, 30);

    topHeaderLayout->addWidget(titleLabel);
    topHeaderLayout->addStretch();
    topHeaderLayout->addWidget(btnRefresh);
    topHeaderLayout->addWidget(btnSearch);

    // Deuxième ligne : Sélection du destinataire
    QHBoxLayout *destinataireLayout = new QHBoxLayout();

    QLabel *labelDestinataire = new QLabel("Discuter avec:");
    labelDestinataire->setStyleSheet(
        "QLabel {"
        "    color: #666666;"
        "    font-size: 12px;"
        "}"
        );

    comboDestinataires = new QComboBox();
    comboDestinataires->setStyleSheet(
        "QComboBox {"
        "    background: #F8F9FA;"
        "    border: 1px solid #E0E0E0;"
        "    border-radius: 15px;"
        "    padding: 8px 15px;"
        "    font-size: 14px;"
        "    min-width: 200px;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "    image: none;"
        "    border-left: 5px solid transparent;"
        "    border-right: 5px solid transparent;"
        "    border-top: 5px solid #666666;"
        "}"
        "QComboBox QAbstractItemView {"
        "    border: 1px solid #E0E0E0;"
        "    border-radius: 10px;"
        "    background: white;"
        "    selection-background-color: #0084FF;"
        "}"
        );

    // Ajouter option "Sélectionner un destinataire"
    comboDestinataires->addItem("👥 Sélectionner un employé", -1);

    destinataireLayout->addWidget(labelDestinataire);
    destinataireLayout->addWidget(comboDestinataires);
    destinataireLayout->addStretch();

    headerLayout->addLayout(topHeaderLayout);
    headerLayout->addLayout(destinataireLayout);

    // === ZONE DES MESSAGES ===
    displayMessagerie = new QTextEdit();
    displayMessagerie->setReadOnly(true);
    displayMessagerie->setStyleSheet(
        "QTextEdit {"
        "    background: #FAFAFA;"
        "    border: none;"
        "    font-family: 'Segoe UI';"
        "    font-size: 14px;"
        "    padding: 10px;"
        "}"
        );

    // === ZONE DE SAISIE ===
    QWidget *inputWidget = new QWidget();
    inputWidget->setFixedHeight(70);
    inputWidget->setStyleSheet(
        "QWidget {"
        "    background: #FFFFFF;"
        "    border-top: 1px solid #E0E0E0;"
        "}"
        );

    QHBoxLayout *inputLayout = new QHBoxLayout(inputWidget);
    inputLayout->setContentsMargins(15, 10, 15, 10);

    inputMessage = new QLineEdit();
    inputMessage->setPlaceholderText("Écrivez votre message...");
    inputMessage->setStyleSheet(
        "QLineEdit {"
        "    background: #F0F0F0;"
        "    border: 1px solid #E0E0E0;"
        "    border-radius: 20px;"
        "    padding: 12px 20px;"
        "    font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "    border-color: #0084FF;"
        "    background: #FFFFFF;"
        "}"
        );

    btnEnvoyerMsg = new QPushButton("Envoyer");
    btnEnvoyerMsg->setFixedSize(80, 40);
    btnEnvoyerMsg->setStyleSheet(
        "QPushButton {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "                              stop:0 #0084FF, stop:1 #00C6FF);"
        "    border: none;"
        "    border-radius: 20px;"
        "    color: white;"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "                              stop:0 #0073E6, stop:1 #00B8E6);"
        "}"
        "QPushButton:disabled {"
        "    background: #CCCCCC;"
        "    color: #666666;"
        "}"
        );

    inputLayout->addWidget(inputMessage);
    inputLayout->addWidget(btnEnvoyerMsg);

    // === ASSEMBLAGE ===
    layoutMessagerie->addWidget(headerWidget);
    layoutMessagerie->addWidget(displayMessagerie, 1);
    layoutMessagerie->addWidget(inputWidget);

    // === CONNECTIONS ===
    connect(btnEnvoyerMsg, &QPushButton::clicked, this, &MainWindow::on_btnEnvoyerMessage_clicked);
    connect(inputMessage, &QLineEdit::returnPressed, this, &MainWindow::on_btnEnvoyerMessage_clicked);
    connect(comboDestinataires, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDestinataireChange);
    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::refreshMessagerie);

    // Timer de rafraîchissement
    msgRefreshTimer = new QTimer(this);
    connect(msgRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshMessagerie);

    // Désactiver l'envoi initialement
    btnEnvoyerMsg->setEnabled(false);
    inputMessage->setEnabled(false);
}

// ==================== AFFICHER MESSAGES AVEC STYLE ====================
// ==================== AFFICHER MESSAGES COMME RÉSEAU SOCIAL ====================
// ==================== AFFICHER MESSAGES STYLE SIMPLE ====================

void MainWindow::afficherMessagesWhatsApp(const QList<QJsonObject>& messages, const QString& nomDestinataire)
{
    QString html = R"(
        <!DOCTYPE html>
        <html>
        <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
            /* STYLE WHATSAPP SIMPLE */
            * {
                margin: 0;
                padding: 0;
                box-sizing: border-box;
            }

            body {
                font-family: 'Segoe UI', Arial, sans-serif;
                background: #0c1317;
                margin: 0;
                padding: 10px 8px 20px 8px;
                color: #e9edef;
            }

            .chat-container {
                display: flex;
                flex-direction: column;
                gap: 3px;
            }

            /* DATE */
            .date-sep {
                text-align: center;
                margin: 15px 0;
                color: #8696a0;
                font-size: 12.5px;
                font-weight: 500;
            }

            /* MESSAGE */
            .msg-row {
                display: flex;
                margin: 2px 0;
            }

            .msg-row.moi {
                justify-content: flex-end;
            }

            .msg-row.lui {
                justify-content: flex-start;
            }

            .msg-bubble {
                max-width: 70%;
                padding: 8px 12px;
                border-radius: 7.5px;
                position: relative;
                word-wrap: break-word;
                box-shadow: 0 1px 0.5px rgba(0,0,0,0.13);
            }

            /* MES MESSAGES À DROITE (VERT) */
            .msg-bubble.moi {
                background: #005c4b;
                color: white;
                border-top-right-radius: 0;
                margin-left: auto;
            }

            .msg-bubble.moi:before {
                content: '';
                position: absolute;
                top: 0;
                right: -8px;
                width: 0;
                height: 0;
                border: 8px solid transparent;
                border-left-color: #005c4b;
                border-top: 0;
            }

            /* MESSAGES REÇUS À GAUCHE (GRIS) */
            .msg-bubble.lui {
                background: #1f2c33;
                color: #e9edef;
                border-top-left-radius: 0;
                margin-right: auto;
            }

            .msg-bubble.lui:before {
                content: '';
                position: absolute;
                top: 0;
                left: -8px;
                width: 0;
                height: 0;
                border: 8px solid transparent;
                border-right-color: #1f2c33;
                border-top: 0;
            }

            .msg-text {
                font-size: 14.2px;
                line-height: 19px;
                word-wrap: break-word;
            }

            .msg-info {
                text-align: right;
                font-size: 11px;
                color: rgba(255,255,255,0.6);
                margin-top: 4px;
                display: flex;
                align-items: center;
                justify-content: flex-end;
                gap: 3px;
            }

            .msg-bubble.lui .msg-info {
                color: rgba(255,255,255,0.45);
            }

            .msg-time {
                opacity: 0.8;
            }

            .msg-status {
                font-size: 13px;
            }

            .sender {
                font-size: 12.5px;
                font-weight: 500;
                color: #00a884;
                margin-bottom: 2px;
            }
        </style>
        </head>
        <body>
        <div class="chat-container">
    )";

    QString currentDate = "";

    for (const QJsonObject &msg : messages) {
        QString texte = msg["message"].toString();
        QString dateEnvoi = msg["date_envoi"].toString();
        int idExpediteur = msg["id_expediteur"].toInt();
        bool isLu = msg["lu"].toBool();

        QStringList dateParts = dateEnvoi.split(" ");
        QString dateStr = dateParts.value(0, "");
        QString timeStr = dateParts.value(1, "").left(5);

        QDate msgDate = QDate::fromString(dateStr, "dd/MM/yyyy");
        if (!msgDate.isValid()) {
            msgDate = QDate::fromString(dateStr, "yyyy-MM-dd");
        }

        if (dateStr != currentDate) {
            currentDate = dateStr;
            QString displayDate;

            if (msgDate == QDate::currentDate()) {
                displayDate = "Aujourd'hui";
            } else if (msgDate == QDate::currentDate().addDays(-1)) {
                displayDate = "Hier";
            } else {
                displayDate = msgDate.toString("dd/MM/yyyy");
            }

            html += QString("<div class='date-sep'>%1</div>").arg(displayDate);
        }

        bool estMoi = (idExpediteur == currentEmployeId);
        QString classe = estMoi ? "moi" : "lui";
        QString senderName = estMoi ? "" : QString("<div class='sender'>%1</div>").arg(nomDestinataire);

        QString statusIcon = "";
        if (estMoi) {
            statusIcon = isLu ? "✓✓" : "✓✓";
        }

        html += QString(R"(
            <div class="msg-row %1">
                <div class="msg-bubble %1">
                    %2
                    <div class="msg-text">%3</div>
                    <div class="msg-info">
                        <span class="msg-time">%4</span>
                        <span class="msg-status">%5</span>
                    </div>
                </div>
            </div>
        )").arg(classe,
                         senderName,
                         texte.toHtmlEscaped().replace("\n", "<br>"),
                         timeStr,
                         statusIcon);
    }

    html += R"(
        </div>
        <script>
            setTimeout(function() {
                window.scrollTo(0, document.body.scrollHeight);
            }, 100);
        </script>
        </body>
        </html>
    )";

    displayMessagerie->setHtml(html);

    QTimer::singleShot(150, this, [this]() {
        QScrollBar *sb = displayMessagerie->verticalScrollBar();
        if (sb) sb->setValue(sb->maximum());
    });
}


// ==================== CHARGER DISCUSSION WHATSAPP ====================
void MainWindow::on_btnEnvoyerMessage_clicked()
{
    static bool enCoursDenvoi = false; // Variable statique pour éviter les appels multiples

    if (enCoursDenvoi) {
        qDebug() << "⚠️ Envoi déjà en cours, ignoré";
        return;
    }

    enCoursDenvoi = true;

    // Vérifications de base
    if (currentEmployeId == -1) {
        QMessageBox::warning(this, "Non connecté", "Connectez-vous d'abord !");
        enCoursDenvoi = false;
        return;
    }

    int idDestinataire = comboDestinataires->currentData().toInt();
    if (idDestinataire == -1) {
        QMessageBox::warning(this, "Destinataire", "Choisissez un employé !");
        enCoursDenvoi = false;
        return;
    }

    QString texte = inputMessage->text().trimmed();
    if (texte.isEmpty()) {
        enCoursDenvoi = false;
        return;
    }

    qDebug() << "=== DÉBUT ENVOI MESSAGE ===";
    qDebug() << "De:" << currentEmployeId << "À:" << idDestinataire;
    qDebug() << "Message:" << texte;

    // 1. Arrêter le timer de rafraîchissement temporairement
    msgRefreshTimer->stop();
    qDebug() << "⏸️ Timer arrêté";

    // 2. Désactiver l'interface
    btnEnvoyerMsg->setEnabled(false);
    inputMessage->setEnabled(false);

    // 3. Vider le champ
    inputMessage->clear();

    // 4. Envoyer le message
    bool succes = Employe::envoyerMessage(
        currentEmployeId,
        currentEmployeNom,
        idDestinataire,
        texte
        );

    if (succes) {
        qDebug() << "✅ Message envoyé à la base";

        // Attendre un peu avant de recharger
        QTimer::singleShot(500, this, [=]() {
            qDebug() << "🔄 Chargement de la conversation...";
            chargerMessagesDiscussion(idDestinataire);

            // Réactiver après 1 seconde
            QTimer::singleShot(1000, this, [=]() {
                btnEnvoyerMsg->setEnabled(true);
                inputMessage->setEnabled(true);
                inputMessage->setFocus();
                enCoursDenvoi = false;

                // Redémarrer le timer après 5 secondes
                QTimer::singleShot(5000, this, [=]() {
                    msgRefreshTimer->start(5000);
                    qDebug() << "▶️ Timer redémarré";
                });

                qDebug() << "✅ Interface réactivée";
            });
        });

    } else {
        qDebug() << "❌ Échec d'envoi";
        QMessageBox::critical(this, "Erreur", "Impossible d'envoyer le message");

        // Réactiver rapidement
        btnEnvoyerMsg->setEnabled(true);
        inputMessage->setEnabled(true);
        inputMessage->setFocus();
        enCoursDenvoi = false;

        // Redémarrer le timer
        msgRefreshTimer->start(5000);
    }

    qDebug() << "=== FIN ENVOI MESSAGE ===";
}
// ==================== ENVOYER MESSAGE WHATSAPP ====================

// ==================== AJOUTER MESSAGE AVEC BON TIMESTAMP ====================
void MainWindow::ajouterMessageEnTempsReel(const QString &message, bool estMoi)
{
    QString currentHtml = displayMessagerie->toHtml();

    // Si vide ou pas de structure, recharger
    if (currentHtml.isEmpty() || !currentHtml.contains("chat-container")) {
        int idDest = comboDestinataires->currentData().toInt();
        if (idDest != -1) {
            chargerMessagesDiscussion(idDest);
        }
        QTimer::singleShot(300, this, [=]() {
            ajouterMessageEnTempsReel(message, estMoi);
        });
        return;
    }

    // Préparer le message
    QString time = QTime::currentTime().toString("HH:mm");
    QString classe = estMoi ? "moi" : "lui";
    QString sender = estMoi ? "" : QString("<div class='sender'>%1</div>")
                                       .arg(comboDestinataires->currentText());
    QString status = estMoi ? "✓" : "";

    // Nouveau message HTML
    QString newMsg = QString(R"(
        <div class="msg-row %1">
            <div class="msg-bubble %1">
                %2
                <div class="msg-text">%3</div>
                <div class="msg-info">
                    <span class="msg-time">%4</span>
                    <span class="msg-status">%5</span>
                </div>
            </div>
        </div>
    )").arg(classe, sender,
                              message.toHtmlEscaped().replace("\n", "<br>"),
                              time, status);

    // Insérer avant la fin du conteneur
    if (currentHtml.contains("</div>\n        <script>")) {
        currentHtml = currentHtml.replace("</div>\n        <script>",
                                          newMsg + "\n        </div>\n        <script>");
    } else if (currentHtml.contains("</div></body>")) {
        currentHtml = currentHtml.replace("</div></body>",
                                          newMsg + "\n</div></body>");
    } else {
        currentHtml += newMsg;
    }

    displayMessagerie->setHtml(currentHtml);

    // Scroll
    QTimer::singleShot(50, this, [this]() {
        QScrollBar *sb = displayMessagerie->verticalScrollBar();
        if (sb) sb->setValue(sb->maximum());
    });
}
void MainWindow::connecterEmploye()
{
    if (currentEmployeId == -1) {
        afficherFormulaireConnexion();

        if (currentEmployeId != -1) {
            ui->tabWidget_48->setCurrentIndex(3);
            refreshMessagerie();
        }
    } else {
        ui->tabWidget_48->setCurrentIndex(3);
        refreshMessagerie();
    }
}
// ==================== AJOUTER MESSAGE EN TEMPS RÉEL (CORRIGÉ) ====================

// ==================== CHANGEMENT DE DESTINATAIRE ====================
void MainWindow::onDestinataireChange()
{
    int idDestinataire = comboDestinataires->currentData().toInt();

    if (idDestinataire == -1) {
        // Aucun destinataire sélectionné
        displayMessagerie->setHtml(R"(
            <div style="text-align: center; margin: 50px 0; color: #666666;">
                <div style="font-size: 48px;">💬</div>
                <div style="font-size: 16px; font-weight: bold; margin: 10px 0;">Sélectionnez un employé</div>
                <div style="font-size: 14px;">Choisissez un collègue avec qui discuter</div>
            </div>
        )");

        // Désactiver l'envoi
        btnEnvoyerMsg->setEnabled(false);
        inputMessage->setEnabled(false);
        inputMessage->setPlaceholderText("Sélectionnez d'abord un destinataire...");
    } else {
        // Destinataire sélectionné
        QString nomDestinataire = comboDestinataires->currentText();

        // Activer l'envoi
        btnEnvoyerMsg->setEnabled(true);
        inputMessage->setEnabled(true);
        inputMessage->setPlaceholderText(QString("Message à %1...").arg(nomDestinataire));

        // Charger la discussion
        chargerMessagesDiscussion(idDestinataire);
    }
}

// ==================== CHARGER MESSAGES ====================
void MainWindow::chargerEmployesDestinataires()
{
    // Sauvegarder la sélection actuelle
    int currentSelection = comboDestinataires->currentData().toInt();

    comboDestinataires->clear();
    comboDestinataires->addItem("👥 Sélectionner un employé", -1);

    // Charger tous les employés
    QList<QPair<int, QString>> employes = Employe::getListeEmployes();

    for (const auto &employe : employes) {
        comboDestinataires->addItem(employe.second, employe.first);
    }

    // Restaurer la sélection
    if (currentSelection != -1) {
        int index = comboDestinataires->findData(currentSelection);
        if (index != -1) {
            comboDestinataires->setCurrentIndex(index);
        }
    }

    qDebug() << "📋" << employes.size() << "employés chargés";
}

// ==================== CHARGER EMPLOYÉS DESTINATAIRES ====================

// ==================== CHARGER DISCUSSION ====================
// ==================== CHARGER DISCUSSION (CORRIGÉ) ====================


void MainWindow::chargerMessages()
{
    if (currentEmployeId == -1) return;

    QList<QJsonObject> messages = Employe::getMessagesPourEmploye(currentEmployeId);

    // Si un destinataire est sélectionné, charger cette discussion
    int idDestinataireSelectionne = comboDestinataires->currentData().toInt();
    if (idDestinataireSelectionne != -1) {
        chargerMessagesDiscussion(idDestinataireSelectionne);
        return;
    }

    // Mode vue d'ensemble (tous les messages)
    QString html = R"(<div style='padding:20px; color:#8696a0; text-align:center;'>
        <h3>💬 Tous vos messages</h3>
        <p>Sélectionnez un employé pour voir la conversation</p>
    </div>)";

    displayMessagerie->setHtml(html);
}
void MainWindow::refreshMessagerie()
{
    if (currentEmployeId != -1) {
        chargerEmployesDestinataires();

        int idDestinataire = comboDestinataires->currentData().toInt();
        if (idDestinataire != -1) {
            chargerMessagesDiscussion(idDestinataire);
        }
    }
}
// ==================== GÉNÉRER MESSAGE PROFESSIONNEL ====================
QString MainWindow::genererMessageProfessionnel(const QString& type, const QString& prenom, const QString& nom)
{
    QString message;

    if (type == "📋 Notification standard") {
        message = QString("🏎️ SMART DRIVING SCHOOL\n\n"
                          "Cher %1,\n\n"
                          "Nous vous souhaitons une excellente journée de travail.\n\n"
                          "N'hésitez pas à nous contacter pour toute question.\n\n"
                          "Cordialement,\nL'équipe de direction")
                      .arg(prenom);
    }
    else if (type == "⚠️ Message urgent") {
        message = QString("🚨 URGENT - SMART DRIVING SCHOOL\n\n"
                          "Cher %1 %2,\n\n"
                          "Message important nécessitant votre attention immédiate.\n\n"
                          "Veuillez nous contacter dès réception de ce message.\n\n"
                          "Tél. : +216 70 000 000")
                      .arg(prenom, nom);
    }
    else if (type == "🔄 Rappel de réunion") {
        message = QString("📅 RAPPEL - SMART DRIVING SCHOOL\n\n"
                          "Cher %1,\n\n"
                          "Rappel : Réunion d'équipe prévue aujourd'hui.\n\n"
                          "📍 Salle de conférence\n"
                          "⏰ 14h00\n"
                          "📋 Ordre du jour : Objectifs trimestriels\n\n"
                          "Votre présence est importante.")
                      .arg(prenom);
    }
    else if (type == "🎯 Objectifs du mois") {
        message = QString("🎯 OBJECTIFS - SMART DRIVING SCHOOL\n\n"
                          "Cher %1 %2,\n\n"
                          "Vos objectifs pour ce mois :\n"
                          "• 20 cours de conduite\n"
                          "• 15 évaluations d'élèves\n"
                          "• Participation aux réunions\n\n"
                          "Merci pour votre engagement !")
                      .arg(prenom, nom);
    }
    else {
        message = QString("🏎️ SMART DRIVING SCHOOL\n\n"
                          "Cher %1 %2,\n\n"
                          "Vous avez un nouveau message.\n\n"
                          "Cordialement,\nL'équipe de direction")
                      .arg(prenom, nom);
    }

    return message;
}

void MainWindow::chargerMessagesDiscussion(int idDestinataire)
{
    static QDateTime dernierChargement;
    static int dernierDestinataire = -1;

    // Débounce: éviter les appels trop rapprochés
    QDateTime maintenant = QDateTime::currentDateTime();
    if (dernierDestinataire == idDestinataire &&
        dernierChargement.msecsTo(maintenant) < 1000) {
        qDebug() << "⏸️ Chargement ignoré (trop rapide)";
        return;
    }

    dernierChargement = maintenant;
    dernierDestinataire = idDestinataire;

    if (currentEmployeId == -1 || idDestinataire == -1) return;

    qDebug() << "=== CHARGEMENT DISCUSSION ===";
    qDebug() << "👤 Employé:" << currentEmployeId;
    qDebug() << "👥 Destinataire:" << idDestinataire;

    // Récupérer les messages
    QList<QJsonObject> messages = Employe::getMessagesDiscussion(currentEmployeId, idDestinataire);

    qDebug() << "📨 Nombre de messages récupérés:" << messages.size();

    // Vérifier les doublons
    QSet<QString> messagesUniques;
    QList<QJsonObject> messagesFiltres;

    for (const QJsonObject &msg : std::as_const(messages)) {
        QString texte = msg["message"].toString();
        QString date = msg["date_envoi"].toString();
        QString cle = texte + "|" + date;

        if (!messagesUniques.contains(cle)) {
            messagesUniques.insert(cle);
            messagesFiltres.append(msg);
        } else {
            qDebug() << "⚠️ Doublon filtré:" << texte.left(30) << "...";
        }
    }

    qDebug() << "✅ Messages uniques après filtrage:" << messagesFiltres.size();

    QString nomDestinataire = comboDestinataires->currentText();

    if (messagesFiltres.isEmpty()) {
        qDebug() << "ℹ️ Aucun message après filtrage";
        displayMessagerie->setHtml(QString(
                                       "<div style='text-align:center; padding:40px; color:#8696a0;'>"
                                       "<div style='font-size:48px;'>💬</div>"
                                       "<h3 style='color:#e9edef;'>Aucun message</h3>"
                                       "<p>Envoyez le premier message à %1 !</p>"
                                       "</div>").arg(nomDestinataire));
    } else {
        qDebug() << "🎨 Affichage des messages";
        afficherMessagesWhatsApp(messagesFiltres, nomDestinataire);
    }

    qDebug() << "=== FIN CHARGEMENT ===";
}
void MainWindow::afficherAucunMessageDiscussion(const QString& nomDestinataire)
{
    QString html = QString(R"(
        <!DOCTYPE html>
        <html>
        <head>
        <style>
            body {
                background: #0c1317;
                color: #8696a0;
                font-family: 'Segoe UI', Arial, sans-serif;
                margin: 0;
                padding: 0;
                height: 100vh;
                display: flex;
                align-items: center;
                justify-content: center;
            }
            .container {
                text-align: center;
                padding: 40px;
            }
            .icon {
                font-size: 64px;
                margin-bottom: 20px;
            }
            .title {
                font-size: 24px;
                font-weight: bold;
                color: #e9edef;
                margin-bottom: 10px;
            }
            .subtitle {
                font-size: 16px;
                max-width: 400px;
                line-height: 1.5;
                margin: 0 auto 30px auto;
            }
            .info {
                font-size: 14px;
                background: #1f2c33;
                padding: 15px;
                border-radius: 10px;
                margin-top: 20px;
            }
        </style>
        </head>
        <body>
        <div class="container">
            <div class="icon">💬</div>
            <div class="title">Commencer une conversation</div>
            <div class="subtitle">
                Vous n'avez pas encore échangé de messages avec <strong>%1</strong>.
                <br>Envoyez le premier message pour démarrer la discussion !
            </div>
            <div class="info">
                📱 Les messages sont sécurisés et privés<br>
                ⚡ Envoyez votre premier message maintenant
            </div>
        </div>
        </body>
        </html>
    )").arg(nomDestinataire.toHtmlEscaped());

    if (displayMessagerie) {
        displayMessagerie->setHtml(html);
    }
}
// ==================== CONNECTER EMPLOYÉ (CORRIGÉ) ====================
void MainWindow::on_btnMessagerie_clicked()
{
    ui->tabWidget_48->setCurrentIndex(3);
    chargerEmployesDestinataires();
    refreshMessagerie();
}

// ==================== BOUTON MESSAGERIE ====================

// ==================== BOUTON AJOUTER EXAMEN ====================



void MainWindow::on_btnAjouter_clicked()
{
    // --- Contrôle de saisie ---
    if(ui->lineEditID->text().isEmpty() || ui->lineEditIDEleve->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Les ID doivent être remplis !");
        return;
    }

    if(ui->lineEditObservation->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "L'observation ne peut pas être vide !");
        return;
    }

    // Vérifier que les ID sont des entiers valides
    bool ok1, ok2;
    ui->lineEditID->text().toInt(&ok1);
    ui->lineEditIDEleve->text().toInt(&ok2);
    if(!ok1 || !ok2) {
        QMessageBox::warning(this, "Erreur", "Les ID doivent être des nombres entiers !");
        return;
    }

    // Vérifier que l'observation contient uniquement des lettres et espaces
    QRegularExpression re("^[A-Za-z ]+$");
    if(!re.match(ui->lineEditObservation->text()).hasMatch()) {
        QMessageBox::warning(this, "Erreur", "L'observation ne peut contenir que des lettres !");
        return;
    }

    // Vérifier que la note est un entier entre 0 et 20
    bool okNote;
    int noteInt = ui->lineEditNote->text().toInt(&okNote);

    if(!okNote) {
        QMessageBox::warning(this, "Erreur", "La note doit être un entier !");
        return;
    }

    if( noteInt < 0 || noteInt > 20) {
        QMessageBox::warning(this, "Erreur", "La note doit être comprise entre 0 et 20 !");
        return;
    }
    // --- Fin du contrôle de saisie ---

    int id = ui->lineEditID->text().toInt();
    QString type = ui->comboBox_type->currentText();
    QDate date_examen = ui->dateEditDate->date();
    float note = ui->lineEditNote->text().toFloat();
    QString resultat = ui->comboBox_res->currentText();
    QString observation = ui->lineEditObservation->text();
    int id_eleve = ui->lineEditIDEleve->text().toInt();

    Examen e(id, type, date_examen, note, resultat, observation, id_eleve);

    if (e.ajouter()) {
        QMessageBox::information(this, "Succès", "Examen ajouté avec succès.");

        ui->tableWidgetExamen->setRowCount(0);
        QSqlQuery query("SELECT ID_EXAMEN, TYPE, DATE_EXAMEN, NOTE, RESULTAT, OBSERVATION, ID_ELEVE FROM EXAMENS");
        int row = 0;

        while (query.next()) {
            ui->tableWidgetExamen->insertRow(row);
            ui->tableWidgetExamen->setItem(row, 0, new QTableWidgetItem(query.value("ID_EXAMEN").toString()));
            ui->tableWidgetExamen->setItem(row, 1, new QTableWidgetItem(query.value("TYPE").toString()));
            ui->tableWidgetExamen->setItem(row, 2, new QTableWidgetItem(query.value("DATE_EXAMEN").toString()));
            ui->tableWidgetExamen->setItem(row, 3, new QTableWidgetItem(query.value("NOTE").toString()));
            ui->tableWidgetExamen->setItem(row, 4, new QTableWidgetItem(query.value("RESULTAT").toString()));
            ui->tableWidgetExamen->setItem(row, 5, new QTableWidgetItem(query.value("OBSERVATION").toString()));
            ui->tableWidgetExamen->setItem(row, 6, new QTableWidgetItem(query.value("ID_ELEVE").toString()));
            row++;
        }
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de l’ajout de l’examen.");
    }
        MainWindow::on_btnStats_clicked();

}


void MainWindow::actualiserTableau()
{
    ui->tableWidgetExamen->setRowCount(0);
    QSqlQuery query("SELECT ID_EXAMEN, TYPE, DATE_EXAMEN, NOTE, RESULTAT, OBSERVATION, ID_ELEVE FROM EXAMENS");
    int row = 0;
    while (query.next()) {
        ui->tableWidgetExamen->insertRow(row);
        ui->tableWidgetExamen->setItem(row, 0, new QTableWidgetItem(query.value("ID_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 1, new QTableWidgetItem(query.value("TYPE").toString()));
        ui->tableWidgetExamen->setItem(row, 2, new QTableWidgetItem(query.value("DATE_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 3, new QTableWidgetItem(query.value("NOTE").toString()));
        ui->tableWidgetExamen->setItem(row, 4, new QTableWidgetItem(query.value("RESULTAT").toString()));
        ui->tableWidgetExamen->setItem(row, 5, new QTableWidgetItem(query.value("OBSERVATION").toString()));
        ui->tableWidgetExamen->setItem(row, 6, new QTableWidgetItem(query.value("ID_ELEVE").toString()));
        row++;
    }
    MainWindow::on_btnStats_clicked();

}


void MainWindow::on_supprimer_clicked()
{
    int id = ui->lineEditID->text().toInt();
    if (id == 0) {
        QMessageBox::warning(this, "Attention", "Veuillez entrer un ID valide pour la suppression.");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation",
                                  "Voulez-vous vraiment supprimer l’examen ayant l’ID " + QString::number(id) + " ?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        Examen e;
        if (e.supprimer(id)) {
            QMessageBox::information(this, "Succès", "Examen supprimé avec succès.");
            actualiserTableau();
        } else {
            QMessageBox::warning(this, "Erreur", "Aucun examen trouvé avec cet ID ou échec de la suppression.");
        }
    }
    MainWindow::on_btnStats_clicked();
}
void MainWindow::on_btnModifier_clicked()
{
    // --- Contrôle de saisie ---
    if(ui->lineEditID->text().isEmpty() || ui->lineEditIDEleve->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Les ID doivent être remplis !");
        return;
    }

    if(ui->lineEditObservation->text().isEmpty()) {
        QMessageBox::warning(this, "Erreur", "L'observation ne peut pas être vide !");
        return;
    }

    // Vérifier que les ID sont des entiers valides
    bool ok1, ok2;
    int id_examen = ui->lineEditID->text().toInt(&ok1);
    int id_eleve = ui->lineEditIDEleve->text().toInt(&ok2);
    if(!ok1 || !ok2) {
        QMessageBox::warning(this, "Erreur", "Les ID doivent être des nombres entiers !");
        return;
    }

    // Vérifier que l'observation contient uniquement des lettres et espaces
    QRegularExpression re("^[A-Za-z ]+$");
    if(!re.match(ui->lineEditObservation->text()).hasMatch()) {
        QMessageBox::warning(this, "Erreur", "L'observation ne peut contenir que des lettres !");
        return;
    }

    // Vérifier la note
    bool okNote;
    double noteDouble = ui->lineEditNote->text().toDouble(&okNote);
    if(!okNote) {
        QMessageBox::warning(this, "Erreur", "La note doit être un nombre !");
        return;
    }
    if(noteDouble < 0 || noteDouble > 20) {
        QMessageBox::warning(this, "Erreur", "La note doit être comprise entre 0 et 20 !");
        return;
    }

    // --- Récupération des valeurs ---
    QString type = ui->comboBox_type->currentText();
    QDate date_examen = ui->dateEditDate->date();
    QString resultat = ui->comboBox_res->currentText();
    QString observation = ui->lineEditObservation->text();

    // --- Requête SQL sécurisée pour Oracle ---
    QSqlQuery query;
    query.prepare("UPDATE EXAMENS SET "
                  "TYPE = :type, "
                  "DATE_EXAMEN = TO_DATE(:date_examen, 'DD/MM/YYYY'), "
                  "NOTE = :note, "
                  "RESULTAT = :resultat, "
                  "OBSERVATION = :observation, "
                  "ID_ELEVE = :id_eleve "
                  "WHERE ID_EXAMEN = :id_examen");

    query.bindValue(":type", type);
    query.bindValue(":date_examen", date_examen.toString("dd/MM/yyyy")); // format sûr
    query.bindValue(":note", noteDouble);
    query.bindValue(":resultat", resultat);
    query.bindValue(":observation", observation);
    query.bindValue(":id_eleve", id_eleve);
    query.bindValue(":id_examen", id_examen);

    // --- Exécution et retour ---
    if(query.exec()) {
        QMessageBox::information(this, "Succès", "Examen modifié avec succès !");
        afficherExamensDansTable(); // rafraîchir le tableau après modification
    } else {
        QMessageBox::critical(this, "Erreur", "La modification a échoué : " + query.lastError().text());
    }
        MainWindow::on_btnStats_clicked();
}


void MainWindow::on_btnStats_clicked()
{
    QLayout *layout = ui->widgetStats->layout();

    if (!layout) {
        layout = new QVBoxLayout(ui->widgetStats);
        ui->widgetStats->setLayout(layout);
    }

    // ------------------------------
    // 2) Supprimer l'ancien graphique
    // ------------------------------
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    // ------------------------------
    // 3) Générer le graphique via ton objet Examen
    // ------------------------------
    QChartView *Qchart = E.genererStatistiquesNotes(ui->tableWidgetExamen);

    if (!Qchart) {
        QMessageBox::information(this, "Statistiques", "Aucune donnée disponible.");
        return;
    }

    // ------------------------------
    // 4) Ajouter le graphique mis à jour
    // ------------------------------
    layout->addWidget(Qchart);

}
void MainWindow::on_pushButton_rechercher_clicked()
{
    QString id_eleve_str = ui->lineEdit_idEleve->text().trimmed();

    // ✅ Vérification de saisie (l'ID doit être un entier)
    QRegularExpression regexInt("^[0-9]+$");
    if (!regexInt.match(id_eleve_str).hasMatch()) {
        QMessageBox::warning(this, "Entrée invalide", "L'ID élève doit contenir uniquement des chiffres !");
        return;
    }

    int id_eleve = id_eleve_str.toInt();

    QSqlQuery query;
    query.prepare("SELECT ID_EXAMEN, TYPE, DATE_EXAMEN, NOTE, RESULTAT, OBSERVATION, ID_ELEVE "
                  "FROM EXAMENS WHERE ID_ELEVE = :id_eleve");
    query.bindValue(":id_eleve", id_eleve);

    if (!query.exec()) {
        QMessageBox::critical(this, "Erreur SQL", query.lastError().text());
        return;
    }

    // ✅ Vider le tableau avant d’afficher le résultat
    ui->tableWidgetExamen->setRowCount(0);

    int row = 0;
    bool found = false;

    // ✅ Remplir le tableau avec les résultats
    while (query.next()) {
        found = true;
        ui->tableWidgetExamen->insertRow(row);

        ui->tableWidgetExamen->setItem(row, 0, new QTableWidgetItem(query.value("ID_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 1, new QTableWidgetItem(query.value("TYPE").toString()));
        ui->tableWidgetExamen->setItem(row, 2, new QTableWidgetItem(query.value("DATE_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 3, new QTableWidgetItem(query.value("NOTE").toString()));
        ui->tableWidgetExamen->setItem(row, 4, new QTableWidgetItem(query.value("RESULTAT").toString()));
        ui->tableWidgetExamen->setItem(row, 5, new QTableWidgetItem(query.value("OBSERVATION").toString()));
        ui->tableWidgetExamen->setItem(row, 6, new QTableWidgetItem(query.value("ID_ELEVE").toString()));

        row++;
    }

    // ✅ Si aucun résultat trouvé
    if (!found) {
        QMessageBox::information(this, "Résultat de la recherche",
                                 "Aucun examen trouvé pour cet élève.");
    } else {
        QMessageBox::information(this, "Résultat de la recherche",
                                 QString("Affichage des examens de l'élève ID %1").arg(id_eleve));
    }
}

void MainWindow::on_pushButton_exporterPDF_clicked()
{

    QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer sous", "", "Fichiers PDF (*.pdf)");
    if (fileName.isEmpty())
        return;
    if (!fileName.endsWith(".pdf"))
        fileName += ".pdf";


    QTextDocument doc;
    QTextCursor cursor(&doc);


    cursor.insertHtml("<h1 style='text-align:center;'>Liste des examens</h1>");
    cursor.insertHtml("<p style='text-align:right; font-size:10pt;'>Date d'export : " +
                      QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss") + "</p>");
    cursor.insertBlock();


    int rows = ui->tableWidgetExamen->rowCount();
    int cols = ui->tableWidgetExamen->columnCount();
    if (rows == 0 || cols == 0) {
        QMessageBox::warning(this, "Erreur", "Aucune donnée à exporter !");
        return;
    }

    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(5);
    tableFormat.setCellSpacing(0);
    tableFormat.setHeaderRowCount(1);
    tableFormat.setAlignment(Qt::AlignCenter);


    QVector<QTextLength> colWidths;
    for (int i = 0; i < cols; ++i)
        colWidths.append(QTextLength(QTextLength::PercentageLength, 100.0 / cols));
    tableFormat.setColumnWidthConstraints(colWidths);

    QTextTable *table = cursor.insertTable(rows + 1, cols, tableFormat);


    for (int col = 0; col < cols; ++col) {
        QTextTableCell cell = table->cellAt(0, col);
        QTextCursor cellCursor = cell.firstCursorPosition();
        QString header = ui->tableWidgetExamen->horizontalHeaderItem(col)->text();
        QTextCharFormat format;
        format.setFontWeight(QFont::Bold);
        format.setBackground(QColor("#D3D3D3")); // gris clair pour en-tête
        cellCursor.insertText(header, format);
    }


    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            QTextTableCell cell = table->cellAt(row + 1, col);
            QTextCursor cellCursor = cell.firstCursorPosition();
            QString text = ui->tableWidgetExamen->item(row, col) ? ui->tableWidgetExamen->item(row, col)->text() : "";

            QTextCharFormat format;


            if (row % 2 == 0)
                format.setBackground(QColor("#F0F0F0")); // gris très clair
            else
                format.setBackground(Qt::white);


            if (ui->tableWidgetExamen->horizontalHeaderItem(col)->text().toUpper() == "RESULTAT") {
                if (text.toLower() == "réussi" || text.toLower() == "reussi")
                    format.setForeground(Qt::green);
                else if (text.toLower() == "échoué" || text.toLower() == "echoue")
                    format.setForeground(Qt::red);
            }

            cellCursor.insertText(text, format);
        }
    }


    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    doc.print(&printer);

    QMessageBox::information(this, "Succès", "Exportation PDF réussie !");
}


void MainWindow::on_tableWidgetExamen_itemClicked(QTableWidgetItem *item)
{
    int row = item->row();

    ui->lineEditID->setText(ui->tableWidgetExamen->item(row, 0)->text());
    ui->comboBox_type->setCurrentText(ui->tableWidgetExamen->item(row, 1)->text());
    QString dateStr = ui->tableWidgetExamen->item(row, 2)->text();
    ui->dateEditDate->setDate(QDate::fromString(dateStr, "yyyy-MM-dd"));

    ui->lineEditNote->setText(ui->tableWidgetExamen->item(row, 3)->text());
    ui->comboBox_res->setCurrentText(ui->tableWidgetExamen->item(row, 4)->text());
    ui->lineEditObservation->setText(ui->tableWidgetExamen->item(row, 5)->text());
    ui->lineEditIDEleve->setText(ui->tableWidgetExamen->item(row, 6)->text());
}
void MainWindow::afficherExamensDansTable()
{
    ui->tableWidgetExamen->setRowCount(0);

    QSqlQuery query("SELECT ID_EXAMEN, TYPE, DATE_EXAMEN, NOTE, RESULTAT, OBSERVATION, ID_ELEVE FROM EXAMENS");

    int row = 0;
    while (query.next()) {
        ui->tableWidgetExamen->insertRow(row);
        ui->tableWidgetExamen->setItem(row, 0, new QTableWidgetItem(query.value("ID_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 1, new QTableWidgetItem(query.value("TYPE").toString()));
        ui->tableWidgetExamen->setItem(row, 2, new QTableWidgetItem(query.value("DATE_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 3, new QTableWidgetItem(query.value("NOTE").toString()));
        ui->tableWidgetExamen->setItem(row, 4, new QTableWidgetItem(query.value("RESULTAT").toString()));
        ui->tableWidgetExamen->setItem(row, 5, new QTableWidgetItem(query.value("OBSERVATION").toString()));
        ui->tableWidgetExamen->setItem(row, 6, new QTableWidgetItem(query.value("ID_ELEVE").toString()));
        row++;
    }

    if (row == 0) {
        qDebug() << "️ Aucun examen trouvé dans la base !";
    } else {
        qDebug()  << row << "examens chargés dans le tableau.";
    }
}

void MainWindow::on_btnTrier_clicked()
{
    QString critere = ui->comboBox_trier->currentText();
    Examen e;
    QSqlQuery query = e.trierParCritere(critere);

    if (!query.isActive()) {
        QMessageBox::warning(this, "Erreur", "Le tri n’a pas pu être exécuté !");
        return;
    }

    ui->tableWidgetExamen->setRowCount(0);
    int row = 0;

    while (query.next()) {
        ui->tableWidgetExamen->insertRow(row);
        ui->tableWidgetExamen->setItem(row, 0, new QTableWidgetItem(query.value("ID_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 1, new QTableWidgetItem(query.value("TYPE").toString()));
        ui->tableWidgetExamen->setItem(row, 2, new QTableWidgetItem(query.value("DATE_EXAMEN").toString()));
        ui->tableWidgetExamen->setItem(row, 3, new QTableWidgetItem(query.value("NOTE").toString()));
        ui->tableWidgetExamen->setItem(row, 4, new QTableWidgetItem(query.value("RESULTAT").toString()));
        ui->tableWidgetExamen->setItem(row, 5, new QTableWidgetItem(query.value("OBSERVATION").toString()));
        ui->tableWidgetExamen->setItem(row, 6, new QTableWidgetItem(query.value("ID_ELEVE").toString()));
        row++;
    }

    if (row == 0)
        QMessageBox::information(this, "Information", "Aucune donnée trouvée pour ce type.");
    else
        QMessageBox::information(this, "Tri réussi", QString("Tri effectué : %1 ( %2 examens )").arg(critere).arg(row));
}




//RANIM

void MainWindow::on_supprimer_2_clicked()
{

    QString id_text = ui->id->text();
    if (id_text.isEmpty()) {
        QMessageBox::warning(this, "Champ vide", "️ Veuillez saisir l'ID de l'élève à supprimer !");
        return;
    }

    int id = id_text.toInt();

    eleve e;
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation","Êtes-vous sûr de vouloir supprimer cet élève ?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (e.supprimer(id)) {
            e.setIdEleve(id);
            e.enregistreristorique("-", "-", "-", "Suppression");

            QMessageBox::information(this, "Succès", "Élève supprimé avec succès !");
            ui->tab->setModel(e.afficher());


        } else {
            QMessageBox::warning(this, "Erreur", "La suppression de l'élève a échoué !");
        }
    }
    afficherStatistiques();
}

void MainWindow::on_ajouter_clicked()
{

    QString id_text = ui->id->text();
    QString nom = ui->nom->text();
    QString prenom = ui->pr->text();
    QDate dateN = ui->daten->date();
    QString tel_text = ui->tel->text();
    QString adresse = ui->adresse->text();
    QString email = ui->email->text();



    if (id_text.isEmpty() || nom.isEmpty() || prenom.isEmpty() ||
        tel_text.isEmpty() || adresse.isEmpty() || email.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Tous les champs sont obligatoires !");
        return;
    }



    bool idOk, telOk;
    int id = id_text.toInt(&idOk);
    int tel = tel_text.toInt(&telOk);

    if (!idOk) {
        QMessageBox::critical(this, "Erreur ID", " L'ID doit être un nombre valide !");
        return;
    }

    if (!telOk) {
        QMessageBox::critical(this, "Erreur Téléphone", " Le téléphone doit contenir uniquement des chiffres !");
        return;
    }

    if (!email.contains("@") || !email.contains(".")) {
        QMessageBox::warning(this, "Email invalide", "Veuillez entrer une adresse email valide (ex : exemple@gmail.com) !");
        return ;
    }

    for (int i = 0; i < nom.length(); i++) {
        if (!nom[i].isLetter() && nom[i] != ' ') {
            QMessageBox::warning(this, "Nom invalide", "Le nom doit contenir uniquement des lettres !");
            return;
        }
    }


    for (int i = 0; i < prenom.length(); i++) {
        if (!prenom[i].isLetter() && prenom[i] != ' ') {
            QMessageBox::warning(this, "Prénom invalide", "Le prénom doit contenir uniquement des lettres !");
            return;
        }
    }
    if (tel_text.length() != 8) {
        QMessageBox::warning(this, "Numéro invalide", "Le numéro de téléphone doit contenir exactement 8 chiffres !");
        return;
    }


    eleve e(id, nom, prenom, dateN, tel_text, adresse, email);




    if (e.ajouter()) {
        QMessageBox::information(this, "Succès", " Élève ajouté avec succès !");
        ui->tab->setModel(e.afficher());
        e.enregistreristorique("-", "-", "-", "Ajout");
        // Envoi automatique du mail de bienvenue
        e.envoyerMail(email, "Bienvenue à Smart Driving School",
                    nom, prenom, dateN.toString("dd/MM/yyyy"),
                    tel_text, adresse);



        ui->id->clear();
        ui->nom->clear();
        ui->pr->clear();
        ui->tel->clear();
        ui->adresse->clear();
        ui->email->clear();
    } else {
        QMessageBox::critical(this, "Erreur", " Échec de l'ajout. Vérifiez les informations saisies !");
    }
    afficherStatistiques();
}

void MainWindow::on_modifier_clicked()
{
    QString idText = ui->id->text();
    if (idText.isEmpty()) {
        QMessageBox::warning(this, "Champs manquant", "Veuillez saisir l'ID de l'élève !");
        return;
    }

    bool idOk;
    int id = idText.toInt(&idOk);
    if (!idOk) {
        QMessageBox::critical(this, "Erreur ID", "L'ID doit être un nombre valide !");
        return;
    }

    // Vérifier si l'élève existe
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM Eleves WHERE id_eleve = :id_eleve");
    check.bindValue(":id_eleve", id);
    if (!check.exec() || !check.next() || check.value(0).toInt() == 0) {
        QMessageBox::critical(this, "Erreur", "Aucun élève trouvé avec cet ID !");
        return;
    }

    // Récupération des nouvelles valeurs depuis l'UI
    QString nom = ui->nom->text();
    QString prenom = ui->pr->text();
    QDate dateN = ui->daten->date();
    QString telephone = ui->tel->text();
    QString adresse = ui->adresse->text();
    QString email = ui->email->text();

    // Validation simple
    if (!telephone.isEmpty()) {
        bool telOk;
        telephone.toLongLong(&telOk);
        if (!telOk) {
            QMessageBox::warning(this, "Téléphone invalide", "Le numéro doit contenir uniquement des chiffres !");
            return;
        }
    }

    if (!email.isEmpty() && (!email.contains("@") || !email.contains("."))) {
        QMessageBox::warning(this, "Email invalide", "Veuillez entrer une adresse email valide !");
        return;
    }

    for (int i = 0; i < nom.length(); i++)
        if (!nom[i].isLetter() && nom[i] != ' ') {
            QMessageBox::warning(this, "Nom invalide", "Le nom doit contenir uniquement des lettres !");
            return;
        }

    for (int i = 0; i < prenom.length(); i++)
        if (!prenom[i].isLetter() && prenom[i] != ' ') {
            QMessageBox::warning(this, "Prénom invalide", "Le prénom doit contenir uniquement des lettres !");
            return;
        }

    if (telephone.length() != 8) {
        QMessageBox::warning(this, "Numéro invalide", "Le numéro de téléphone doit contenir exactement 8 chiffres !");
        return;
    }

    // Confirmation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation","Êtes-vous sûr de vouloir modifier cet élève ?",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No)
        return;

    // Récupérer les anciennes valeurs (Oracle sensible à la casse)
    QSqlQuery q;
    q.prepare("SELECT nom, prenom, DATENAISSANCE, telephone, adresse, email FROM Eleves WHERE id_eleve = :id");
    q.bindValue(":id", id);
    if(!q.exec() || !q.next()) {
        QMessageBox::critical(this, "Erreur", "Impossible de récupérer les anciennes valeurs !");
        return;
    }
    QString ancienNom = q.value(0).toString();
    QString ancienPrenom = q.value(1).toString();
    QString ancienneDate = q.value(2).toDate().toString("dd/MM/yyyy");
    QString ancienTel = q.value(3).toString();
    QString ancienneAdresse = q.value(4).toString();
    QString ancienEmail = q.value(5).toString();

    // Modifier
    eleve e(id, nom, prenom, dateN, telephone, adresse, email);
    if (e.modifier()) {

        if (ancienNom != nom)
            e.enregistreristorique("Nom", ancienNom, nom, "Modification");

        if (ancienPrenom != prenom)
            e.enregistreristorique("Prénom", ancienPrenom, prenom, "Modification");

        if (ancienneDate != dateN.toString("dd/MM/yyyy"))
            e.enregistreristorique("Date de naissance", ancienneDate, dateN.toString("dd/MM/yyyy"), "Modification");

        if (ancienTel != telephone)
            e.enregistreristorique("Téléphone", ancienTel, telephone, "Modification");

        if (ancienneAdresse != adresse)
            e.enregistreristorique("Adresse", ancienneAdresse, adresse, "Modification");

        if (ancienEmail != email)
            e.enregistreristorique("Email", ancienEmail, email, "Modification");

        ui->tab->setModel(e.afficher());



        // Envoi du mail de confirmation
        if(!email.isEmpty())
            e.envoyerMail(email, "Modification de vos informations",
                          nom, prenom, dateN.toString("dd/MM/yyyy"),
                          telephone, adresse);

    } else {
        QMessageBox::critical(this, "Erreur", "Échec de la modification. Vérifiez l'ID et les données saisies !");
    }

    afficherStatistiques();
}




void MainWindow::on_annuler_clicked()
{
    ui->id->clear();
    ui->nom->clear();
    ui->pr->clear();
    ui->tel->clear();
    ui->adresse->clear();
    ui->email->clear();
}

void MainWindow::on_rechercher_clicked()
{
    QString texte = ui->rech->text().trimmed();

    if (texte.isEmpty())
    {
        QMessageBox::warning(this, "Champ vide", "Veuillez entrer un identifiant ou un nom pour effectuer une recherche.");
        return;
    }


    int id = texte.toInt();
    QString nom = texte;
    eleve e;
    QSqlQueryModel *model = e.rechercher(id, nom);

    if (model && model->rowCount() > 0)
    {
        ui->tab->setModel(model);
        ui->tab->resizeColumnsToContents();
    }
    else
    {
        QMessageBox::information(this, "Aucun résultat", "Aucun élève trouvé pour cette recherche.");
    }
}

void MainWindow::afficherStatistiques()
{

    eleve e;
    QChartView *chartAge = e.genererStatistiquesAgeVille();
    if(chartAge)
    {

        QLayout *layout = ui->widget_12->layout();
        if(layout){
            QLayoutItem *item;
            while((item = layout->takeAt(0)) != nullptr){
                delete item->widget();
                delete item;
            }
            delete layout;
        }


        QVBoxLayout *newLayout = new QVBoxLayout(ui->widget_12);
        newLayout->setContentsMargins(0,0,0,0);  // pas de marge
        newLayout->setSpacing(0);


        chartAge->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        chartAge->setMinimumSize(ui->widget_12->size());  // correspond à la taille du widget
        newLayout->addWidget(chartAge);
        ui->widget_12->setLayout(newLayout);
    }



    qDebug() << "Graphiques circulaires ajustés parfaitement à l'espace !";
}


void MainWindow::remplirChampsDepuisTable(const QModelIndex &index)
{
    QSqlQueryModel *model = qobject_cast<QSqlQueryModel*>(ui->tab->model());
    if (!model)
        return;


    int row = index.row();


    ui->id->setText(model->data(model->index(row, 0)).toString());

    ui->nom->setText(model->data(model->index(row, 1)).toString());
    ui->pr->setText(model->data(model->index(row, 2)).toString());
    ui->daten->setDate(QDate::fromString(model->data(model->index(row, 3)).toString(), "yyyy-MM-dd"));
    ui->tel->setText(model->data(model->index(row, 4)).toString());
    ui->adresse->setText(model->data(model->index(row, 5)).toString());
    ui->email->setText(model->data(model->index(row, 6)).toString());
}

void MainWindow::trierTableau()
{
    QString critere = ui->tri_2->currentText(); // "Non trié", "Nom" ou "Date de naissance"

    eleve e;
    QSqlQueryModel* model = e.trier(critere);

    if (model->lastError().isValid())
    {
        QMessageBox::critical(this, "Erreur SQL", model->lastError().text());
        return;
    }

    ui->tab->setModel(model);
    ui->tab->resizeColumnsToContents();
}


void MainWindow::on_exporter_clicked()
{
    // 1️⃣ Choisir le fichier PDF
    QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer sous", "", "Fichiers PDF (*.pdf)");
    if(fileName.isEmpty())
        return;
    if(!fileName.endsWith(".pdf")) fileName += ".pdf";

    // 2️⃣ Préparer le document
    QTextDocument doc;
    QTextCursor cursor(&doc);

    cursor.insertHtml("<h1 style='text-align:center;'>Liste des élèves</h1>");
    cursor.insertHtml("<p style='text-align:right; font-size:10pt;'>Date d'export : " +
                      QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss") + "</p>");
    cursor.insertBlock();

    // 3️⃣ Récupérer les données du QTableView
    QAbstractItemModel *model = ui->tab->model();  // remplacer ui->tab par ton nom réel si nécessaire
    int rows = model->rowCount();
    int cols = model->columnCount();

    if(rows == 0 || cols == 0)
    {
        QMessageBox::warning(this, "Erreur", "Aucune donnée à exporter !");
        return;
    }

    // 4️⃣ Configurer le format du tableau
    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(5);
    tableFormat.setCellSpacing(0);
    tableFormat.setHeaderRowCount(1);
    tableFormat.setAlignment(Qt::AlignCenter);

    QVector<QTextLength> colWidths;
    for(int i=0; i<cols; i++)
        colWidths.append(QTextLength(QTextLength::PercentageLength, 100.0 / cols));
    tableFormat.setColumnWidthConstraints(colWidths);

    QTextTable *table = cursor.insertTable(rows+1, cols, tableFormat);

    // 5️⃣ Entêtes
    for(int col=0; col<cols; col++)
    {
        QTextTableCell cell = table->cellAt(0, col);
        QTextCursor cellCursor = cell.firstCursorPosition();
        QTextCharFormat format;
        format.setFontWeight(QFont::Bold);
        format.setBackground(QColor("#D3D3D3"));
        QString header = model->headerData(col, Qt::Horizontal).toString();
        cellCursor.insertText(header, format);
    }

    // 6️⃣ Remplissage des données avec lignes alternées
    for(int row=0; row<rows; row++)
    {
        for(int col=0; col<cols; col++)
        {
            QTextTableCell cell = table->cellAt(row+1, col);
            QTextCursor cellCursor = cell.firstCursorPosition();
            QTextCharFormat format;

            if(row % 2 == 0)
                format.setBackground(QColor("#F0F0F0"));
            else
                format.setBackground(Qt::white);

            QString text = model->data(model->index(row,col)).toString();
            cellCursor.insertText(text, format);
        }
    }

    // 7️⃣ Générer le PDF
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    doc.print(&printer);

    QMessageBox::information(this, "Succès", "Exportation PDF des élèves réussie !");
}


void MainWindow::on_historique_clicked()
{
    QString idText = ui->id->text();

    bool showAll = false;
    int id = -1;

    // Si vide → afficher tout l'historique
    if (idText.isEmpty()) {
        showAll = true;
    } else {
        bool ok;
        id = idText.toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, "Erreur ID", "L'ID doit être un nombre valide !");
            return;
        }
    }

    QFile file("historique.json");
    QJsonArray array;

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        array = doc.array();
        file.close();
    }

    // Création de la fenêtre d'affichage
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Historique des modifications");
    dialog->resize(900, 450);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTableWidget *table = new QTableWidget(dialog);

    table->setColumnCount(6);
    table->setHorizontalHeaderLabels(
        {"ID Élève", "Mouvement", "Champ", "Ancienne", "Nouvelle", "Date"}
        );

    // Remplissage du tableau
    for (auto v : array) {
        QJsonObject obj = v.toObject();

        if (!showAll && obj["id_eleve"].toInt() != id)
            continue;

        int row = table->rowCount();
        table->insertRow(row);

        table->setItem(row, 0, new QTableWidgetItem(QString::number(obj["id_eleve"].toInt())));
        table->setItem(row, 1, new QTableWidgetItem(obj["mouvement"].toString()));
        table->setItem(row, 2, new QTableWidgetItem(obj["champ"].toString()));
        table->setItem(row, 3, new QTableWidgetItem(obj["ancienne"].toString()));
        table->setItem(row, 4, new QTableWidgetItem(obj["nouvelle"].toString()));
        table->setItem(row, 5, new QTableWidgetItem(obj["date"].toString()));
    }

    table->resizeColumnsToContents();
    layout->addWidget(table);
    dialog->setLayout(layout);
    dialog->exec();
}



// RANIM_FIN


void MainWindow::chargerTableEquipements(const QString &sql)
{
    QSqlQuery query;
    if (!query.exec(sql)) {
        QMessageBox::warning(this, "Erreur SQL", query.lastError().text());
        return;
    }

    ui->t_eq->clear();
    ui->t_eq->setRowCount(0);
    ui->t_eq->setColumnCount(7);  // 7 colonnes maintenant

    QStringList headers = {
        "ID", "Catégorie", "État", "Fournisseur",
        "Date d'acquisition", "Lieu", "Image"
    };
    ui->t_eq->setHorizontalHeaderLabels(headers);

    int row = 0;
    while (query.next()) {
        ui->t_eq->insertRow(row);

        ui->t_eq->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        ui->t_eq->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        ui->t_eq->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        ui->t_eq->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        ui->t_eq->setItem(row, 4, new QTableWidgetItem(query.value(4).toDate().toString("dd/MM/yyyy")));
        ui->t_eq->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));

        // --- Colonne IMAGE (index 6) ---
        QByteArray imgData = query.value(6).toByteArray();
        QTableWidgetItem *imgItem = new QTableWidgetItem();
        if (!imgData.isEmpty()) {
            QPixmap pix;
            pix.loadFromData(imgData);
            QIcon icon(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            imgItem->setIcon(icon);
        }
        ui->t_eq->setItem(row, 6, imgItem);

        row++;
    }

    ui->t_eq->resizeColumnsToContents();
    afficherStatistiquesEquipements();
}


// Rafraîchir la table
void MainWindow::rafraichirTableEquipements()
{
    chargerTableEquipements(
        "SELECT ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, "
        "       DATE_ACQUISITION, LIEU, IMAGE "
        "FROM EQUIPEMENTS ORDER BY ID_EQUIPEMENT ASC"
        );
}


// AJOUT



// MODIFICATION
void MainWindow::on_pushButton_Modifier_Equip_clicked()
{
    int id = ui->le_id_eq->text().toInt();

    // 🔥 1) Charger les anciennes valeurs depuis la BD
    QSqlQuery q;
    q.prepare("SELECT CATEGORIE, ETAT, FOURNISSEUR, DATE_ACQUISITION, LIEU, IMAGE "
              "FROM EQUIPEMENTS WHERE ID_EQUIPEMENT = :id");
    q.bindValue(":id", id);

    if (!q.exec() || !q.next()) {
        QMessageBox::warning(this, "Erreur", "Impossible de charger l'équipement.");
        return;
    }

    QString oldCat = q.value(0).toString();
    QString oldEtat = q.value(1).toString();
    QString oldFourn = q.value(2).toString();
    QDate   oldDate = q.value(3).toDate();
    QString oldLieu = q.value(4).toString();
    QByteArray oldImage = q.value(5).toByteArray();

    // 🔥 2) Récupérer les nouvelles valeurs UNIQUEMENT si remplies
    QString newCat = ui->le_categorie->text().isEmpty() ? oldCat : ui->le_categorie->text();
    QString newEtat = ui->le_etat->currentText().isEmpty() ? oldEtat : ui->le_etat->currentText();
    QString newFourn = ui->le_fournisseur->text().isEmpty() ? oldFourn : ui->le_fournisseur->text();
    QDate newDate = ui->de_date_acq->date().isValid() ? ui->de_date_acq->date() : oldDate;
    QString newLieu = ui->le_lieu->text().isEmpty() ? oldLieu : ui->le_lieu->text();

    // 🔥 3) IMAGE : si tu n’as pas inséré une nouvelle → garder l'ancienne
    QByteArray newImage = imageEquipementData.isEmpty() ? oldImage : imageEquipementData;

    // 🔥 4) UPDATE final – SEULS les champs modifiés changent
    QSqlQuery upd;
    upd.prepare("UPDATE EQUIPEMENTS SET "
                "CATEGORIE = :c, "
                "ETAT = :e, "
                "FOURNISSEUR = :f, "
                "DATE_ACQUISITION = :d, "
                "LIEU = :l, "
                "IMAGE = :img "
                "WHERE ID_EQUIPEMENT = :id");

    upd.bindValue(":c", newCat);
    upd.bindValue(":e", newEtat);
    upd.bindValue(":f", newFourn);
    upd.bindValue(":d", newDate);
    upd.bindValue(":l", newLieu);
    upd.bindValue(":img", newImage);
    upd.bindValue(":id", id);

    if (upd.exec()) {
        QMessageBox::information(this, "Succès", "Équipement modifié !");
        rafraichirTableEquipements();
    } else {
        QMessageBox::warning(this, "Erreur", "La modification a échoué.");
    }
}


// SUPPRESSION
void MainWindow::on_pushButton_Supprimer_Equip_clicked()
{
    QString id = ui->le_id_eq->text().trimmed();

    if (id.isEmpty()) {
        QMessageBox::warning(this, "Attention", "Veuillez saisir un ID à supprimer.");
        return;
    }

    Equipement e;
    if (e.supprimer(id)) {
        QMessageBox::information(this, "Succès", "Équipement supprimé avec succès.");
        rafraichirTableEquipements();
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de la suppression.");
    }
}

// RECHERCHE
void MainWindow::on_pushButton_Rechercher_Equip_clicked()
{
    QString texte = ui->le_recherche_eq->text().trimmed();

    QString sql =
        "SELECT ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, "
        "       DATE_ACQUISITION, LIEU, IMAGE "
        "FROM EQUIPEMENTS "
        "WHERE ID_EQUIPEMENT LIKE '%" + texte + "%' "
                  "   OR CATEGORIE LIKE '%" + texte + "%' "
                  "   OR ETAT LIKE '%" + texte + "%' "
                  "   OR FOURNISSEUR LIKE '%" + texte + "%' "
                  "   OR LIEU LIKE '%" + texte + "%'";

    chargerTableEquipements(sql);
}


// TRI
#include <QDebug>

// TRI AUTO par la combobox cb_tri_eq (UNE SEULE COMBO)
void MainWindow::on_cb_tri_eq_currentIndexChanged(int index)
{
    if (index < 0) return;

    // 1) Récupérer le texte visible tel qu’il est dans l’UI
    const QString t = ui->cb_tri_eq->currentText().trimmed().toLower();

    // 2) Mapper le texte vers la vraie colonne SQL (toutes variantes couvertes)
    QString col;
    if (t.contains("id"))                 col = "ID_EQUIPEMENT";
    else if (t.contains("nom") || t.contains("cat")) col = "CATEGORIE";
    else if (t.contains("date"))          col = "DATE_ACQUISITION";
    else if (t.contains("etat"))          col = "ETAT";
    else if (t.contains("fourn"))         col = "FOURNISSEUR";
    else if (t.contains("lieu"))          col = "LIEU";
    else {
        qDebug() << "[TRI] Texte combo non reconnu:" << t;
        return; // rien à faire si "--" ou libellé inattendu
    }

    // 3) Construire la requête sûre (ASC par défaut)
    const QString sql =
        "SELECT ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, "
        "       DATE_ACQUISITION, LIEU, IMAGE "
        "FROM EQUIPEMENTS ORDER BY " + col + " ASC";


    qDebug() << "[TRI] critere=" << t << "-> colonne=" << col;

    // 4) Rafraîchir l’affichage (ta fonction existante)
    chargerTableEquipements(sql);
    afficherStatistiquesEquipements();
}



void MainWindow::clearFormEquipement()
{
    // Champs de saisie
    ui->le_id_eq->clear();
    ui->le_categorie->clear();
    ui->le_fournisseur->clear();
    ui->le_lieu->clear();

    // Combobox État (si tu veux aucune sélection, mets -1)
    if (ui->le_etat->count() > 0)
        ui->le_etat->setCurrentIndex(0);  // ou -1 pour vider la sélection

    // Date par défaut = aujourd’hui
    ui->de_date_acq->setDate(QDate::currentDate());

    // Recherche / Tri (optionnel)
    ui->le_recherche_eq->clear();
    if (ui->cb_tri_eq->count() > 0)       ui->cb_tri_eq->setCurrentIndex(0);

    // Table : enlever toute sélection éventuelle
    if (ui->t_eq) {
        ui->t_eq->clearSelection();
        ui->t_eq->setCurrentCell(-1, -1);
    }

    // Focus pratique
    ui->le_id_eq->setFocus();
}

// Bouton "Annuler" (aucune écriture en base)
void MainWindow::on_pushButton_Annuler_Equip_clicked()
{
    clearFormEquipement();

    // Si tu veux aussi recharger l’affichage complet après un essai d’édition :
    // (sinon, commente cette ligne)
    rafraichirTableEquipements();
    afficherStatistiquesEquipements();
}

void MainWindow::on_pushButton_exporterpdf_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Exporter équipements", "", "PDF (*.pdf)");
    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(".pdf")) fileName += ".pdf";

    QTextDocument doc;
    QTextCursor cursor(&doc);

    cursor.insertHtml("<h1 style='text-align:center;'>Liste des équipements</h1>");
    cursor.insertHtml("<p style='text-align:right;'>Date : " +
                      QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") +
                      "</p><br>");

    int rows = ui->t_eq->rowCount();
    int cols = ui->t_eq->columnCount();

    if (rows == 0) {
        QMessageBox::warning(this, "Erreur", "Aucun équipement à exporter !");
        return;
    }

    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(5);

    QVector<QTextLength> colWidths;
    for (int i = 0; i < cols; ++i)
        colWidths.append(QTextLength(QTextLength::PercentageLength, 100.0/cols));
    tableFormat.setColumnWidthConstraints(colWidths);

    QTextTable *table = cursor.insertTable(rows+1, cols, tableFormat);

    for (int col = 0; col < cols; ++col) {
        QTextTableCell cell = table->cellAt(0, col);
        QTextCursor cur = cell.firstCursorPosition();
        QTextCharFormat fmt;
        fmt.setFontWeight(QFont::Bold);
        fmt.setBackground(QColor("#DDDDDD"));
        cur.insertText(ui->t_eq->horizontalHeaderItem(col)->text(), fmt);
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            QTextTableCell cell = table->cellAt(r+1, c);
            QTextCursor cur = cell.firstCursorPosition();
            QString text = ui->t_eq->item(r,c) ? ui->t_eq->item(r,c)->text() : "";
            cur.insertText(text);
        }
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    doc.print(&printer);

    QMessageBox::information(this, "Succès", "PDF exporté !");
}
void MainWindow::afficherStatistiquesEquipements()
{
    // --- Récupération des données ---
    QSqlQuery query;
    query.exec("SELECT etat FROM equipements");

    int nbNeuf = 0, nbEnPanne = 0, nbEnService = 0;

    while (query.next())
    {
        QString etat = query.value(0).toString().toLower();

        if (etat.contains("neuf"))
            nbNeuf++;
        else if (etat.contains("en panne"))
            nbEnPanne++;
        else if (etat.contains("en service"))
            nbEnService++;
    }

    int total = nbNeuf + nbEnPanne + nbEnService;
    if (total == 0) total = 1;

    QString lblNeuf     = "Neuf (" + QString::number(nbNeuf * 100 / total) + "%)";
    QString lblPanne    = "En panne (" + QString::number(nbEnPanne * 100 / total) + "%)";
    QString lblService  = "En service (" + QString::number(nbEnService * 100 / total) + "%)";



    // ================================================================
    //      STATISTIQUE 1 : widgetStatDisponibilite (AVEC CERCLE)
    // ================================================================
    QPieSeries *series1 = new QPieSeries();
    series1->append(lblNeuf, nbNeuf);
    series1->append(lblPanne, nbEnPanne);
    series1->append(lblService, nbEnService);

    series1->setLabelsVisible(true);

    series1->slices()[0]->setBrush(QColor("#4CAF50"));
    series1->slices()[1]->setBrush(QColor("#F44336"));
    series1->slices()[2]->setBrush(QColor("#2196F3"));

    QChart *chart1 = new QChart();
    chart1->addSeries(series1);
    chart1->setTitle("Statistique de disponibilité");
    chart1->legend()->setAlignment(Qt::AlignRight);

    QChartView *chartView1 = new QChartView(chart1);
    chartView1->setRenderHint(QPainter::Antialiasing);

    chartView1->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 #001F3F, stop:1 #005F9E);"
        "border-radius: 20px;"
        );

    if (ui->widgetStatDisponibilite->layout())
        delete ui->widgetStatDisponibilite->layout();

    QVBoxLayout *layout1 = new QVBoxLayout(ui->widgetStatDisponibilite);
    layout1->addWidget(chartView1);



    // ================================================================
    //      STATISTIQUE 2 : widgetStats_2 (SANS CERCLE)
    // ================================================================

    // ❌ Supprimer ancien layout
    if (ui->widgetStats_2->layout())
        delete ui->widgetStats_2->layout();

    // ✔ Nouveau layout texte (pas de QChart)
    QVBoxLayout *layout2 = new QVBoxLayout(ui->widgetStats_2);

    QLabel *title = new QLabel("Répartition par état");
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
    title->setAlignment(Qt::AlignCenter);

    QLabel *l1 = new QLabel("🟢 Neuf : " + QString::number(nbNeuf));
    QLabel *l2 = new QLabel("🔴 En panne : " + QString::number(nbEnPanne));
    QLabel *l3 = new QLabel("🔵 En service : " + QString::number(nbEnService));

    l1->setStyleSheet("font-size:17px; color:white;");
    l2->setStyleSheet("font-size:17px; color:white;");
    l3->setStyleSheet("font-size:17px; color:white;");

    l1->setAlignment(Qt::AlignCenter);
    l2->setAlignment(Qt::AlignCenter);
    l3->setAlignment(Qt::AlignCenter);

    layout2->addWidget(title);
    layout2->addWidget(l1);
    layout2->addWidget(l2);
    layout2->addWidget(l3);

    ui->widgetStats_2->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "stop:0 #001F3F, stop:1 #005F9E);"
        "border-radius:20px;"
        );
}

void MainWindow::on_pushButton_Ajouter_Equip_2_clicked()
{
    // 1) Choisir une image
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Choisir une image",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp)"
        );

    if (filePath.isEmpty())
        return;

    // 2) Vérification IA (ta fonction existe déjà)
    if (!isEquipmentImage(filePath))
    {
        QMessageBox::critical(this, "Image refusée",
                              "❌ Cette image n’est pas reconnue comme un équipement.\n"
                              "Veuillez choisir une image valide.");
        return;
    }

    // 3) Charger le fichier en BLOB (QByteArray)
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, "Erreur",
                             "Impossible de lire le fichier image.");
        return;
    }
    imageEquipementData = f.readAll();   // <<< stocké en mémoire
    f.close();

    QMessageBox::information(this, "Image validée",
                             "✔ L’image a été acceptée et sera enregistrée avec l’équipement.");

    // 👉 Tu peux maintenant supprimer le QLabel label_imageEquip de l’UI,
    //     on n’en a plus besoin.
}


void MainWindow::on_pushButton_Localiser_Equip_clicked()
{
    // 1) Vérifier qu'une ligne est sélectionnée dans le tableau des équipements
    int row = ui->t_eq->currentIndex().row();
    if (row < 0) {
        QMessageBox::warning(this, "Aucun équipement sélectionné",
                             "Veuillez sélectionner un équipement dans le tableau avant de localiser le fournisseur.");
        return;
    }

    // 2) Récupérer le nom du fournisseur (adapter l'index de colonne si besoin)
    int colFournisseur = 3;   // colonne FOURNISSEUR dans ton tableau t_eq
    QModelIndex indexF = ui->t_eq->model()->index(row, colFournisseur);
    QString fournisseur = ui->t_eq->model()->data(indexF).toString().trimmed();

    if (fournisseur.isEmpty()) {
        QMessageBox::warning(this, "Fournisseur vide",
                             "Le fournisseur de cet équipement n'est pas renseigné.");
        return;
    }

    // 3) Afficher la carte hors-ligne (tunis_map.png) dans le QGraphicsView
    ui->graphicsView_map->setVisible(true);   // la rendre visible
    ui->graphicsView_map->raise();            // la mettre au premier plan

    // Option simple : centrer la vue sur toute la carte (déjà chargée dans m_mapScene)
    ui->graphicsView_map->fitInView(m_mapScene->sceneRect(), Qt::KeepAspectRatio);

    // Option bonus : zoomer approximativement sur le fournisseur (par son nom)
    for (QGraphicsItem *item : m_mapScene->items()) {
        QVariant v = item->data(0);
        if (v.canConvert<SupplierInfo>()) {
            SupplierInfo info = v.value<SupplierInfo>();
            if (info.name.compare(fournisseur, Qt::CaseInsensitive) == 0) {
                QRectF r = item->sceneBoundingRect().adjusted(-40, -40, 40, 40);
                ui->graphicsView_map->fitInView(r, Qt::KeepAspectRatio);
                break;
            }
        }
    }
}



void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (ui->graphicsView_map->isVisible()) {

        const double scaleFactor = 1.15;

        if (event->angleDelta().y() > 0) {
            ui->graphicsView_map->scale(scaleFactor, scaleFactor); // Zoom +
        } else {
            ui->graphicsView_map->scale(1.0 / scaleFactor, 1.0 / scaleFactor); // Zoom -
        }
    }
}
void MainWindow::on_t_eq_itemClicked(QTableWidgetItem *item)
{
    int row = item->row();

    ui->le_id_eq->setText(ui->t_eq->item(row, 0)->text());
    ui->le_categorie->setText(ui->t_eq->item(row, 1)->text());
    ui->le_etat->setCurrentText(ui->t_eq->item(row, 2)->text());
    ui->le_fournisseur->setText(ui->t_eq->item(row, 3)->text());

    QString dateStr = ui->t_eq->item(row, 4)->text();
    ui->de_date_acq->setDate(QDate::fromString(dateStr, "dd/MM/yyyy"));

    ui->le_lieu->setText(ui->t_eq->item(row, 5)->text());

    // Charger image depuis DB
    QSqlQuery q;
    q.prepare("SELECT IMAGE FROM EQUIPEMENTS WHERE ID_EQUIPEMENT = :id");
    q.bindValue(":id", ui->le_id_eq->text().toInt());

    if (q.exec() && q.next()) {
        imageEquipementData = q.value(0).toByteArray();
    }
}


// Example popup button
void MainWindow::on_btnShowPopup_clicked()
{
    QMessageBox::information(this, "Horaires Libres", "08/10/2025 08:30");
}

void MainWindow::insertData()
{
    QString idEmployeStr = ui->comboBox_15->currentText().trimmed();
    QString matricule = ui->comboBox_18->currentText().trimmed();

    if (idEmployeStr.isEmpty() || matricule.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Veuillez sélectionner un employé et un véhicule.");
        return;
    }

    bool okIdEmp;
    int idEmploye = idEmployeStr.toInt(&okIdEmp);
    if (!okIdEmp || idEmploye <= 0) {
        QMessageBox::warning(this, "ID Employé invalide", "L'ID de l'employé doit être un entier positif.");
        return;
    }

    QString idStr = ui->lineEdit_53->text().trimmed();
    QString date = ui->dateEdit_9->date().toString("yyyy-MM-dd");
    QString heureDebut = ui->timeEdit->time().toString("HH:mm");
    QString heureFin = ui->timeEdit_2->time().toString("HH:mm");
    QString type = ui->comboBox_11->currentText().trimmed();
    QString circuit = ui->lineEdit_55->text().trimmed();
    QString tarifStr = ui->lineEdit_57->text().trimmed();
    /* QString idEleveStr = ui->lineEdit_68->text().trimmed();  // 🔹 NEW*/
    QString idEleveStr = ui->comboBox->currentText().trimmed();


    // 🔹 1. Check for empty fields
    if (idStr.isEmpty() || date.isEmpty() || heureDebut.isEmpty() || heureFin.isEmpty()
        || type.isEmpty() || circuit.isEmpty() || tarifStr.isEmpty() || idEleveStr.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "Veuillez remplir tous les champs avant d'ajouter un cours.");
        return;
    }

    // 🔹 2. Validate numeric values
    bool okId, okTarif, okIdEleve;
    int id = idStr.toInt(&okId);
    int idEleve = idEleveStr.toInt(&okIdEleve);
    double tarif = tarifStr.toDouble(&okTarif);

    if (!okId || id <= 0) {
        QMessageBox::warning(this, "ID invalide", "L'identifiant du cours doit être un entier positif.");
        return;
    }
    if (!okTarif || tarif <= 0) {
        QMessageBox::warning(this, "Tarif invalide", "Le tarif doit être un nombre réel positif (ex: 50.0).");
        return;
    }
    if (!okIdEleve || idEleve <= 0) {
        QMessageBox::warning(this, "ID Élève invalide", "L'ID de l'élève doit être un entier positif.");
        return;
    }

    // 🔹 3. Validate heureDebut < heureFin (BEFORE adding)
    QTime debut = QTime::fromString(heureDebut, "HH:mm");
    QTime fin = QTime::fromString(heureFin, "HH:mm");

    if (!debut.isValid() || !fin.isValid()) {
        QMessageBox::warning(this, "Erreur d'heure", "Format d'heure invalide.");
        return;
    }

    int dureeMinutes = debut.secsTo(fin) / 60;
    if (dureeMinutes <= 0) {
        QMessageBox::warning(this, "Heures invalides", "L'heure de fin doit être postérieure à l'heure de début.");
        return;  // ❌ stop before adding
    }

    // 🔹 4. Check if ID_ELEVE exists in ELEVES table
    QSqlQuery eleveCheck;
    eleveCheck.prepare("SELECT COUNT(*) FROM ELEVES WHERE ID_ELEVE = :idEleve");
    eleveCheck.bindValue(":idEleve", idEleve);
    if (!eleveCheck.exec()) {
        QMessageBox::critical(this, "Erreur SQL", "Erreur lors de la vérification de l'élève : " + eleveCheck.lastError().text());
        return;
    }
    eleveCheck.next();
    if (eleveCheck.value(0).toInt() == 0) {
        QMessageBox::warning(this, "Élève introuvable", QString("Aucun élève avec l'ID %1 n'existe.").arg(idEleve));
        return;
    }

    // 🔹 5. Check if ID_COURS already exists
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM COURS WHERE ID_COURS = :id");
    checkQuery.bindValue(":id", id);
    if (!checkQuery.exec()) {
        QMessageBox::critical(this, "Erreur SQL", "Erreur lors de la vérification de l'ID du cours : " + checkQuery.lastError().text());
        return;
    }
    checkQuery.next();
    if (checkQuery.value(0).toInt() > 0) {
        QMessageBox::warning(this, "ID existant", QString("Un cours avec l'ID %1 existe déjà.").arg(id));
        return;
    }

    // 🔹 6a. Check for schedule conflicts if checkBox_9 is checked
    if (ui->checkBox_9->isChecked()) {
        QSqlQuery conflictQuery;
        conflictQuery.prepare(R"(
        SELECT COUNT(*)
        FROM COURS
        WHERE ID_EMPLOYE = :idEmploye
          AND DATE_COURS = TO_DATE(:date, 'YYYY-MM-DD')
          AND (
              (:heureDebut < HEURE_FIN AND :heureFin > HEURE_DEBUT)
          )
    )");
        conflictQuery.bindValue(":idEmploye", idEmploye);
        conflictQuery.bindValue(":date", date);
        conflictQuery.bindValue(":heureDebut", heureDebut);
        conflictQuery.bindValue(":heureFin", heureFin);

        if (!conflictQuery.exec()) {
            QMessageBox::critical(this, "Erreur SQL", "Erreur lors de la vérification des conflits : " + conflictQuery.lastError().text());
            return;
        }
        conflictQuery.next();
        if (conflictQuery.value(0).toInt() > 0)
        {
            QMessageBox::warning(
                this,
                "Conflit horaire",
                "Le cours chevauche un autre cours existant pour cet employé.\n"
                "Voici les créneaux disponibles :"
                );

            // 🔥 Show popup with available free time
            showFreeSlotsPopup(ui->dateEdit_9->date());

            return; // Stop insertion
        }

    }







    // ✅ 6. Create the course object (add ID_ELEVE)
    Cours c(id, date, heureDebut, heureFin, type, circuit, tarif, idEleve, idEmploye, matricule);

    if (c.ajouter()) {
        // 🔹 7. Calculate and update duration
        int heures = dureeMinutes / 60;
        int minutes = dureeMinutes % 60;
        QString dureeStr = QString("%1h %2min").arg(heures).arg(minutes, 2, 10, QChar('0'));

        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE COURS SET DUREE = :duree WHERE ID_COURS = :id");
        updateQuery.bindValue(":duree", dureeStr);
        updateQuery.bindValue(":id", id);
        updateQuery.exec();  // non-critical

        QMessageBox::information(this, "Succès", "Cours ajouté avec succès !");
        loadDataToTable();
        clearFields();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de l'ajout du cours. Veuillez vérifier vos informations.");
    }
}




// 🧹 Clear all form fields
void MainWindow::clearFields()
{
    ui->lineEdit_53->clear();
    ui->lineEdit_55->clear();
    ui->lineEdit_57->clear();
    ui->comboBox_11->setCurrentIndex(0);
    ui->dateEdit_9->setDate(QDate::currentDate());
    ui->timeEdit->setTime(QTime::currentTime());
    ui->timeEdit_2->setTime(QTime::currentTime());
}




void MainWindow::loadDataToTable()
{
    QSqlQueryModel *model = cours.afficher();

    ui->tableWidget_9->setRowCount(model->rowCount());
    ui->tableWidget_9->setColumnCount(model->columnCount());

    QStringList headers = {"ID_COURS", "DATE_COURS", "HEURE_DEBUT", "HEURE_FIN",
                           "TYPE_COURS", "CIRCUIT", "DUREE", "TARIF",
                           "ID_ELEVE", "ID_EMPLOYE", "MATRICULE"};
    ui->tableWidget_9->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < model->rowCount(); ++i) {
        for (int j = 0; j < model->columnCount(); ++j) {
            QVariant value = model->data(model->index(i, j));

            // TYPE_COURS → QComboBox
            if (j == 4) {
                QComboBox *combo = new QComboBox();
                combo->addItems({"Code", "Conduite"});
                combo->setCurrentText(value.toString());
                ui->tableWidget_9->setCellWidget(i, j, combo);
            }
            // DATE formatting
            else if (j == 1) {
                QDate date = value.toDate();
                ui->tableWidget_9->setItem(i, j, new QTableWidgetItem(date.toString("yyyy-MM-dd")));
            }
            else {
                QTableWidgetItem *item = new QTableWidgetItem(value.toString());
                if (j == 0) item->setData(Qt::UserRole, model->data(model->index(i, 0))); // store ID
                ui->tableWidget_9->setItem(i, j, item);
            }
        }
    }

    ui->tableWidget_9->resizeColumnsToContents();
    delete model;
    cours.refreshStatistiques();

}

// ❌ Delete a selected course
void MainWindow::on_pushButton_88_clicked()
{
    int selectedRow = ui->tableWidget_9->currentRow();
    if (selectedRow < 0) {
        QMessageBox::warning(this, "Sélection requise", "Veuillez sélectionner une ligne à supprimer.");
        return;
    }

    int id = ui->tableWidget_9->item(selectedRow, 0)->text().toInt();

    if (QMessageBox::question(this, "Confirmation", "Supprimer ce cours ?") == QMessageBox::Yes) {
        if (cours.supprimer(id)) {
            QMessageBox::information(this, "Supprimé", "Cours supprimé avec succès.");
            loadDataToTable();
        } else {
            QMessageBox::critical(this, "Erreur", "Échec de la suppression.");
        }
    }
    cours.refreshStatistiques();

}



void MainWindow::modifierCoursDepuisTable()
{
    int row = ui->tableWidget_9->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Attention", "Veuillez sélectionner une ligne à modifier.");
        return;
    }

    QTableWidgetItem *idItem = ui->tableWidget_9->item(row, 0);
    if (!idItem) return;

    int oldId = idItem->data(Qt::UserRole).toInt();
    int newId = idItem->text().toInt();

    // 🔹 HeureDebut and HeureFin from QTimeEdit if used, otherwise text
    QString date = ui->tableWidget_9->item(row, 1)->text().trimmed();

    QTime debut, fin;
    // If using QTimeEdit widgets in table:
    if (QTimeEdit *debutEdit = qobject_cast<QTimeEdit*>(ui->tableWidget_9->cellWidget(row, 2)))
        debut = debutEdit->time();
    else
        debut = QTime::fromString(ui->tableWidget_9->item(row, 2)->text().trimmed(), "HH:mm");

    if (QTimeEdit *finEdit = qobject_cast<QTimeEdit*>(ui->tableWidget_9->cellWidget(row, 3)))
        fin = finEdit->time();
    else
        fin = QTime::fromString(ui->tableWidget_9->item(row, 3)->text().trimmed(), "HH:mm");

    if (!debut.isValid() || !fin.isValid()) {
        QMessageBox::warning(this, "Erreur d'heure", "Les heures doivent être au format HH:mm.");
        loadDataToTable();
        return;
    }

    if (debut >= fin) {
        QMessageBox::warning(this, "Heures invalides", "L'heure de fin doit être postérieure à l'heure de début.");
        loadDataToTable();
        return;
    }

    // 🔹 Type validation
    QString type;
    if (QComboBox *typeCombo = qobject_cast<QComboBox*>(ui->tableWidget_9->cellWidget(row, 4))) {
        type = typeCombo->currentText().trimmed();
    } else {
        type = ui->tableWidget_9->item(row, 4)->text().trimmed();
    }

    if (type != "Code" && type != "Conduite") {
        QMessageBox::warning(this, "Type invalide", "Le type de cours doit être 'Code' ou 'Conduite'.");
        loadDataToTable();
        return;
    }

    QString circuit = ui->tableWidget_9->item(row, 5)->text().trimmed();
    QString tarifStr = ui->tableWidget_9->item(row, 7)->text().trimmed();
    QString idEmployeStr = ui->tableWidget_9->item(row, 9)->text().trimmed();
    QString matricule = ui->tableWidget_9->item(row, 10)->text().trimmed();

    bool okTarif, okEmp;
    double tarif = tarifStr.toDouble(&okTarif);
    int idEmploye = idEmployeStr.toInt(&okEmp);

    if (!okTarif || tarif <= 0) {
        QMessageBox::warning(this, "Tarif invalide", "Le tarif doit être un nombre positif.");
        loadDataToTable();
        return;
    }
    if (!okEmp || idEmploye <= 0) {
        QMessageBox::warning(this, "Employé invalide", "L'ID employé doit être un entier positif.");
        loadDataToTable();
        return;
    }

    QSqlQuery query;
    query.prepare(R"(
        UPDATE COURS SET
            ID_COURS = :newId,
            DATE_COURS = TO_DATE(:date, 'YYYY-MM-DD'),
            HEURE_DEBUT = :debut,
            HEURE_FIN = :fin,
            TYPE_COURS = :type,
            CIRCUIT = :circuit,
            TARIF = :tarif,
            ID_EMPLOYE = :idEmp,
            MATRICULE = :matricule
        WHERE ID_COURS = :oldId
    )");

    query.bindValue(":newId", newId);
    query.bindValue(":date", date);
    query.bindValue(":debut", debut.toString("HH:mm"));
    query.bindValue(":fin", fin.toString("HH:mm"));
    query.bindValue(":type", type);
    query.bindValue(":circuit", circuit);
    query.bindValue(":tarif", tarif);
    query.bindValue(":idEmp", idEmploye);
    query.bindValue(":matricule", matricule);
    query.bindValue(":oldId", oldId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Erreur SQL", query.lastError().text());
        loadDataToTable();
        return;
    }

    QMessageBox::information(this, "Succès", "Cours modifié avec succès !");
    loadDataToTable();
}

// 🖨️ Export table to PDF
void MainWindow::on_pushButton_117_clicked()
{
    if (cours.exporterPDF(ui->tableWidget_9, this)) {
        QMessageBox::information(this, "Succès", "PDF généré avec succès !");
    }
}

// 🔍 Filter courses by Type
/*void MainWindow::on_pushButton_140_clicked()
{
    QString selected = ui->comboBox_10->currentText();  // "Tout afficher", "Code", or "Conduite"
    int typeColumn = 4; // Column where TYPE (Code/Conduite) combo boxes exist

    // --- SHOW ALL ---
    if (selected == "Aucun")
    {
        for (int row = 0; row < ui->tableWidget_9->rowCount(); ++row)
        {
            ui->tableWidget_9->setRowHidden(row, false);
        }
        return;
    }

    // --- FILTER CODE OR CONDUITE ---
    for (int row = 0; row < ui->tableWidget_9->rowCount(); ++row)
    {
        QWidget *w = ui->tableWidget_9->cellWidget(row, typeColumn);
        QComboBox *cb = qobject_cast<QComboBox*>(w);

        QString value = cb ? cb->currentText() : "";

        bool match = (value.compare(selected, Qt::CaseInsensitive) == 0);

        ui->tableWidget_9->setRowHidden(row, !match);
    }
}*/
/*void MainWindow::on_pushButton_140_clicked()
{
    QTableWidget *table = ui->tableWidget_9;
    QString selected = ui->comboBox_10->currentText();
    int typeColumn = 4;

    // --- Case: Aucun → remove sort and show all normally ---
    if (selected == "Aucun")
    {
        table->setSortingEnabled(false);
        return;
    }

    // Ensure we have a hidden column for sorting
    int sortColumn = table->columnCount();
    table->insertColumn(sortColumn);
    table->setColumnHidden(sortColumn, true);

    // Assign "0" (match) or "1" (not match) to the hidden column
    for (int row = 0; row < table->rowCount(); row++)
    {
        QComboBox *cb = qobject_cast<QComboBox*>(table->cellWidget(row, typeColumn));
        QString value = cb ? cb->currentText() : "";

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setText(value == selected ? "0" : "1");

        table->setItem(row, sortColumn, item);
    }

    // Sort using the hidden column
    table->setSortingEnabled(true);
    table->sortItems(sortColumn, Qt::AscendingOrder);

    // Disable sorting to prevent user affecting future operations
    table->setSortingEnabled(false);

    // Remove the temporary hidden column
    table->removeColumn(sortColumn);
}*/
void MainWindow::on_pushButton_Ajouter_Equip_clicked()
{
    // Récupération des données
    QString id = ui->le_id_eq->text().trimmed();
    QString categorie = ui->le_categorie->text().trimmed();
    QString etat = ui->le_etat->currentText().trimmed();
    QString fournisseur = ui->le_fournisseur->text().trimmed();
    QDate date = ui->de_date_acq->date();
    QString lieu = ui->le_lieu->text().trimmed();

    // Validation
    if (id.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "L'ID est obligatoire !");
        ui->le_id_eq->setFocus();
        return;
    }

    if (categorie.isEmpty() || etat.isEmpty() || fournisseur.isEmpty() || lieu.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis !");
        return;
    }

    // Vérifier la date
    if (!date.isValid() || date > QDate::currentDate()) {
        QMessageBox::warning(this, "Erreur", "Date d'acquisition invalide !");
        ui->de_date_acq->setFocus();
        return;
    }

    // Créer et ajouter l'équipement - 6 paramètres seulement
    Equipement equip(id, categorie, etat, fournisseur, date, lieu);

    if (equip.ajouter()) {
        QMessageBox::information(this, "Succès", "Équipement ajouté avec succès !");

        // Vider les champs
        ui->le_id_eq->clear();
        ui->le_categorie->clear();
        ui->le_etat->setCurrentIndex(0);
        ui->le_fournisseur->clear();
        ui->de_date_acq->setDate(QDate::currentDate());
        ui->le_lieu->clear();

        // Rafraîchir l'affichage
        afficherEquipements();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de l'ajout !\nVérifiez que l'ID n'existe pas déjà.");
    }
}
void MainWindow::afficherEquipements()
{
    Equipement equip;
    QSqlQueryModel *model = equip.afficher();

    // Si vous avez un QTableView, utilisez ceci:
    // ui->tableView_Equipements->setModel(model); // Pour QTableView

    // Si vous avez un QTableWidget, utilisez cette approche:
    if (ui->t_eq) { // Assurez-vous que t_eq est un QTableWidget
        // Effacer le contenu existant
        ui->t_eq->clearContents();
        ui->t_eq->setRowCount(0);

        // Récupérer les données du modèle
        if (model) {
            int rowCount = model->rowCount();
            int columnCount = model->columnCount();

            // Configurer les en-têtes
            QStringList headers;
            for (int i = 0; i < columnCount; ++i) {
                headers << model->headerData(i, Qt::Horizontal).toString();
            }
            ui->t_eq->setColumnCount(columnCount);
            ui->t_eq->setHorizontalHeaderLabels(headers);

            // Remplir les données
            ui->t_eq->setRowCount(rowCount);
            for (int row = 0; row < rowCount; ++row) {
                for (int col = 0; col < columnCount; ++col) {
                    QTableWidgetItem *item = new QTableWidgetItem(
                        model->data(model->index(row, col)).toString());
                    ui->t_eq->setItem(row, col, item);
                }
            }

            // Ajuster la taille des colonnes
            ui->t_eq->resizeColumnsToContents();
        }
    }
}
void MainWindow::on_pushButton_140_clicked()
{
    QTableWidget *table = ui->tableWidget_9;
    QString selected = ui->comboBox_10->currentText();
    int typeColumn = 4;

    // Permanent hidden column for original order
    int originalCol = table->columnCount() - 1;

    // If selected = "Aucun" → restore original order
    if (selected == "Aucun")
    {
        table->setSortingEnabled(true);
        table->sortItems(originalCol, Qt::AscendingOrder);
        table->setSortingEnabled(false);
        return;
    }

    // Temporary hidden column for tri
    int sortCol = table->columnCount();
    table->insertColumn(sortCol);
    table->setColumnHidden(sortCol, true);

    // Fill "0" for matching rows, "1" for others
    for (int row = 0; row < table->rowCount(); row++)
    {
        QComboBox *cb = qobject_cast<QComboBox*>(table->cellWidget(row, typeColumn));
        QString value = cb ? cb->currentText() : "";

        QTableWidgetItem *item = new QTableWidgetItem(value == selected ? "0" : "1");
        table->setItem(row, sortCol, item);
    }

    // Sort by tri then by original order to keep internal stability
    table->setSortingEnabled(true);
    table->sortItems(sortCol, Qt::AscendingOrder);
    table->setSortingEnabled(false);

    // Remove temporary column
    table->removeColumn(sortCol);
}

void MainWindow::loadComboBoxes()
{
    ui->comboBox_15->clear();
    ui->comboBox_18->clear();

    // Load ID_EMPLOYE
    QSqlQuery queryEmp;
    if (queryEmp.exec("SELECT ID_EMPLOYE FROM EMPLOYES")) {
        while (queryEmp.next()) {
            ui->comboBox_15->addItem(queryEmp.value(0).toString());
        }
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de charger les employés : " + queryEmp.lastError().text());
    }

    // Load MATRICULE
    QSqlQuery queryVeh;
    if (queryVeh.exec("SELECT MATRICULE FROM VEHICULES")) {
        while (queryVeh.next()) {
            ui->comboBox_18->addItem(queryVeh.value(0).toString());
        }
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de charger les véhicules : " + queryVeh.lastError().text());
    }
    QSqlQuery queryEleve;
    if (queryEleve.exec("SELECT ID_ELEVE FROM ELEVES")) {
        while (queryEleve.next()) {
            ui->comboBox->addItem(queryEleve.value(0).toString());
        }
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible de charger les élèves : " + queryEleve.lastError().text());
    }
}


void MainWindow::on_pushButton_38_clicked()
{
    modifierCoursDepuisTable();  // call your function that updates the table
    cours.refreshStatistiques();

}
// Display all vehicles in the table
void MainWindow::afficherVehicules()
{
    QList<Vehicule> vehicules = Vehicule::afficher();
    afficherVehicules(vehicules);
}

// Display a list of vehicles in the table
void MainWindow::afficherVehicules(QList<Vehicule> vehicules)
{
    // Clear the table
    ui->listeVoiture->setRowCount(0);

    // Set the table to have the correct number of rows
    ui->listeVoiture->setRowCount(vehicules.size());

    // Fill the table with vehicle data
    QStringList matricules;
    for (int i = 0; i < vehicules.size(); i++) {
        Vehicule v = vehicules[i];

        ui->listeVoiture->setItem(i, 0, new QTableWidgetItem(v.getMarque()));
        ui->listeVoiture->setItem(i, 1, new QTableWidgetItem(v.getModele()));
        ui->listeVoiture->setItem(i, 2, new QTableWidgetItem(v.getMatricule()));
        ui->listeVoiture->setItem(i, 3, new QTableWidgetItem(QString::number(v.getKilometrage())));
        ui->listeVoiture->setItem(i, 4, new QTableWidgetItem(QString::number(v.getKilometrageLimite())));
        ui->listeVoiture->setItem(i, 5, new QTableWidgetItem(v.getDateMiseService().toString("dd/MM/yyyy")));
        
        // Add warning icon for vehicles in maintenance or exceeding limit
        QString etat = v.getEtat();
        bool shouldWarn = false;
        
        if (etat.toLower().trimmed() == "en maintenance") {
            shouldWarn = true;
        } else if (v.getKilometrageLimite() > 0 && v.getKilometrage() > v.getKilometrageLimite()) {
            shouldWarn = true;
            etat = "En Maintenance";  // Force maintenance if exceeded
        }
        
        QString etatDisplay = shouldWarn ? "⚠️ " + etat : etat;
        QTableWidgetItem *etatItem = new QTableWidgetItem(etatDisplay);
        
        // Color code the état
        if (shouldWarn) {
            etatItem->setBackground(QColor("#FFA500"));  // Orange background
            etatItem->setForeground(QColor("#8B0000"));  // Dark red text
            etatItem->setFont(QFont("Segoe UI", 9, QFont::Bold));
        } else if (etat.toLower().trimmed() == "en marche") {
            etatItem->setForeground(Qt::darkGreen);
        } else if (etat.toLower().trimmed() == "en panne") {
            etatItem->setForeground(Qt::red);
        }
        
        ui->listeVoiture->setItem(i, 6, etatItem);
        
        matricules << v.getMatricule();
    }

    qDebug() << "Displayed" << vehicules.size() << "vehicles in table";
}

// Clear the vehicle form
void MainWindow::viderFormulaireVehicule()
{
    ui->lineEdit_45->clear();  // Marque
    ui->lineEdit_42->clear();  // Modèle
    ui->lineEdit_43->clear();  // Matricule
    ui->lineEdit_44->clear();  // Kilométrage
    ui->lineEdit_47->clear();  // Kilométrage Limite
    ui->dateEdit_7->setDate(QDate::currentDate());  // Date_Mise_Service
    ui->lineEdit_41->clear();  // État

    selectedMatricule = "";

    qDebug() << "Form cleared";
}

// Fill the form with vehicle data
void MainWindow::remplirFormulaireVehicule(Vehicule v)
{
    ui->lineEdit_45->setText(v.getMarque());
    ui->lineEdit_42->setText(v.getModele());
    ui->lineEdit_43->setText(v.getMatricule());
    ui->lineEdit_44->setText(QString::number(v.getKilometrage()));
    ui->lineEdit_47->setText(QString::number(v.getKilometrageLimite()));
    ui->dateEdit_7->setDate(v.getDateMiseService());
    ui->lineEdit_41->setText(v.getEtat());

    selectedMatricule = v.getMatricule();
}

// Add or update a vehicle
void MainWindow::on_valider_clicked()
{
    // Get data from form
    QString marque = ui->lineEdit_45->text().trimmed();
    QString modele = ui->lineEdit_42->text().trimmed();
    QString matricule = ui->lineEdit_43->text().trimmed();
    QString kilometrageStr = ui->lineEdit_44->text().trimmed();
    QString kilometrageLimiteStr = ui->lineEdit_47->text().trimmed();
    QDate dateMiseService = ui->dateEdit_7->date();
    QString etat = ui->lineEdit_41->text().trimmed();

    // Validate input
    if (marque.isEmpty() || modele.isEmpty() || matricule.isEmpty() || kilometrageStr.isEmpty() || etat.isEmpty()) {
        QMessageBox::warning(this, "Validation Error",
                             "Please fill all required fields:\n"
                             "- Marque\n"
                             "- Modèle\n"
                             "- Matricule\n"
                             "- Kilométrage\n"
                             "- État");
        return;
    }

    bool ok;
    int kilometrage = kilometrageStr.toInt(&ok);
    if (!ok || kilometrage < 0) {
        QMessageBox::warning(this, "Validation Error", "Kilométrage must be a positive number!");
        return;
    }

    int kilometrageLimite = 0;
    if (!kilometrageLimiteStr.isEmpty()) {
        kilometrageLimite = kilometrageLimiteStr.toInt(&ok);
        if (!ok || kilometrageLimite < 0) {
            QMessageBox::warning(this, "Validation Error", "Kilométrage Limite must be a positive number!");
            return;
        }
    }

    // Check if kilometrage exceeds limite - trigger alert and auto-change état
    if (kilometrageLimite > 0 && kilometrage > kilometrageLimite) {
        verifierKilometrageLimite(kilometrage, kilometrageLimite, matricule);
        etat = "En Maintenance";  // Automatically set to maintenance
        ui->lineEdit_41->setText(etat);  // Update the form
    }

    // Create vehicle object
    Vehicule v(matricule, marque, modele, kilometrage, kilometrageLimite, dateMiseService, etat, "", QDate(), "");

    bool success = false;

    // Check if we're updating or adding
    if (!selectedMatricule.isEmpty() && selectedMatricule == matricule) {
        // Update existing vehicle
        success = v.modifier();
        if (success) {
            QMessageBox::information(this, "Success", "Vehicle updated successfully!");
        } else {
            QMessageBox::critical(this, "Error", "Failed to update vehicle!");
        }
    } else {
        // Add new vehicle
        success = v.ajouter();
        if (success) {
            QMessageBox::information(this, "Success", "Vehicle added successfully!");
        } else {
            QMessageBox::critical(this, "Error", "Failed to add vehicle!\n"
                                                 "The matricule might already exist.");
        }
    }

    if (success) {
        viderFormulaireVehicule();
        afficherVehicules();
    }
}

// Clear the form
void MainWindow::on_boutonViderFormulaireVoiture_clicked()
{
    viderFormulaireVehicule();
}

// Delete selected vehicle
void MainWindow::on_boutonSupprimerVoiture_clicked()
{
    int currentRow = ui->listeVoiture->currentRow();

    if (currentRow < 0) {
        QMessageBox::warning(this, "Selection Error", "Please select a vehicle to delete!");
        return;
    }

    // Get matricule from the selected row
    QString matricule = ui->listeVoiture->item(currentRow, 2)->text();

    // Confirm deletion
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Deletion",
                                  "Are you sure you want to delete the vehicle with matricule: " + matricule + "?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        Vehicule v = Vehicule::getByMatricule(matricule);

        if (v.supprimer()) {
            QMessageBox::information(this, "Success", "Vehicle deleted successfully!");
            viderFormulaireVehicule();
            afficherVehicules();
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete vehicle!");
        }
    }
}

// Load selected vehicle data into form for editing
void MainWindow::on_boutonModifierVoiture_clicked()
{
    int currentRow = ui->listeVoiture->currentRow();

    if (currentRow < 0) {
        QMessageBox::warning(this, "Selection Error", "Please select a vehicle to modify!");
        return;
    }

    // Get matricule from the selected row
    QString matricule = ui->listeVoiture->item(currentRow, 2)->text();

    // Load vehicle data
    Vehicule v = Vehicule::getByMatricule(matricule);
    remplirFormulaireVehicule(v);

    QMessageBox::information(this, "Edit Mode",
                             "Vehicle loaded into form.\n"
                             "Modify the fields and click 'Valider' to save changes.");
}

// Search for vehicles
void MainWindow::on_boutonChercherVoiture_clicked()
{
    QString critere = ui->lineEdit_46->text().trimmed();

    if (critere.isEmpty()) {
        afficherVehicules();
    } else {
        QList<Vehicule> vehicules = Vehicule::rechercher(critere);
        afficherVehicules(vehicules);

        if (vehicules.isEmpty()) {
            QMessageBox::information(this, "Search Result", "No vehicles found matching: " + critere);
        }
    }
}

// Handle sorting button click (label_103)
void MainWindow::on_label_103_clicked()
{
    int index = ui->comboBox_7->currentIndex();
    QString critere = ui->comboBox_7->currentText();
    
    if (index == 0 || critere == "Aucun") {
        // Aucun - No sorting, display all vehicles in default order
        afficherVehicules();
    } else if (index == 1 || critere == "Matricule") {
        // Matricule - Sort by matricule ascending
        QList<Vehicule> vehicules = Vehicule::afficherTrie("MATRICULE", true);
        afficherVehicules(vehicules);
    }
}

// Handle cell click in the table
void MainWindow::on_listeVoiture_cellClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row >= 0) {
        QString matricule = ui->listeVoiture->item(row, 2)->text();
        selectedMatricule = matricule;
    }
}

// Handle statistics button click
void MainWindow::on_label_143_clicked()
{
    afficherStatistiquesVehicules();
}

// Display vehicle statistics by état
void MainWindow::afficherStatistiquesVehicules()
{
    QList<Vehicule> vehicules = Vehicule::afficher();
    
    // Hide text edit and remove old charts/scroll areas
    ui->lineEdit_56->hide();
    QWidget *container = ui->lineEdit_56->parentWidget();
    QList<QScrollArea*> oldScrolls = container->findChildren<QScrollArea*>("vehiculeAnalyseScroll");
    for (QScrollArea *oldScroll : oldScrolls) {
        oldScroll->deleteLater();
    }
    QList<QWidget*> oldWidgets = container->findChildren<QWidget*>("vehiculeAnalyseChart");
    for (QWidget *oldWidget : oldWidgets) {
        oldWidget->deleteLater();
    }
    QList<QChartView*> oldCharts = container->findChildren<QChartView*>("vehiculeStatChart");
    for (QChartView *oldChart : oldCharts) {
        oldChart->deleteLater();
    }
    
    int enMarche = 0;
    int enPanne = 0;
    int enMaintenance = 0;
    int autres = 0;
    
    // Count vehicles by état
    for (const Vehicule &v : vehicules) {
        QString etat = v.getEtat().toLower().trimmed();
        
        if (etat == "en marche") {
            enMarche++;
        } else if (etat == "en panne") {
            enPanne++;
        } else if (etat == "en maintenance") {
            enMaintenance++;
        } else {
            autres++;
        }
    }
    
    int total = vehicules.size();
    
    // Calculate percentages
    double pctMarche = total > 0 ? (enMarche * 100.0 / total) : 0;
    double pctPanne = total > 0 ? (enPanne * 100.0 / total) : 0;
    double pctMaintenance = total > 0 ? (enMaintenance * 100.0 / total) : 0;
    
    // Create pie chart for état distribution
    QPieSeries *series = new QPieSeries();
    
    if (enMarche > 0) {
        QPieSlice *slice = series->append(QString("En Marche: %1").arg(enMarche), enMarche);
        slice->setBrush(QColor("#27AE60")); // Green
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.15);
    }
    
    if (enPanne > 0) {
        QPieSlice *slice = series->append(QString("En Panne: %1").arg(enPanne), enPanne);
        slice->setBrush(QColor("#E74C3C")); // Red
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.15);
    }
    
    if (enMaintenance > 0) {
        QPieSlice *slice = series->append(QString("En Maintenance: %1").arg(enMaintenance), enMaintenance);
        slice->setBrush(QColor("#F39C12")); // Orange
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.15);
    }
    
    if (autres > 0) {
        QPieSlice *slice = series->append(QString("Autres: %1").arg(autres), autres);
        slice->setBrush(QColor("#95A5A6")); // Gray
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.15);
    }
    
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("STATISTIQUES - REPARTITION PAR ETAT");
    chart->setTitleFont(QFont("Segoe UI", 12, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    // Create chart view
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setObjectName("vehiculeStatChart");
    chartView->setParent(container);
    chartView->setGeometry(ui->lineEdit_56->geometry());
    chartView->show();
    
    QMessageBox::information(this, "Statistiques", 
                             QString("Statistiques generees avec succes!\n\n"
                                     "Total vehicules: %1\n"
                                     "En Marche: %2 (%3%)\n"
                                     "En Panne: %4 (%5%)\n"
                                     "En Maintenance: %6 (%7%)")
                             .arg(total)
                             .arg(enMarche).arg(pctMarche, 0, 'f', 1)
                             .arg(enPanne).arg(pctPanne, 0, 'f', 1)
                             .arg(enMaintenance).arg(pctMaintenance, 0, 'f', 1));
}

// Export vehicles table to PDF
void MainWindow::on_boutonExporterPdf_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Enregistrer sous", "", "Fichiers PDF (*.pdf)");
    if (fileName.isEmpty())
        return;
    if (!fileName.endsWith(".pdf"))
        fileName += ".pdf";

    QTextDocument doc;
    QTextCursor cursor(&doc);

    // Title and export date
    cursor.insertHtml("<h1 style='text-align:center;'>Liste des Véhicules</h1>");
    cursor.insertHtml("<p style='text-align:right; font-size:10pt;'>Date d'export : " +
                      QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss") + "</p>");
    cursor.insertBlock();

    int rows = ui->listeVoiture->rowCount();
    int cols = ui->listeVoiture->columnCount();
    if (rows == 0 || cols == 0) {
        QMessageBox::warning(this, "Erreur", "Aucune donnée à exporter !");
        return;
    }

    // Table format
    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(5);
    tableFormat.setCellSpacing(0);
    tableFormat.setHeaderRowCount(1);
    tableFormat.setAlignment(Qt::AlignCenter);

    // Column widths
    QVector<QTextLength> colWidths;
    for (int i = 0; i < cols; ++i)
        colWidths.append(QTextLength(QTextLength::PercentageLength, 100.0 / cols));
    tableFormat.setColumnWidthConstraints(colWidths);

    QTextTable *table = cursor.insertTable(rows + 1, cols, tableFormat);

    // Insert headers
    for (int col = 0; col < cols; ++col) {
        QTextTableCell cell = table->cellAt(0, col);
        QTextCursor cellCursor = cell.firstCursorPosition();
        QString header = ui->listeVoiture->horizontalHeaderItem(col)->text();
        QTextCharFormat format;
        format.setFontWeight(QFont::Bold);
        format.setBackground(QColor("#D3D3D3"));
        cellCursor.insertText(header, format);
    }

    // Insert data
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            QTextTableCell cell = table->cellAt(row + 1, col);
            QTextCursor cellCursor = cell.firstCursorPosition();
            QString text = ui->listeVoiture->item(row, col) ? ui->listeVoiture->item(row, col)->text() : "";

            QTextCharFormat format;

            // Alternate row colors
            if (row % 2 == 0)
                format.setBackground(QColor("#F0F0F0"));
            else
                format.setBackground(Qt::white);

            // Color code for État column
            if (col == 6) { // État column
                if (text.toLower() == "en marche")
                    format.setForeground(Qt::darkGreen);
                else if (text.toLower() == "en panne")
                    format.setForeground(Qt::red);
                else if (text.toLower() == "en maintenance")
                    format.setForeground(QColor("#FFA500")); // Orange
            }

            cellCursor.insertText(text, format);
        }
    }

    // Print to PDF
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageOrientation(QPageLayout::Landscape); // Landscape for wide table

    doc.print(&printer);

    QMessageBox::information(this, "Succès", "Exportation PDF réussie !");
}

// Handle intelligent recommendation button click
void MainWindow::on_Analyse_clicked()
{
    // Get all vehicles
    QList<Vehicule> vehicules = Vehicule::afficher();
    
    if (vehicules.isEmpty()) {
        QMessageBox::information(this, "Aucun véhicule", "Aucun véhicule disponible pour analyse.");
        return;
    }
    
    // Score and find best vehicle
    struct VehicleScore {
        Vehicule vehicle;
        double score;
        QString reason;
    };
    
    QList<VehicleScore> scored;
    
    for (const Vehicule &v : vehicules) {
        QString etat = v.getEtat().toLower().trimmed();
        
        // Skip vehicles not operational
        if (etat == "en panne" || etat == "en maintenance") {
            continue;
        }
        
        VehicleScore vs;
        vs.vehicle = v;
        vs.score = 0;
        QStringList reasons;
        
        // Calculate score based on usage and health
        int km = v.getKilometrage();
        int limite = v.getKilometrageLimite();
        
        if (limite > 0) {
            double usagePercent = (km * 100.0) / limite;
            
            // Health score (40% weight): lower usage = better
            double healthScore = (100 - usagePercent) * 0.4;
            vs.score += healthScore;
            
            // Maintenance proximity (30% weight): far from limit = better
            double maintenanceScore = qMax(0.0, (100 - usagePercent) * 0.3);
            vs.score += maintenanceScore;
            
            // Usage balance (30% weight): prefer under-utilized
            if (usagePercent < 30) {
                vs.score += 30;
                reasons << "Faible utilisation (" + QString::number(usagePercent, 'f', 0) + "%)";
            } else if (usagePercent < 50) {
                vs.score += 20;
                reasons << "Utilisation modérée (" + QString::number(usagePercent, 'f', 0) + "%)";
            } else {
                vs.score += 10;
            }
            
            // Distance from maintenance limit
            int kmRemaining = limite - km;
            if (kmRemaining > 5000) {
                reasons << "Loin de la limite (" + QString::number(kmRemaining) + " km restants)";
            }
        } else {
            // No limit set - give medium score
            vs.score = 50;
            reasons << "Pas de limite définie";
        }
        
        // Bonus for "En Marche" status
        if (etat == "en marche") {
            vs.score += 10;
            reasons << "État: En Marche";
        }
        
        vs.reason = reasons.join(", ");
        scored.append(vs);
    }
    
    if (scored.isEmpty()) {
        QMessageBox::warning(this, "Aucun véhicule disponible", 
                            "Aucun véhicule opérationnel disponible pour le moment.\n\n"
                            "Tous les véhicules sont en panne ou en maintenance.");
        return;
    }
    
    // Sort by score (highest first)
    std::sort(scored.begin(), scored.end(), [](const VehicleScore &a, const VehicleScore &b) {
        return a.score > b.score;
    });
    
    // Get the best vehicle
    VehicleScore best = scored.first();
    
    // Create simple, clean recommendation dialog
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("🤖 Suggestion Automatique");
    dialog->setModal(true);
    dialog->resize(480, 280);
    dialog->setStyleSheet("QDialog { background-color: white; } "
                          "QLabel { color: black; }");
    
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Title
    QLabel *titleLabel = new QLabel("🤖 RECOMMANDATION INTELLIGENTE");
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold; color: #2C3E50;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    // Recommended vehicle
    QLabel *vehicleLabel = new QLabel(QString("Véhicule recommandé: <b style='color:#27AE60; font-size:13pt;'>%1</b>")
                                     .arg(best.vehicle.getMatricule()));
    vehicleLabel->setStyleSheet("font-size: 11pt; padding: 10px; background-color: #E8F8F5; border-radius: 5px;");
    vehicleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(vehicleLabel);
    
    // Score
    QLabel *scoreLabel = new QLabel(QString("Score: %1/100").arg(QString::number(best.score, 'f', 0)));
    scoreLabel->setStyleSheet("font-size: 10pt; color: #7F8C8D;");
    scoreLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(scoreLabel);
    
    layout->addSpacing(10);
    
    // Reasons section
    QLabel *reasonsTitle = new QLabel("Raisons:");
    reasonsTitle->setStyleSheet("font-size: 11pt; font-weight: bold; color: #2C3E50;");
    layout->addWidget(reasonsTitle);
    
    // Build reasons list
    QStringList reasonsList;
    int km = best.vehicle.getKilometrage();
    int limite = best.vehicle.getKilometrageLimite();
    
    if (limite > 0) {
        double usagePercent = (km * 100.0) / limite;
        reasonsList << QString("• Utilisation: %1%").arg(QString::number(usagePercent, 'f', 0));
        reasonsList << QString("• Kilométrage: %1 / %2 km").arg(km).arg(limite);
    } else {
        reasonsList << QString("• Kilométrage: %1 km").arg(km);
    }
    
    QString etat = best.vehicle.getEtat();
    reasonsList << QString("• État: %1").arg(etat);
    reasonsList << QString("• Marque: %1 %2").arg(best.vehicle.getMarque()).arg(best.vehicle.getModele());
    
    QString reasonsText = reasonsList.join("\n");
    QLabel *reasonsLabel = new QLabel(reasonsText);
    reasonsLabel->setStyleSheet("font-size: 10pt; padding: 10px; color: #34495E; line-height: 1.5;");
    reasonsLabel->setWordWrap(true);
    layout->addWidget(reasonsLabel);
    
    layout->addSpacing(10);
    
    // Conclusion
    QLabel *conclusionLabel = new QLabel("Ce véhicule est optimal pour le prochain cours.");
    conclusionLabel->setStyleSheet("font-size: 10pt; font-style: italic; color: #7F8C8D; padding: 10px; "
                                  "background-color: #F8F9FA; border-radius: 5px;");
    conclusionLabel->setAlignment(Qt::AlignCenter);
    conclusionLabel->setWordWrap(true);
    layout->addWidget(conclusionLabel);
    
    layout->addStretch();
    
    // OK button
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *okBtn = new QPushButton("OK");
    okBtn->setStyleSheet("QPushButton { background-color: #3498DB; color: white; padding: 8px 30px; "
                        "border-radius: 5px; font-weight: bold; } "
                        "QPushButton:hover { background-color: #2980B9; }");
    okBtn->setMinimumWidth(100);
    connect(okBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    btnLayout->addWidget(okBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);
    
    dialog->exec();
}

// Old analytics function - keep for reference but not used
void MainWindow::afficherAnalyseVehicules()
{
    QList<Vehicule> vehicules = Vehicule::afficher();
    
    if (vehicules.isEmpty()) {
        QMessageBox::information(this, "Analyse", "Aucun véhicule disponible pour l'analyse.");
        return;
    }
    
    // === CALCULATIONS ===
    int dangerZone = 0;    // >80% of limit
    int mediumZone = 0;    // 50-80% of limit
    int safeZone = 0;      // <50% of limit
    int noLimit = 0;       // No limit set
    
    long long totalKm = 0;
    int vehiculesAvecLimite = 0;
    double totalUsageRate = 0;
    
    struct VehiculeUsage {
        QString matricule;
        QString marque;
        int km;
        int limite;
        double percentage;
        QString riskLevel;
    };
    
    QList<VehiculeUsage> usageList;
    
    // Analyze each vehicle
    for (const Vehicule &v : vehicules) {
        int km = v.getKilometrage();
        int limite = v.getKilometrageLimite();
        totalKm += km;
        
        VehiculeUsage usage;
        usage.matricule = v.getMatricule();
        usage.marque = v.getMarque();
        usage.km = km;
        usage.limite = limite;
        
        if (limite <= 0) {
            noLimit++;
            usage.percentage = 0;
            usage.riskLevel = "Non défini";
        } else {
            vehiculesAvecLimite++;
            double percentage = (km * 100.0) / limite;
            usage.percentage = percentage;
            totalUsageRate += percentage;
            
            if (percentage > 80) {
                dangerZone++;
                usage.riskLevel = "CRITIQUE";
            } else if (percentage >= 50) {
                mediumZone++;
                usage.riskLevel = "MOYEN";
            } else {
                safeZone++;
                usage.riskLevel = "BON";
            }
        }
        
        usageList.append(usage);
    }
    
    // Sort by usage percentage (descending)
    std::sort(usageList.begin(), usageList.end(), [](const VehiculeUsage &a, const VehiculeUsage &b) {
        return a.percentage > b.percentage;
    });
    
    int totalVehicules = vehicules.size();
    double avgKmPerVehicle = totalVehicules > 0 ? (totalKm / (double)totalVehicules) : 0;
    double avgUsageRate = vehiculesAvecLimite > 0 ? (totalUsageRate / vehiculesAvecLimite) : 0;
    
    // === CREATE VISUAL DASHBOARD WITH MULTIPLE CHARTS ===
    
    // Remove old charts and scroll areas
    QWidget *container = ui->lineEdit_56->parentWidget();
    QList<QWidget*> oldWidgets = container->findChildren<QWidget*>("vehiculeAnalyseChart");
    for (QWidget *oldWidget : oldWidgets) {
        oldWidget->deleteLater();
    }
    QList<QScrollArea*> oldScrolls = container->findChildren<QScrollArea*>("vehiculeAnalyseScroll");
    for (QScrollArea *oldScroll : oldScrolls) {
        oldScroll->deleteLater();
    }
    ui->lineEdit_56->hide();
    
    // Create scroll area
    QScrollArea *scrollArea = new QScrollArea(container);
    scrollArea->setObjectName("vehiculeAnalyseScroll");
    scrollArea->setGeometry(ui->lineEdit_56->geometry());
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background-color: white; border: 1px solid #ccc; }");
    
    // Create main widget for all charts
    QWidget *dashboardWidget = new QWidget();
    dashboardWidget->setObjectName("vehiculeAnalyseChart");
    dashboardWidget->setStyleSheet("QWidget { background-color: white; }");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(dashboardWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // === CHART 1: PIE CHART - Risk Distribution ===
    QPieSeries *pieRisk = new QPieSeries();
    
    if (dangerZone > 0) {
        QPieSlice *slice = pieRisk->append(QString("Critique: %1").arg(dangerZone), dangerZone);
        slice->setBrush(QColor("#E53935"));
        slice->setLabelVisible(true);
        slice->setLabelColor(Qt::white);
        slice->setLabelPosition(QPieSlice::LabelInsideHorizontal);
    }
    if (mediumZone > 0) {
        QPieSlice *slice = pieRisk->append(QString("Moyen: %1").arg(mediumZone), mediumZone);
        slice->setBrush(QColor("#FB8C00"));
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelInsideHorizontal);
    }
    if (safeZone > 0) {
        QPieSlice *slice = pieRisk->append(QString("Bon: %1").arg(safeZone), safeZone);
        slice->setBrush(QColor("#43A047"));
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelInsideHorizontal);
    }
    if (noLimit > 0) {
        QPieSlice *slice = pieRisk->append(QString("Sans limite: %1").arg(noLimit), noLimit);
        slice->setBrush(QColor("#9E9E9E"));
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelInsideHorizontal);
    }
    
    QChart *chartRisk = new QChart();
    chartRisk->addSeries(pieRisk);
    chartRisk->setTitle("RÉPARTITION PAR NIVEAU DE RISQUE");
    chartRisk->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
    chartRisk->legend()->setAlignment(Qt::AlignRight);
    chartRisk->setAnimationOptions(QChart::SeriesAnimations);
    
    QChartView *chartViewRisk = new QChartView(chartRisk);
    chartViewRisk->setRenderHint(QPainter::Antialiasing);
    chartViewRisk->setMinimumHeight(250);
    chartViewRisk->setMaximumHeight(300);
    
    // === CHART 2: BAR CHART - Top 5 Most Used Vehicles ===
    QBarSet *barSet = new QBarSet("Utilisation (%)");
    QStringList categories;
    
    for (int i = 0; i < qMin(5, usageList.size()); i++) {
        const VehiculeUsage &v = usageList[i];
        if (v.limite > 0) {
            *barSet << v.percentage;
            categories << v.matricule;
            
            // Color based on risk
            if (v.percentage > 80) {
                barSet->setColor(QColor("#E53935"));
            } else if (v.percentage >= 50) {
                barSet->setColor(QColor("#FB8C00"));
            } else {
                barSet->setColor(QColor("#43A047"));
            }
        }
    }
    
    QBarSeries *barSeries = new QBarSeries();
    barSeries->append(barSet);
    
    QChart *chartTop5 = new QChart();
    chartTop5->addSeries(barSeries);
    chartTop5->setTitle("TOP 5 VÉHICULES PAR UTILISATION");
    chartTop5->setTitleFont(QFont("Segoe UI", 10, QFont::Bold));
    chartTop5->setAnimationOptions(QChart::SeriesAnimations);
    
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chartTop5->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, 100);
    axisY->setTitleText("Utilisation (%)");
    chartTop5->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    chartTop5->legend()->setVisible(false);
    
    QChartView *chartViewTop5 = new QChartView(chartTop5);
    chartViewTop5->setRenderHint(QPainter::Antialiasing);
    chartViewTop5->setMinimumHeight(250);
    chartViewTop5->setMaximumHeight(300);
    
    // === TEXT SUMMARY: Overview & Recommendations ===
    QString summary = QString("<div style='font-family:Segoe UI; font-size:9pt; padding:10px; color:black;'>"
                             "<b style='font-size:11pt;'>VUE D'ENSEMBLE</b><br>"
                             "Total: <b>%1</b> vehicules | "
                             "Total km: <b>%2</b> | "
                             "Moy/vehicule: <b>%3</b> km | "
                             "Taux moyen: <b>%4%</b><br><br>"
                             "<b style='font-size:11pt;'>RECOMMANDATIONS</b><br>")
                     .arg(totalVehicules)
                     .arg(QString::number(totalKm))
                     .arg(QString::number(avgKmPerVehicle, 'f', 0))
                     .arg(QString::number(avgUsageRate, 'f', 1));
    
    if (dangerZone > 0) {
        summary += QString("<b>ALERTE:</b> %1 vehicule(s) CRITIQUE - Maintenance immediate!<br>").arg(dangerZone);
    }
    
    if (!usageList.isEmpty() && usageList.first().limite > 0) {
        const VehiculeUsage &mostUsed = usageList.first();
        summary += QString("Plus utilise: <b>%1</b> (%2%%)").arg(mostUsed.matricule).arg(QString::number(mostUsed.percentage, 'f', 1));
        if (mostUsed.percentage > 80) {
            summary += " - Reduire affectations";
        }
        summary += "<br>";
    }
    
    VehiculeUsage leastUsed;
    bool foundLeastUsed = false;
    for (int i = usageList.size() - 1; i >= 0; i--) {
        if (usageList[i].limite > 0) {
            leastUsed = usageList[i];
            foundLeastUsed = true;
            break;
        }
    }
    
    if (foundLeastUsed) {
        summary += QString("Moins utilise: <b>%1</b> (%2%%) - Augmenter affectations<br>")
                  .arg(leastUsed.matricule).arg(QString::number(leastUsed.percentage, 'f', 1));
    }
    
    summary += "<br><b>Sante Flotte:</b> ";
    if (dangerZone == 0 && mediumZone < totalVehicules * 0.3) {
        summary += "Excellente";
    } else if (dangerZone < totalVehicules * 0.2) {
        summary += "Attention requise";
    } else {
        summary += "Critique!";
    }
    summary += "</div>";
    
    QLabel *summaryLabel = new QLabel(summary);
    summaryLabel->setWordWrap(true);
    summaryLabel->setStyleSheet("QLabel { background-color: #f5f5f5; border: 1px solid #ccc; border-radius: 5px; padding: 10px; }");
    summaryLabel->setMinimumHeight(150);
    
    // Add all to layout
    mainLayout->addWidget(chartViewRisk);
    mainLayout->addWidget(chartViewTop5);
    mainLayout->addWidget(summaryLabel);
    mainLayout->addStretch();
    
    // Set the dashboard widget inside scroll area
    scrollArea->setWidget(dashboardWidget);
    scrollArea->show();
    
    // Show info message
    QMessageBox::information(this, "Analyse de la Flotte",
                             QString("Dashboard genere avec succes!\n\n"
                                     "Critique: %1 | Moyen: %2 | Bon: %3\n"
                                     "Top vehicule: %4 (%5%%)")
                             .arg(dangerZone).arg(mediumZone).arg(safeZone)
                             .arg(usageList.isEmpty() ? "N/A" : usageList.first().matricule)
                             .arg(usageList.isEmpty() ? 0 : usageList.first().percentage, 0, 'f', 1));
}

// Check if kilometrage exceeds limite and trigger alert
void MainWindow::verifierKilometrageLimite(int kilometrage, int kilometrageLimite, const QString &matricule)
{
    if (kilometrageLimite > 0 && kilometrage > kilometrageLimite) {
        // Text alert
        QString alertMessage = QString("⚠️ ALERTE KILOMÉTRAGE ⚠️\n\n"
                                      "Le véhicule %1 a dépassé sa limite!\n\n"
                                      "Kilométrage actuel: %2 km\n"
                                      "Limite autorisée: %3 km\n"
                                      "Dépassement: %4 km\n\n"
                                      "Le véhicule sera automatiquement mis EN MAINTENANCE!")
                              .arg(matricule)
                              .arg(kilometrage)
                              .arg(kilometrageLimite)
                              .arg(kilometrage - kilometrageLimite);
        
        QMessageBox::warning(this, "Alerte Kilométrage Dépassé", alertMessage);
        
        // Speech alert
        QString speechText = QString("Attention! Le véhicule %1 a dépassé sa limite de kilométrage. "
                                    "Kilométrage actuel: %2 kilomètres. Limite: %3 kilomètres. "
                                    "Le véhicule sera mis en maintenance automatiquement.")
                            .arg(matricule)
                            .arg(kilometrage)
                            .arg(kilometrageLimite);
        
        //speech->say(speechText);
    }
}


// Show a persistent maintenance alert dialog on startup/login
void MainWindow::showMaintenanceAlerts()
{
    QList<Vehicule> vehicules = Vehicule::afficher();
    QStringList alerts;

    for (const Vehicule &v : vehicules) {
        QString etat = v.getEtat().toLower().trimmed();
        bool exceeded = (v.getKilometrageLimite() > 0 && v.getKilometrage() > v.getKilometrageLimite());
        if (etat == "en maintenance" || exceeded) {
            QString reason;
            if (etat == "en maintenance")
                reason = "(en maintenance)";
            if (exceeded)
                reason += QString(" dépassement: %1 / %2 km").arg(v.getKilometrage()).arg(v.getKilometrageLimite());

            alerts << QString("%1 %2").arg(v.getMatricule()).arg(reason);
        }
    }

    if (alerts.isEmpty())
        return; // nothing to alert

    // Build the textual alert with warning icon
    QString message = QString("⚠️ ALERTE MAINTENANCE ⚠️\n\n"
                             "Il y a %1 véhicule(s) en maintenance ou dépassant la limite:\n\n").arg(alerts.size());
    for (const QString &line : alerts) {
        message += "⚠️ " + line + "\n";
    }

    // Show a dialog that appears automatically on startup
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("⚠️ Alertes Maintenance");
    dialog->setModal(false);
    dialog->resize(520, 320);
    dialog->setStyleSheet("QDialog { background-color: white; } "
                          "QLabel { color: black; background-color: white; } "
                          "QPushButton { background-color: #f0f0f0; color: black; }");

    QVBoxLayout *lay = new QVBoxLayout(dialog);
    QLabel *lbl = new QLabel(message, dialog);
    lbl->setWordWrap(true);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lbl->setStyleSheet("QLabel { color: black; font-size: 10pt; padding: 10px; font-weight: bold; }");
    lay->addWidget(lbl);

    QHBoxLayout *btnLay = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton("OK", dialog);
    QPushButton *btnClose = new QPushButton("Fermer", dialog);
    btnLay->addStretch();
    btnLay->addWidget(btnOk);
    btnLay->addWidget(btnClose);
    lay->addLayout(btnLay);

    connect(btnOk, &QPushButton::clicked, dialog, &QDialog::accept);
    connect(btnClose, &QPushButton::clicked, dialog, &QDialog::close);

    // Speak a concise summary for the user
    QString speechText = QString("Attention. %1 véhicule(s) en maintenance ou dépassant la limite. ").arg(alerts.size());
    QStringList matricules;
    for (const QString &a : alerts) {
        // take first token (matricule)
        QString m = a.section(' ', 0, 0);
        matricules << m;
    }
    if (!matricules.isEmpty()) {
        speechText += QString("Matricules: %1.").arg(matricules.join(", "));
    }

   // if (speech) speech->say(speechText);

    // Show dialog non-modally so it doesn't block login
    dialog->show();
}

void MainWindow::on_btnQR_clicked()
{
    // Vérifie qu'une ligne est sélectionnée
    int row = ui->tableWidgetExamen->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un examen.");
        return;
    }

    // Récupère l'ID de l'examen depuis la première colonne
    int idExamen = ui->tableWidgetExamen->item(row, 0)->text().toInt();

    Examen ex;
    QString texte = ex.genererTexteQRCode(idExamen);

    // Génère le QR Code en QPixmap
    QPixmap qr = ex.genererQRCodeImage(texte);

    // Choix du chemin pour sauvegarder le QR Code
    QString chemin = QFileDialog::getSaveFileName(
        this,
        "Enregistrer le QR Code",
        QDir::homePath() + QString("/qr_examen_%1.png").arg(idExamen),
        "Images PNG (*.png)"
        );

    if (!chemin.isEmpty()) {
        if (qr.save(chemin, "PNG")) {
            QMessageBox::information(this, "Succès", "QR Code sauvegardé !");
        } else {
            QMessageBox::critical(this, "Erreur", "Impossible de sauvegarder l'image.");
        }
    }
}
void MainWindow::filterCoursTable()
{
    QString searchText = ui->lineEdit_59->text().trimmed();
    QString selected = ui->comboBox_12->currentText();

    int col = -1;

    if (selected == "Circuit")
        col = 5;
    else if (selected == "Tarif")
        col = 7;

    if (col < 0)
        return;

    for (int row = 0; row < ui->tableWidget_9->rowCount(); ++row)
    {
        QTableWidgetItem *item = ui->tableWidget_9->item(row, col);

        bool match = (item && item->text().contains(searchText, Qt::CaseInsensitive));

        ui->tableWidget_9->setRowHidden(row, !match);
    }
}
QTime morningStart(8,0), morningEnd(12,0);
QTime afternoonStart(14,0), afternoonEnd(18,0);
/*void MainWindow::on_btnShowPopup_clicked()
{
    QString date = ui->dateEdit_9->date().toString("yyyy-MM-dd"); // selected date

    // Step 1: Get all employees from EMPLOYES table
    QSqlQuery empQuery;
    empQuery.prepare("SELECT ID_EMPLOYE, NOM, PRENOM FROM EMPLOYES");
    if (!empQuery.exec()) {
        QMessageBox::critical(this, "Erreur SQL", "Impossible de récupérer les employés : " + empQuery.lastError().text());
        return;
    }

    QString resultText;

    while (empQuery.next()) {
        int idEmp = empQuery.value("ID_EMPLOYE").toInt();
        QString nom = empQuery.value("NOM").toString();
        QString prenom = empQuery.value("PRENOM").toString();

        // Step 2: Get courses for this employee on the selected date from COURS table
        QSqlQuery courseQuery;
        courseQuery.prepare(R"(
            SELECT HEURE_DEBUT, HEURE_FIN
            FROM COURS
            WHERE ID_EMPLOYE = :idEmp
              AND TO_CHAR(DATE_COURS, 'YYYY-MM-DD') = :date
            ORDER BY HEURE_DEBUT
        )");
        courseQuery.bindValue(":idEmp", idEmp);
        courseQuery.bindValue(":date", date);
        if (!courseQuery.exec()) {
            QMessageBox::warning(this, "Erreur SQL", "Impossible de récupérer les cours pour l'employé " + nom + " : " + courseQuery.lastError().text());
            continue;
        }

        // Collect existing intervals
        QList<QPair<QTime,QTime>> occupied;
        while (courseQuery.next()) {
            QTime start = QTime::fromString(courseQuery.value("HEURE_DEBUT").toString(), "HH:mm");
            QTime end = QTime::fromString(courseQuery.value("HEURE_FIN").toString(), "HH:mm");
            if (start.isValid() && end.isValid()) {
                occupied.append(qMakePair(start, end));
            }
        }

        // Step 3: Calculate free slots in morning and afternoon
        QList<QPair<QTime,QTime>> freeSlots;

        auto addFreeSlot = [&](QTime intervalStart, QTime intervalEnd){
            QTime lastEnd = intervalStart;
            for (auto &o : occupied) {
                if (o.second <= intervalStart || o.first >= intervalEnd) continue; // outside interval
                if (lastEnd < o.first) freeSlots.append(qMakePair(lastEnd, o.first));
                if (o.second > lastEnd) lastEnd = o.second;
            }
            if (lastEnd < intervalEnd) freeSlots.append(qMakePair(lastEnd, intervalEnd));
        };

        addFreeSlot(QTime(8,0), QTime(12,0));   // morning
        addFreeSlot(QTime(14,0), QTime(18,0));  // afternoon

        // Step 4: Display results for this employee
        resultText += QString("Employé: %1 %2 (ID: %3)\n").arg(nom, prenom).arg(idEmp);
        if (freeSlots.isEmpty()) {
            resultText += "  Pas de créneaux libres.\n\n";
        } else {
            for (auto &slot : freeSlots) {
                resultText += QString("  %1 - %2\n").arg(slot.first.toString("HH:mm")).arg(slot.second.toString("HH:mm"));
            }
            resultText += "\n";
        }
    }

    // Step 5: Show popup
    if (resultText.isEmpty())
        resultText = "Aucun employé trouvé ou aucun créneau disponible.";
    QMessageBox::information(this, "Créneaux disponibles", resultText);
}*/
/*void MainWindow::on_btnShowPopup_clicked()
{
    // Step 1: Get the selected date from the calendar
    QDate selectedDate = ui->calendarWidget->selectedDate();
    QString dateStr = selectedDate.toString("yyyy-MM-dd"); // Format for Oracle

    // Step 2: Get all employees from EMPLOYES table
    QSqlQuery empQuery;
    empQuery.prepare("SELECT ID_EMPLOYE, NOM, PRENOM FROM EMPLOYES");
    if (!empQuery.exec()) {
        QMessageBox::critical(this, "Erreur SQL", "Impossible de récupérer les employés : " + empQuery.lastError().text());
        return;
    }

    QString resultText;

    while (empQuery.next()) {
        int idEmp = empQuery.value("ID_EMPLOYE").toInt();
        QString nom = empQuery.value("NOM").toString();
        QString prenom = empQuery.value("PRENOM").toString();

        // Step 3: Get courses for this employee on the selected date
        QSqlQuery courseQuery;
        courseQuery.prepare(R"(
            SELECT HEURE_DEBUT, HEURE_FIN
            FROM COURS
            WHERE ID_EMPLOYE = :idEmp
              AND TO_CHAR(DATE_COURS, 'YYYY-MM-DD') = :date
            ORDER BY HEURE_DEBUT
        )");
        courseQuery.bindValue(":idEmp", idEmp);
        courseQuery.bindValue(":date", dateStr);

        if (!courseQuery.exec()) {
            QMessageBox::warning(this, "Erreur SQL", "Impossible de récupérer les cours pour " + nom + " : " + courseQuery.lastError().text());
            continue;
        }

        // Step 4: Collect existing intervals
        QList<QPair<QTime,QTime>> occupied;
        while (courseQuery.next()) {
            QTime start = QTime::fromString(courseQuery.value("HEURE_DEBUT").toString(), "HH:mm");
            QTime end = QTime::fromString(courseQuery.value("HEURE_FIN").toString(), "HH:mm");
            if (start.isValid() && end.isValid()) {
                occupied.append(qMakePair(start, end));
            }
        }

        // Step 5: Calculate free slots
        QList<QPair<QTime,QTime>> freeSlots;
        auto addFreeSlot = [&](QTime intervalStart, QTime intervalEnd){
            QTime lastEnd = intervalStart;
            for (auto &o : occupied) {
                if (o.second <= intervalStart || o.first >= intervalEnd) continue;
                if (lastEnd < o.first) freeSlots.append(qMakePair(lastEnd, o.first));
                if (o.second > lastEnd) lastEnd = o.second;
            }
            if (lastEnd < intervalEnd) freeSlots.append(qMakePair(lastEnd, intervalEnd));
        };

        addFreeSlot(QTime(8,0), QTime(12,0));   // morning
        addFreeSlot(QTime(14,0), QTime(18,0));  // afternoon

        // Step 6: Format result
        resultText += QString("Employé: %1 %2 (ID: %3)\n").arg(nom, prenom).arg(idEmp);
        if (freeSlots.isEmpty()) {
            resultText += "  Pas de créneaux libres.\n\n";
        } else {
            for (auto &slot : freeSlots) {
                resultText += QString("  %1 - %2\n").arg(slot.first.toString("HH:mm")).arg(slot.second.toString("HH:mm"));
            }
            resultText += "\n";
        }
    }

    // Step 7: Show popup
    if (resultText.isEmpty())
        resultText = "Aucun employé trouvé ou aucun créneau disponible.";
    QMessageBox::information(this, QString("Créneaux disponibles pour %1").arg(dateStr), resultText);
}*/
/*void MainWindow::on_btnShowPopup_clicked()
{
    // Step 1: Create a popup dialog with a calendar
    QDialog calendarDialog(this);
    calendarDialog.setWindowTitle("Sélectionnez la date");

    QVBoxLayout *layout = new QVBoxLayout(&calendarDialog);
    QCalendarWidget *calendar = new QCalendarWidget(&calendarDialog);
    calendar->setGridVisible(true);
    layout->addWidget(calendar);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &calendarDialog);
    layout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, &calendarDialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &calendarDialog, &QDialog::reject);

    if (calendarDialog.exec() != QDialog::Accepted) {
        // User cancelled
        return;
    }

    // Step 2: Get the selected date
    QDate selectedDate = calendar->selectedDate();
    QString dateStr = selectedDate.toString("yyyy-MM-dd"); // Oracle-friendly format

    // Step 3: Query employees
    QSqlQuery empQuery;
    empQuery.prepare("SELECT ID_EMPLOYE, NOM, PRENOM FROM EMPLOYES");
    if (!empQuery.exec()) {
        QMessageBox::critical(this, "Erreur SQL", "Impossible de récupérer les employés : " + empQuery.lastError().text());
        return;
    }

    QString resultText;

    while (empQuery.next()) {
        int idEmp = empQuery.value("ID_EMPLOYE").toInt();
        QString nom = empQuery.value("NOM").toString();
        QString prenom = empQuery.value("PRENOM").toString();

        // Step 4: Get courses for this employee on selected date
        QSqlQuery courseQuery;
        courseQuery.prepare(R"(
            SELECT HEURE_DEBUT, HEURE_FIN
            FROM COURS
            WHERE ID_EMPLOYE = :idEmp
              AND TO_CHAR(DATE_COURS, 'YYYY-MM-DD') = :date
            ORDER BY HEURE_DEBUT
        )");
        courseQuery.bindValue(":idEmp", idEmp);
        courseQuery.bindValue(":date", dateStr);

        if (!courseQuery.exec()) continue;

        QList<QPair<QTime,QTime>> occupied;
        while (courseQuery.next()) {
            QTime start = QTime::fromString(courseQuery.value("HEURE_DEBUT").toString(), "HH:mm");
            QTime end = QTime::fromString(courseQuery.value("HEURE_FIN").toString(), "HH:mm");
            if (start.isValid() && end.isValid())
                occupied.append(qMakePair(start, end));
        }

        // Step 5: Calculate free slots
        QList<QPair<QTime,QTime>> freeSlots;
        auto addFreeSlot = [&](QTime intervalStart, QTime intervalEnd){
            QTime lastEnd = intervalStart;
            for (auto &o : occupied) {
                if (o.second <= intervalStart || o.first >= intervalEnd) continue;
                if (lastEnd < o.first) freeSlots.append(qMakePair(lastEnd, o.first));
                if (o.second > lastEnd) lastEnd = o.second;
            }
            if (lastEnd < intervalEnd) freeSlots.append(qMakePair(lastEnd, intervalEnd));
        };

        addFreeSlot(QTime(8,0), QTime(12,0));   // morning
        addFreeSlot(QTime(14,0), QTime(18,0));  // afternoon

        // Step 6: Format results
        resultText += QString("Employé: %1 %2 (ID: %3)\n").arg(nom, prenom).arg(idEmp);
        if (freeSlots.isEmpty()) {
            resultText += "  Pas de créneaux libres.\n\n";
        } else {
            for (auto &slot : freeSlots) {
                resultText += QString("  %1 - %2\n").arg(slot.first.toString("HH:mm")).arg(slot.second.toString("HH:mm"));
            }
            resultText += "\n";
        }
    }

    // Step 7: Show final popup with available times
    if (resultText.isEmpty())
        resultText = "Aucun employé trouvé ou aucun créneau disponible.";
    QMessageBox::information(this, QString("Créneaux disponibles pour %1").arg(dateStr), resultText);
}*/
// ------------------------------------------------------------------
// SHOW AVAILABLE FREE TIMES FOR ALL INSTRUCTORS FOR A GIVEN DATE
// ------------------------------------------------------------------
void MainWindow::showFreeSlotsPopup(const QDate &selectedDate)
{
    QString dateStr = selectedDate.toString("yyyy-MM-dd");

    // Query all employees
    QSqlQuery empQuery;
    empQuery.prepare("SELECT ID_EMPLOYE, NOM, PRENOM FROM EMPLOYES");
    if (!empQuery.exec()) {
        QMessageBox::critical(this, "Erreur SQL",
                              "Impossible de récupérer les employés : " +
                                  empQuery.lastError().text());
        return;
    }

    QString resultText;

    while (empQuery.next()) {
        int idEmp = empQuery.value("ID_EMPLOYE").toInt();
        QString nom = empQuery.value("NOM").toString();
        QString prenom = empQuery.value("PRENOM").toString();

        // Query occupied time slots for the selected date
        QSqlQuery courseQuery;
        courseQuery.prepare(R"(
            SELECT HEURE_DEBUT, HEURE_FIN
            FROM COURS
            WHERE ID_EMPLOYE = :idEmp
              AND TO_CHAR(DATE_COURS, 'YYYY-MM-DD') = :date
            ORDER BY HEURE_DEBUT
        )");
        courseQuery.bindValue(":idEmp", idEmp);
        courseQuery.bindValue(":date", dateStr);
        courseQuery.exec();

        QList<QPair<QTime,QTime>> occupied;

        while (courseQuery.next()) {
            QTime start = QTime::fromString(courseQuery.value("HEURE_DEBUT").toString(), "HH:mm");
            QTime end   = QTime::fromString(courseQuery.value("HEURE_FIN").toString(), "HH:mm");
            if (start.isValid() && end.isValid())
                occupied.append(qMakePair(start, end));
        }

        // Compute free slots
        QList<QPair<QTime,QTime>> freeSlots;

        auto addFreeSlot = [&](QTime intervalStart, QTime intervalEnd){
            QTime lastEnd = intervalStart;
            for (auto &o : occupied)
            {
                if (o.second <= intervalStart || o.first >= intervalEnd)
                    continue;

                if (lastEnd < o.first)
                    freeSlots.append(qMakePair(lastEnd, o.first));

                if (o.second > lastEnd)
                    lastEnd = o.second;
            }
            if (lastEnd < intervalEnd)
                freeSlots.append(qMakePair(lastEnd, intervalEnd));
        };

        // Working hours
        addFreeSlot(QTime(8,0), QTime(12,0));
        addFreeSlot(QTime(14,0), QTime(18,0));

        // Build text
        resultText += QString("Employé: %1 %2 (ID: %3)\n")
                          .arg(nom, prenom)
                          .arg(idEmp);

        if (freeSlots.isEmpty()) {
            resultText += "   Pas de créneaux libres.\n\n";
        } else {
            for (auto &slot : freeSlots) {
                resultText += QString("   %1 - %2\n")
                .arg(slot.first.toString("HH:mm"))
                    .arg(slot.second.toString("HH:mm"));
            }
            resultText += "\n";
        }
    }

    if (resultText.isEmpty())
        resultText = "Aucun créneau disponible.";

    QMessageBox::information(
        this,
        QString("Créneaux disponibles pour le %1").arg(dateStr),
        resultText
        );
}
void MainWindow::on_btnShowCalendar_clicked()
{
    showCalendarWithCourses();
}

void MainWindow::showCalendarWithCourses()
{
    // 1️⃣ Create popup dialog
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Calendrier des cours");
    dialog->resize(800, 600);

    QHBoxLayout *mainLayout = new QHBoxLayout(dialog);

    // 2️⃣ Calendar
    QCalendarWidget *calendar = new QCalendarWidget(dialog);
    calendar->setGridVisible(true);
    mainLayout->addWidget(calendar);

    // 3️⃣ Table to display courses for selected date
    QTableWidget *table = new QTableWidget(dialog);
    table->setColumnCount(7);
    QStringList headers = {"Heure Début", "Heure Fin", "Type", "Circuit",
                           "ID Élève", "Nom Élève", "ID Employé", "Nom Employé"};
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(table);

    // 4️⃣ When a date is clicked
    connect(calendar, &QCalendarWidget::clicked, this, [=](const QDate &date){
        table->setRowCount(0); // clear previous data
        QString dateStr = date.toString("yyyy-MM-dd");

        QSqlQuery query;
        query.prepare(R"(
            SELECT c.HEURE_DEBUT, c.HEURE_FIN, c.TYPE_COURS, c.CIRCUIT,
                   e.ID_ELEVE, e.NOM AS NOM_ELEVE, emp.ID_EMPLOYE, emp.NOM AS NOM_EMPLOYE
            FROM COURS c
            JOIN ELEVES e ON c.ID_ELEVE = e.ID_ELEVE
            JOIN EMPLOYES emp ON c.ID_EMPLOYE = emp.ID_EMPLOYE
            WHERE TO_CHAR(c.DATE_COURS, 'YYYY-MM-DD') = :date
            ORDER BY c.HEURE_DEBUT
        )");
        query.bindValue(":date", dateStr);

        if(!query.exec()) {
            QMessageBox::critical(dialog, "Erreur SQL",
                                  "Impossible de récupérer les cours : " + query.lastError().text());
            return;
        }

        int row = 0;
        while(query.next()) {
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(query.value("HEURE_DEBUT").toString()));
            table->setItem(row, 1, new QTableWidgetItem(query.value("HEURE_FIN").toString()));
            table->setItem(row, 2, new QTableWidgetItem(query.value("TYPE_COURS").toString()));
            table->setItem(row, 3, new QTableWidgetItem(query.value("CIRCUIT").toString()));
            table->setItem(row, 4, new QTableWidgetItem(query.value("ID_ELEVE").toString()));
            table->setItem(row, 5, new QTableWidgetItem(query.value("NOM_ELEVE").toString()));
            table->setItem(row, 6, new QTableWidgetItem(query.value("ID_EMPLOYE").toString()));
            table->setItem(row, 7, new QTableWidgetItem(query.value("NOM_EMPLOYE").toString()));
            row++;
        }
    });

    dialog->exec();
}






