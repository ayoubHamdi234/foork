#include "employe.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QPrinter>
#include <QTextDocument>
#include <QFileDialog>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

// ==================== CONSTRUCTEURS ====================

Employe::Employe()
    : id(0), nom(""), prenom(""), genre(""), cin(""), email(""),
    dateNaissance(QDate::currentDate()), adresse(""), telephone(""),
    poste(""), dateEmbauche(QDate::currentDate()), mot_de_passe(""),
    messages("[]")
{}

Employe::Employe(QString n, QString p, QString g, QString c, QString e, QDate dn,
                 QString a, QString t, QString po, QDate de, QString mdp,
                 QString q, QString r, QString pin_code, int i)
    : nom(n), prenom(p), genre(g), cin(c), email(e), dateNaissance(dn),
    adresse(a), telephone(t), poste(po), dateEmbauche(de),
    mot_de_passe(mdp), question(q), reponse(r), pin(pin_code), id(i),
    messages("[]")
{}

// ==================== MÉTHODES CRUD ====================

bool Employe::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO Employes "
                  "(nom, prenom, genre, cin, email, datenaissance, adresse, telephone, poste, dateembauche, mot_de_passe, question, reponse, pin) "
                  "VALUES "
                  "(:nom, :prenom, :genre, :cin, :email, :datenaissance, :adresse, :telephone, :poste, :dateembauche, :mot_de_passe, :question, :reponse, :pin)");

    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":genre", genre);
    query.bindValue(":cin", cin);
    query.bindValue(":email", email);
    query.bindValue(":datenaissance", dateNaissance);
    query.bindValue(":adresse", adresse);
    query.bindValue(":telephone", telephone);
    query.bindValue(":poste", poste);
    query.bindValue(":dateembauche", dateEmbauche);
    query.bindValue(":mot_de_passe", mot_de_passe);
    query.bindValue(":question", question);
    query.bindValue(":reponse", reponse);
    query.bindValue(":pin", pin);

    if (query.exec()) {
        qDebug() << "✅ Employé ajouté avec succès";
        return true;
    } else {
        qDebug() << "❌ Erreur ajout employé:" << query.lastError().text();
        return false;
    }
}


bool Employe::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE Employes SET nom=:nom, prenom=:prenom, genre=:genre, cin=:cin, email=:email, "
                  "datenaissance=:datenaissance, adresse=:adresse, telephone=:telephone, poste=:poste, dateembauche=:dateembauche "
                  "WHERE id_employe=:id");
    query.bindValue(":nom", nom);
    query.bindValue(":prenom", prenom);
    query.bindValue(":genre", genre);
    query.bindValue(":cin", cin);
    query.bindValue(":email", email);
    query.bindValue(":datenaissance", dateNaissance);
    query.bindValue(":adresse", adresse);
    query.bindValue(":telephone", telephone);
    query.bindValue(":poste", poste);
    query.bindValue(":dateembauche", dateEmbauche);
    query.bindValue(":id", id);

    if (query.exec()) {
        qDebug() << "✅ Employé modifié avec succès";
        return true;
    } else {
        qDebug() << "❌ Erreur modification employé:" << query.lastError().text();
        return false;
    }
}

bool Employe::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM Employes WHERE id_employe=:id");
    query.bindValue(":id", id);

    if (query.exec()) {
        qDebug() << "✅ Employé supprimé avec succès";
        return true;
    } else {
        qDebug() << "❌ Erreur suppression employé:" << query.lastError().text();
        return false;
    }
}

QSqlQueryModel* Employe::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT id_employe, nom, prenom, genre, cin, email, datenaissance, adresse, telephone, poste, dateembauche FROM Employes");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Genre"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("CIN"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Date Naissance"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(9, Qt::Horizontal, QObject::tr("Poste"));
    model->setHeaderData(10, Qt::Horizontal, QObject::tr("Date Embauche"));

    return model;
}

// ==================== MÉTHODES D'INTERFACE ====================

void Employe::remplirTableWidget(QTableWidget* tableWidget)
{
    if (!tableWidget) return;

    QSqlQueryModel* model = afficher();
    tableWidget->setRowCount(model->rowCount());
    tableWidget->setColumnCount(model->columnCount());

    // Définir les en-têtes
    QStringList headers;
    headers << "ID" << "Nom" << "Prénom" << "Genre" << "CIN" << "Email"
            << "Date Naissance" << "Adresse" << "Téléphone" << "Poste" << "Date Embauche";
    tableWidget->setHorizontalHeaderLabels(headers);

    // Remplir les données
    for(int i = 0; i < model->rowCount(); ++i) {
        for(int j = 0; j < model->columnCount(); ++j) {
            QTableWidgetItem* item = new QTableWidgetItem(model->data(model->index(i,j)).toString());
            tableWidget->setItem(i, j, item);
        }
    }

    tableWidget->resizeColumnsToContents();
    delete model;
}

void Employe::trierEtRemplirTable(QTableWidget* tableWidget, int index)
{
    if (!tableWidget) return;

    QString orderBy;

    switch(index) {
    case 1: orderBy = "nom ASC"; break;
    case 2: orderBy = "prenom ASC"; break;
    case 3: orderBy = "id_employe ASC"; break;
    case 4: orderBy = "nom DESC"; break;
    case 5: orderBy = "prenom DESC"; break;
    case 6: orderBy = "id_employe DESC"; break;
    default: orderBy = "id_employe ASC"; break;
    }

    QSqlQuery query;
    QString queryStr = QString("SELECT id_employe, nom, prenom, genre, cin, email, datenaissance, adresse, telephone, poste, dateembauche "
                               "FROM Employes ORDER BY %1").arg(orderBy);

    if (query.exec(queryStr)) {
        tableWidget->setRowCount(0);

        int row = 0;
        while (query.next()) {
            tableWidget->insertRow(row);
            for (int col = 0; col < 11; col++) {
                QTableWidgetItem* item = new QTableWidgetItem(query.value(col).toString());
                tableWidget->setItem(row, col, item);
            }
            row++;
        }

        // Définir les en-têtes
        QStringList headers;
        headers << "ID" << "Nom" << "Prénom" << "Genre" << "CIN" << "Email"
                << "Date Naissance" << "Adresse" << "Téléphone" << "Poste" << "Date Embauche";
        tableWidget->setHorizontalHeaderLabels(headers);

        tableWidget->resizeColumnsToContents();
    } else {
        qDebug() << "❌ Erreur tri employés:" << query.lastError().text();
    }
}

void Employe::rechercherEtRemplirTable(QTableWidget* tableWidget, const QString& texteRecherche, int typeRecherche)
{
    if (!tableWidget) return;

    QString whereClause;
    QSqlQuery query;

    if (typeRecherche == 0 || texteRecherche.isEmpty()) {
        // Aucun filtre
        query.prepare("SELECT id_employe, nom, prenom, genre, cin, email, datenaissance, adresse, telephone, poste, dateembauche FROM Employes");
    } else {
        switch(typeRecherche) {
        case 1: whereClause = "id_employe = :valeur"; break;
        case 2: whereClause = "LOWER(nom) LIKE LOWER(:valeur)"; break;
        case 3: whereClause = "LOWER(prenom) LIKE LOWER(:valeur)"; break;
        default: whereClause = "id_employe = :valeur"; break;
        }

        query.prepare(QString("SELECT id_employe, nom, prenom, genre, cin, email, datenaissance, adresse, telephone, poste, dateembauche "
                              "FROM Employes WHERE %1").arg(whereClause));

        if (typeRecherche == 1) {
            query.bindValue(":valeur", texteRecherche.toInt());
        } else {
            query.bindValue(":valeur", "%" + texteRecherche + "%");
        }
    }

    if (query.exec()) {
        tableWidget->setRowCount(0);

        int row = 0;
        while (query.next()) {
            tableWidget->insertRow(row);
            for (int col = 0; col < 11; col++) {
                QTableWidgetItem* item = new QTableWidgetItem(query.value(col).toString());
                tableWidget->setItem(row, col, item);
            }
            row++;
        }

        // Définir les en-têtes
        QStringList headers;
        headers << "ID" << "Nom" << "Prénom" << "Genre" << "CIN" << "Email"
                << "Date Naissance" << "Adresse" << "Téléphone" << "Poste" << "Date Embauche";
        tableWidget->setHorizontalHeaderLabels(headers);

        tableWidget->resizeColumnsToContents();
    } else {
        qDebug() << "❌ Erreur recherche employés:" << query.lastError().text();
    }
}

bool Employe::exporterPDF_Employes(QTableWidget* tableWidget, const QString& fileName)
{
    if (!tableWidget) return false;

    QString filePath = fileName;
    if (filePath.isEmpty()) {
        filePath = QFileDialog::getSaveFileName(nullptr, "Enregistrer PDF", "", "*.pdf");
        if (filePath.isEmpty())
            return false;
        if (!filePath.endsWith(".pdf"))
            filePath += ".pdf";
    }

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15));
    printer.setOutputFileName(filePath);

    QTextDocument doc;

    // CSS
    QString style = R"(
        <style>
        body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 20px; }
        .header {
            text-align: center;
            color: #2C3E50;
            margin-bottom: 30px;
            border-bottom: 2px solid #3498DB;
            padding-bottom: 10px;
        }
        .title {
            font-size: 24px;
            font-weight: bold;
            margin-bottom: 5px;
        }
        .subtitle {
            font-size: 14px;
            color: #7F8C8D;
        }
        table {
            border-collapse: collapse;
            width: 100%;
            font-size: 12px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
            margin-top: 20px;
        }
        th {
            background-color: #3498DB;
            color: white;
            padding: 12px 8px;
            text-align: left;
            font-weight: bold;
            font-size: 13px;
            border: 1px solid #2980B9;
        }
        td {
            padding: 10px 8px;
            border: 1px solid #BDC3C7;
            vertical-align: top;
        }
        tr:nth-child(even) {
            background-color: #F8F9F9;
        }
        tr:nth-child(odd) {
            background-color: white;
        }
        .footer {
            margin-top: 30px;
            text-align: center;
            font-size: 11px;
            color: #95A5A6;
            border-top: 1px solid #ECF0F1;
            padding-top: 10px;
        }
        </style>
    )";

    QString html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>" + style + "</head><body>";

    // En-tête
    html += "<div class='header'>";
    html += "<div class='title'>ÉCOLE DE CONDUITE SMART DRIVING</div>";
    html += "<div class='subtitle'>Liste des Employés</div>";
    html += "<div class='subtitle'>" + QDate::currentDate().toString("dd/MM/yyyy") + "</div>";
    html += "</div>";

    // Tableau
    html += "<table>";
    html += "<thead><tr>";
    for (int c = 0; c < tableWidget->columnCount(); ++c) {
        QString headerText = tableWidget->horizontalHeaderItem(c) ?
                                 tableWidget->horizontalHeaderItem(c)->text() : QString("Colonne %1").arg(c+1);
        html += "<th>" + headerText + "</th>";
    }
    html += "</tr></thead><tbody>";

    // Données
    for (int r = 0; r < tableWidget->rowCount(); ++r) {
        html += "<tr>";
        for (int c = 0; c < tableWidget->columnCount(); ++c) {
            QTableWidgetItem* item = tableWidget->item(r, c);
            QString cellText = item ? item->text() : "";
            html += "<td>" + cellText + "</td>";
        }
        html += "</tr>";
    }

    html += "</tbody></table>";

    // Pied de page
    html += "<div class='footer'>";
    html += "Document généré le " + QDateTime::currentDateTime().toString("dd/MM/yyyy à HH:mm") + " | ";
    html += "École de Conduite Smart Driving © 2024";
    html += "</div>";

    html += "</body></html>";

    doc.setHtml(html);
    doc.setPageSize(printer.pageRect(QPrinter::Point).size());
    doc.print(&printer);

    return true;
}

QMap<QString, int> Employe::getStatistiquePoste()
{
    QMap<QString, int> compteurPoste;

    QSqlQuery query("SELECT poste, COUNT(*) FROM Employes GROUP BY poste");
    while (query.next()) {
        QString poste = query.value(0).toString();
        int count = query.value(1).toInt();
        compteurPoste[poste] = count;
    }

    return compteurPoste;
}

// ==================== MÉTHODES DE VALIDATION ====================

bool Employe::validerTelephone(const QString& telephone, QString& messageErreur)
{
    QString telClean = telephone;
    telClean = telClean.replace(" ", "").replace("-", "").replace(".", "");

    QRegularExpression regex("^(\\+216|00216)?[24579][0-9]{7}$");

    if (!regex.match(telClean).hasMatch()) {
        messageErreur = "Format de téléphone invalide. Doit commencer par +216/00216 suivi de 8 chiffres (premier: 2,4,5,7,9)";
        return false;
    }
    return true;
}

bool Employe::validerEmail(const QString& email, QString& messageErreur)
{
    QRegularExpression regex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!regex.match(email).hasMatch()) {
        messageErreur = "Format d'email invalide. Exemple: exemple@domaine.com";
        return false;
    }
    return true;
}

bool Employe::validerCIN(const QString& cin, QString& messageErreur)
{
    QRegularExpression regex("^[0-9]{8}$");
    if (!regex.match(cin).hasMatch()) {
        messageErreur = "Le CIN doit contenir exactement 8 chiffres";
        return false;
    }
    return true;
}

// ==================== MÉTHODES SMS ====================

bool Employe::envoyerSMS(const QString& to, const QString& message)
{
    qDebug() << "🔔 ========== DÉBUT ENVOI SMS ==========";

    const QString accountSid = "AC357620bc3addc054bc37ad2112da4527";
    const QString authToken  = "671e669df18f022568056addc756606e";
    const QString fromNumber = "+16408671299";

    qDebug() << "📋 Configuration Twilio:";
    qDebug() << "   Account SID:" << accountSid;
    qDebug() << "   Auth Token:" << (authToken.isEmpty() ? "❌ VIDE" : "✅ PRÉSENT");
    qDebug() << "   From Number:" << fromNumber;
    qDebug() << "   To Number:" << to;
    qDebug() << "   Message:" << message;

    if (accountSid.isEmpty() || authToken.isEmpty() || fromNumber.isEmpty()) {
        qDebug() << "❌ PARAMÈTRES TWILIO MANQUANTS";
        return false;
    }

    if (to.isEmpty()) {
        qDebug() << "❌ NUMÉRO DE DESTINATION VIDE";
        return false;
    }

    QString cleanTo = to;
    cleanTo = cleanTo.replace(" ", "").replace("-", "").replace(".", "");
    qDebug() << "   Numéro nettoyé:" << cleanTo;

    if (!cleanTo.startsWith("+216")) {
        qDebug() << "❌ FORMAT NUMÉRO INVALIDE - Doit commencer par +216";
        qDebug() << "   Format reçu:" << cleanTo;
        qDebug() << "   Format attendu: +216XXXXXXXX";
        return false;
    }

    if (cleanTo.length() != 12) {
        qDebug() << "❌ LONGUEUR NUMÉRO INVALIDE";
        qDebug() << "   Longueur reçue:" << cleanTo.length();
        qDebug() << "   Longueur attendue: 12 caractères (+216 + 8 chiffres)";
        return false;
    }

    if (cleanTo == fromNumber) {
        qDebug() << "❌ FROM ET TO IDENTIQUES";
        return false;
    }

    try {
        QUrl url(QString("https://api.twilio.com/2010-04-01/Accounts/%1/Messages.json").arg(accountSid));
        qDebug() << "🌐 URL Twilio:" << url.toString();

        QNetworkRequest request(url);

        QString credentials = accountSid + ":" + authToken;
        QByteArray base64Credentials = credentials.toUtf8().toBase64();
        request.setRawHeader("Authorization", "Basic " + base64Credentials);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QUrlQuery params;
        params.addQueryItem("From", fromNumber);
        params.addQueryItem("To", cleanTo);
        params.addQueryItem("Body", message);

        QByteArray postData = params.toString(QUrl::FullyEncoded).toUtf8();
        qDebug() << "📦 Données POST:" << postData;

        qDebug() << "📡 Envoi requête à Twilio...";

        QNetworkAccessManager manager;
        QNetworkReply *reply = manager.post(request, postData);

        QTimer timer;
        timer.setSingleShot(true);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000);

        loop.exec();

        bool success = false;
        if (timer.isActive()) {
            timer.stop();
            success = (reply->error() == QNetworkReply::NoError);
            QByteArray response = reply->readAll();

            if (success) {
                qDebug() << "✅ SMS ENVOYÉ AVEC SUCCÈS!";
                qDebug() << "📨 Réponse Twilio:" << response;
            } else {
                qDebug() << "❌ ERREUR SMS:" << reply->errorString();
                qDebug() << "🔍 Code erreur:" << reply->error();
                qDebug() << "📄 Détails:" << response;
            }
        } else {
            qDebug() << "⏰ TIMEOUT - Pas de réponse du serveur Twilio après 30s";
            reply->abort();
        }

        reply->deleteLater();
        qDebug() << "🔔 ========== FIN ENVOI SMS ==========";
        return success;

    } catch (const std::exception& e) {
        qDebug() << "💥 EXCEPTION:" << e.what();
        qDebug() << "🔔 ========== FIN ENVOI SMS ==========";
        return false;
    }
}

// ==================== MÉTHODES D'AUTHENTIFICATION ====================

int Employe::authentifierEmploye(const QString& nom, const QString& motDePasse)
{
    QSqlQuery query;
    query.prepare("SELECT id_employe FROM Employes WHERE (nom = :nom OR prenom = :nom) AND mot_de_passe = :mdp");
    query.bindValue(":nom", nom);
    query.bindValue(":mdp", motDePasse);

    if (query.exec() && query.next()) {
        int id = query.value(0).toInt();
        qDebug() << "✅ Employé authentifié:" << nom << "ID:" << id;
        return id;
    } else {
        qDebug() << "❌ Échec authentification pour:" << nom;
        return -1;
    }
}

QString Employe::getNomEmploye(int idEmploye)
{
    QSqlQuery query;
    query.prepare("SELECT nom, prenom FROM Employes WHERE id_employe = :id");
    query.bindValue(":id", idEmploye);

    if (query.exec() && query.next()) {
        return query.value(0).toString() + " " + query.value(1).toString();
    }
    return "Inconnu";
}

QList<QPair<int, QString>> Employe::getListeEmployes(int idExclu)
{
    QList<QPair<int, QString>> employes;

    QString queryStr = "SELECT id_employe, nom, prenom FROM Employes WHERE 1=1";

    if (idExclu != -1) {
        queryStr += " AND id_employe != :id_exclu";
    }

    QSqlQuery query;
    query.prepare(queryStr);

    if (idExclu != -1) {
        query.bindValue(":id_exclu", idExclu);
    }

    if (query.exec()) {
        while (query.next()) {
            int id = query.value(0).toInt();
            QString nomComplet = query.value(1).toString() + " " + query.value(2).toString();
            employes.append(qMakePair(id, nomComplet));
        }
    } else {
        qDebug() << "❌ Erreur chargement employés:" << query.lastError().text();
    }

    return employes;
}

// ==================== MÉTHODES DE MESSAGERIE ====================

// Dans employe.cpp
// Dans employe.cpp, assurez-vous que envoyerMessage() utilise "ID_EMPLOYE" et "MESSAGES" :
bool Employe::envoyerMessage(int idExp, const QString &nomExp, int idDest, const QString &messageTexte)
{
    qDebug() << "=== ENVOI MESSAGE ===";
    qDebug() << "📨 Expéditeur:" << idExp << nomExp;
    qDebug() << "👥 Destinataire:" << idDest;
    qDebug() << "💬 Message:" << messageTexte.left(50) << "...";

    // 1. Récupérer les messages actuels de l'expéditeur
    QSqlQuery queryExp;
    queryExp.prepare("SELECT MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    queryExp.bindValue(":id", idExp);

    if (!queryExp.exec() || !queryExp.next()) {
        qDebug() << "❌ Expéditeur ID" << idExp << "non trouvé dans la table EMPLOYES";
        return false;
    }

    QString messagesJson = queryExp.value(0).toString();
    QJsonArray messagesArray;

    if (!messagesJson.isEmpty() && messagesJson != "[]") {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messagesJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
            messagesArray = doc.array();
        }
    }

    // 2. Créer le nouveau message pour l'expéditeur
    QJsonObject nouveauMessageExp;
    nouveauMessageExp["type"] = "envoye";
    nouveauMessageExp["a"] = idDest;
    nouveauMessageExp["de"] = idExp;
    nouveauMessageExp["de_nom"] = nomExp;
    nouveauMessageExp["texte"] = messageTexte;
    nouveauMessageExp["date"] = QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss");
    nouveauMessageExp["lu"] = true;

    messagesArray.append(nouveauMessageExp);

    // 3. Mettre à jour l'expéditeur
    QJsonDocument newDocExp(messagesArray);
    QString newJsonExp = newDocExp.toJson(QJsonDocument::Compact);

    QSqlQuery updateExp;
    updateExp.prepare("UPDATE EMPLOYES SET MESSAGES = :messages WHERE ID_EMPLOYE = :id");
    updateExp.bindValue(":messages", newJsonExp);
    updateExp.bindValue(":id", idExp);

    if (!updateExp.exec()) {
        qDebug() << "❌ Erreur mise à jour expéditeur:" << updateExp.lastError().text();
        return false;
    }

    qDebug() << "✅ Message enregistré pour expéditeur";

    // 4. Récupérer les messages actuels du destinataire
    QSqlQuery queryDest;
    queryDest.prepare("SELECT MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    queryDest.bindValue(":id", idDest);

    if (!queryDest.exec() || !queryDest.next()) {
        qDebug() << "❌ Destinataire ID" << idDest << "non trouvé";
        return false;
    }

    QString messagesDestJson = queryDest.value(0).toString();
    QJsonArray messagesDestArray;

    if (!messagesDestJson.isEmpty() && messagesDestJson != "[]") {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messagesDestJson.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
            messagesDestArray = doc.array();
        }
    }

    // 5. Créer le message pour le destinataire
    QJsonObject nouveauMessageDest;
    nouveauMessageDest["type"] = "recu";
    nouveauMessageDest["a"] = idDest;
    nouveauMessageDest["de"] = idExp;
    nouveauMessageDest["de_nom"] = nomExp;
    nouveauMessageDest["texte"] = messageTexte;
    nouveauMessageDest["date"] = QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm:ss");
    nouveauMessageDest["lu"] = false;

    messagesDestArray.append(nouveauMessageDest);

    // 6. Mettre à jour le destinataire
    QJsonDocument newDocDest(messagesDestArray);
    QString newJsonDest = newDocDest.toJson(QJsonDocument::Compact);

    QSqlQuery updateDest;
    updateDest.prepare("UPDATE EMPLOYES SET MESSAGES = :messages WHERE ID_EMPLOYE = :id");
    updateDest.bindValue(":messages", newJsonDest);
    updateDest.bindValue(":id", idDest);

    if (!updateDest.exec()) {
        qDebug() << "❌ Erreur mise à jour destinataire:" << updateDest.lastError().text();
        return false;
    }

    qDebug() << "✅ Message enregistré pour destinataire";
    return true;
}

// Dans employe.cpp, remplacez getMessagesPourEmploye() par :
QList<QJsonObject> Employe::getMessagesPourEmploye(int idEmploye)
{
    QList<QJsonObject> messagesList;
    qDebug() << "🔍 Récupération des messages pour l'employé ID:" << idEmploye;

    // Récupérer tous les employés et leurs messages
    QSqlQuery query;
    query.prepare("SELECT ID_EMPLOYE, NOM, PRENOM, MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE != :id");
    query.bindValue(":id", idEmploye);

    if (query.exec()) {
        while (query.next()) {
            int autreEmployeId = query.value(0).toInt();
            QString nomAutre = query.value(1).toString() + " " + query.value(2).toString();
            QString messagesJson = query.value(3).toString();

            if (!messagesJson.isEmpty() && messagesJson != "[]") {
                QJsonDocument doc = QJsonDocument::fromJson(messagesJson.toUtf8());
                if (doc.isArray()) {
                    QJsonArray messagesArray = doc.array();

                    // Chercher le dernier message échangé avec cet employé
                    QJsonObject dernierMessage;
                    QDateTime derniereDate;

                    for (const QJsonValue &value : std::as_const(messagesArray)) {
                        QJsonObject msg = value.toObject();
                        int aQui = msg["a"].toInt();
                        int deQui = msg["de"].toInt();

                        // Vérifier si c'est un message entre ces deux employés
                        if ((aQui == idEmploye && deQui == autreEmployeId) ||
                            (aQui == autreEmployeId && deQui == idEmploye)) {

                            QString dateStr = msg["date"].toString();
                            QDateTime dateMsg = QDateTime::fromString(dateStr, "dd/MM/yyyy HH:mm:ss");

                            if (!derniereDate.isValid() || dateMsg > derniereDate) {
                                derniereDate = dateMsg;
                                dernierMessage = msg;
                                dernierMessage["contact_id"] = autreEmployeId;
                                dernierMessage["contact_nom"] = nomAutre;
                                dernierMessage["dernier_message"] = msg["texte"].toString();
                                dernierMessage["dernier_date"] = dateStr;
                            }
                        }
                    }

                    if (!dernierMessage.isEmpty()) {
                        messagesList.append(dernierMessage);
                    }
                }
            }
        }
    }

    // Trier par date décroissante
    std::sort(messagesList.begin(), messagesList.end(), [](const QJsonObject &a, const QJsonObject &b) {
        QString dateA = a["dernier_date"].toString();
        QString dateB = b["dernier_date"].toString();
        return QDateTime::fromString(dateA, "dd/MM/yyyy HH:mm:ss") >
               QDateTime::fromString(dateB, "dd/MM/yyyy HH:mm:ss");
    });

    qDebug() << "✅ Total conversations:" << messagesList.size();
    return messagesList;
}

void Employe::marquerMessagesLus(int idEmploye)
{
    QSqlQuery querySelect;
    querySelect.prepare("SELECT MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    querySelect.bindValue(":id", idEmploye);

    if (querySelect.exec() && querySelect.next()) {
        QString messagesJson = querySelect.value(0).toString();

        if (!messagesJson.isEmpty() && messagesJson != "[]") {
            QJsonDocument doc = QJsonDocument::fromJson(messagesJson.toUtf8());
            QJsonArray messagesArray = doc.array();

            // Marquer tous les messages comme lus
            for (int i = 0; i < messagesArray.size(); ++i) {
                QJsonObject msgObj = messagesArray[i].toObject();
                msgObj["lu"] = true;
                messagesArray[i] = msgObj;
            }

            // Sauvegarder
            QSqlQuery queryUpdate;
            queryUpdate.prepare("UPDATE EMPLOYES SET MESSAGES = :messages WHERE ID_EMPLOYE = :id");
            queryUpdate.bindValue(":messages", QJsonDocument(messagesArray).toJson(QJsonDocument::Compact));
            queryUpdate.bindValue(":id", idEmploye);
            queryUpdate.exec();
        }
    }
}

int Employe::getNombreMessagesNonLus(int idEmploye)
{
    int count = 0;

    QSqlQuery query;
    query.prepare("SELECT MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    query.bindValue(":id", idEmploye);

    if (query.exec() && query.next()) {
        QString messagesJson = query.value(0).toString();

        if (!messagesJson.isEmpty() && messagesJson != "[]") {
            QJsonDocument doc = QJsonDocument::fromJson(messagesJson.toUtf8());
            QJsonArray messagesArray = doc.array();

            for (const QJsonValue &value : messagesArray) {
                QJsonObject msgObj = value.toObject();
                if (!msgObj["lu"].toBool()) {
                    count++;
                }
            }
        }
    }

    return count;
}
QList<QJsonObject> Employe::getMessagesDiscussion(int idExp, int idDest)
{
    QList<QJsonObject> messages;
    qDebug() << "🔍 Chargement discussion entre" << idExp << "et" << idDest;

    // 1. Récupérer les messages de l'expéditeur
    QSqlQuery queryExp;
    queryExp.prepare("SELECT MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    queryExp.bindValue(":id", idExp);

    if (queryExp.exec() && queryExp.next()) {
        QString messagesJson = queryExp.value(0).toString();

        if (!messagesJson.isEmpty() && messagesJson != "[]") {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(messagesJson.toUtf8(), &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                QJsonArray array = doc.array();

                for (const QJsonValue &value : array) {
                    QJsonObject msg = value.toObject();
                    int aQui = msg["a"].toInt();
                    int deQui = msg["de"].toInt();

                    // Vérifier si c'est un message pour/depuis cette conversation
                    if ((aQui == idDest && deQui == idExp) ||
                        (aQui == idExp && deQui == idDest)) {

                        msg["id_expediteur"] = deQui;
                        msg["id_destinataire"] = aQui;
                        msg["message"] = msg["texte"].toString();
                        msg["date_envoi"] = msg["date"].toString();
                        msg["lu"] = msg["lu"].toBool();

                        messages.append(msg);
                    }
                }
            } else {
                qDebug() << "❌ Erreur parsing JSON expéditeur:" << parseError.errorString();
            }
        }
    } else {
        qDebug() << "❌ Erreur SQL expéditeur:" << queryExp.lastError().text();
    }

    // 2. Récupérer les messages du destinataire
    QSqlQuery queryDest;
    queryDest.prepare("SELECT MESSAGES FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    queryDest.bindValue(":id", idDest);

    if (queryDest.exec() && queryDest.next()) {
        QString messagesJson = queryDest.value(0).toString();

        if (!messagesJson.isEmpty() && messagesJson != "[]") {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(messagesJson.toUtf8(), &parseError);

            if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                QJsonArray array = doc.array();

                for (const QJsonValue &value : array) {
                    QJsonObject msg = value.toObject();
                    int aQui = msg["a"].toInt();
                    int deQui = msg["de"].toInt();

                    if ((aQui == idDest && deQui == idExp) ||
                        (aQui == idExp && deQui == idDest)) {

                        msg["id_expediteur"] = deQui;
                        msg["id_destinataire"] = aQui;
                        msg["message"] = msg["texte"].toString();
                        msg["date_envoi"] = msg["date"].toString();
                        msg["lu"] = msg["lu"].toBool();

                        messages.append(msg);
                    }
                }
            } else {
                qDebug() << "❌ Erreur parsing JSON destinataire:" << parseError.errorString();
            }
        }
    } else {
        qDebug() << "❌ Erreur SQL destinataire:" << queryDest.lastError().text();
    }

    // 3. Trier par date
    std::sort(messages.begin(), messages.end(), [](const QJsonObject &a, const QJsonObject &b) {
        QString dateA = a["date_envoi"].toString();
        QString dateB = b["date_envoi"].toString();
        return QDateTime::fromString(dateA, "dd/MM/yyyy HH:mm:ss") <
               QDateTime::fromString(dateB, "dd/MM/yyyy HH:mm:ss");
    });

    qDebug() << "📨" << messages.size() << "messages trouvés dans la conversation";

    // Debug: afficher les 3 premiers messages
    for (int i = 0; i < qMin(3, messages.size()); i++) {
        qDebug() << "  Message" << i << ":"
                 << "De" << messages[i]["id_expediteur"].toInt()
                 << "À" << messages[i]["id_destinataire"].toInt()
                 << ":" << messages[i]["message"].toString().left(30) << "...";
    }

    return messages;
}
// ==================== MÉTHODES PRIVÉES ====================

QString Employe::getTelephoneFromId(int idEmploye)
{
    QSqlQuery query;
    query.prepare("SELECT telephone FROM Employes WHERE id_employe = :id");
    query.bindValue(":id", idEmploye);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return "";
}
