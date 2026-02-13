#include "infodialog.h"
#include <QApplication>
#include <QFont>

InfoDialog::InfoDialog(QWidget *parent) : QDialog(parent)
{
    setupUI();
    setWindowTitle("About MineRecordEX");
    setFixedSize(500, 400);
    setModal(true);
}

InfoDialog::~InfoDialog()
{
}

void InfoDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);

    // Create title label with larger font
    titleLabel = new QLabel("MineRecordEX", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // Create info label with application description
    infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft | Qt::AlignJustify);
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setText(
        "<p>MineRecordEX is a gaming utility software application that helps with creating gameplay recordings for the Minecraft video game. "
        "This software also allows the user to export their in-game stats to a viewable spreadsheet file for later reference. If you enjoyed"
        " using this program, please feel free to give a donation on my Ko-Fi page. This program is "
        "categorized under Free Open-Sourced Software (FOSS)</p>"
        "<p><b>Author: </b> Mahir Faisal</p>"
        "<p><b>Source Code Respository: </b><a href=https://github.com/m24faisal/MineRecord-EX >Github</a></p>"
        "<p><b>Donate: </b><a href=https://ko-fi.com/m24faisal >Ko-Fi"
        );

    // Create OK button with fixed width
    okButton = new QPushButton("OK", this);
    okButton->setFixedWidth(80);  // Set a fixed width of 80 pixels
    connect(okButton, &QPushButton::clicked, this, &InfoDialog::onOkClicked);

    // Add widgets to layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);

    // Create a horizontal layout for the button to center it
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();  // Add stretch before the button
    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();  // Add stretch after the button

    // Add the button layout to the main layout
    mainLayout->addLayout(buttonLayout);

    // Set margins and spacing - adjusted to position infoLabel higher
    mainLayout->setContentsMargins(20, 10, 20, 20);  // Reduced top margin from 20 to 10
    mainLayout->setSpacing(5);  // Further reduced spacing from 10 to 5

    // Set stretch factors to control layout
    mainLayout->setStretchFactor(titleLabel, 0);
    mainLayout->setStretchFactor(infoLabel, 3);

    // Set a fixed height for the title to reduce space
    titleLabel->setFixedHeight(30);
}

void InfoDialog::onOkClicked()
{
    accept(); // Close the dialog
}
