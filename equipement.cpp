#include "equipement.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

Equipement::Equipement() {}

// Constructeur avec 6 paramètres seulement
Equipement::Equipement(QString id, QString cat, QString et,
                       QString four, QDate date, QString l)
    : id_equipement(id),
    categorie(cat),
    etat(et),
    fournisseur(four),
    date_acquisition(date),
    lieu(l)
{
    // Pas d'initialisation d'image
}

bool Equipement::ajouter()
{
    QSqlQuery query;

    // Vérifier si l'ID existe déjà
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM EQUIPEMENTS WHERE ID_EQUIPEMENT = :id");
    checkQuery.bindValue(":id", id_equipement);
    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        qDebug() << "❌ ERREUR: L'ID" << id_equipement << "existe déjà!";
        return false;
    }

    // Préparation de la requête - SEULEMENT 6 colonnes
    query.prepare("INSERT INTO EQUIPEMENTS (ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, DATE_ACQUISITION, LIEU) "
                  "VALUES (:id, :categorie, :etat, :fournisseur, TO_DATE(:date_acquisition, 'YYYY-MM-DD'), :lieu)");

    // Vérification des valeurs
    if (id_equipement.isEmpty()) {
        qDebug() << "❌ ERREUR: ID est vide!";
        return false;
    }

    query.bindValue(":id", id_equipement);
    query.bindValue(":categorie", categorie);
    query.bindValue(":etat", etat);
    query.bindValue(":fournisseur", fournisseur);

    // Gestion de la date
    if (date_acquisition.isValid()) {
        QString dateStr = date_acquisition.toString("yyyy-MM-dd");
        query.bindValue(":date_acquisition", dateStr);
        qDebug() << "Date formatée pour Oracle:" << dateStr;
    } else {
        qDebug() << "❌ ERREUR: Date invalide!";
        query.bindValue(":date_acquisition", QVariant());
    }

    query.bindValue(":lieu", lieu);

    // Exécution
    if (query.exec()) {
        qDebug() << "✅ Équipement ajouté avec succès! ID:" << id_equipement;
        return true;
    } else {
        qDebug() << "❌ ERREUR lors de l'ajout:";
        qDebug() << "   Message:" << query.lastError().text();
        qDebug() << "   Requête:" << query.lastQuery();
        qDebug() << "   ID:" << id_equipement;
        return false;
    }
}

bool Equipement::modifier(QString id_equipement, QString categorie, QString etat,
                          QString fournisseur, QDate date_acquisition, QString lieu)
{
    QSqlQuery query;

    // MISE À JOUR SEULEMENT des 6 colonnes
    query.prepare("UPDATE EQUIPEMENTS SET "
                  "CATEGORIE = :categorie, "
                  "ETAT = :etat, "
                  "FOURNISSEUR = :fournisseur, "
                  "DATE_ACQUISITION = TO_DATE(:date_acquisition, 'YYYY-MM-DD'), "
                  "LIEU = :lieu "
                  "WHERE ID_EQUIPEMENT = :id");

    query.bindValue(":id", id_equipement);
    query.bindValue(":categorie", categorie);
    query.bindValue(":etat", etat);
    query.bindValue(":fournisseur", fournisseur);

    if (date_acquisition.isValid()) {
        query.bindValue(":date_acquisition", date_acquisition.toString("yyyy-MM-dd"));
    } else {
        query.bindValue(":date_acquisition", QVariant());
    }

    query.bindValue(":lieu", lieu);

    if (!query.exec()) {
        qDebug() << "Erreur modification:" << query.lastError().text();
        return false;
    }

    qDebug() << "✅ Équipement modifié avec succès! ID:" << id_equipement;
    return true;
}

bool Equipement::supprimer(QString id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM EQUIPEMENTS WHERE ID_EQUIPEMENT = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suppression:" << query.lastError().text();
        return false;
    }

    qDebug() << "✅ Équipement supprimé avec succès! ID:" << id;
    return true;
}

QSqlQueryModel* Equipement::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery query;
    query.prepare("SELECT ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, "
                  "TO_CHAR(DATE_ACQUISITION, 'DD/MM/YYYY') as DATE_ACQUISITION, "
                  "LIEU FROM EQUIPEMENTS ORDER BY ID_EQUIPEMENT");

    if (query.exec()) {
        model->setQuery(query);
        model->setHeaderData(0, Qt::Horizontal, "ID");
        model->setHeaderData(1, Qt::Horizontal, "Catégorie");
        model->setHeaderData(2, Qt::Horizontal, "État");
        model->setHeaderData(3, Qt::Horizontal, "Fournisseur");
        model->setHeaderData(4, Qt::Horizontal, "Date d'acquisition");
        model->setHeaderData(5, Qt::Horizontal, "Lieu");
    } else {
        qDebug() << "Erreur affichage:" << query.lastError().text();
    }

    return model;
}

QSqlQueryModel* Equipement::rechercher(QString valeur)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery query;

    query.prepare("SELECT ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, "
                  "TO_CHAR(DATE_ACQUISITION, 'DD/MM/YYYY') as DATE_ACQUISITION, "
                  "LIEU FROM EQUIPEMENTS WHERE "
                  "UPPER(ID_EQUIPEMENT) LIKE UPPER(:valeur) OR "
                  "UPPER(CATEGORIE) LIKE UPPER(:valeur) OR "
                  "UPPER(ETAT) LIKE UPPER(:valeur) OR "
                  "UPPER(FOURNISSEUR) LIKE UPPER(:valeur) OR "
                  "UPPER(LIEU) LIKE UPPER(:valeur) "
                  "ORDER BY ID_EQUIPEMENT");

    QString searchPattern = "%" + valeur + "%";
    query.bindValue(":valeur", searchPattern);

    if (query.exec()) {
        model->setQuery(query);
        model->setHeaderData(0, Qt::Horizontal, "ID");
        model->setHeaderData(1, Qt::Horizontal, "Catégorie");
        model->setHeaderData(2, Qt::Horizontal, "État");
        model->setHeaderData(3, Qt::Horizontal, "Fournisseur");
        model->setHeaderData(4, Qt::Horizontal, "Date d'acquisition");
        model->setHeaderData(5, Qt::Horizontal, "Lieu");
    } else {
        qDebug() << "Erreur recherche:" << query.lastError().text();
    }

    return model;
}

QSqlQueryModel* Equipement::trier(QString critere)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    // Validation du critère pour éviter l'injection SQL
    QStringList criteresValides = {"ID_EQUIPEMENT", "CATEGORIE", "ETAT",
                                   "FOURNISSEUR", "DATE_ACQUISITION", "LIEU"};

    if (!criteresValides.contains(critere.toUpper())) {
        critere = "ID_EQUIPEMENT"; // Valeur par défaut
    }

    QSqlQuery query;
    query.prepare("SELECT ID_EQUIPEMENT, CATEGORIE, ETAT, FOURNISSEUR, "
                  "TO_CHAR(DATE_ACQUISITION, 'DD/MM/YYYY') as DATE_ACQUISITION, "
                  "LIEU FROM EQUIPEMENTS ORDER BY " + critere);

    if (query.exec()) {
        model->setQuery(query);
        model->setHeaderData(0, Qt::Horizontal, "ID");
        model->setHeaderData(1, Qt::Horizontal, "Catégorie");
        model->setHeaderData(2, Qt::Horizontal, "État");
        model->setHeaderData(3, Qt::Horizontal, "Fournisseur");
        model->setHeaderData(4, Qt::Horizontal, "Date d'acquisition");
        model->setHeaderData(5, Qt::Horizontal, "Lieu");
    } else {
        qDebug() << "Erreur tri:" << query.lastError().text();
    }

    return model;
}
