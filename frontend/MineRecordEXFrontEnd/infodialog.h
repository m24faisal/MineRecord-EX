#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class InfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InfoDialog(QWidget *parent = nullptr);
    ~InfoDialog();

private slots:
    void onOkClicked();

private:
    void setupUI();

    QLabel *titleLabel;
    QLabel *infoLabel;
    QPushButton *okButton;
    QVBoxLayout *mainLayout;
};

#endif // INFODIALOG_H
