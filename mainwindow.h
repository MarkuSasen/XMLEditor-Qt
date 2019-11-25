#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStyledItemDelegate>
#include "XMLModel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class DepartmentsHolder;

class MainWindow : public QMainWindow
{
    Q_OBJECT


    DepartmentsHolder *model;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void on_treeView_clicked(const QModelIndex &index);

    void on_OpenButt_clicked();

    void on_Undo_clicked();

    void OnDataEdit();

    void on_savebutt_clicked();

    //void on_EditButt_clicked();

    void on_Redo_clicked();

    void on_addDep_clicked();

    void on_addEmp_clicked();

    void on_delDep_clicked();

    void on_delEmp_clicked();

private:
    Ui::MainWindow *ui;

    void updateDepartments();
};


class LineEditDelegator : public QStyledItemDelegate
{
    Q_OBJECT

public:
    LineEditDelegator(QObject *parent = nullptr) : QStyledItemDelegate(parent){}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
};


#endif // MAINWINDOW_H
