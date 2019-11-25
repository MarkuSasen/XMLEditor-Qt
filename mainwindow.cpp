#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardItemModel>
#include <QFileSystemModel>
#include <QFileDialog>
#include <QXmlStreamReader>
#include <QMessageBox>
#include "XMLModel.h"
#include <QDebug>
#include <iostream>
#include <QLineEdit>
#include <QSpinBox>

#define DELETOR(PTR) if(PTR) {delete PTR; PTR = nullptr;}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), model(nullptr)//, deps(nullptr)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    connect(ui->treeView, &QTreeView::clicked,
            this, &MainWindow::on_treeView_clicked);

    ui->treeView->header()->setStretchLastSection(true);
    ui->treeView->header()->setHighlightSections(true);
    ui->treeView->header()->setSectionsClickable(true);
    ui->treeView->header()->setSectionsMovable(true);
    ui->treeView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);

    ui->treeView->setItemDelegate(new LineEditDelegator());

    connect(ui->treeView->itemDelegate(), &QAbstractItemDelegate::commitData,
            this, &MainWindow::OnDataEdit);

}

MainWindow::~MainWindow()
{
    DELETOR(model)
    delete ui;
}


void MainWindow::on_treeView_clicked(const QModelIndex &index)
{

}
//Open

void MainWindow::on_OpenButt_clicked()
{
    QString OFile;

    OFile = QFileDialog::getOpenFileName(this, tr("Choose XML file"), QDir::currentPath(), tr("XML files (*.xml)"));
    if(OFile.isEmpty()) {
        return;
    }

    QFile xmlFile(OFile);
    if(!xmlFile.open(QIODevice::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, "Error", "Can't open the file");
        return;
    }

    DELETOR(model)
    //DELETOR(deps);
    model = new DepartmentsHolder(QStringList({"Отдел", "Кол-во сотрудников", "Средняя зп"}));


    XMLDepartmentsReader xml;
    switch(xml.readData(&xmlFile, model))
    {
        case 0: ui->lineEdit->setText("xml file opened"); break;
        case -1: ui->lineEdit->setText("Error: Cannot Open File"); DELETOR(model) return;
        default:
                ui->lineEdit->setText("Error: xml is not matching specified format!");
                DELETOR(model)
                return;
    }

    ui->treeView->setModel(model);

    ui->treeView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
    ui->treeView->setEditTriggers(QAbstractItemView::DoubleClicked);

    xmlFile.close();
}


//Добавить департамент
void MainWindow::on_addDep_clicked()
{
    if(!model) return;

    model->execCMD(new AddD(model,nullptr));

    ui->treeView->header()->resizeSections(QHeaderView::ResizeMode::ResizeToContents);
}



//Добавить Сотрудника
void MainWindow::on_addEmp_clicked()
{
    if(!model) return;
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();

    if(index.isValid() && model->parent(index) == QModelIndex())
    {
        model->execCMD(new AddE(model,model->getItem(index)));
    }

    ui->treeView->collapse(index);
    ui->treeView->expand(index);
    updateDepartments();
}


void MainWindow::on_savebutt_clicked()
{
    if(!model)
        return;
    QString OFile;

    OFile = QFileDialog::getSaveFileName(this,tr("Choose File"),QDir::currentPath());

    if(OFile.isEmpty()) {
        return;
    }

    QFile xmlFile(OFile);
    if(!xmlFile.open(QIODevice::Text | QIODevice::WriteOnly))
    {
        QMessageBox::warning(this,"Error", "Can't open the file");

        return;
    }

    XMLDepartmentsWriter xml;
    xml.writeData(&xmlFile, model);


    xmlFile.close();
}


void MainWindow::on_Undo_clicked()
{
    if(!model) return;
    model->undo();
    updateDepartments();
    ui->lineEdit->setText("Undo");
}

void MainWindow::on_Redo_clicked()
{
    if(!model) return;
    model->redo();
    updateDepartments();
    ui->lineEdit->setText("Redo");
}

/////////////////////////////


void MainWindow::on_delDep_clicked()
{

    if(!model) return;

    QModelIndex index = ui->treeView->selectionModel()->currentIndex();

    if(!index.isValid() || index.parent() != QModelIndex())
        return;

    model->execCMD(new RemD(model,model->getItem(index)));
}

void MainWindow::on_delEmp_clicked()
{
    if(!model) return;
    const QModelIndex index = ui->treeView->selectionModel()->currentIndex();

    if(!index.isValid() || index.parent() == QModelIndex())
        return;

    model->execCMD(new RemE(model,model->getItem(index)));

    updateDepartments();
}

void MainWindow::updateDepartments()
{
    for(int i = 0; i < model->rowCount(); i++)
    {
        QModelIndex index = model->index(i,0);
        static_cast<Department*>(model->getItem(index))->recalculate();
        for(int j = 0; j < 3; j++)
        {
            model->setData(model->index(i,j), model->getItem(index)->data(j), Qt::EditRole);
        }
    }
}


void MainWindow::OnDataEdit()
{
}

//////////////////////////////////////////////

QWidget *LineEditDelegator::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem &/* option */,
                                       const QModelIndex & index) const
{
    QWidget *editor;
    if(index.column() < 2)
    {
        editor = new QLineEdit(parent);
        static_cast<QLineEdit*>(editor)->setFrame(false);
    }
    else {
        editor = new QSpinBox(parent);
        static_cast<QSpinBox*>(editor)->setMaximum(0xFFFFFFF);
    }
    return editor;
}



void LineEditDelegator::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QString value = index.model()->data(index, Qt::EditRole).toString();
    if(index.column() < 2)
    {
        QLineEdit *spinBox = static_cast<QLineEdit*>(editor);
        spinBox->setText(value);
    }
    else {
        QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
        spinBox->setValue(value.toInt());
    }
}



void LineEditDelegator::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    if(index.parent() == QModelIndex() && index.column() > 0) return;

    QWidget *spinBox;
    QString value;
    if(index.column() >= 2)
    {
        spinBox = static_cast<QSpinBox*>(editor);
        value = static_cast<QSpinBox*>(spinBox)->text();
    }
    else
    {
        spinBox = static_cast<QLineEdit*>(editor);
        value = static_cast<QLineEdit*>(spinBox)->text();
    }

    DepartmentsHolder *h = static_cast<DepartmentsHolder*>(model);
    h->execCMD(new ChgD(h,h->getItem(index)));


    h->setData(index, value, Qt::EditRole);

    if(index.parent() != QModelIndex())
    {
        static_cast<Department*>(h->getItem(index.parent()))->recalculate();
        for(int i = 0; i < 3; i++)
        {
            h->setData(h->index(index.parent().row(),i), h->getItem(index.parent())->data(i), Qt::EditRole);
        }
    }
}



void LineEditDelegator::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


