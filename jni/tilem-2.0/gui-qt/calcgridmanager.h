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

#ifndef _CALC_GRID_MANAGER_H_
#define _CALC_GRID_MANAGER_H_

/*!
	\file calcgridmanager.h
	\brief Definition of the CalcGridManager class
*/

#include <QTreeView>

class CalcGrid;
class CalcTreeModel;

class CalcGridManager : public QTreeView
{
	public:
		CalcGridManager(CalcGrid *g);
		
	private:
		CalcGrid *m_grid;
		CalcTreeModel *m_model;
};

#endif
