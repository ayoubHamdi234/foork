#include "login.h"
#include "qsqlerror.h"
#include "ui_login.h"
#include <QSqlQuery>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>

Login::Login(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    QPixmap bgPixmap(":/images/bg.png");  // ton image
    ui->bg->setPixmap(bgPixmap);
    ui->bg->setScaledContents(true);       // pour qu'elle remplisse tout le QLabel
    ui->bg->lower();                       // mettre derrière tous les widgets

    // page 2
    QPixmap bg2Pixmap(":/images/bg.png");
    ui->bg2->setPixmap(bg2Pixmap);
    ui->bg2->setScaledContents(true);
    ui->bg2->lower();

    // page 3
    QPixmap bg3Pixmap(":/images/bg.png");
    ui->bg3->setPixmap(bg3Pixmap);
    ui->bg3->setScaledContents(true);
    ui->bg3->lower();

    // page 4
    QPixmap bg4Pixmap(":/images/bg.png");
    ui->bg4->setPixmap(bg4Pixmap);
    ui->bg4->setScaledContents(true);
    ui->bg4->lower();

    // Configuration initiale
    ui->stackedWidgetLogin->setCurrentIndex(0);
    ui->lineEdit_mdp->setEchoMode(QLineEdit::Password);

    // Effacer les champs au démarrage
    clearForgotPasswordFields();

    // DEBUG: Vérifier le contenu de la base
    debugDatabaseContent();  // À garder pour le débogage

    // Navigation
    connect(ui->pushButton_oublie, &QPushButton::clicked,
            this, &Login::openForgotPasswordPage);

    // Le bouton login est déjà connecté automatiquement par Qt
    // grâce au nom on_pushButton_login_clicked()

    connect(ui->btnBackQuestion, &QPushButton::clicked,
            this, &Login::backToLogin);

    connect(ui->btnBackPIN, &QPushButton::clicked,
            this, &Login::openQuestionPage);

    connect(ui->btnBackNewPass, &QPushButton::clicked,
            this, &Login::openPinPage);

    // Actions
    connect(ui->checkBoxVerifierNom, &QCheckBox::checkStateChanged,
            this, &Login::onVerifyUsernameChecked);

    connect(ui->btnValiderQuestion, &QPushButton::clicked,
            this, &Login::onValidateQuestionClicked);

    connect(ui->btnValiderPIN, &QPushButton::clicked,
            this, &Login::onValidatePinClicked);

    connect(ui->btnResetFinal, &QPushButton::clicked,
            this, &Login::onResetPasswordClicked);

    // Password visibility
    connect(ui->checkBoxAfficherMDP, &QCheckBox::checkStateChanged,
            this, &Login::togglePasswordVisibility);

    // Réinitialiser quand on quitte la page de récupération
    connect(ui->stackedWidgetLogin, &QStackedWidget::currentChanged,
            [this](int index) {
                if (index == 0) { // Retour à la page login
                    clearForgotPasswordFields();
                }
            });

    // Permettre de se connecter avec la touche Entrée
    if (ui->lineEdit_mdp) {
        connect(ui->lineEdit_mdp, &QLineEdit::returnPressed,
                this, &Login::on_pushButton_login_clicked);
    }
}

Login::~Login()
{
    delete ui;
}

// ============ MÉTHODE D'AUTHENTIFICATION ============
void Login::on_pushButton_login_clicked()
{
    // Vérifiez le nom exact de vos lineEdits dans le .ui
    // Ils pourraient s'appeler différemment
     ui->pushButton_login->setEnabled(false);
    QString username;
    QString password;

    // Essayer différents noms possibles
    if (ui->lineEdit_nom) {
        username = ui->lineEdit_nom->text().trimmed();
    } else if (ui->lineEdit_nom) {
        username = ui->lineEdit_nom->text().trimmed();
    } else {
        QMessageBox::warning(this, "Erreur", "Champ nom d'utilisateur non trouvé!");
        return;
    }

    password = ui->lineEdit_mdp->text();

    qDebug() << "=== TENTATIVE DE CONNEXION ===";
    qDebug() << "Nom d'utilisateur:" << username;
    qDebug() << "Mot de passe:" << (password.isEmpty() ? "VIDE" : "***");

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez remplir tous les champs");
        return;
    }

    if (authenticateUser(username, password)) {
        qDebug() << "✅ Authentification réussie pour:" << username;

        // Émettre le signal de succès
        emit loginSuccessful();

        // Fermer la fenêtre de login
        this->accept();

    } else {
        QMessageBox::warning(this, "Erreur",
                             "Nom d'utilisateur ou mot de passe incorrect");
        qDebug() << "❌ Échec d'authentification";

        // Effacer le champ mot de passe
        ui->lineEdit_mdp->clear();
        ui->lineEdit_mdp->setFocus();

    }
    ui->pushButton_login->setEnabled(true);
}


bool Login::authenticateUser(const QString& username, const QString& password)
{
    QSqlQuery q;

    // Vérifier par nom ou prénom
    q.prepare("SELECT id_employe, nom, prenom, mot_de_passe FROM employes "
              "WHERE (nom = :username OR prenom = :username) "
              "AND mot_de_passe = :password");
    q.bindValue(":username", username);
    q.bindValue(":password", password);

    if (q.exec() && q.next()) {
        int userId = q.value(0).toInt();
        QString nom = q.value(1).toString();
        QString prenom = q.value(2).toString();

        qDebug() << "✅ Employé trouvé:";
        qDebug() << "   ID:" << userId;
        qDebug() << "   Nom:" << nom;
        qDebug() << "   Prénom:" << prenom;

        return true;
    }

    qDebug() << "❌ Aucun employé trouvé avec ces identifiants";
    return false;
}

// ============ RESTE DU CODE (inchangé) ============

// Méthode pour effacer tous les champs de récupération
void Login::clearForgotPasswordFields()
{
    ui->lineEditResetUsername->clear();
    ui->labelQuestion->clear();
    ui->lineEditResetAnswer->clear();
    ui->lineEditResetPIN->clear();
    ui->lineEditResetNewPass->clear();
    ui->lineEditResetConfirm->clear();
    ui->checkBoxVerifierNom->setChecked(false);
    currentUsername.clear();
}

// Méthode de débogage
void Login::debugDatabaseContent()
{
    qDebug() << "=== DÉBOGAGE BASE DE DONNÉES EMPLOYES ===";

    QSqlQuery countQuery;
    if (countQuery.exec("SELECT COUNT(*) FROM employes")) {
        if (countQuery.next()) {
            qDebug() << "Nombre total d'employés:" << countQuery.value(0).toInt();
        }
    } else {
        qDebug() << "Erreur COUNT:" << countQuery.lastError().text();
    }

    QSqlQuery query;
    if (query.exec("SELECT id_employe, nom, prenom, mot_de_passe, question, reponse, pin FROM employes")) {
        int count = 0;
        while (query.next()) {
            count++;
            qDebug() << "--- Employé" << count << "---";
            qDebug() << "ID:" << query.value(0).toInt();
            qDebug() << "Nom:" << query.value(1).toString();
            qDebug() << "Prénom:" << query.value(2).toString();
            qDebug() << "Mot de passe:" << (query.value(3).toString().isEmpty() ? "VIDE" : "***");
            qDebug() << "Question:" << (query.value(4).toString().isEmpty() ? "VIDE" : query.value(4).toString());
            qDebug() << "Réponse:" << (query.value(5).toString().isEmpty() ? "VIDE" : query.value(5).toString());
            qDebug() << "PIN:" << (query.value(6).toString().isEmpty() ? "VIDE" : query.value(6).toString());
        }
        if (count == 0) {
            qDebug() << "⚠️ Table employes est VIDE!";
        }
    } else {
        qDebug() << "Erreur SELECT:" << query.lastError().text();
    }
    qDebug() << "=== FIN DÉBOGAGE ===";
}

// Navigation
void Login::openForgotPasswordPage()
{
    clearForgotPasswordFields();
    ui->stackedWidgetLogin->setCurrentIndex(1);
}

void Login::backToLogin()
{
    clearForgotPasswordFields();
    ui->stackedWidgetLogin->setCurrentIndex(0);
}

void Login::openQuestionPage()
{
    ui->stackedWidgetLogin->setCurrentIndex(1);
}

void Login::openPinPage()
{
    ui->stackedWidgetLogin->setCurrentIndex(2);
}

void Login::openNewPasswordPage()
{
    ui->stackedWidgetLogin->setCurrentIndex(3);
}

// Récupérer une valeur de la base de données
QString Login::getValue(QString column)
{
    if (currentUsername.isEmpty()) {
        qDebug() << "getValue: currentUsername est vide!";
        return "";
    }

    QSqlQuery q;
    q.prepare("SELECT " + column + " FROM employes WHERE nom = :nom");
    q.bindValue(":nom", currentUsername);

    if (q.exec()) {
        if (q.next()) {
            QString value = q.value(0).toString();
            qDebug() << "getValue(" << column << ") pour" << currentUsername << "=" << value;
            return value;
        } else {
            qDebug() << "getValue: Aucun résultat pour" << currentUsername;
        }
    } else {
        qDebug() << "getValue Erreur SQL:" << q.lastError().text();
    }

    return "";
}

// Vérification du nom d'utilisateur
void Login::onVerifyUsernameChecked(int state)
{
    QString username = ui->lineEditResetUsername->text().trimmed();

    if (state != Qt::Checked) {
        ui->labelQuestion->setText("");
        currentUsername.clear();
        return;
    }

    if (username.isEmpty()) {
        ui->labelQuestion->setText("Veuillez entrer un nom d'utilisateur");
        ui->checkBoxVerifierNom->setChecked(false);
        return;
    }

    qDebug() << "=== RECHERCHE DU NOM: '" << username << "' ===";

    QSqlQuery q;
    q.prepare("SELECT nom, question FROM employes WHERE nom = :nom");
    q.bindValue(":nom", username);

    if (q.exec()) {
        if (q.next()) {
            QString dbNom = q.value(0).toString();
            QString question = q.value(1).toString();

            qDebug() << "✅ Nom trouvé dans employes:" << dbNom;
            qDebug() << "Question:" << question;

            if (!question.isEmpty() && question.trimmed() != "") {
                currentUsername = dbNom;
                ui->labelQuestion->setText(question);
                qDebug() << "✅ Question affichée pour:" << currentUsername;
            } else {
                ui->labelQuestion->setText("❌ Aucune question de sécurité définie");
                ui->checkBoxVerifierNom->setChecked(false);
                qDebug() << "⚠️ Question vide pour:" << dbNom;
            }
        } else {
            qDebug() << "❌ Aucun employé trouvé avec le nom:" << username;

            QSqlQuery q2;
            q2.prepare("SELECT nom, question FROM employes WHERE prenom = :prenom");
            q2.bindValue(":prenom", username);

            if (q2.exec() && q2.next()) {
                QString dbNom = q2.value(0).toString();
                QString question = q2.value(1).toString();
                qDebug() << "✅ Trouvé par prénom! Nom:" << dbNom;

                if (!question.isEmpty()) {
                    currentUsername = dbNom;
                    ui->labelQuestion->setText(question);
                } else {
                    ui->labelQuestion->setText("❌ Aucune question définie");
                    ui->checkBoxVerifierNom->setChecked(false);
                }
            } else {
                ui->labelQuestion->setText("❌ Nom introuvable !");
                ui->checkBoxVerifierNom->setChecked(false);
                currentUsername.clear();
            }
        }
    } else {
        qDebug() << "❌ Erreur SQL:" << q.lastError().text();
        ui->labelQuestion->setText("❌ Erreur de connexion à la base");
        ui->checkBoxVerifierNom->setChecked(false);
    }
}

// Validation de la réponse
void Login::onValidateQuestionClicked()
{
    if (currentUsername.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez d'abord vérifier votre nom d'utilisateur");
        qDebug() << "❌ Validation réponse: currentUsername vide!";
        return;
    }

    QString answer = ui->lineEditResetAnswer->text().trimmed();

    if (answer.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez répondre à la question de sécurité");
        return;
    }

    qDebug() << "=== VÉRIFICATION RÉPONSE ===";
    qDebug() << "Utilisateur:" << currentUsername;
    qDebug() << "Question affichée:" << ui->labelQuestion->text();
    qDebug() << "Réponse fournie:" << answer;

    QString dbAnswer = getValue("reponse");

    qDebug() << "Réponse en base:" << dbAnswer;
    qDebug() << "Longueur réponse fournie:" << answer.length();
    qDebug() << "Longueur réponse base:" << dbAnswer.length();

    QString cleanAnswer = answer.toLower().trimmed();
    QString cleanDbAnswer = dbAnswer.toLower().trimmed();

    qDebug() << "Réponse nettoyée (fournie):" << cleanAnswer;
    qDebug() << "Réponse nettoyée (base):" << cleanDbAnswer;

    if (cleanAnswer == cleanDbAnswer) {
        qDebug() << "✅ Réponses correspondent!";
        openPinPage();
    } else {
        QMessageBox::warning(this, "Erreur", "Réponse incorrecte !");
        qDebug() << "❌ Réponses ne correspondent pas!";
        qDebug() << "  Fournie:" << answer;
        qDebug() << "  Base:" << dbAnswer;
        ui->lineEditResetAnswer->clear();
        ui->lineEditResetAnswer->setFocus();
    }
}

// Validation du PIN
void Login::onValidatePinClicked()
{
    if (currentUsername.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Erreur de session, revenez au début");
        backToLogin();
        return;
    }

    QString pin = ui->lineEditResetPIN->text().trimmed();

    if (pin.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez entrer le code PIN");
        return;
    }

    QRegularExpression regex("^[0-9]+$");
    if (!regex.match(pin).hasMatch()) {
        QMessageBox::warning(this, "Erreur", "Le PIN doit contenir uniquement des chiffres");
        ui->lineEditResetPIN->clear();
        return;
    }

    QString dbPin = getValue("pin");

    qDebug() << "=== VÉRIFICATION PIN ===";
    qDebug() << "PIN fourni:" << pin;
    qDebug() << "PIN en base:" << dbPin;

    if (pin == dbPin) {
        openNewPasswordPage();
        qDebug() << "✅ PIN correct!";
    } else {
        QMessageBox::warning(this, "Erreur", "Code PIN incorrect !");
        ui->lineEditResetPIN->clear();
        ui->lineEditResetPIN->setFocus();
        qDebug() << "❌ PIN incorrect";
    }
}

// Réinitialisation du mot de passe
void Login::onResetPasswordClicked()
{
    if (currentUsername.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Erreur de session");
        backToLogin();
        return;
    }

    QString newPass = ui->lineEditResetNewPass->text();
    QString confirm = ui->lineEditResetConfirm->text();

    if (newPass.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs sont obligatoires");
        return;
    }

    if (newPass != confirm) {
        QMessageBox::warning(this, "Erreur", "Les mots de passe ne correspondent pas");
        ui->lineEditResetConfirm->clear();
        ui->lineEditResetConfirm->setFocus();
        return;
    }

    if (newPass.length() < 6) {
        QMessageBox::warning(this, "Erreur", "Le mot de passe doit contenir au moins 6 caractères");
        return;
    }

    qDebug() << "=== RÉINITIALISATION MDP ===";
    qDebug() << "Utilisateur:" << currentUsername;

    QSqlQuery q;
    q.prepare("UPDATE employes SET mot_de_passe = :p WHERE nom = :nom");
    q.bindValue(":p", newPass);
    q.bindValue(":nom", currentUsername);

    if (q.exec()) {
        QMessageBox::information(this, "Succès", "Mot de passe réinitialisé avec succès !");
        qDebug() << "✅ Mot de passe mis à jour pour:" << currentUsername;
        backToLogin();
    } else {
        auto placeholder = QMessageBox::critical(
            this, "Erreur",
            "Erreur lors de la mise à jour du mot de passe:\n" +
                q.lastError().text());
        qDebug() << "❌ Erreur SQL:" << q.lastError().text();
    }
}

// Visibilité du mot de passe
void Login::togglePasswordVisibility(int state)
{
    if (ui->stackedWidgetLogin->currentIndex() == 0) {
        ui->lineEdit_mdp->setEchoMode(
            state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password
            );
    } else if (ui->stackedWidgetLogin->currentIndex() == 3) {
        ui->lineEditResetNewPass->setEchoMode(
            state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password
            );
        ui->lineEditResetConfirm->setEchoMode(
            state == Qt::Checked ? QLineEdit::Normal : QLineEdit::Password
            );
    }
}
