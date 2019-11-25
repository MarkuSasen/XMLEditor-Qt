#ifndef XMLMODEL_H
#define XMLMODEL_H

#include <QString>
#include <QAbstractItemModel>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <stack>
#include <QTreeView>
#include <QStandardItem>
#include <QDebug>

class Department;
class DepartmentsHolder;
class Employee;

class XMLDepartmentsReader
{

public:
        XMLDepartmentsReader();
        ~XMLDepartmentsReader();

    int readData(QIODevice *file, DepartmentsHolder* hld);

private:
    QXmlStreamReader readxml;

    void readDepartment();
    void readEmployees(Department *dep);
    void readEmployee(Department *dep);
    DepartmentsHolder* buf;

    int error;
};

class XMLDepartmentsWriter
{

public:
    XMLDepartmentsWriter();
    ~XMLDepartmentsWriter();

    int writeData(QIODevice *file, const DepartmentsHolder* dep);

private:

    QXmlStreamWriter wrtxml;
    DepartmentsHolder* buf;

    void writeDepartment(Department *dep);
    void writeEmployees(Department *dep);
    void writeEmployee(Employee *empl);

};

class TreeItem
{
public:
    explicit TreeItem(const QVector<QVariant> &data, TreeItem *parent = nullptr);
    virtual ~TreeItem();

    virtual TreeItem *child(int number);
    virtual int childCount() const {return childItems.count();}
    virtual int columnCount() const;
    virtual QVariant data(int column) const;

    virtual bool insertChildren(int position, int count, int columns);
    virtual bool insertColumns(int position, int columns);
    virtual TreeItem *parent();

    virtual bool removeChildren(int position, int count);
    virtual bool removeColumns(int position, int columns);
    virtual int childNumber() const;
    virtual bool setData(int column, const QVariant &value);

protected:

    QVector<TreeItem*> childItems;
    QVector<QVariant> itemData;
    TreeItem *parentItem;

public:

    class Memento{
        QVector<QVariant> data;
        int row, columns; // relative to parent
        DepartmentsHolder* hld;

    public:
        Memento(TreeItem* wsave);
        Memento(const QVector<QVariant>& _data, int _row, int _column);
        void restore(TreeItem* parent);

        void setDHL(DepartmentsHolder *hld);
    };


    Memento* MEM();
};

class Employee : public TreeItem {


public:
    Employee(const QVector<QVariant> &data, TreeItem *parent = nullptr);
    Employee(const Employee& empl);
    ~Employee();
};


class Department : public TreeItem {

public:
        Department(const QVector<QVariant> &data, TreeItem *parent = nullptr);
        Department(const Department& dep);

        void recalculate();
};

class Command;

class DepartmentsHolder : public QAbstractItemModel {

    friend class Command;

    std::stack<Command*> cmd_Stack;
    std::stack<Command*> cmd_redoStack;
    Q_OBJECT
public:
    DepartmentsHolder(const QStringList &headers,
              QObject *parent = nullptr);
    ~DepartmentsHolder() override;

       QVariant data(const QModelIndex &index, int role) const override;
           QVariant headerData(int section, Qt::Orientation orientation,
                               int role = Qt::DisplayRole) const override;

           QModelIndex index(int row, int column,
                             const QModelIndex &parent = QModelIndex()) const override;
           QModelIndex parent(const QModelIndex &index) const override;
            QModelIndex getIndexFromItem(TreeItem* item) const;
           int rowCount(const QModelIndex &parent = QModelIndex()) const override;
           int columnCount(const QModelIndex &parent = QModelIndex()) const override;



           Qt::ItemFlags flags(const QModelIndex &index) const override;
           bool setData(const QModelIndex &index, const QVariant &value,
                        int role = Qt::EditRole) override;
           bool setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role = Qt::EditRole) override;

           bool insertColumns(int position, int columns,
                              const QModelIndex &parent = QModelIndex()) override;
           bool removeColumns(int position, int columns,
                              const QModelIndex &parent = QModelIndex()) override;
           bool insertRows(int position, int rows,
                           const QModelIndex &parent = QModelIndex()) override;
           bool removeRows(int position, int rows,
                           const QModelIndex &parent = QModelIndex()) override;


           bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

           TreeItem *getItem(const QModelIndex &index) const;


            void undo();
            void redo();
            void clearUndo();


            void execCMD(Command* cmd);
            void clearRedos();

       private:

           TreeItem *rootItem;

};


class Command {

protected:
    DepartmentsHolder* model;
    TreeItem::Memento* mem;
public:
    Command(DepartmentsHolder* _dH, Department::Memento* me);
    virtual ~Command();
    virtual void undo() = 0;
    virtual void redo() = 0;

    virtual void exec() = 0;

    inline void setDH(DepartmentsHolder* dh);
};


class AddD : public Command {
protected:
    int department_number;
public:
    AddD(DepartmentsHolder* _dH, Department::Memento* me);

    virtual void exec() override;

    virtual void undo() override;

    virtual void redo() override;
};

class RemD : public Command {
protected:
    int department_number;
    QVector<TreeItem::Memento*> children;
public:
    RemD(DepartmentsHolder* _dH, TreeItem* item);
    ~RemD() override;
    virtual void exec() override;

    virtual void undo() override;

    virtual void redo() override;

};

class AddE : public Command{
protected:
    int emp_number,dep_number;
public:
    AddE(DepartmentsHolder* _dH, TreeItem* parent);

    virtual void exec() override;

    virtual void undo() override;

    virtual void redo() override;
};

class RemE : public Command{
protected:
    int dep_num, emp_num;
public:
    RemE(DepartmentsHolder* _dH, TreeItem *item);


    virtual void exec() override;

    virtual void redo() override;

    virtual void undo() override;
};


class ChgD : public Command{
protected:
    int dep_num, emp_num;
    TreeItem::Memento* after;
    QVector<TreeItem::Memento*> children;
public:
    ChgD(DepartmentsHolder* dh, TreeItem* item);
    ~ChgD() override;

    virtual void exec() override;

    virtual void redo() override;

    virtual void undo() override;
};

#endif // XMLMODEL_H
