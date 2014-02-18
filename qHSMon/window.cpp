/****************************************************************************
**
** Copyright (C) 2010 Michael Markstaller / Elaborated Networks GmbH.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the Free
** Software Foundation; either version 3 of the License, or (at your option)
** any later version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
** more details.
**
** You should have received a copy of the GNU General Public License
** in the file LICENSE.GPL included in the packaging of this file.
** Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
** If not, write to the Free Software Foundation, Inc.,
** 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
**
** This program is partly based on the the examples of the Qt Toolkit.
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
****************************************************************************/

#include <QtGui>
#include <QtNetwork>

#include "window.h"

Window::Window()
{
    createMonitorModel();
    //setSourceModel(createMonitorModel());
    proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(model);
    //slower
    proxyModel->setDynamicSortFilter(true);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

//    readSettings();
//    sourceGroupBox = new QGroupBox(tr("Original Model"));
    proxyGroupBox = new QGroupBox(tr("Live-Monitor"));

/*
    sourceView = new QTreeView;
    sourceView->setRootIsDecorated(false);
    sourceView->setAlternatingRowColors(true);
*/

    proxyView = new QTreeView;
    //dead slow! proxyView = new QTableView;
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setModel(proxyModel);
    proxyView->setSortingEnabled(true);
    //proxyView->setMaximumHeight(200);



    //QItemSelectionModel *selectionModel = proxyView->selectionModel();
    //proxyView->setSelectionBehavior(QAbstractItemView::SelectRows);
    //QItemSelectionModel::Rows
    //Qt::ItemIsSelectable
    // see address book example for selectionmodel -> send

    sortCaseSensitivityCheckBox = new QCheckBox(tr("Case sensitive sorting"));
    filterCaseSensitivityCheckBox = new QCheckBox(tr("Case sensitive filter"));

    filterPatternLineEdit = new QLineEdit;
    filterPatternLabel = new QLabel(tr("&Filter pattern:"));
    filterPatternLabel->setBuddy(filterPatternLineEdit);

    filterSyntaxComboBox = new QComboBox;
    filterSyntaxComboBox->addItem(tr("Regular expression"), QRegExp::RegExp);
    filterSyntaxComboBox->addItem(tr("Wildcard"), QRegExp::Wildcard);
    filterSyntaxComboBox->addItem(tr("Fixed string"), QRegExp::FixedString);
    filterSyntaxLabel = new QLabel(tr("Filter &syntax:"));
    filterSyntaxLabel->setBuddy(filterSyntaxComboBox);

    filterColumnComboBox = new QComboBox;
    QStringList colnames;
    colnames << "Log"<<"GA"<<"Name"<<"Count"<<"Time"<<"Value"<<"Type"<<"Init"<<"Min"
            <<"Max"<<"Step"<<"List"<<"GANum"<<"COGWs"<<"COGWr"<<"Id"<<"Used"
            <<"Path"<<"FMT"<<"Rem"<<"Scan"<<"SBC"<<"Read"<<"Transmit"<<"Length";
    filterColumnComboBox->addItems(colnames);
    filterColumnLabel = new QLabel(tr("Filter &column:"));
    filterColumnLabel->setBuddy(filterColumnComboBox);

    connect(filterPatternLineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(filterRegExpChanged()));
    connect(filterSyntaxComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(filterRegExpChanged()));
    connect(filterColumnComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(filterColumnChanged()));
    connect(filterCaseSensitivityCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(filterRegExpChanged()));
    connect(sortCaseSensitivityCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(sortChanged()));


/*
    QHBoxLayout *sourceLayout = new QHBoxLayout;
    sourceLayout->addWidget(sourceView);
    sourceGroupBox->setLayout(sourceLayout);
*/

    QGridLayout *proxyLayout = new QGridLayout;
    proxyLayout->addWidget(proxyView, 0, 0, 1, 3);
    proxyLayout->addWidget(filterPatternLabel, 1, 0);
    proxyLayout->addWidget(filterPatternLineEdit, 1, 1, 1, 2);
    proxyLayout->addWidget(filterSyntaxLabel, 2, 0);
    proxyLayout->addWidget(filterSyntaxComboBox, 2, 1, 1, 2);
    proxyLayout->addWidget(filterColumnLabel, 3, 0);
    proxyLayout->addWidget(filterColumnComboBox, 3, 1, 1, 2);
    proxyLayout->addWidget(filterCaseSensitivityCheckBox, 4, 0, 1, 2);
    proxyLayout->addWidget(sortCaseSensitivityCheckBox, 4, 2);
    proxyGroupBox->setLayout(proxyLayout);

    createOtherDockWindows();
    QVBoxLayout *mainLayout = new QVBoxLayout;

    //    mainLayout->addWidget(sourceGroupBox);
    mainLayout->addWidget(proxyGroupBox);
    //setLayout(mainLayout);
    setCentralWidget(proxyGroupBox);



    setWindowTitle(qApp->applicationName() + " " + qApp->applicationVersion());
    //resize(500, 450);
    //QMetaObject::invokeMethod(this, "loadSettings", Qt::QueuedConnection);

    //sortby ganum
    proxyView->sortByColumn(12, Qt::AscendingOrder);
    filterColumnComboBox->setCurrentIndex(1);
    createDockWindows();

    //context-menu global im Fenster:
    //setContextMenuPolicy(Qt::ActionsContextMenu);
    //addAction(copyAct);
    //geht ned
    //proxyView->contextMenuEvent(copyAct);
    //->contextMenuPolicy(Qt::ActionsContextMenu);

    //filterPatternLineEdit->setText("Andy|Grace");
    filterCaseSensitivityCheckBox->setChecked(false);
    sortCaseSensitivityCheckBox->setChecked(false);

    //model view signals for select/log
    connect(proxyView->selectionModel(),
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this, SLOT(selectionChangedAction(const QItemSelection &)));
    connect(proxyView,SIGNAL(clicked(const QModelIndex &)),
        this, SLOT(selectionClickedAction(const QModelIndex &)));
    //the real processing
    connect(&http, SIGNAL(done(bool)), this, SLOT(parseCO(bool)));
    connect(this, SIGNAL(parseDone()), this, SLOT(connectKOGW()));
    loadSettings();
}

void Window::selectionClickedAction(const QModelIndex &index)
{
    int myrow = proxyModel->mapToSource(index).row();
    int mycol = proxyModel->mapToSource(index).column();
    qDebug() << "CLICK!" << model->data(model->index(myrow, 0))
            << model->data(model->index(myrow, 1))
            << model->data(model->index(myrow, 2))
            << model->data(model->index(myrow, 12));
    if (mycol == 0) { //only click on checkbox-field!
        if (gaLogList.contains(model->data(model->index(myrow, 12)).toInt())) {
            //is already selected
            model->setData(model->index(myrow, 0),Qt::Unchecked,Qt::CheckStateRole);
            gaLogList.removeOne(model->data(model->index(myrow, 12)).toInt());
            //logfilter
            model->setData(model->index(myrow, 25),0);
        } else {
            //check it
            model->setData(model->index(myrow, 0),Qt::Checked,Qt::CheckStateRole);
            gaLogList.append(model->data(model->index(myrow, 12)).toInt());
            //logfilter
            model->setData(model->index(myrow, 25),1);
        }
    }
}

void Window::toggleSelectedRows()
{
    for (int myrow=0; myrow < model->rowCount();myrow++) {
        if (gaLogList.contains(model->data(model->index(myrow, 12)).toInt())) {
            //is already selected
            model->setData(model->index(myrow, 0),Qt::Unchecked,Qt::CheckStateRole);
            gaLogList.removeOne(model->data(model->index(myrow, 12)).toInt());
        } else {
            //check it
            model->setData(model->index(myrow, 0),Qt::Checked,Qt::CheckStateRole);
            gaLogList.append(model->data(model->index(myrow, 12)).toInt());
        }
    }
}


void Window::selectionChangedAction(const QItemSelection &selection)
{
     QModelIndexList indexes = selection.indexes();

     if (!indexes.isEmpty()) {
         //we only need the row but must be mapped from proxymodel to sourcemodel!
         int myrow = proxyModel->mapToSource(indexes.first()).row();
         //print GA,Name,ganum
         qDebug() << "SELECT" << indexes.count() << indexes.first().row()
                 << model->data(model->index(myrow, 1))
                 << model->data(model->index(myrow, 2))
                 << model->data(model->index(myrow, 12));
         //fill in GA in sendwindow
         selectedGA = model->data(model->index(myrow, 12)).toInt();
         QString gatype = model->data(model->index(myrow, 6)).toString();
         //set Input-validator for sendvalue
         if (gatype.compare("text")==0) {
             sendvalue->setInputMask("");
             sendvalue->setValidator(0);
             if (model->data(model->index(myrow, 12)).toInt() < 204800) { //internal
                sendvalue->setMaxLength(14);
             } else {
                sendvalue->setMaxLength(30000);
             }
             qDebug() << "sel text";
         } else if (gatype.compare("unknown")==0 or gatype.compare("integer")==0) {
             sendvalue->setInputMask("");
             sendvalue->setValidator(new QIntValidator(
                     model->data(model->index(myrow, 8)).toInt(), //min
                     model->data(model->index(myrow, 9)).toInt(), //max
                 sendvalue));
             sendvalue->setMaxLength(10);
             qDebug() << "sel int";
         } else if (gatype.compare("number")==0) {
             sendvalue->setInputMask("");
             sendvalue->setValidator(new QDoubleValidator(
                     model->data(model->index(myrow, 8)).toDouble(), //min
                     model->data(model->index(myrow, 9)).toDouble(), //max
                     4,sendvalue));
             sendvalue->setMaxLength(10);
             qDebug() << "sel number";
         } else if (gatype.compare("date")==0 or gatype.compare("time")==0) {
             sendvalue->setValidator(0);
             //dunno: sendvalue->setInputMask("00.00.0000");
             sendvalue->setInputMask("");
             sendvalue->setMaxLength(10);
             qDebug() << "sel datetime";
         }
         sendvalue->setEnabled(true);
         //sendvalue->clear();

         //editAct->setEnabled(true);
     } else {
         //?? empty
     }
}

void Window::setSourceModel(QAbstractItemModel *model)
{
    //useless was called from main.cpp
    //createMonitorModel();
    //proxyModel->setSourceModel(model);
//    sourceView->setModel(model);
}

void Window::filterRegExpChanged()
{
    QRegExp::PatternSyntax syntax =
            QRegExp::PatternSyntax(filterSyntaxComboBox->itemData(
                    filterSyntaxComboBox->currentIndex()).toInt());
    Qt::CaseSensitivity caseSensitivity =
            filterCaseSensitivityCheckBox->isChecked() ? Qt::CaseSensitive
                                                       : Qt::CaseInsensitive;

    QRegExp regExp(filterPatternLineEdit->text(), caseSensitivity, syntax);
    proxyModel->setFilterRegExp(regExp);
    //QRegExp regExp2(filterlogPatternLineEdit->text(), caseSensitivity, syntax);
    //proxylogModel->setFilterRegExp(regExp2);
}

void Window::filterColumnChanged()
{
    proxyModel->setFilterKeyColumn(filterColumnComboBox->currentIndex());
    //proxylogModel->setFilterKeyColumn(filterlogColumnComboBox->currentIndex());
}

void Window::sortChanged()
{
    proxyModel->setSortCaseSensitivity(
            sortCaseSensitivityCheckBox->isChecked() ? Qt::CaseSensitive
                                                     : Qt::CaseInsensitive);
}

void Window::about()
 {
    QMessageBox::about(this, tr("About Application"),
             tr("HS Monitor written in C++/Qt4\n\n"
                "(c) 2010 Michael Markstaller\n\n"
                "This program is free software; you can redistribute it and/or modify it"
                "under the terms of the GNU General Public License as published by the Free"
                "Software Foundation; either version 3 of the License, or (at your option)"
                "any later version. See included file LICENSE"));
 }

void Window::configDlg()
 {
    QSettings settings("ElabNET", "qHSMon");
    //settings.clear();
    bool ok;
    QString text = QInputDialog::getText(this, tr("HS IP/Hostname"),
                                           tr("HS IP/Hostname"), QLineEdit::Normal,
                                           settings.value("HSIP").toString(), &ok);
    if (ok && !text.isEmpty())
          settings.setValue("HSIP", text);

    int hsport = QInputDialog::getDouble(this, tr("KO-Gateway Port"),
                                 tr("KO-Gateway Port"),
                                 settings.value("HSPort",7003).toInt(),
                                 1, 65535, 0, &ok);
    if (ok)
        settings.setValue("HSPort", hsport);

    int httpport = QInputDialog::getDouble(this, tr("HS HTTP Port"),
                                 tr("HS HTTP Port"),
                                 settings.value("HSHttpPort",80).toInt(),
                                 1, 65535, 0, &ok);
    if (ok)
        settings.setValue("HSHttpPort", httpport);

    QString sectext = QInputDialog::getText(this, tr("Key"),
                                           tr("HS KO secret/key"), QLineEdit::Normal,
                                           settings.value("HSSecret").toString(), &ok);
    if (ok && !text.isEmpty())
          settings.setValue("HSSecret", sectext);

    settings.sync();

//    echoLineEdit = new QLineEdit;
//    echoLineEdit->setFocus();
 }

void Window::createActions()
{
/*
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
*/
    exitAct = new QAction(tr("E&xit"), this);
#if QT_VERSION >= 0x040600
    exitAct->setShortcuts(QKeySequence::Quit);
#endif
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
/*
    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, SIGNAL(triggered()), textEdit, SLOT(cut()));
*/
    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    //connect(copyAct, SIGNAL(triggered()), textEdit, SLOT(copy()));

    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    //connect(pasteAct, SIGNAL(triggered()), textEdit, SLOT(paste()));

    toggleSelectedAct = new QAction(tr("&Toggle"), this);
    toggleSelectedAct->setShortcuts(QKeySequence::SelectAll);
    toggleSelectedAct->setStatusTip(tr("Toggle selection rows to Log"));
    toggleSelectedAct->setToolTip(tr("Toggle selection rows to Log"));
    connect(toggleSelectedAct, SIGNAL(triggered()), this, SLOT(toggleSelectedRows()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

/*configDlg
    configAct = new QAction(tr("&Config"), this);
    configAct->setStatusTip(tr("Confgure connection"));
    connect(configAct, SIGNAL(triggered()), this, SLOT(configDlg()));
configDlg*/
    connectAct = new QAction(tr("&Connect"), this);
    connectAct->setStatusTip(tr("Connect!"));
    connect(connectAct, SIGNAL(triggered()), this, SLOT(connectHS()));

    /*
    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    connect(textEdit, SIGNAL(copyAvailable(bool)),
            cutAct, SLOT(setEnabled(bool)));
    connect(textEdit, SIGNAL(copyAvailable(bool)),
            copyAct, SLOT(setEnabled(bool)));
*/
}

void Window::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
/*
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
*/
    //fileMenu->addAction(configAct);
    fileMenu->addAction(connectAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    //editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(toggleSelectedAct);

    menuBar()->addSeparator();

    viewMenu = menuBar()->addMenu(tr("&View"));

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void Window::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    //fileToolBar->addAction(newAct);
    //fileToolBar->addAction(openAct);
    //fileToolBar->addAction(saveAct);
    //fileToolBar->addAction(configAct);
    fileToolBar->addAction(connectAct);
    fileToolBar->setObjectName("FileToolBar");

    editToolBar = addToolBar(tr("Edit"));
    //editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    editToolBar->addAction(toggleSelectedAct);
    editToolBar->setObjectName("EditToolBar");
}

void Window::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
    pb = new QProgressBar(statusBar());
    pb->setTextVisible(false);
    pb->hide();
    statusBar()->addPermanentWidget(pb);
    sblabel1 = new QLabel(this);
    statusBar()->addPermanentWidget(sblabel1);
    sblabel2 = new QLabel(this);
    statusBar()->addPermanentWidget(sblabel2);
    msgcounter=0;
}

void Window::closeEvent(QCloseEvent *)
{
    saveSettings();
}

void Window::saveSettings()
{
    QSettings settings("ElabNET", "qHSMon");
    // save window layout
    settings.beginGroup("mainWindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.endGroup();
    //settings.sync();
    csvLogFile.close();
    xmlLogFile.close();
}

void Window::loadSettings()
{
    // Load base settings
    QSettings settings("ElabNET", "qHSMon");
    //load mainwindow layout
    settings.beginGroup("mainWindow");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("state").toByteArray());
    settings.endGroup();
    maxlogrows = 5;
    // auto-connect
    if (!settings.value("HSIP").isNull()) {
        connectHS();
    }
}

void Window::connectHS()
{

    QSettings settings("ElabNET", "qHSMon");
    connectAct->setText("connecting..");
    model->removeRows(0,model->rowCount());

    file.setFileName(QDir::tempPath()+"/cobjects.xml");
    if (!file.open(QIODevice::ReadWrite)) {
        statusBar()->showMessage(tr("Error opening file cobjects.xml"));
        return;
    }

    http.setHost(settings.value("HSIP").toString(), settings.value("HSHttpPort").toUInt());
    QHttpRequestHeader header("GET", "/hscl?sys/cobjects.xml",1,0);
    header.setValue("Host", settings.value("HSIP").toString());

    statusBar()->showMessage(tr("Status: Downloading cobjects.xml ..."));
    http.request(header, 0, &file);

    return;
}

void Window::reconnectHS()
{

    connectAct->setText("reconnecting..");
    return;
}

void Window::parseCO(bool error)
{
    connectAct->setText("parsing..");
    if (error) {
        statusBar()->showMessage(tr("Status: Download cobjects.xml failed!"));
        //QMessageBox::critical (this, "Error",http.errorString() + http.errorString() + http.lastResponse().toString());
                  return;
    } else {
        statusBar()->showMessage(tr("Status: Download cobjects.xml success!"));
    }
   QTime t;
   t.start();
   file.seek(0);
   int allcount = 0;
   int addcount = 0;
   QXmlStreamReader xml(&file);
   QString tag;
   int counter = 0;
    while (!xml.atEnd()) {
          xml.readNext();
          if (xml.name()=="cobject") {
              allcount++;
              QXmlStreamAttributes attributes = xml.attributes();
              QXmlStreamAttributes attrs = xml.attributes();
              QList<QXmlStreamAttribute> list = attrs.toList();
              //we are dumb: 17 = internal, 21 = eib
              if (attrs.value( "ga" ).toString() > "") {
                  //or configed display all?
                  QString ganum = attrs.value( "ganum" ).toString();
                  addcount++;
                  sblabel1->setNum(addcount);
                  //int rownum = model->rowCount();
                  model->insertRow(0);
                  //array mapping item-index to ga
                  gamap[ganum.toInt()] = model->rowCount();
                  //cechkbox, result->dataChanged
                  model->setData(model->index(0, 0), Qt::Unchecked, Qt::CheckStateRole);
                  model->setData(model->index(0, 1), attrs.value( "ga" ).toString());
                  model->setData(model->index(0, 2), attrs.value( "name" ).toString());
                  model->setData(model->index(0, 6), attrs.value("fmtex").toString());
                  model->setData(model->index(0, 7), attrs.value( "init" ).toString());
                  model->setData(model->index(0, 8), attrs.value( "min" ).toString());
                  model->setData(model->index(0, 9), attrs.value( "max" ).toString());
                  model->setData(model->index(0, 10), attrs.value( "step" ).toString());
                  model->setData(model->index(0, 11), attrs.value( "list" ).toString());
                  model->setData(model->index(0, 12), ganum.toInt());
                  QString cogws = attrs.value( "cogws" ).toString();
                  model->setData(model->index(0, 13), cogws.toShort());
                  QString cogwr = attrs.value( "cogwr" ).toString();
                  model->setData(model->index(0, 14), cogwr.toShort());
                  model->setData(model->index(0, 15), attrs.value( "id" ).toString());
                  model->setData(model->index(0, 16), attrs.value( "used" ).toString());
                  model->setData(model->index(0, 17), attrs.value( "path" ).toString());
                  model->setData(model->index(0, 18), attrs.value( "fmt" ).toString());
                  model->setData(model->index(0, 19), attrs.value( "rem" ).toString());

                  if (attributes.count() == 21 ) {
                      model->setData(model->index(0, 20), attrs.value( "scan" ).toString());
                      model->setData(model->index(0, 21), attrs.value( "sbc" ).toString());
                      model->setData(model->index(0, 22), attrs.value( "read" ).toString());
                      model->setData(model->index(0, 23), attrs.value( "transmit" ).toString());
                  }
                  model->setData(model->index(0, 25), 0);
              }

                  counter++;
                  //if (counter > 10) { return; }
                  xml.readNext();
              }
    }

    if (xml.hasError()) {
        statusBar()->showMessage(tr("Error parsing cobjects.xml: ").append(xml.errorString()));
        QMessageBox::information(this, tr("qHSMon"),
                                 tr("The following error occurred parsing cobjects.xml: %1.")
                                 .arg(xml.errorString()));
        // DEBUG: No return on XML-error: return;
    }
    statusBar()->showMessage(tr("Parsed cobjects.xml successful! (%1)").arg(addcount));
    connectAct->setText("Reset connection");
    //,addcount,allcount
    qDebug() << "time to parse:" << (t.elapsed()/1000);
    file.close();
    file.remove();
    // Adjust colums
    proxyView->setColumnWidth(0,25);
    proxyView->resizeColumnToContents(1);
    proxyView->resizeColumnToContents(2);
    proxyView->resizeColumnToContents(3);
    proxyView->resizeColumnToContents(4);
    for (int i = 6; i<model->columnCount();++i) {
        proxyView->resizeColumnToContents(i);
    }
    // now the TCP-stuff
    emit parseDone();
}

void Window::createMonitorModel()
{
    model = new QStandardItemModel(0, 26, this);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Log"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("GA"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Name"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Count"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Time"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Value"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Type"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Init"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Min"));
    model->setHeaderData(9, Qt::Horizontal, QObject::tr("Max"));
    model->setHeaderData(10, Qt::Horizontal, QObject::tr("Step"));
    model->setHeaderData(11, Qt::Horizontal, QObject::tr("List"));
    model->setHeaderData(12, Qt::Horizontal, QObject::tr("GANum"));
    model->setHeaderData(13, Qt::Horizontal, QObject::tr("COGWs"));
    model->setHeaderData(14, Qt::Horizontal, QObject::tr("COGWr"));
    model->setHeaderData(15, Qt::Horizontal, QObject::tr("Id"));
    model->setHeaderData(16, Qt::Horizontal, QObject::tr("Used"));
    model->setHeaderData(17, Qt::Horizontal, QObject::tr("Path"));
    model->setHeaderData(18, Qt::Horizontal, QObject::tr("FMT"));
    model->setHeaderData(19, Qt::Horizontal, QObject::tr("Rem"));
    model->setHeaderData(20, Qt::Horizontal, QObject::tr("Scan"));
    model->setHeaderData(21, Qt::Horizontal, QObject::tr("Sbc"));
    model->setHeaderData(22, Qt::Horizontal, QObject::tr("Read"));
    model->setHeaderData(23, Qt::Horizontal, QObject::tr("Transmit"));
    model->setHeaderData(24, Qt::Horizontal, QObject::tr("Length"));
    model->setHeaderData(25, Qt::Horizontal, QObject::tr("Logfilter"));
}

void Window::createLogModel()
{
    logmodel = new QStandardItemModel(0, 5, this);
    logmodel->setHeaderData(0, Qt::Horizontal, QObject::tr("Timestamp"));
    logmodel->setHeaderData(1, Qt::Horizontal, QObject::tr("GA"));
    logmodel->setHeaderData(2, Qt::Horizontal, QObject::tr("Value"));
    logmodel->setHeaderData(3, Qt::Horizontal, QObject::tr("Name"));
    logmodel->setHeaderData(4, Qt::Horizontal, QObject::tr("Type"));
}

void Window::connectKOGW()
{
    QSettings settings("ElabNET", "qHSMon");
    QString koHost = settings.value("HSIP").toString();

    QByteArray koSecret (settings.value("HSSecret").toByteArray());
    koSecret.append(QChar::Null);

    qDebug() << "connecting KO-Gateway";
    tcpSocket = new QTcpSocket(this);
//    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
//            this, SLOT(displayError(QAbstractSocket::SocketError)));
    //tcpSocket->bind(45454);
    statusBar()->showMessage(tr("Connecting KO gateway..."));
    tcpSocket->connectToHost(koHost,
                              settings.value("HSPort").toInt());
    //if (tcpSocket->isOpen()) {
    pb->show();
    pb->setRange(0, tcpSocket->bytesAvailable());
    if (tcpSocket->waitForConnected(2000)) {
        connect(tcpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingData()));
        tcpSocket->write(koSecret);
        statusBar()->showMessage(tr("KO gateway connected"));
        //signal disconnected -> do it again sam
        connect(tcpSocket, SIGNAL(disconnected()),
            this, SLOT(connectKOGW()));
    } else {
        //failed
        tcpSocket->reset();
        pb->hide();
        statusBar()->showMessage(tr("Connection to KO gateway failed - will retry"));
        // try again later
        QTimer::singleShot(1000, this, SLOT(connectKOGW()));
    }
}

void Window::processPendingData()
{
    //char readchar;
    int i;
    bool completed;
    int mcount=0;
    //start timer
    QTime t;
    t.start();

    QByteArray arr = tcpSocket->readAll();
    pb->setRange(0, arr.size());
    pb->setValue(0);
    for (i = 0;i < arr.size();++i) {
        completed = false;
        message += arr.at(i);
        if (arr.at(i) == 0) {
            //disable signal for model->dataChanged?
#ifndef QT_NO_DEBUG
            //qDebug() << "1time: " << t.elapsed();
#endif
            mcount++;
            msgcounter++;
            pb->setValue(i);
            // shoot out message
            int type = message.section("|",0,0).toInt();
            int address = message.section("|",1,1).toInt();
            int rownum = model->rowCount() - gamap.value(address);
            QString value = message.section("|",2,2);
            value.chop(1);
#ifndef QT_NO_DEBUG
            //qDebug() << "2time: " << t.elapsed();
#endif
            QString straddress;
            straddress.sprintf("%d/%d/%d", (address >> 11) & 0xff, (address >> 8) & 0x07, (address) & 0xff);
            if (type == 1) { //Update
                gacounter[address]++;
                model->setData(model->index(rownum, 3), gacounter.value(address));
                model->setData(model->index(rownum, 4), QTime::currentTime());
                model->setData(model->index(rownum, 5), value); //slow/cpu-intense when sorted by value!
                model->setData(model->index(rownum, 24), value.length());

                if (gaLogList.contains(address) && logViewCheck->isChecked()) {//write to log-window
                    // Create and add a row in the treewidget/view for this entry.
                    QTreeWidgetItem *localitem = new QTreeWidgetItem(logWidget);
                    localitem->setText(0, QDate::currentDate().toString(Qt::ISODate).append(" ").append(QTime::currentTime().toString(Qt::ISODate)));
                    localitem->setText(1, straddress);
                    localitem->setText(2, value);
                    localitem->setToolTip(2, tr("Length: %1<br>")
                                     .arg(value.length()));
                    localitem->setText(3, model->data(model->index(rownum, 2)).toString());
                    localitem->setText(4, model->data(model->index(rownum, 6)).toString());
                    //item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                    //item->setTextAlignment(1, Qt::AlignHCenter);
                    //save to logfile?
                    localitem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
                    /*
                    if (logWidget->columnCount() > maxlogrows) {
                        //remove from bottom?
                    }
                    */

                }
                if (gaLogList.contains(address) && csvLogFile.isOpen()) {//write to CSV
                    *tsCSVLog << QDate::currentDate().toString(Qt::ISODate) << " "
                              << QTime::currentTime().toString(Qt::ISODate) << ","
                              << straddress << "," << value << ","
                              << model->data(model->index(rownum, 2)).toString() << ","
                              << model->data(model->index(rownum, 6)).toString() << "\n";
                            //.append(" ").append(QTime::currentTime().toString(Qt::ISODate));
                    //is this expensive/slow?
                    tsCSVLog->flush();
                }
                if (gaLogList.contains(address) && logXMLCheck->isChecked()) {//write to XML
                    //implement me
                }
            } else if (type == 2) { //Init
                model->setData(model->index(rownum, 3), 0);
                model->setData(model->index(rownum, 5), value); //slow/cpu-intense when sorted by value!
                model->setData(model->index(rownum, 24), value.length());
            }
#ifndef QT_NO_DEBUG
            //qDebug() << "3time: " << t.elapsed() << " msg:" << message;
#endif
            message = "";
            completed = true;
        }
    }
    //leftover bytes
    /*
    if (!completed) {
        qDebug() << "leftover " << message;
    }
    */
#ifndef QT_NO_DEBUG
    //dumb to avoid div/0 when msg exeeds 8192 bytes
    if (mcount==0) { mcount = 1; }
    qDebug() << "msize:" << arr.size() << " msgs:" << mcount << " permsg:" << t.elapsed()/mcount << " time: " << t.elapsed();
#endif
    sblabel2->setNum(msgcounter);
}

void Window::createDockWindows() {
    QDockWidget *dock = new QDockWidget(tr("Log"), this);
    //dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    logWidget = new QTreeWidget(dock);
    dock->setWidget(logWidget);
    //DEBUG?LAYOUT
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());
    dock->setObjectName("LogWindow");
    QStringList headers;
    headers << tr("Timestamp") << tr("GA") << tr("Value")
             << tr("Name") << tr("Type");
    logWidget->setHeaderLabels(headers);
    logWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    logWidget->setAlternatingRowColors(true);
    //logWidget->header()->setResizeMode(2,QHeaderView::Stretch);
    //logWidget->acceptDrops()
    logWidget->setRootIsDecorated(false);
    //slow?
    logWidget->setSortingEnabled(true);
    logWidget->setSelectionBehavior(QAbstractItemView::SelectItems);


    dock = new QDockWidget(tr("Send"), this);
    dock->setObjectName("Send");
    QWidget *sendWidget = new QWidget(dock);
    QGridLayout *sendLayout = new QGridLayout;
    sendvaluelabel = new QLabel(tr("Value"));
    //sendvaluelabel = QString().sprintf("%d/%d/%d", (*selectedGA >> 11) & 0xff, (*selectedGA >> 8) & 0x07, (*selectedGA) & 0xff);
    sendvalue = new QLineEdit();
    sendvalue->setEnabled(false);

    sendbutton = new QPushButton;
    sendbutton->setText("Send");
    sendbutton->setEnabled(false);

    base64Check = new QCheckBox(tr("to Base64"));
    base64Check->setToolTip("encode to Base64 before sending");
    rawdataCheck = new QCheckBox(tr("raw/Hex"));
    rawdataCheck->setToolTip("send raw Hex-Values i.e. 010304 for 0x01,0x03x,0x04");

    sendLayout->addWidget(sendvaluelabel,0,0);
    sendLayout->addWidget(sendvalue,0,1);
    sendLayout->addWidget(sendbutton,1,0);
    sendLayout->addWidget(base64Check,2,0);
    sendLayout->addWidget(rawdataCheck,2,1);

    dock->setWidget(sendWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());
    sendWidget->setLayout(sendLayout);

    //connect signal to enable send-button
    connect(sendvalue, SIGNAL(editingFinished()), this, SLOT(enableSendButton()));
    connect(sendvalue, SIGNAL(returnPressed()), this, SLOT(sendValue()));
    connect(sendbutton, SIGNAL(clicked()), this, SLOT(sendValue()));
    // connect keyboard-shortcut to sendbutton?

    QSettings settings("ElabNET", "qHSMon");
    dock = new QDockWidget(tr("Config"), this);
    dock->setObjectName("Config");
    QWidget *configWidget = new QWidget(dock);
    QGridLayout *configLayout = new QGridLayout;

    QLabel *lconfigHSIP = new QLabel(tr("HS IP"));
    configHSIP = new QLineEdit;
    configHSIP->setText(settings.value("HSIP").toString());
    //configHSIP->setInputMask("000.000.000.000;_");
    QLabel *lconfigHSPort = new QLabel(tr("HS Port"));
    configHSPort = new QSpinBox;
    configHSPort->setRange(0, 65535);
    configHSPort->setValue(settings.value("HSPort",7003).toInt());
    QLabel *lconfigHSHttpPort = new QLabel(tr("HS HTTP Port"));
    configHSHttpPort = new QSpinBox;
    configHSHttpPort->setRange(0, 65535);
    configHSHttpPort->setValue(settings.value("HSHttpPort",80).toInt());
    QLabel *lconfigHSSecret = new QLabel(tr("HS Secret"));
    configHSSecret = new QLineEdit;
    configHSSecret->setText(settings.value("HSSecret").toString());
    configHSSecret->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    logCSVCheck = new QCheckBox(tr("Log to CSV"));
    logCSVCheck->setChecked(settings.value("logCSV",0).toBool());
    logCSVCheck->setToolTip("Log to CSV in Homedir");
    logXMLCheck = new QCheckBox(tr("Log to XML"));
    logXMLCheck->setChecked(settings.value("logXML",0).toBool());
    logXMLCheck->setToolTip("Log to XML in Homedir");
    logViewCheck = new QCheckBox(tr("Live-Log"));
    logViewCheck->setChecked(settings.value("logView",1).toBool());
    logViewCheck->setToolTip("Live Log View");

    configLayout->addWidget(lconfigHSIP,0,0);
    configLayout->addWidget(configHSIP,0,1);
    configLayout->addWidget(lconfigHSPort,1,0);
    configLayout->addWidget(configHSPort,1,1);
    configLayout->addWidget(lconfigHSHttpPort,2,0);
    configLayout->addWidget(configHSHttpPort,2,1);
    configLayout->addWidget(lconfigHSSecret,3,0);
    configLayout->addWidget(configHSSecret,3,1);
    configLayout->addWidget(logCSVCheck,4,0);
    //configLayout->addWidget(logXMLCheck,4,1);
    configLayout->addWidget(logViewCheck,4,2);

    dock->setWidget(configWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());
    configWidget->setLayout(configLayout);

    //connect signal to save settings
    connect(configHSIP, SIGNAL(editingFinished()), this, SLOT(saveSettingsDock()));
    connect(configHSPort, SIGNAL(editingFinished()), this, SLOT(saveSettingsDock()));
    connect(configHSHttpPort, SIGNAL(editingFinished()), this, SLOT(saveSettingsDock()));
    connect(configHSSecret, SIGNAL(editingFinished()), this, SLOT(saveSettingsDock()));
    connect(logCSVCheck, SIGNAL(clicked()), this, SLOT(saveSettingsDock()));
    connect(logXMLCheck, SIGNAL(clicked()), this, SLOT(saveSettingsDock()));
    connect(logViewCheck, SIGNAL(clicked()), this, SLOT(saveSettingsDock()));
}

void Window::saveSettingsDock() {
    QSettings settings("ElabNET", "qHSMon");
    settings.setValue("HSIP", configHSIP->displayText());
    settings.setValue("HSPort", configHSPort->value());
    settings.setValue("HSHttpPort", configHSHttpPort->value());
    settings.setValue("HSSecret", configHSSecret->text());
    settings.setValue("logCSVCheck", logCSVCheck->isChecked());
    settings.setValue("logXMLCheck", logXMLCheck->isChecked());
    settings.setValue("logViewCheck", logViewCheck->isChecked());
    settings.sync();

    if (logCSVCheck->isChecked() and !csvLogFile.openMode()>0) {
        //open logfile
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                                    QDir::homePath() + "/qHSMon.csv",
                                    tr("CSV (*.csv)"),0,QFileDialog::DontConfirmOverwrite);
        csvLogFile.setFileName(fileName);
        if (!csvLogFile.open(QIODevice::Append)) {
            logCSVCheck->setChecked(0);
            return;
        }
        tsCSVLog = new QTextStream (&csvLogFile);
    } else if (!logCSVCheck->isChecked() and csvLogFile.openMode()>0){
        //close logfile
        csvLogFile.close();
    }

}

void Window::enableSendButton() {
    if (selectedGA && sendvalue->hasAcceptableInput()) {
        sendbutton->setEnabled(true);
    }
}

void Window::sendValue() {
    if (selectedGA && sendvalue->hasAcceptableInput()) {
        QByteArray message;
        message.append("1|");
        message.append(QByteArray::number(selectedGA));
        message.append("|");
        if (base64Check->isChecked()) {
            message.append(sendvalue->text().toAscii().toBase64());
        } else if (rawdataCheck->isChecked()) {
            message.append(QByteArray::fromHex(sendvalue->text().toAscii()));
        } else {
            message.append(sendvalue->text().toLatin1());
        }

        message.append(QChar::Null);
        if (tcpSocket->isWritable()) {
            tcpSocket->write(message);
            qDebug() << "sendvalue " << selectedGA << " val" << sendvalue->text();
            qDebug() << "msg " << message;
        }
    }
}

void Window::createOtherDockWindows() {
/*
    dock = new QDockWidget(tr("Paragraphs"), this);
    paragraphsList = new QListWidget(dock);
    paragraphsList->addItems(QStringList()
            << "Thank you for your payment which we have received today."
            << "Your order has been dispatched and should be with you "
               "within 28 days."
            << "We have dispatched those items that were in stock. The "
               "rest of your order will be dispatched once all the "
               "remaining items have arrived at our warehouse. No "
               "additional shipping charges will be made."
            << "You made a small overpayment (less than $5) which we "
               "will keep on account for you, or return at your request."
            << "You made a small underpayment (less than $1), but we have "
               "sent your order anyway. We'll add this underpayment to "
               "your next bill."
            << "Unfortunately you did not send enough money. Please remit "
               "an additional $. Your order will be dispatched as soon as "
               "the complete amount has been received."
            << "You made an overpayment (more than $5). Do you wish to "
               "buy more items, or should we return the excess to you?");
    dock->setWidget(paragraphsList);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());

    connect(customerList, SIGNAL(currentTextChanged(const QString &)),
            this, SLOT(insertCustomer(const QString &)));
*/
}

void Window::displayError(QAbstractSocket::SocketError socketError)
{
    /*
      doesn't work
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("qHSMon"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("qHSMon"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the server is running and reachable, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("qHSMon"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }

    connectAct->setText("Connect");
    */
}
