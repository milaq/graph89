/****************************************************************************
**
** Copyright (C) 2010 Hugues Luc BRUANT aka fullmetalcoder 
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

#ifndef _CALC_LOG_VIEW_H_
#define _CALC_LOG_VIEW_H_

/*!
	\file calclogview.h
	\brief Definition of the CalcLogView class
*/

#include <QListView>

#include <tilem.h>

class Calc;
class CalcLogModel;

class QSortFilterProxyModel;

class CalcLogView : public QListView
{
	Q_OBJECT
	
	public:
		CalcLogView(QWidget *p = 0);
		
	protected:
		virtual void contextMenuEvent(QContextMenuEvent *e);
		
	public slots:
		void addCalc(Calc *c);
		
	private slots:
		void log(const QString& message, int type, dword addr);
		
	private:
		CalcLogModel *m_model;
		QSortFilterProxyModel *m_proxy;
};

#endif
