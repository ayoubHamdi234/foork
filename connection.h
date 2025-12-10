#ifndef CONNECTION_H
#define CONNECTION_H

#include "QtSql/qsqldatabase.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QMessageBox>

class Connection
{
public:
    Connection();
    bool createConnect();
    void closeConnect();

    QSqlDatabase db;
};

#endif // CONNECTION_H
