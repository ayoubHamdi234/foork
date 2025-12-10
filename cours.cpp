#include "cours.h"
#include <QDebug>
#include <QSqlError>
#include <QComboBox>


Cours::Cours(int id, QString date, QString heureDebut, QString heureFin,
             QString type, QString circuit, double tarif,int id_eleve, int idEmploye, QString matricule)
{
    this->id = id;
    this->date = date;
    this->heureDebut = heureDebut;
    this->heureFin = heureFin;
    this->type = type;
    this->circuit = circuit;
    this->tarif = tarif;
    this->id_eleve = id_eleve;
    this->idEmploye = idEmploye;
    this->matricule = matricule;
}

// ➕ Ajouter un cours (updated to include idEmploye and matricule)
bool Cours::ajouter()
{
    QSqlQuery query;
    query.prepare("INSERT INTO COURS (ID_COURS, DATE_COURS, HEURE_DEBUT, HEURE_FIN, "
                  "TYPE_COURS, CIRCUIT, TARIF, ID_ELEVE, ID_EMPLOYE, MATRICULE) "
                  "VALUES (:id, TO_DATE(:date, 'YYYY-MM-DD'), :heureDebut, :heureFin, "
                  ":type, :circuit, :tarif, :id_eleve, :idEmploye, :matricule)");

    query.bindValue(":id", id);
    query.bindValue(":date", date);
    query.bindValue(":heureDebut", heureDebut);
    query.bindValue(":heureFin", heureFin);
    query.bindValue(":type", type);
    query.bindValue(":circuit", circuit);
    query.bindValue(":tarif", tarif);
    query.bindValue(":id_eleve", id_eleve);
    query.bindValue(":idEmploye", idEmploye);
    query.bindValue(":matricule", matricule);

    if (!query.exec()) {
        qDebug() << "SQL Error:" << query.lastError().text();
        qDebug() << "Query executed:" << query.lastQuery();
        return false;
    }
    return true;
}




QSqlQueryModel* Cours::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT ID_COURS, DATE_COURS, HEURE_DEBUT, HEURE_FIN, TYPE_COURS, CIRCUIT, DUREE, TARIF, ID_ELEVE, ID_EMPLOYE, MATRICULE FROM COURS");
    return model;
}


// ❌ Supprimer un cours (unchanged)
bool Cours::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM COURS WHERE ID_COURS = :id");
    query.bindValue(":id", id);
    return query.exec();
}

// ✏️ Modifier un cours (updated)
bool Cours::modifier()
{
    QSqlQuery query;
    query.prepare("UPDATE COURS SET DATE_COURS = TO_DATE(:date, 'YYYY-MM-DD'), HEURE_DEBUT = :debut, "
                  "HEURE_FIN = :fin, TYPE_COURS = :type, CIRCUIT = :circuit, TARIF = :tarif, "
                  "ID_EMPLOYE = :idEmp, MATRICULE = :matricule "
                  "WHERE ID_COURS = :id");
    query.bindValue(":id", id);
    query.bindValue(":date", date);
    query.bindValue(":debut", heureDebut);
    query.bindValue(":fin", heureFin);
    query.bindValue(":type", type);
    query.bindValue(":circuit", circuit);
    query.bindValue(":tarif", tarif);
    query.bindValue(":idEmp", idEmploye);
    query.bindValue(":matricule", matricule);

    return query.exec();
}


// 📊 Charger les cours dans un QTableWidget (updated)
void Cours::chargerDansTable(QTableWidget *table)
{
    QSqlQuery query("SELECT ID_COURS, DATE_COURS, HEURE_DEBUT, HEURE_FIN, TYPE_COURS, CIRCUIT, TARIF, ID_EMPLOYE, MATRICULE FROM COURS");
    table->clearContents();
    table->setRowCount(0);
    table->setColumnCount(9);

    int row = 0;
    while (query.next()) {
        table->insertRow(row);
        for (int col = 0; col < 9; ++col) {
            table->setItem(row, col, new QTableWidgetItem(query.value(col).toString()));
        }
        row++;
    }
}
// 🖨️ Exporter en PDF

/*bool Cours::exporterPDF(QTableWidget *table, QWidget *parent)
{
    QString fileName = QFileDialog::getSaveFileName(parent, "Enregistrer en PDF", "", "*.pdf");
    if (fileName.isEmpty()) return false;
    if (!fileName.endsWith(".pdf")) fileName += ".pdf";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QPainter painter(&printer);
    if (!painter.isActive()) return false;

    double margin = 50;
    double y = margin;

    QRectF pageRectF = printer.pageRect(QPrinter::DevicePixel); // Use QRectF

    // --- Draw Logo ---
    QPixmap logo(":/images/logo.png"); // adjust path in your qrc
    if (!logo.isNull()) {
        double logoHeight = 50;
        double logoWidth = logo.width() * logoHeight / logo.height(); // maintain aspect ratio
        painter.drawPixmap(margin, y, logoWidth, logoHeight, logo);
    }

    // --- Draw App Name ---
    QFont appFont("Arial", 16, QFont::Bold);
    painter.setFont(appFont);
    int logoOffset = 60; // space to the right of logo
    painter.setPen(Qt::red);
    painter.drawText(margin + logoOffset, y + 30, "SMART");
    painter.setPen(Qt::green);
    painter.drawText(margin + logoOffset + 60, y + 30, "DRIVE");

    y += 70; // space after logo and app name

    // --- Draw Table Title ---
    QFont titleFont("Arial", 14, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    painter.drawText(QRectF(margin, y, pageRectF.width() - 2 * margin, 30),
                     Qt::AlignCenter, "TABLE COURS");
    y += 40;

    int rows = table->rowCount();
    int cols = table->columnCount();

    // --- Calculate column widths dynamically ---
    double tableWidth = pageRectF.width() - 2 * margin;
    double cellW = tableWidth / cols;
    double cellH = 30;

    // Optional: limit minimum cell width to 50 pixels
    if (cellW < 50) cellW = 50;

    // --- Draw column headers ---
    QFont headerFont("Arial", 10, QFont::Bold);
    painter.setFont(headerFont);
    for (int c = 0; c < cols; ++c) {
        QRectF rect(margin + c * cellW, y, cellW, cellH);
        painter.drawRect(rect);
        painter.drawText(rect, Qt::AlignCenter,
                         table->horizontalHeaderItem(c) ? table->horizontalHeaderItem(c)->text() : "");
    }
    y += cellH;

    // --- Draw table data ---
    QFont cellFont("Arial", 10);
    painter.setFont(cellFont);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            QRectF rect(margin + c * cellW, y, cellW, cellH);
            painter.drawRect(rect);
            QTableWidgetItem *item = table->item(r, c);
            painter.drawText(rect, Qt::AlignCenter, item ? item->text() : "");
        }
        y += cellH;

        // Check for page break
        if (y + cellH > pageRectF.height() - margin) {
            printer.newPage();
            y = margin;

            // Redraw column headers on new page
            painter.setFont(headerFont);
            for (int c = 0; c < cols; ++c) {
                QRectF rect(margin + c * cellW, y, cellW, cellH);
                painter.drawRect(rect);
                painter.drawText(rect, Qt::AlignCenter,
                                 table->horizontalHeaderItem(c) ? table->horizontalHeaderItem(c)->text() : "");
            }
            y += cellH;
            painter.setFont(cellFont);
        }
    }

    painter.end();
    QMessageBox::information(parent, "Succès", "PDF généré avec succès : " + fileName);
    return true;
}*/
bool Cours::exporterPDF(QTableWidget *table, QWidget *parent)
{
    QString fileName = QFileDialog::getSaveFileName(parent, "Enregistrer en PDF", "", "*.pdf");
    if (fileName.isEmpty()) return false;
    if (!fileName.endsWith(".pdf")) fileName += ".pdf";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QPainter painter(&printer);
    if (!painter.isActive()) return false;

    double topMargin = 50;
    double leftMargin = 20;   // moved table slightly left
    double rightMargin = 40;
    double y = topMargin;

    QRectF pageRectF = printer.pageRect(QPrinter::DevicePixel);

    // --- Draw Logo centered ---
    QPixmap logo(":/images/logo.png"); // adjust path in your qrc
    if (!logo.isNull()) {
        double logoHeight = 50;
        double logoWidth = logo.width() * logoHeight / logo.height();
        double centerX = pageRectF.width() / 2.0;
        painter.drawPixmap(centerX - logoWidth / 2, y, logoWidth, logoHeight, logo);
    }

    // --- Draw App Name centered below logo ---
    QFont appFont("Arial", 16, QFont::Bold);
    painter.setFont(appFont);
    QFontMetrics fm(appFont);
    double centerX = pageRectF.width() / 2.0;
    int totalTextWidth = fm.horizontalAdvance("SMARTDRIVE");
    painter.setPen(Qt::red);
    painter.drawText(centerX - totalTextWidth / 2, y + 60, "SMART");
    painter.setPen(Qt::green);
    painter.drawText(centerX - totalTextWidth / 2 + fm.horizontalAdvance("SMART"), y + 60, "DRIVE");

    y += 90; // move below logo + app name

    // --- Draw Table Title centered ---
    QFont titleFont("Arial", 14, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::black);
    painter.drawText(QRectF(leftMargin, y, pageRectF.width() - leftMargin - rightMargin, 30),
                     Qt::AlignCenter, "TABLE COURS");
    y += 40;

    int rows = table->rowCount();
    int cols = table->columnCount();

    // --- Calculate column widths dynamically ---
    double tableWidth = pageRectF.width() - leftMargin - rightMargin - 5; // extra padding
    double cellW = tableWidth / cols;
    double cellH = 30;
    if (cellW < 50) cellW = 50;

    // --- Draw column headers with smaller font ---
    QFont headerFont("Arial", 6, QFont::Bold); // smaller to fit long titles
    painter.setFont(headerFont);
    for (int c = 0; c < cols; ++c) {
        QRectF rect(leftMargin + c * cellW, y, cellW, cellH);
        painter.drawRect(rect);
        QString headerText = table->horizontalHeaderItem(c) ? table->horizontalHeaderItem(c)->text() : "";
        painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, headerText);
    }
    y += cellH;

    // --- Draw table cells ---
    QFont cellFont("Arial", 10);
    painter.setFont(cellFont);

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            QRectF rect(leftMargin + c * cellW, y, cellW, cellH);
            painter.drawRect(rect);

            QString text;
            QTableWidgetItem *item = table->item(r, c);
            QWidget *widget = table->cellWidget(r, c);

            if (widget) {
                QComboBox *combo = qobject_cast<QComboBox*>(widget);
                if (combo) text = combo->currentText();
            } else if (item) {
                text = item->text();
            }

            painter.drawText(rect, Qt::AlignCenter, text);
        }
        y += cellH;

        // --- Page break if needed ---
        if (y + cellH > pageRectF.height() - topMargin) {
            printer.newPage();
            y = topMargin;

            // redraw column headers on new page
            painter.setFont(headerFont);
            for (int c = 0; c < cols; ++c) {
                QRectF rect(leftMargin + c * cellW, y, cellW, cellH);
                painter.drawRect(rect);
                QString headerText = table->horizontalHeaderItem(c) ? table->horizontalHeaderItem(c)->text() : "";
                painter.drawText(rect, Qt::AlignCenter | Qt::TextWordWrap, headerText);
            }
            y += cellH;
            painter.setFont(cellFont);
        }
    }

    painter.end();
    QMessageBox::information(parent, "Succès", "PDF généré avec succès : " + fileName);
    return true;
}




QChartView* Cours::creerStatistiques(QWidget *parent)
{
    int codeCount = 0, conduiteCount = 0;
    QSqlQuery query("SELECT TYPE_COURS, COUNT(*) FROM COURS GROUP BY TYPE_COURS");

    while (query.next()) {
        QString type = query.value(0).toString().toLower();
        int count = query.value(1).toInt();
        if (type == "code") codeCount = count;
        else if (type == "conduite") conduiteCount = count;
    }

    QPieSeries *series = new QPieSeries();
    series->append("Code", codeCount);
    series->append("Conduite", conduiteCount);

    for (QPieSlice *slice : series->slices()) {
        slice->setLabel(QString("%1 (%2%)").arg(slice->label()).arg(int(slice->percentage() * 100)));
        slice->setLabelVisible(true);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->setBackgroundVisible(false);

    // ✅ assign to member variable
    chartView = new QChartView(chart, parent);
    chartView->setRenderHint(QPainter::Antialiasing);

    return chartView;
}

// 🔄 Rafraîchir les statistiques – Qt 5 compatible
void Cours::refreshStatistiques()
{
    if (!chartView) return; // safety

    int codeCount = 0, conduiteCount = 0;
    QSqlQuery query("SELECT TYPE_COURS, COUNT(*) FROM COURS GROUP BY TYPE_COURS");

    while (query.next()) {
        QString type = query.value(0).toString().toLower();
        int count = query.value(1).toInt();
        if (type == "code") codeCount = count;
        else if (type == "conduite") conduiteCount = count;
    }

    // Qt 5 safe cast
    QPieSeries *series = nullptr;
    if (!chartView->chart()->series().isEmpty()) {
        series = qobject_cast<QPieSeries*>(chartView->chart()->series().first());
    }

    if (!series) return; // safety check

    series->clear();
    series->append("Code", codeCount);
    series->append("Conduite", conduiteCount);

    for (QPieSlice *slice : series->slices()) {
        slice->setLabel(QString("%1 (%2%)").arg(slice->label()).arg(int(slice->percentage() * 100)));
        slice->setLabelVisible(true);
    }

    chartView->chart()->update();
}
