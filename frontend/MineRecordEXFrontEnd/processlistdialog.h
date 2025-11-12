#ifndef PROCESSLISTDIALOG_H
#define PROCESSLISTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include "qprocessinfo.h"

class QVBoxLayout;
class QPushButton;
class QHBoxLayout;

class ProcessListDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessListDialog(QWidget *parent = nullptr);
    ~ProcessListDialog();

    void refresh(); // Public method to refresh the list
    QProcessInfo getSelectedProcess() const; // Returns the selected process

signals:
    void processSelected(const QProcessInfo &processInfo); // Signal emitted when "Add Process" is clicked

private slots:
    void onAddProcessClicked();

private:
    void setupUI();
    void populateProcessList();

    QTableWidget *processTable;
    QPushButton *addButton;
    QPushButton *closeButton;
    QVBoxLayout *mainLayout;
    QHBoxLayout *buttonLayout;
};

#endif // PROCESSLISTDIALOG_H
