/****************************************************************************
**
** Copyright (C) 2009,2010 Hugues Luc BRUANT aka fullmetalcoder 
**                    <non.deterministic.finite.organism@gmail.com>
** 
** This file may be used under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation and appearing in the
** file GPL.txt included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "tilemqt.h"

/*!
	\file tilemqt.cpp
	\brief Implementation of the TilEmQt class
*/

#include "calcview.h"
#include "calcgrid.h"
#include "calclogview.h"
#include "calcdebugger.h"
#include "calcgridmanager.h"
#include "connectionmanager.h"

#include <QAction>
#include <QToolBar>
#include <QDockWidget>
#include <QFileDialog>

/*!
	\class TilEmQt
	\brief TilEmQt main window
	
	This class is responsible for top level GUI.
*/

TilEmQt::TilEmQt(QWidget *p)
 : QMainWindow(p)
{
	m_trayIcon = new QSystemTrayIcon(QIcon(""), this);
	m_trayIcon->setVisible(true);
	
	connect(m_trayIcon	, SIGNAL( activated(QSystemTrayIcon::ActivationReason) ),
			this		, SLOT  ( trayActivated(QSystemTrayIcon::ActivationReason) ));
	
	m_calcGrid = new CalcGrid(this);
	m_calcLogView = new CalcLogView(this);
	m_calcDebugger = new CalcDebugger(m_calcGrid, this);
	m_calcManager = new CalcGridManager(m_calcGrid);
	
	m_calcManager->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	m_calcDebugger->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	
	QAction *a;
	QToolBar *tb = addToolBar(tr("Emulation"));
	
	a = tb->addAction(tr("Quit"));
	connect(a, SIGNAL( triggered() ), qApp, SLOT( quit() ) );
	
	a = tb->addAction(tr("Pause"));
	m_calcGrid->connect(a, SIGNAL( triggered() ), SLOT( pause() ) );
	
	a = tb->addAction(tr("Add calc"));
	connect(a, SIGNAL( triggered() ), SLOT( addCalc() ) );
	
	setCentralWidget(m_calcGrid);
	
	QDockWidget *mgr = new QDockWidget(this);
	mgr->setWindowTitle(tr("Manager"));
	mgr->setWidget(m_calcManager);
	addDockWidget(Qt::LeftDockWidgetArea, mgr);
	tb->addAction(mgr->toggleViewAction());
	mgr->hide();
	
	QDockWidget *dbg = new QDockWidget(this);
	dbg->setWindowTitle(tr("Debugger"));
	dbg->setWidget(m_calcDebugger);
	addDockWidget(Qt::RightDockWidgetArea, dbg);
	tb->addAction(dbg->toggleViewAction());
	dbg->hide();
	
	QDockWidget *log = new QDockWidget(this);
	log->setWindowTitle(tr("Log"));
	log->setWidget(m_calcLogView);
	addDockWidget(Qt::BottomDockWidgetArea, log);
	tb->addAction(log->toggleViewAction());
	log->hide();
}

TilEmQt::~TilEmQt()
{
	
}

void TilEmQt::addCalc()
{
	QString rom = QFileDialog::getOpenFileName(this, tr("Choose a ROM file"));
	addCalc(rom);
}

void TilEmQt::addCalc(const QString& rom)
{
	if ( QFile::exists(rom) )
	{
		int c = m_calcGrid->calcCount();
		
		m_calcGrid->addCalc(rom);
		
		if ( c < m_calcGrid->calcCount() )
			m_calcLogView->addCalc(m_calcGrid->calc(c)->calc());
	}
}

void TilEmQt::closeEvent(QCloseEvent *e)
{
	e->ignore();
	hide();
}

void TilEmQt::trayActivated(QSystemTrayIcon::ActivationReason r)
{
	if ( isVisible() )
		hide();
	else
		show();
}
