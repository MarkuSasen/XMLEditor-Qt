#include "XMLModel.h"
#include <QMessageBox>
#include <iostream>
#include <QTreeWidget>
#include <QStandardItemModel>
#include <QDebug>


#define DELETOR(PTR) if(PTR) delete PTR

Employee::Employee(const QVector<QVariant> &data, TreeItem *parent) : TreeItem(data,parent)
{}


Employee::Employee(const Employee& empl) : TreeItem(empl.itemData,empl.parentItem)
{

}

Employee::~Employee(){}





/////////////////
/////////////////
/// /////////////
///

Department::Department(const QVector<QVariant> &data, TreeItem *parent) : TreeItem(data,parent)
{}

Department::Department(const Department& dep) : TreeItem(dep.itemData, dep.parentItem){}

void Department::recalculate()
{
    int empcount = childCount(); float Asal = 0.f;
    for(int i = 0; i < childCount(); ++i)
    {
        Asal += child(i)->data(2).toFloat();
    }
    Asal /= (float) empcount;
    setData(1,empcount);
    setData(2,Asal);
}


////////////////////
/////////////////////
/// /////////////////
/// ///////////////


TreeItem::TreeItem(const QVector<QVariant> &data, TreeItem *parent) : itemData(data), parentItem(parent)
{}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}


TreeItem *TreeItem::parent()
{
    return parentItem;
}

TreeItem *TreeItem::child(int number)
{
    if (number < 0 || number >= childItems.size())
        return nullptr;
    return childItems.at(number);
}


int TreeItem::  childNumber() const
{
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    return 0;
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    if (column < 0 || column >= itemData.size())
        return QVariant();
    return itemData.at(column);
}

bool TreeItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}




bool TreeItem::insertChildren(int position, int count, int columns)
{
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        TreeItem *item = new TreeItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool TreeItem::removeChildren(int position, int count)
{
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);


    return true;
}



bool TreeItem::insertColumns(int position, int columns)
{
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    for (TreeItem *child : qAsConst(childItems))
        child->insertColumns(position, columns);

    return true;
}

bool TreeItem::removeColumns(int position, int columns)
{
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    for (TreeItem *child : qAsConst(childItems))
        child->removeColumns(position, columns);

    return true;
}



TreeItem::Memento::Memento(TreeItem *wsave) : data(wsave->columnCount()), columns(wsave->columnCount())
{
    row = wsave->childNumber();
    for(int i = 0; i < columns; i++)
    {
        data[i] = wsave->data(i);
    }
}

TreeItem::Memento::Memento(const QVector<QVariant> &_data, int _row, int _column) :
    data(_data), row(_row), columns(_column)
{}

void TreeItem::Memento::restore(TreeItem* parent)
{
    if(hld->insertRows(row,1,hld->getIndexFromItem(parent)))
    {
        for(int i = 0; i < columns; i++)
            parent->child(row)->setData(i,data.at(i));
    }
}

void TreeItem::Memento::setDHL(DepartmentsHolder *hld)
{
    this->hld = hld;
}

TreeItem::Memento *TreeItem::MEM()
{
    return new Memento(this);
}


/////////////////
/////////////////
/// /////////////


DepartmentsHolder::DepartmentsHolder(const QStringList &headers, QObject *parent)
    : QAbstractItemModel(parent)
{
    QVector<QVariant> rootData;
    for (const QString &header : headers)
        rootData << header;

    rootItem = new TreeItem(rootData);
}

DepartmentsHolder::~DepartmentsHolder()
{
    delete rootItem;
    while(cmd_redoStack.size())
    {
        DELETOR(cmd_redoStack.top());
        cmd_redoStack.pop();
    }
    while(cmd_Stack.size())
    {
        DELETOR(cmd_Stack.top());
        cmd_Stack.pop();
    }
}

TreeItem *DepartmentsHolder::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

void DepartmentsHolder::undo()
{
    if(cmd_Stack.size())
    {
        cmd_redoStack.push(cmd_Stack.top());
        cmd_Stack.top()->undo();
        cmd_Stack.pop();
    }
}

void DepartmentsHolder::redo()
{
    if(cmd_redoStack.size())
    {
        cmd_Stack.push(cmd_redoStack.top());
        cmd_redoStack.top()->redo();
        cmd_redoStack.pop();
    }
}

void DepartmentsHolder::clearUndo()
{
//    while(undostate.size())
//    {
//        delete undostate.top();
//        undostate.pop();
//    }
}

void DepartmentsHolder::execCMD(Command *cmd)
{
    cmd->exec();
    cmd_Stack.push(cmd);
}

void DepartmentsHolder::clearRedos()
{
    while(cmd_redoStack.size())
   {
       delete cmd_redoStack.top();
       cmd_redoStack.pop();
   }
}

int DepartmentsHolder::rowCount(const QModelIndex &parent) const
{
    const TreeItem *parentItem = getItem(parent);

    return parentItem ? parentItem->childCount() : 0;
}

int DepartmentsHolder::columnCount(const QModelIndex &parent) const
{
    return rootItem->columnCount();
}

Qt::ItemFlags DepartmentsHolder::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}



QModelIndex DepartmentsHolder::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();
    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex DepartmentsHolder::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
    TreeItem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

QModelIndex DepartmentsHolder::getIndexFromItem(TreeItem *item) const
{
    for(int i = 0; i < rowCount(); i++)
    {
        TreeItem *node = rootItem->child(i);
        if(node == item) return index(i,0);
        for(int j = 0; j < node->childCount(); j++)
        {
            if(node->child(j) == item) return index(j,0,index(i,0));
        }
    }
    return QModelIndex();
}


bool DepartmentsHolder::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

    return result;
}

bool DepartmentsHolder::setHeaderData(int section, Qt::Orientation orientation,
                              const QVariant &value, int role)
{
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    const bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

bool DepartmentsHolder::insertColumns(int position, int columns, const QModelIndex &parent)
{
    beginInsertColumns(parent, position, position + columns - 1);
    const bool success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool DepartmentsHolder::insertRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return false;

    beginInsertRows(parent, position, position + rows - 1);
    const bool success = parentItem->insertChildren(position,
                                                    rows,
                                                    rootItem->columnCount());
    endInsertRows();

    return success;
}

bool DepartmentsHolder::removeColumns(int position, int columns, const QModelIndex &parent)
{
    beginRemoveColumns(parent, position, position + columns - 1);
    const bool success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool DepartmentsHolder::removeRows(int position, int rows, const QModelIndex &parent)
{
    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return false;

    beginRemoveRows(parent, position, position + rows - 1);
    const bool success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

bool DepartmentsHolder::hasChildren(const QModelIndex &parent) const
{
    return getItem(parent)->childCount();
}

QVariant DepartmentsHolder::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QVariant DepartmentsHolder::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    return item->data(index.column());
}


/////////////////
/////////////////
/// /////////////
///

XMLDepartmentsReader::XMLDepartmentsReader() : error(0) {

}


int XMLDepartmentsReader::readData(QIODevice *file, DepartmentsHolder *dh){

    buf = dh;
    if(!file->isOpen() || !buf)
        if(!file->open(QFile::ReadOnly))
        {
            return -1;
        }


    readxml.setDevice(file);
    readxml.readNextStartElement();

    //Q_ASSERT(readxml.name() == "departments");

    if(readxml.name() != "departments")
    {
        error = 1;
        readxml.raiseError("Ошибка парсинга xml, не найден тег <departments>!");
        return error;
    }

    while(readxml.readNextStartElement())
    {
        if(readxml.name() == "department")
        {
            readDepartment();
            if(error) return error;
        }
        else {
            readxml.skipCurrentElement();
        }
    }

    return error;
}

void XMLDepartmentsReader::readDepartment()
{
//    Q_ASSERT(readxml.isStartElement() && readxml.name() == "department");
//    Q_ASSERT(readxml.attributes().hasAttribute("name"));
    if(!(readxml.isStartElement() && readxml.name() == "department" && readxml.attributes().hasAttribute("name")))
    {
        error = 1;
        return;
    }

    bool ok = buf->insertRows(buf->rowCount(),1);

    if(ok)
    {
        Department *dep = nullptr;
        dep = static_cast<Department*>(buf->getItem(buf->index(buf->rowCount()-1,0)));
        dep->setData(0,readxml.attributes().value("name").toString());

        while(readxml.readNextStartElement()){
            if(readxml.name() == "employments")
            {
                readEmployees(dep);
            }
            else {
                readxml.skipCurrentElement();
            }
        }
        dep->recalculate();
    } else error = 1;
}

void XMLDepartmentsReader::readEmployees(Department *dep)
{
    if(!readxml.isStartElement() || readxml.name() != "employments")
    {
        error = 1;
        return;
    }

    while(readxml.readNextStartElement())
    {
        if(readxml.name() == "employment")
        {
            readEmployee(dep);
        } else readxml.skipCurrentElement();
    }
}

void XMLDepartmentsReader::readEmployee(Department *dep)
{
    if(!readxml.isStartElement() || readxml.name() != "employment")
    {
        error = 1;
        return;
    }

    bool ok = dep->insertChildren(dep->childCount(),1,3);
    if(!ok) {
        error = 1;
        return;
    }

    Employee *emp = static_cast<Employee*>(dep->child(dep->childCount()-1));
    QString FIO;

    while(readxml.readNextStartElement())
    {
        if(readxml.name() == "surname")
        {
            FIO += readxml.readElementText();

        }else if(readxml.name() == "name")
        {

            FIO += " ";
            FIO += readxml.readElementText();

        }else if(readxml.name() == "middleName")
        {

            FIO += " ";
            FIO += readxml.readElementText();

        }else if(readxml.name() == "function")
        {
            emp->setData(1, readxml.readElementText());
        }else if(readxml.name() == "salary")
        {
            emp->setData(2, readxml.readElementText().toFloat());
        } else readxml.skipCurrentElement();
    }
    emp->setData(0,FIO);

}

XMLDepartmentsReader::~XMLDepartmentsReader(){

}



/////////////////
/////////////////
/// /////////////
///
///
///
///





XMLDepartmentsWriter::XMLDepartmentsWriter() //_model(model)
{

}

XMLDepartmentsWriter::~XMLDepartmentsWriter()
{

}

int XMLDepartmentsWriter::writeData(QIODevice *file, const DepartmentsHolder* model)
{
    wrtxml.setDevice(file);
    wrtxml.setAutoFormatting(true);
    wrtxml.setAutoFormattingIndent(4);

    wrtxml.writeStartDocument();
    wrtxml.writeStartElement("departments");

    for(int i = 0; i < model->rowCount(); i++)
    {
        writeDepartment(static_cast<Department*>(model->getItem(model->index(i,0))));
    }

    wrtxml.writeEndElement();
    wrtxml.writeEndDocument();
    return 0;
}

void XMLDepartmentsWriter::writeDepartment(Department *dep)
{
    wrtxml.writeStartElement("department");
    wrtxml.writeAttribute("name", dep->data(0).toString());

        writeEmployees(dep);

    wrtxml.writeEndElement();
}

void XMLDepartmentsWriter::writeEmployees(Department *dep)
{
    wrtxml.writeStartElement("employments");

    for(int i = 0; i < dep->childCount(); i++)
    {
        writeEmployee(static_cast<Employee*>(dep->child(i)));
    }

    wrtxml.writeEndElement();
}

void XMLDepartmentsWriter::writeEmployee(Employee *e)
{
    wrtxml.writeStartElement("employment");

    QStringList F_I_O = e->data(0).toString().split(" ");
    while(F_I_O.count() < 3) F_I_O << "";

    wrtxml.writeTextElement("surname",F_I_O.value(F_I_O.count() - 3));
    wrtxml.writeTextElement("name",F_I_O.value(F_I_O.count() - 2));
    wrtxml.writeTextElement("middleName",F_I_O.value(F_I_O.count() - 1));
    wrtxml.writeTextElement("function",e->data(1).toString());
    wrtxml.writeTextElement("salary",e->data(2).toString());

    wrtxml.writeEndElement();
}

/////////////////////////////////////////////////////////////////////////////////////////

Command::Command(DepartmentsHolder *_dH, TreeItem::Memento *me) : model(_dH), mem(me)
{}

Command::~Command()
{
    DELETOR(mem);
}

void Command::setDH(DepartmentsHolder *dh)
{
    model = dh;
}







AddD::AddD(DepartmentsHolder *_dH, TreeItem::Memento *me) : Command(_dH, me), department_number(-1)
{

}

void AddD::exec()
{

    // model->getItem(QModelIndex());

    if(model->insertRows(model->rowCount(),1,QModelIndex())){

        TreeItem* item = model->getItem(model->index(model->rowCount()-1,0));

        item->setData(0,"[no name]");
        item->setData(1,0);
        item->setData(2,0);

        mem = item->MEM();
        mem->setDHL(model);
        department_number = item->childNumber();

        qDebug() << "Dep inserted at "  << item->childNumber();
    }
    model->clearRedos();
}

void AddD::undo()
{
    if(department_number >= 0)
    {
        model->removeRows(department_number,1,QModelIndex());
    }
}

void AddD::redo()
{
    if(department_number >= 0)
    {
        mem->restore(model->getItem(QModelIndex()));
    }
}






RemD::RemD(DepartmentsHolder *_dH, TreeItem* item) : Command(_dH,item->MEM()), department_number(item->childNumber())
{
    mem->setDHL(model);

    for(int i = 0; i < item->childCount(); i++)
    {
        children.append(item->child(i)->MEM());
        children[i]->setDHL(model);
    }
}

RemD::~RemD()
{
    for(auto &e : children)
        DELETOR(e);
}

void RemD::exec()
{
    model->removeRows(department_number,1,QModelIndex());
        qDebug () << "Department removed";
    model->clearRedos();
}

void RemD::undo(){
    mem->restore(model->getItem(QModelIndex()));
    for(auto &e : children)
        e->restore(model->getItem(model->index(department_number,0)));
}

void RemD::redo(){
    model->removeRows(department_number,1,QModelIndex());
}









AddE::AddE(DepartmentsHolder *_dH, TreeItem *parent) : Command(_dH, nullptr), emp_number(-1), dep_number(parent->childNumber())
{

}


void AddE::exec(){

    if(emp_number == -1)
    {
        Department* dep = static_cast<Department*>(model->getItem(QModelIndex())->child(dep_number));
        emp_number = dep->childCount();
        if(model->insertRows(dep->childCount(),1,model->getIndexFromItem(dep)))//dep->insertChildren(dep->childCount(),1,3))
        {
            emp_number = dep->childCount()-1;
            Employee* emp = static_cast<Employee*>(dep->child(emp_number));

                emp->setData(0,"[no name]");
                emp->setData(1,"[no func]");
                emp->setData(2,0);

                mem = emp->MEM();
                mem->setDHL(model);
        }
    }
    model->clearRedos();
}

void AddE::undo(){
    Department* dep = static_cast<Department*>(model->getItem(QModelIndex())->child(dep_number));
    model->removeRows(emp_number,1,model->getIndexFromItem(dep));
}

void AddE::redo(){
    Department* dep = static_cast<Department*>(model->getItem(QModelIndex())->child(dep_number));
    mem->restore(dep);
}










RemE::RemE(DepartmentsHolder *_dH, TreeItem *item) : Command(_dH, item->MEM()), dep_num(item->parent()->childNumber()),
    emp_num(item->childNumber())
{
    mem->setDHL(model);
}

void RemE::exec(){

    if(model->removeRows(emp_num,1,model->index(dep_num,0,QModelIndex())))
    {

    }
    model->clearRedos();
}

void RemE::undo(){
   mem->restore(model->getItem(QModelIndex())->child(dep_num));
}

void RemE::redo(){
   model->removeRows(emp_num,1,model->index(dep_num,0,QModelIndex()));
}



ChgD::ChgD(DepartmentsHolder *dh, TreeItem *item) : Command(dh,item->MEM()) , dep_num(-1), emp_num(-1), after(nullptr)
{
    QModelIndex ind = model->getIndexFromItem(item);
    if(ind.parent() == QModelIndex())
    {
        dep_num = ind.row();
        for(int i = 0; i < item->childCount(); i++)
        {
            children.append(item->child(i)->MEM());
            children[i]->setDHL(model);
        }
    }
    else{
        emp_num = ind.row();
        dep_num = ind.parent().row();
    }
    mem->setDHL(model);
}

ChgD::~ChgD()
{
    for (auto &e : children)
        DELETOR(e);
    DELETOR(after);
}

void ChgD::exec()
{
    model->clearRedos();
}

void ChgD::redo()
{
    if(emp_num >= 0)
    {
        model->removeRows(emp_num,1,model->index(dep_num,0));
        after->restore(model->getItem(model->index(dep_num,0)));
    }else
    {
        model->removeRows(dep_num,1);
        after->restore(model->getItem(QModelIndex()));

        for(auto &e : children)
            e->restore(model->getItem(model->index(dep_num,0)));
    }

}

void ChgD::undo()
{


    if(emp_num >= 0)
    {
        if(!after)
        {
            after = model->getItem(model->index(emp_num,0,model->index(dep_num,0)))->MEM();
            after->setDHL(model);
        }

        model->removeRows(emp_num,1,model->index(dep_num,0));
        mem->restore(model->getItem(model->index(dep_num,0)));
    }else
    {
        if(!after)
        {
            after = model->getItem(model->index(dep_num,0))->MEM();
            after->setDHL(model);
        }

        model->removeRows(dep_num,1);
        mem->restore(model->getItem(QModelIndex()));

        for(auto &e : children)
            e->restore(model->getItem(model->index(dep_num,0)));
    }

}
