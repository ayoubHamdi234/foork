#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>

namespace Ui {
class Login;
}

class Login : public QDialog
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();

signals:
    void loginSuccessful();  // Signal quand la connexion réussit

private slots:
    // Navigation
    void openForgotPasswordPage();
    void openQuestionPage();
    void openPinPage();
    void openNewPasswordPage();
    void backToLogin();

    // Authentification
    void on_pushButton_login_clicked();  // CORRIGEZ LE NOM

    // Vérification
    void onVerifyUsernameChecked(int state);
    void onValidateQuestionClicked();
    void onValidatePinClicked();
    void onResetPasswordClicked();

    // UI
    void togglePasswordVisibility(int state);

private:
    Ui::Login *ui;
    QString currentUsername;

    // Méthodes privées
    QString getValue(QString column);
    void clearForgotPasswordFields();
    void debugDatabaseContent();

    // Méthode d'authentification
    bool authenticateUser(const QString& username, const QString& password);
};

#endif // LOGIN_H
