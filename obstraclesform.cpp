#include "obstraclesform.h"
#include "ui_obstraclesform.h"
#include "obstracleshandler.h"
#include <QDebug>
#include <QStandardItemModel>
#include <QToolBar>
#include <QToolButton>
#include <QFileDialog>
#include <QSaveFile>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QtMath>
#include "listitemdelegate.h"
#include "tablemodel.h"
#include "searchmodel.h"
#include "waitingspinnerwidget.h"
#include "qgroupheaderview.h"
#include "filterpanel.h"
#include "databaseaccess.h"
#include "sidebar.h"
#include "sortsearchfiltertablemodel.h"
#include "checkboxitemdelegate.h"
#include "obstraclestyleditemdelegate.h"
#include "mapview.h"

ObstraclesForm::ObstraclesForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ObstraclesForm)
{
    ui->setupUi(this);
    exportButton = new QToolButton(this);
    exportButton->setEnabled(false);
    exportButton->setText(tr("Export"));
    exportButton->setIconSize(QSize(32, 32));
    exportButton->setIcon(QIcon(":/images/res/img/filesave.png"));
    exportButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

//    filterButton = new QToolButton(this);
//    filterButton->setText(tr("Filter"));
//    filterButton->setIconSize(QSize(32, 32));
//    filterButton->setIcon(QIcon(":/images/res/img/filter.png"));
//    filterButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    filterButton->setEnabled(false);

    toolBar = new QToolBar(this);
    toolBar->addWidget(exportButton);
//    toolBar->addWidget(filterButton);

    sideBar = new SideBar(this);

    ui->splitter->setSizes(QList<int>() << 150 << 300);

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(toolBar, 0, 0, 1, 2);
    layout->addWidget(ui->splitter, 1, 0, 2, 2);
    setLayout(layout);

    airfieldsModel = new QStandardItemModel(this);
    searchAirfieldsModel = new SearchModel(this);
    searchAirfieldsModel->setSourceModel(airfieldsModel);
    searchAirfieldsModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    ui->listView->setItemDelegate(new ListItemDelegate());
    ui->listView->setModel(searchAirfieldsModel);

//    ui->listView->setStyleSheet("QListView::item:selected { background: #66b3ff;color: white; }"
//                                "QListView::item:hover { background: #e6f2ff;color: black; }");

    obstraclesModel = new TableModel(this);
    sortSearchFilterTableModel = new SortSearchFilterTableModel(this);
    sortSearchFilterTableModel->setSourceModel(obstraclesModel);
    ui->tableView->setModel(sortSearchFilterTableModel);

    QGroupHeaderView *groupHeaderView = new QGroupHeaderView(Qt::Horizontal, ui->tableView);
//    groupHeaderView->setStyleSheet("QHeaderView::section { color: black;border: 0.5px solid #bfbfbf; }");
    groupHeaderView->setCheckable(true);
    obstraclesModel->setHorizontalHeaderLabels(QStringList() << tr("*")
                                                             << tr("ID")
                                                             << tr("Name")
                                                             << tr("Type of\n configuration")
                                                             << tr("Human\n settlement")
                                                             << tr("Location options | coordinate\n system")
                                                             << tr("Location options | latitude")
                                                             << tr("Location options | longitude")
                                                             << tr("Location options | latitude of\n center of\n arc/circle")
                                                             << tr("Location options | longitude of\n center of\n arc/circle")
                                                             << tr("Location options | arc/circle\n radius (m)")
                                                             << tr("Location options | horizontal\n accuracy (m)")
                                                             << tr("Height | orthometric\n height MSL (m)")
                                                             << tr("Height | relative\n height AGL (m)")
                                                             << tr("Height | vertical\n accuracy (m)")
                                                             << tr("Design parameters | type of\n material")
                                                             << tr("Design parameters | fragility")
                                                             << tr("Marking day | Yes/no")
                                                             << tr("Marking day | template")
                                                             << tr("Marking day | color")
                                                             << tr("Night marking | Yes/no")
                                                             << tr("Night marking | color")
                                                             << tr("Night marking | type of\n light")
                                                             << tr("Night marking | intensity")
                                                             << tr("Night marking | lights working\n time")
                                                             << tr("Night marking | compliance\n 14 ADJ. ICAO")
                                                             << tr("Data source | supplier")
                                                             << tr("Data source | date of\n submission"));

    ui->tableView->setHorizontalHeader(groupHeaderView);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->horizontalHeader()->setSortIndicator(1, Qt::AscendingOrder);
    ui->tableView->horizontalHeader()->setSortIndicatorShown(true);
    ui->tableView->horizontalHeader()->setSectionsClickable(true);
    ui->tableView->setItemDelegateForColumn(0, new CheckboxItemDelegate(this));
    ui->tableView->setItemDelegate(new ObstracleStyledItemDelegate(this));

    updateModelAirfields();

    if (airfieldsModel->rowCount() == 0) {
        spinner = new WaitingSpinnerWidget(this);
        spinner->setRoundness(70.0);
        spinner->setMinimumTrailOpacity(15.0);
        spinner->setTrailFadePercentage(70.0);
        spinner->setNumberOfLines(12);
        spinner->setLineLength(15);
        spinner->setLineWidth(4);
        spinner->setInnerRadius(10);
        spinner->setRevolutionsPerSecond(1);
        spinner->setColor(QColor(81, 4, 71));
        spinner->start();
        connect(airfieldsModel, SIGNAL(modelReset()), spinner, SLOT(stop()));
    }

    readSettings();

    connect(ui->listView, SIGNAL(clicked(QModelIndex)), this, SLOT(updateModelObstracles(QModelIndex)));
    connect(sortSearchFilterTableModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QVector<int>)), this, SLOT(enabledToolButton()));
    connect(exportButton, SIGNAL(clicked(bool)), this, SLOT(exportToFile()));
//    connect(filterButton, SIGNAL(clicked(bool)), this, SLOT(showFilterPanel()));
    connect(ui->searchLineEdit, SIGNAL(textChanged(QString)), searchAirfieldsModel, SLOT(setFilterRegExp(QString)));
    connect(sideBar, SIGNAL(searchTextChanged(QString)), sortSearchFilterTableModel, SLOT(setFilterRegExp(QString)));
    connect(sideBar, SIGNAL(changedFilterProperty(QString, QVariant)), sortSearchFilterTableModel, SLOT(setFilterProperty(QString, QVariant)));
    connect(sideBar, SIGNAL(filterRadius()), this, SLOT(setFilterRadius()));
    connect(sideBar, SIGNAL(displayObstracles(QVariant)), this, SLOT(showObstracles(QVariant)));
    connect(groupHeaderView, SIGNAL(clickedCheckBox(bool)), this, SLOT(setCheckedAllRowTable(bool)));
    connect(ui->tableView, SIGNAL(clicked(QModelIndex)), this, SLOT(showTags(QModelIndex)));
    connect(DatabaseAccess::getInstance(), SIGNAL(updated()), this, SLOT(updateModelObstracles()));
}

ObstraclesForm::~ObstraclesForm()
{
    writeSettings();
//    delete obstraclesHandler;
    delete ui;
}

void ObstraclesForm::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    sideBar->setGeometry(width() - 20, toolBar->height(), sideBar->size().width(), height() - toolBar->height());
}

void ObstraclesForm::writeSettings()
{
    QSettings settings;

    settings.beginGroup("geometry");
    settings.setValue(ui->splitter->objectName(), ui->splitter->saveState());
    settings.setValue(ui->tableView->objectName(), ui->tableView->horizontalHeader()->saveState());
    settings.endGroup();
}

void ObstraclesForm::readSettings()
{
    QSettings settings;

    settings.beginGroup("geometry");
    ui->splitter->restoreState(settings.value(ui->splitter->objectName()).toByteArray());
    ui->tableView->horizontalHeader()->restoreState(settings.value(ui->tableView->objectName()).toByteArray());
    settings.endGroup();
}

void ObstraclesForm::updateModelAirfields()
{
    QVector<QVariantList> airfields = DatabaseAccess::getInstance()->getAirfields();

    // remove all rows
    airfieldsModel->clear();

    for (QVector<QVariantList>::iterator it = airfields.begin(); it != airfields.end(); ++it) {
        QStandardItem *item = new QStandardItem;
        item->setData((*it).at(0).toString(), ListItemDelegate::NameAirfieldRole);
        item->setData((*it).at(1).toString(), ListItemDelegate::CodeICAORole);
        item->setData((*it).at(2).toUInt(), ListItemDelegate::IdRole);
        airfieldsModel->appendRow(item);
    }

    ui->listView->repaint();
}

void ObstraclesForm::updateModelObstracles(const QModelIndex &index)
{
    uint idAirfield = 0;
    if (index.isValid())
        idAirfield = searchAirfieldsModel->data(index, ListItemDelegate::IdRole).toUInt();
    else
        idAirfield = searchAirfieldsModel->data(ui->listView->currentIndex(), ListItemDelegate::IdRole).toUInt();

    QVector<QVariantList> obstracles = DatabaseAccess::getInstance()->getObstracles(idAirfield);
    // uncheked header
    qobject_cast<QGroupHeaderView*>(ui->tableView->horizontalHeader())->setChecked(false);
    // remove all rows
    while (obstraclesModel->rowCount() > 0) {
        obstraclesModel->removeRow(0);
    }

    for (int i = 0; i < obstracles.size(); i++) {
        QList<QStandardItem *> items;
        QVariantList fields = obstracles.at(i);

        QStandardItem *item = new QStandardItem();
        item->setData(fields.takeLast().toString(), Qt::UserRole + 1);      // set tags for obstracles to first column
        item->setData(fields.takeLast().toString(), Qt::UserRole + 2);      // set datetime last updated
        items.append(item);
        for (int j = 0; j < fields.size(); j++) {
            items.append(new QStandardItem(fields.at(j).toString()));
        }
        obstraclesModel->appendRow(items);
    }
}

void ObstraclesForm::enabledToolButton()
{
    bool isEnable = false;
    for (int row = 0; row < obstraclesModel->rowCount(); row++) {
        if (obstraclesModel->item(row)->data(Qt::UserRole).toBool()) {
            isEnable = true;
            break;
        }
    }
    exportButton->setEnabled(isEnable);
    return;
}

void ObstraclesForm::exportToFile()
{
    QString nameFile = QFileDialog::getSaveFileName(this, tr("Save file"), QString("C:/prep.txt"));
    if (nameFile.isEmpty()) {
        qDebug() << "Empty name save file";
        return;
    }

    QSaveFile file(nameFile);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << file.errorString();
        return;
    }

    QTextStream out(&file);
    for (int row = 0; row < obstraclesModel->rowCount(); row++) {
        if (obstraclesModel->item(row)->data(Qt::UserRole).toBool()) {
            out << obstraclesModel->item(row, 6)->text().replace("с", "N").remove(QRegExp("[\\s\\.]")) << endl;
            out << obstraclesModel->item(row, 7)->text().replace("в", "E").remove(QRegExp("[\\s\\.]")) << endl;
            out << obstraclesModel->item(row, 12)->text() << endl;
            out << endl;
            out << "1" << endl;
        }
    }
    file.commit();
}

void ObstraclesForm::showFilterPanel()
{
    FilterPanel *filterPanel = new FilterPanel(this);
    QPoint pos = ui->tableView->mapToGlobal(QPoint(0, 0));
    int widthTableView = ui->tableView->geometry().width();
    int heightTableView = ui->tableView->geometry().height();

    filterPanel->setGeometry(pos.x() + widthTableView / 2, pos.y() + heightTableView / 2,
                             filterPanel->width(), filterPanel->height());
    filterPanel->show();
}

void ObstraclesForm::setCheckedAllRowTable(bool checked)
{
    if (obstraclesModel->rowCount() == 0)
        return;

    for (int row = 0; row < obstraclesModel->rowCount(); row++)
        obstraclesModel->item(row)->setData(checked, Qt::UserRole);
}

void ObstraclesForm::setFilterRadius()
{
    sortSearchFilterTableModel->setFilterRadius(sideBar->getLat(), sideBar->getLon(), sideBar->getRadius());
}

QVariantList ObstraclesForm::getCheckedObstralcles()
{
    QVariantList idSelectedObstracles;

    for (int row = 0; row < obstraclesModel->rowCount(); row++) {
        if (obstraclesModel->item(row)->data(Qt::UserRole).toBool())
            idSelectedObstracles.append(obstraclesModel->item(row, 1)->data(Qt::DisplayRole).toString());
    }
    return idSelectedObstracles;
}

void ObstraclesForm::showTags(const QModelIndex &index)
{

}

void ObstraclesForm::showObstracles(QVariant coordinate)
{
    qDebug() << "showOBstracle" << coordinate;
    coordinate = QVariant(QPointF(55.7522, 37.6156));
    mapView = new MapView(coordinate);

    QVector<ObstraclePoint> obstracles;

    for (int row = 0; row < obstraclesModel->rowCount(); row++) {
        if (obstraclesModel->item(row)->data(Qt::UserRole).toBool()) {
            ObstraclePoint obstraclePoint;
            obstraclePoint.b= qDegreesToRadians(parserCoordinate(obstraclesModel->item(row, 6)->data(Qt::DisplayRole).toString()));
            obstraclePoint.l= qDegreesToRadians(parserCoordinate(obstraclesModel->item(row, 7)->data(Qt::DisplayRole).toString()));
            obstraclePoint.height = obstraclesModel->item(row, 12)->data(Qt::DisplayRole).toInt();
            obstracles.append(obstraclePoint);
        }
    }
    mapView->setData(obstracles);
    mapView->show();
}

double ObstraclesForm::parserCoordinate(QString str)
{
    QRegExp regExp("\\S(\\d{1,3})\\s(\\d{1,2})\\s([\\d\\.]+)");
    double coord = 0;

    if (regExp.indexIn(str) != -1) {
        coord = regExp.cap(1).toInt() + regExp.cap(2).toFloat() / 60 + regExp.cap(3).toFloat() / 3600;
    }

    return coord;
}
