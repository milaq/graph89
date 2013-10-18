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

#include "calclogview.h"

/*!
	\file calclogview.cpp
	\brief Implementation of the CalcLogView class
*/

#include "calc.h"

#include <QInputDialog>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

class CalcLogModel : public QAbstractListModel
{
	public:
		CalcLogModel(QObject *p = 0)
		 : QAbstractListModel(p)
		{
			
		}
		
		~CalcLogModel()
		{
			qDeleteAll(m_logs);
		}
		
		virtual int rowCount(const QModelIndex& idx) const
		{
			return idx.isValid() ? 0 : m_logs.count();
		}
		
		virtual QVariant data(const QModelIndex& idx, int role) const
		{
			if ( idx.isValid() && idx.row() < m_logs.count() && idx.row() >= 0 )
			{
				LogEntry *e = m_logs.at(idx.row());
				
				if ( role == Qt::DisplayRole )
				{
					QString snd;
					
					if ( e->sender )
						snd = QString().sprintf("%s : 0x%08x", qPrintable(e->sender->name()), e->addr);
					else
						snd = "?";
					
					return tr("(%2) %1").arg(e->text).arg(snd);
				} else if ( role == Qt::BackgroundRole ) {
					
					if ( e->level == Calc::Internal )
						return QColor(Qt::red);
					else if ( e->level == Calc::Warning )
						return QColor(Qt::yellow);
					
					return QVariant();
				}
			}
			
			return QVariant();
		}
		
		void addLogEntry(Calc *c, const QString& message, int type, dword addr)
		{
			LogEntry *e = new LogEntry;
			e->level = type;
			e->text = message;
			e->sender = c;
			e->addr = addr;
			
			beginInsertRows(QModelIndex(), m_logs.count(), m_logs.count());
			m_logs << e;
			endInsertRows();
		}
		
	private:
		struct LogEntry
		{
			int level;
			QString text;
			Calc *sender;
			dword addr;
		};
		
		QList<LogEntry*> m_logs;
};

/*!
	\class CalcLogView
	\brief A widget to display TilEm logs
	
*/

CalcLogView::CalcLogView(QWidget *p)
 : QListView(p)
{
	setUniformItemSizes(true);
	setSelectionMode(ExtendedSelection);
	
	m_model = new CalcLogModel(this);
	
	m_proxy = new QSortFilterProxyModel(this);
	m_proxy->setSourceModel(m_model);
	m_proxy->setDynamicSortFilter(true);
	
	setModel(m_proxy);
}

void CalcLogView::addCalc(Calc *c)
{
	connect(c	, SIGNAL( log(QString, int, dword) ), 
			this, SLOT  ( log(QString, int, dword) ) );
	
}

void CalcLogView::log(const QString& message, int type, dword addr)
{
	m_model->addLogEntry(qobject_cast<Calc*>(sender()), message, type, addr);
}

void CalcLogView::contextMenuEvent(QContextMenuEvent *e)
{
	m_proxy->setFilterRegExp(
					QInputDialog::getText(
										this,
										tr("Change Filter"),
										tr("New Filter"),
										QLineEdit::Normal,
										m_proxy->filterRegExp().pattern()
									)
				);
}
