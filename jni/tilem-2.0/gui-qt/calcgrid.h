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

#ifndef _CALC_GRID_H_
#define _CALC_GRID_H_

/*!
	\file calcgrid.h
	\brief Definition of the CalcGrid class
*/

#include <QDebug>

#include <QScrollArea>

class QLayout;

class CalcView;
class CalcGridInternalWidget;

#define SCROLLABLE_CALC_GRID

#ifdef SCROLLABLE_CALC_GRID
#define CalcGridAncestor QScrollArea
#else
#define CalcGridAncestor QWidget
#endif

class CalcGrid : public CalcGridAncestor
{
	friend class CalcGridInternalWidget;
	
	Q_OBJECT
	
	public:
		enum DisplayMode
		{
			MergedWindows,
			FloatingWindows
		};
		
		CalcGrid(QWidget *p = 0);
		~CalcGrid();
		
		int calcCount() const;
		
		int index(CalcView *v) const;
		CalcView* calc(int idx) const;
		
		virtual QSize sizeHint() const;
		virtual QSize minimumSizeHint() const;
		
	public slots:
		void pause();
		void resume();
		
		int addCalc(CalcView *c);
		int addCalc(const QString& romfile);
		
		void removeCalc(int idx, bool del = true);
		void removeCalc(CalcView *c, bool del = true);
		
		void dockCalc(CalcView *v);
		void floatCalc(CalcView *v);
		
		void dockAllCalcs();
		void floatAllCalcs();
		
	signals:
		void beginAddCalc(int i);
		void endAddCalc();
		
		void beginRemoveCalc(int i);
		void endRemoveCalc();
		
		void focusedCalcChanged(CalcView *v);
		
	protected:
		void contextMenuEvent(QContextMenuEvent *e);
		
	protected slots:
		void paused();
		void resumed();
		
		void toggleDocking();
		
		void deleted(QObject *o);
		
	protected:
		virtual void closeEvent(QCloseEvent *e);
		virtual void keyPressEvent(QKeyEvent *e);
		
		virtual bool focusNextPrevChild(bool next);
		
		int focusedCalc() const;
		
	private:
		QLayout *m_grid;
		QList<CalcView*> m_calcs;
};

#endif
