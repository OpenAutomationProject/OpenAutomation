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

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QFile>
#include <QHttp>
#include <QTcpSocket>
#include <QtXml/QXmlStreamReader>
#include <QModelIndex>
#include <QItemSelection>

class QAbstractItemModel;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class QTreeView;
class QTableView;
class QAction;
class QMenu;
class QCloseEvent;
class QTcpSocket;
class QSettings;
class QUrl;
class QTcpSocket;
class QProgressBar;
class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;
class QPushButton;
class QCheckBox;
class QSpinBox;
class QFile;
class QTextStream;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window();

    void setSourceModel(QAbstractItemModel *model);

protected:
    void closeEvent(QCloseEvent *event);

signals:
    void parseDone();
    void selectionChanged (const QItemSelection &selected);
    void clicked(const QModelIndex &index);
    void valueChanged(const int &value);
    void stateChanged(const int &state);

private slots:
    void filterRegExpChanged();
    void filterColumnChanged();
    void sortChanged();
    void about();
    void saveSettings();
    void loadSettings();
    void configDlg();
    void connectHS();
    void connectKOGW();
    void parseCO(bool);
    void processPendingData();
    void reconnectHS();
    void displayError(QAbstractSocket::SocketError socketError);
    void selectionChangedAction(const QItemSelection &selection);
    void selectionClickedAction(const QModelIndex &index);
    void enableSendButton();
    void sendValue();
    void saveSettingsDock();
    void toggleSelectedRows();
    //clicked void changeCurrentSelect(const QModelIndex &current, const QModelIndex &previous);


private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWindows();
    void createOtherDockWindows();
    void createMonitorModel();
    void createLogModel();
    //void readSettings();
    //void writeSettings();

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    //QAction *newAct;
    //QAction *openAct;
    //QAction *saveAct;
    //QAction *saveAsAct;
    QAction *exitAct;
    //QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *connectAct;
    QAction *configAct;
    QAction *toggleSelectedAct;
    QMap<int, int> gamap;
    QMap<int, int> gacounter;
    int msgcounter;
    QLabel *sblabel1;
    QLabel *sblabel2;

    QAbstractItemModel *model;
    QAbstractItemModel *logmodel;
    QSortFilterProxyModel *proxyModel;
    QSortFilterProxyModel *proxylogModel;

    QGroupBox *sourceGroupBox;
    QGroupBox *proxyGroupBox;
    QGroupBox *proxylogGroupBox;
    QTreeView *sourceView;
    QTreeView *proxyView;
    //dead slow! QTableView *proxyView;
    QTreeView *proxylogView;
    QCheckBox *filterCaseSensitivityCheckBox;
    QCheckBox *sortCaseSensitivityCheckBox;
    QLabel *filterPatternLabel;
    QLabel *filterlogPatternLabel;
    QLabel *filterSyntaxLabel;
    QLabel *filterColumnLabel;
    QLabel *filterlogColumnLabel;
    QLineEdit *filterPatternLineEdit;
    QLineEdit *filterlogPatternLineEdit;
    QComboBox *filterSyntaxComboBox;
    QComboBox *filterColumnComboBox;
    QComboBox *filterlogColumnComboBox;
    QSettings *settings;
    QHttp http;
    //QTime *globaltime;
    QFile file;
    QByteArray httpdata;
    QXmlStreamReader xml;
    QXmlStreamAttributes attributes;
    QTcpSocket *tcpSocket;
    QString message;
    QProgressBar *pb;
    QList<int> gaLogList;
    int maxlogrows;
    QTreeWidget *logWidget;
    QTreeWidgetItem *item;
    QGridLayout *sendWidget;
    //QLabel *labels[4];
    //QLineEdit *lineEdits[4];
    int selectedGA;
    QLabel *sendvaluelabel;
    QLineEdit *sendvalue;
    QPushButton *sendbutton;
    QCheckBox *base64Check;
    QCheckBox *rawdataCheck;
    QLineEdit *configHSIP;
    QSpinBox *configHSPort;
    QSpinBox *configHSHttpPort;
    QLineEdit *configHSSecret;
    QCheckBox *logCSVCheck;
    QCheckBox *logXMLCheck;
    QCheckBox *logViewCheck;
    QFile csvLogFile;
    QFile xmlLogFile;
    QTextStream *tsCSVLog;
};

#endif

