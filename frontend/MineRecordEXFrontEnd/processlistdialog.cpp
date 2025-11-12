#include "processlistdialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QHBoxLayout>

ProcessListDialog::ProcessListDialog(QWidget *parent)
    : QDialog(parent), processTable(new QTableWidget(this)),
    addButton(new QPushButton("Add Process", this)),
    closeButton(new QPushButton("Close", this)),
    mainLayout(new QVBoxLayout(this)),
    buttonLayout(new QHBoxLayout())
{
    setupUI();
    populateProcessList();
}

ProcessListDialog::~ProcessListDialog()
{
}

void ProcessListDialog::setupUI()
{
    // Set up the table
    processTable->setColumnCount(3);
    processTable->setHorizontalHeaderLabels({"PID", "Name", "Window Title"});
    processTable->horizontalHeader()->setStretchLastSection(true);
    processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    processTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    processTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Set up the buttons
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(closeButton);

    // Set up the main layout
    mainLayout->addWidget(processTable);
    mainLayout->addLayout(buttonLayout);

    // Connect signals and slots
    connect(addButton, &QPushButton::clicked, this, &ProcessListDialog::onAddProcessClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    setLayout(mainLayout);
    setWindowTitle("Running Processes");
    resize(600, 400);
}

void ProcessListDialog::populateProcessList()
{
    QProcessList processes = QProcessInfo::enumerate();

    processTable->setRowCount(processes.size());

    for (int i = 0; i < processes.size(); ++i)
    {
        const QProcessInfo &info = processes.at(i);

        QTableWidgetItem *pidItem = new QTableWidgetItem(QString::number(info.pid()));
        QTableWidgetItem *nameItem = new QTableWidgetItem(info.name());
        QTableWidgetItem *titleItem = new QTableWidgetItem(info.windowTitle());

        processTable->setItem(i, 0, pidItem);
        processTable->setItem(i, 1, nameItem);
        processTable->setItem(i, 2, titleItem);
    }
}

void ProcessListDialog::onAddProcessClicked()
{
    QList<QTableWidgetItem*> selectedItems = processTable->selectedItems();

    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a process first.");
        return;
    }

    // Get the selected row
    int row = selectedItems.first()->row();

    // Create a QProcessInfo object for the selected process
    QProcessInfo selectedProcess;
    selectedProcess.setPid(processTable->item(row, 0)->text().toUInt());
    selectedProcess.setName(processTable->item(row, 1)->text());
    selectedProcess.setWindowTitle(processTable->item(row, 2)->text());

    // Emit the signal with the selected process
    emit processSelected(selectedProcess);

    // Close the dialog
    accept();
}

QProcessInfo ProcessListDialog::getSelectedProcess() const
{
    QList<QTableWidgetItem*> selectedItems = processTable->selectedItems();

    if (selectedItems.isEmpty()) {
        return QProcessInfo(); // Return an empty process if nothing is selected
    }

    // Get the selected row
    int row = selectedItems.first()->row();

    // Create and return a QProcessInfo object for the selected process
    QProcessInfo selectedProcess;
    selectedProcess.setPid(processTable->item(row, 0)->text().toUInt());
    selectedProcess.setName(processTable->item(row, 1)->text());
    selectedProcess.setWindowTitle(processTable->item(row, 2)->text());

    return selectedProcess;
}

void ProcessListDialog::refresh()
{
    populateProcessList();
}
