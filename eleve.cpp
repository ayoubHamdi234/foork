#include "eleve.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QSqlQuery>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QChart>
#include <QtCharts/QLegend>
#include <QtCharts/QValueAxis>
#include <QDate>
#include <QtCharts/QPieSeries>
#include <QPdfWriter>
#include <QPainter>
#include <QDateTime>
#include <QPageSize>
#include <QProcess>
#include <QString>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>






// Constru par défaut
eleve::eleve()
{
    id_eleve = 0;
    nom = "";
    prenom = "";
    telephone = "";
    adresse = "";
    email = "";
}

// Constructeur paramétré
eleve::eleve(int id, QString n, QString p, QDate d, QString t, QString a, QString e)
{
    id_eleve = id;
    nom = n;
    prenom = p;
    dateNaissance = d;
    telephone = t;
    adresse = a;
    email = e;
}

// Destru
eleve::~eleve() {}


bool eleve::supprimer(int id)
{
    QSqlQuery query;
    QString res = QString::number(id);

    query.prepare("DELETE FROM Eleves WHERE id_eleve = :id_eleve");
    query.bindValue(":id_eleve", res);

    if (!query.exec()) {
        qDebug() << "Erreur suppression élève :" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qDebug() << "Aucun élève avec cet ID n'a été trouvé.";
        return false;
    }

    qDebug() << "Suppression réussie pour l'élève ID =" << id;
    return true;
}


bool eleve::ajouter()
{
    QSqlQuery query;

    query.prepare("INSERT INTO Eleves (id_eleve, nom, prenom, dateNaissance, telephone, adresse, email) "
                  "VALUES (:id_eleve, :nom, :prenom, :dateNaissance, :telephone, :adresse, :email)");

    query.bindValue(":id_eleve", id_eleve);
    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":dateNaissance", dateNaissance);
    query.bindValue(":telephone", telephone);
    query.bindValue(":adresse", adresse);
    query.bindValue(":email", email);

    if (!query.exec()) {
        qDebug() << " Erreur lors de l'ajout de l'élève :" << query.lastError().text();
        return false;
    }

    qDebug() << " Élève ajouté avec succès ! ID =" << id_eleve;
    return true;
}




QSqlQueryModel *eleve::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();

    // 1️⃣ Récupérer la connexion existante
    QSqlDatabase db = QSqlDatabase::database(); // utilise la connexion créée par Connection

    // 2️⃣ Requête SQL avec majuscules (Oracle sensible à la casse)
    QString requete = "SELECT ID_ELEVE, NOM, PRENOM, DATENAISSANCE, TELEPHONE, ADRESSE, EMAIL FROM ELEVES";
    model->setQuery(requete, db);

    // 3️⃣ Vérifier les erreurs
    if (model->lastError().isValid()) {
        qDebug() << "Erreur d'affichage :" << model->lastError().text();
        qDebug() << "Requête :" << requete;
        return nullptr;
    }

    // 4️⃣ Définir les noms des colonnes pour QTableView
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID Élève"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Date de Naissance"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Email"));

    qDebug() << "Données des élèves récupérées avec succès !";
    return model;
}



bool eleve::modifier()
{
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT nom, prenom, dateNaissance, telephone, adresse, email FROM Eleves WHERE id_eleve = :id_eleve");
    checkQuery.bindValue(":id_eleve", id_eleve);

    if (!checkQuery.exec()) {
        qDebug() << " Erreur de vérification de l'ID :" << checkQuery.lastError().text();
        return false;
    }

    if (!checkQuery.next()) {
        qDebug() << " Aucun élève trouvé avec cet ID.";
        return false;
    }

    QString oldNom = checkQuery.value(0).toString();
    QString oldPrenom = checkQuery.value(1).toString();
    QDate oldDateNaissance = checkQuery.value(2).toDate();
    QString oldTelephone = checkQuery.value(3).toString();
    QString oldAdresse = checkQuery.value(4).toString();
    QString oldEmail = checkQuery.value(5).toString();

    QString newNom = nom.isEmpty() ? oldNom : nom;
    QString newPrenom = prenom.isEmpty() ? oldPrenom : prenom;
    QDate newDateNaissance = dateNaissance.isValid() ? dateNaissance : oldDateNaissance;
    QString newTelephone = telephone.isEmpty() ? oldTelephone : telephone;
    QString newAdresse = adresse.isEmpty() ? oldAdresse : adresse;
    QString newEmail = email.isEmpty() ? oldEmail : email;

    QSqlQuery query;
    query.prepare("UPDATE Eleves SET nom = :nom, prenom = :prenom, dateNaissance = :dateNaissance, "
                  "telephone = :telephone, adresse = :adresse, email = :email "
                  "WHERE id_eleve = :id_eleve");

    query.bindValue(":id_eleve", id_eleve);
    query.bindValue(":nom", newNom);
    query.bindValue(":prenom", newPrenom);
    query.bindValue(":dateNaissance", newDateNaissance);
    query.bindValue(":telephone", newTelephone);
    query.bindValue(":adresse", newAdresse);
    query.bindValue(":email", newEmail);

    if (!query.exec()) {
        qDebug() << " Erreur lors de la mise à jour :" << query.lastError().text();
        return false;
    }

    qDebug() << " Élève modifié avec succès (ID =" << id_eleve << ")";
    return true;
}






QSqlQueryModel* eleve::rechercher(int id, QString nom)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QString sql;
    if (id != 0)
    {
        sql = QString("SELECT * FROM ELEVES WHERE ID_ELEVE = %1").arg(id);
    }
    else if (!nom.isEmpty())
    {
        sql = QString("SELECT * FROM ELEVES WHERE NOM LIKE '%%1%'").arg(nom);
    }
    else
    {
        qDebug() << "Veuillez entrer un ID ou un nom !";
        return nullptr;
    }

    model->setQuery(sql);

    if (model->lastError().isValid())
    {
        qDebug() << "Erreur SQL:" << model->lastError().text();
        delete model;
        return nullptr;
    }

    // S’il n’y a aucun résultat
    if (model->rowCount() == 0)
    {
        delete model;
        return nullptr;
    }

    // ✅ Sinon on retourne le modèle à afficher dans la table
    return model;
}




QChartView* eleve::genererStatistiquesAgeVille()
{
    QMap<QString,int> trancheAge;
    trancheAge["18-28"] = 0;
    trancheAge["29-33+"] = 0;


    QSqlQuery query;
    if(query.exec("SELECT dateNaissance FROM ELEVES"))
    {
        while(query.next())
        {
            QDate naissance = query.value("dateNaissance").toDate();
            if(naissance.isValid())
            {
                int age = naissance.daysTo(QDate::currentDate()) / 365;

                if(age >= 18 && age <= 25) trancheAge["18-28"]++;
                else if(age >= 26 && age >= 33) trancheAge["29-33+"]++;

                // si l'élève est <18, il ne sera pas compté (optionnel)
            }
        }
    }
    else qDebug() << "Erreur SQL: " << query.lastError().text();

    // valeurs par défaut si vide
    bool vide = true;
    for(auto v: trancheAge) { if(v>0){vide=false; break;} }
    if(vide){ for(auto &k : trancheAge.keys()) trancheAge[k]=1; }

    QPieSeries *series = new QPieSeries();
    for(auto it = trancheAge.begin(); it != trancheAge.end(); ++it)
        series->append(it.key(), it.value());

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des élèves par tranche d'âge");
    chart->legend()->setAlignment(Qt::AlignRight);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // remplissage du widget
    return chartView;
}


bool eleve::exporterPDF(QAbstractItemModel *model, const QString &filePath)
{
    if (!model || model->rowCount() == 0)
        return false;

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize::A4);
    writer.setPageMargins(QMarginsF(15, 15, 15, 15));

    QPainter painter(&writer);
    if (!painter.isActive())
        return false;

    QFont titleFont("Arial", 16, QFont::Bold);
    QFont headerFont("Arial", 11, QFont::Bold);
    QFont cellFont("Arial", 10);

    QRectF pageRect = writer.pageLayout().paintRectPixels(writer.resolution());
    qreal x = pageRect.left();
    qreal y = pageRect.top();
    qreal w = pageRect.width();

    const int rows = model->rowCount();
    const int cols = model->columnCount();

    qreal headerHeight = 25;
    qreal rowHeight = 20;
    QVector<qreal> colWidths(cols, w / cols);

    // Title
    painter.setFont(titleFont);
    painter.drawText(QRectF(x, y, w, 40), Qt::AlignCenter, "Liste des Élèves");
    y += 50;



    // Headers
    painter.setFont(headerFont);
    qreal curX = x;
    for (int c = 0; c < cols; ++c) {
        QString header = model->headerData(c, Qt::Horizontal).toString();
        QRectF rect(curX, y, colWidths[c], headerHeight);
        painter.drawRect(rect);
        painter.drawText(rect, Qt::AlignCenter, header);
        curX += colWidths[c];
    }
    y += headerHeight;

    painter.setFont(cellFont);

    // Rows
    for (int r = 0; r < rows; ++r) {
        curX = x;
        for (int c = 0; c < cols; ++c) {
            QString data = model->data(model->index(r, c)).toString();
            QRectF rect(curX, y, colWidths[c], rowHeight);
            painter.drawRect(rect);
            painter.drawText(rect.adjusted(3, 0, -3, 0), Qt::AlignVCenter | Qt::AlignLeft, data);
            curX += colWidths[c];
        }
        y += rowHeight;

        if (y + rowHeight > pageRect.bottom() - 50) {
            writer.newPage();
            y = pageRect.top();

            painter.setFont(headerFont);
            curX = x;
            for (int c = 0; c < cols; ++c) {
                QString header = model->headerData(c, Qt::Horizontal).toString();
                QRectF rect(curX, y, colWidths[c], headerHeight);
                painter.drawRect(rect);
                painter.drawText(rect, Qt::AlignCenter, header);
                curX += colWidths[c];
            }
            y += headerHeight;
            painter.setFont(cellFont);
        }
    }

    painter.end();
    return true;
}

QSqlQueryModel* eleve::trier(QString critere)
{
    if (critere == "Non trié")
        return afficher(); // afficher() renvoie QSqlQueryModel*

    QSqlQueryModel* model = new QSqlQueryModel();
    if (critere == "Nom")
        model->setQuery("SELECT * FROM ELEVES ORDER BY NOM ASC");
    else if (critere == "Date de naissance")
        model->setQuery("SELECT * FROM ELEVES ORDER BY DATENAISSANCE ASC");

    return model;
}




void eleve::envoyerMail(QString to, QString sujet,
                        QString nom, QString prenom, QString dateNaissance,
                        QString telephone, QString adresse)
{
    QString corps = "Bonjour " + nom + " " + prenom + ",\n\n"
                                                      "Voici vos informations :\n"
                                                      "Nom : " + nom + "\n"
                            "Prénom : " + prenom + "\n"
                               "Date de naissance : " + dateNaissance + "\n"
                                      "Téléphone : " + telephone + "\n"
                                  "Adresse : " + adresse + "\n\n"
                                "Merci de votre confiance !";

    QString cmd = "powershell -Command \"Send-MailMessage "
                  "-From 'tonmail@gmail.com' "
                  "-To '" + to + "' "
                         "-Subject '" + sujet + "' "
                            "-Body '" + corps + "' "
                            "-SmtpServer 'smtp.gmail.com' "
                            "-Port 587 "
                            "-UseSsl "
"-Credential (New-Object System.Management.Automation.PSCredential('neodrive76@gmail.com',(ConvertTo-SecureString 'ovos fjfr lhvu wzig' -AsPlainText -Force)))\"";
    QProcess::execute(cmd);
}



void eleve::enregistreristorique(QString champ, QString ancienne, QString nouvelle, QString mouvement)
{
    QFile file("historique.json");
    QJsonArray array;

    // Lire l'ancien contenu
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        array = doc.array();
        file.close();
    }

    // Nouvelle entrée
    QJsonObject obj;
    obj["id_eleve"] = id_eleve;
    obj["champ"] = champ;
    obj["ancienne"] = ancienne;
    obj["nouvelle"] = nouvelle;
    obj["mouvement"] = mouvement;
    obj["date"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    array.append(obj);

    // Sauvegarde
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(array);
        file.write(doc.toJson());
        file.close();
    }
}
