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

#include <QApplication>
#include "window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("qHSMon");
    app.setOrganizationDomain("http://www.elabnet.de");
    app.setOrganizationName("ElabNET");
    app.setApplicationVersion("0.94");

    Window window;
    window.show();
    return app.exec();
}
