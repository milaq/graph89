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

#include "calcgridmanager.h"

/*!
	\file calcgridmanager.cpp
	\brief Implementation of the CalcGridManager class
*/

#include "calc.h"
#include "calclink.h"
#include "calcview.h"
#include "calcgrid.h"
#include "connectionmanager.h"

#include <QLineEdit>
#include <QComboBox>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>

/*!
	\internal
	\class CalcTreeModel
	\brief Internal mode of the CalcGridManager
	
	Internal id of indices :
	* 8 bits : unused
	* 8 bits : depth (at this stage either 0 or 1...)
	* 16 bits : calc index
	
	Data to make accessible :
	
	Calc name | ?
		rom file ?
		status : running/stopped
		visibility : in grid/floating/hidden
		has external link?
		calc2calc link?
		
*/
class CalcTreeModel : public QAbstractItemModel
{
	Q_OBJECT
	
	public:
		enum Properties
		{
			RomFile,
			Status,
			Visibility,
			ExternalLink,
			Calc2CalcLink,
			PropCount
		};
		
		CalcTreeModel(CalcGrid *g, QObject *p = 0)
		 : QAbstractItemModel(p), m_grid(g)
		{
			connect(g, SIGNAL( beginAddCalc(int) ), this, SLOT( beginAddCalc(int) ));
			connect(g, SIGNAL( endAddCalc() ), this, SLOT( endAddCalc() ));
			connect(g, SIGNAL( beginRemoveCalc(int) ), this, SLOT( beginRemoveCalc(int) ));
			connect(g, SIGNAL( endRemoveCalc() ), this, SLOT( endRemoveCalc() ));
		}
		
		inline int depth(const QModelIndex& i) const
		{
			return i.isValid() ? ((i.internalId() >> 16) & 0x000000ff) : 0;
		}
		
		inline CalcView* calc(const QModelIndex& i) const
		{
			return i.isValid() ? m_grid->calc(i.internalId() & 0x0000ffff) : 0;
		}
		
		virtual QModelIndex index(int row, int col, const QModelIndex& p) const
		{
			if ( col < 0 || col >= 2 || row < 0 )
				return QModelIndex();
			
			if ( !p.isValid() )
			{
				// top level
				if ( row < m_grid->calcCount() )
					return createIndex(row, col, row & 0x0000ffff);
			} else if ( depth(p) == 0 ) {
				// prop level
				if ( row < PropCount )
					return createIndex(row, col, (p.row() & 0x0000ffff) | (1 << 16));
			} else {
				// problem...
			}
			
			return QModelIndex();
		}
		
		virtual QModelIndex parent(const QModelIndex& i) const
		{
			int idx = i.internalId() & 0x0000ffff;
			
			// if depth somehow is greater than one we're up for troubles...
			return depth(i) == 1 ? createIndex(idx, 0, idx) : QModelIndex();
		}
		
		virtual int rowCount(const QModelIndex& i) const
		{
			return i.isValid() ? (depth(i) ? 0 : PropCount) : m_grid->calcCount();
		}
		
		virtual int columnCount(const QModelIndex& i) const
		{
			return 2;
		}
		
		virtual Qt::ItemFlags flags(const QModelIndex& i) const
		{
			Qt::ItemFlags f = QAbstractItemModel::flags(i);
			
			int d = depth(i);
			
			if ( d == 1 && i.column() == 1 )
			{
				f |= Qt::ItemIsEditable;
			}
			
			return f;
		}
		
		virtual QVariant data(const QModelIndex& i, int r) const
		{
			CalcView *v = calc(i);
			
			if ( !v )
				return QVariant();
			
			Calc *c = v->calc();
			
			switch ( depth(i) )
			{
				case 0 :
					if ( r == Qt::DisplayRole )
						return i.column() ? QString() : c->name();
					
					
					break;
					
				case 1:
					if ( r == Qt::DisplayRole )
					{
						switch ( i.row() )
						{
							case CalcTreeModel::RomFile:
								return i.column() ? c->romFile() : tr("ROM file");
								
							case CalcTreeModel::Status:
								return i.column() ? (v->isPaused() ? tr("Paused") : tr("Running")) : tr("Status");
								
							case CalcTreeModel::Visibility:
								return i.column() ? (v->isVisible() ? (v->parent() ? tr("Docked") : tr("Floating")) : tr("Hidden")) : tr("Visibility");
								
							case CalcTreeModel::ExternalLink:
								return i.column() ? (v->link()->hasExternalLink() ? tr("Yes") : tr("No")) : tr("External link");
								
							case CalcTreeModel::Calc2CalcLink:
							{
								Calc *cc = ConnectionManager::instance()->connection(c);
								return i.column() ? (cc ? cc->name() : tr("None")) : tr("Link");
							}
								
							default:
								break;
						}
					}
					
					break;
					
				default:
					break;
			}
			
			return QVariant();
		}
		
	private slots:
		void beginAddCalc(int i)
		{
			beginInsertRows(QModelIndex(), i, i);
		}
		
		void endAddCalc()
		{
			endInsertRows();
		}
		
		void beginRemoveCalc(int i)
		{
			beginRemoveRows(QModelIndex(), i, i);
		}
		
		void endRemoveCalc()
		{
			endRemoveRows();
		}
		
	private:
		CalcGrid *m_grid;
};

class CalcTreeDelegate : public QStyledItemDelegate
{
	Q_OBJECT
	
	public:
		CalcTreeDelegate(CalcGrid *g, QObject *p = 0)
		 : QStyledItemDelegate(p), m_grid(g)
		{
			
		}
		
		inline CalcView* calc(const QModelIndex& i) const
		{
			return i.isValid() ? m_grid->calc(i.internalId() & 0x0000ffff) : 0;
		}
		
		inline int depth(const QModelIndex& i) const
		{
			return i.isValid() ? ((i.internalId() >> 16) & 0x000000ff) : 0;
		}
		
		QWidget* createEditor(QWidget *p, const QStyleOptionViewItem& o, const QModelIndex& i) const
		{
			QWidget *w = 0;
			
			if ( (depth(i) == 1) && (i.column() == 1) )
			{
				switch ( i.row() )
				{
					case CalcTreeModel::RomFile:
						w = new QLineEdit(p);
						
						connect(w	, SIGNAL( editingFinished() ),
								this, SLOT  ( editingFinished() ) );
						
						break;
						
					case CalcTreeModel::Status:
					case CalcTreeModel::Visibility:
					case CalcTreeModel::ExternalLink:
					case CalcTreeModel::Calc2CalcLink:
						w = new QComboBox(p);
						
						connect(w	, SIGNAL( currentIndexChanged(int) ),
								this, SLOT  ( editingFinished() ) );
						
						break;
						
					default:
						break;
				}
			}
			
			return w ? w : QStyledItemDelegate::createEditor(p, o, i);
		}
		
		void setEditorData(QWidget *e, const QModelIndex& i) const
		{
			CalcView *v = calc(i);
			Calc *c = v ? v->calc() : 0;
			
			if ( c && (depth(i) == 1) && (i.column() == 1) )
			{
				QComboBox *cb = qobject_cast<QComboBox*>(e);
				
				switch ( i.row() )
				{
					case CalcTreeModel::RomFile:
						qobject_cast<QLineEdit*>(e)->setText(c->romFile());
						return;
						
					case CalcTreeModel::Status:
						cb->addItem(tr("Running"));
						cb->addItem(tr("Paused"));
						
						cb->setCurrentIndex(v->isPaused() ? 1 : 0);
						return;
						
					case CalcTreeModel::Visibility:
						cb->addItem(tr("Docked"));
						cb->addItem(tr("Floating"));
						cb->addItem(tr("Hidden"));
						
						cb->setCurrentIndex(v->isVisible() ? (v->parent() ? 0 : 1 ) : 2);
						return;
						
					case CalcTreeModel::ExternalLink:
						cb->addItem(tr("Yes"));
						cb->addItem(tr("No"));
						
						cb->setCurrentIndex(v->link()->hasExternalLink() ? 0 : 1);
						return;
						
					case CalcTreeModel::Calc2CalcLink:
					{
						cb->addItem(tr("None"));
						
						int cci = 0;
						Calc *cc = ConnectionManager::instance()->connection(c);
						
						for ( int k = 0; k < m_grid->calcCount(); ++k )
						{
							Calc *ck = m_grid->calc(k)->calc();
							
							if ( ck != c )
							{
								cb->addItem(ck->name());
								
								if ( ck == cc )
									cci = k;
							}
						}
						
						cb->setCurrentIndex(cci);
						return;
					}
						
					default:
						break;
				}
			}
			
			QStyledItemDelegate::setEditorData(e, i);
		}
		
		void setModelData(QWidget *e, QAbstractItemModel *m, const QModelIndex& i) const
		{
			CalcView *v = calc(i);
			Calc *c = v ? v->calc() : 0;
			
			if ( c && (depth(i) == 1) && (i.column() == 1) )
			{
				QLineEdit *le = qobject_cast<QLineEdit*>(e);
				QComboBox *cb = qobject_cast<QComboBox*>(e);
				
				int cbi = cb ? cb->currentIndex() : 0;
				
				switch ( i.row() )
				{
					case CalcTreeModel::RomFile:
						if ( le->isModified() && le->isUndoAvailable() && le->text() != c->romFile() )
							c->load(le->text());
						return;
						
					case CalcTreeModel::Status:
						if ( cbi )
							v->pause();
						else
							v->resume();
						
						return;
						
					case CalcTreeModel::Visibility:
						switch ( cbi )
						{
							case 0:
								v->show();
								m_grid->dockCalc(v);
								break;
								
							case 1:
								v->show();
								m_grid->floatCalc(v);
								break;
								
							case 2:
								v->hide();
								break;
								
							default:
								break;
						}
						return;
						
					case CalcTreeModel::ExternalLink:
						if ( cbi )
							v->link()->releaseExternalLink();
						else
							v->link()->grabExternalLink();
						
						return;
						
					case CalcTreeModel::Calc2CalcLink:
						if ( cbi )
						{
							--cbi;
							
							ConnectionManager::instance()
								->addConnection(
										c,
										m_grid->calc(cbi < m_grid->index(v) ? cbi : cbi + 1)->calc()
									);
						} else {
							ConnectionManager::instance()->removeConnection(c);
						}
						return;
						
					default:
						break;
				}
			}
			
			QStyledItemDelegate::setModelData(e, m, i);
		}
		
	private slots:
		void editingFinished()
		{
			emit commitData(qobject_cast<QWidget*>(sender()));
		}
		
	private:
		CalcGrid *m_grid;
};

#include "calcgridmanager.moc"

/*!
	\class CalcGridManager
	\brief Widget for managing properties of CalcViews in a CalcGrid
*/

CalcGridManager::CalcGridManager(CalcGrid *g)
 : QTreeView(g), m_grid(g)
{
	//setHeaderHidden(true);
	
	m_model = new CalcTreeModel(g, this);
	
	setModel(m_model);
	setItemDelegate(new CalcTreeDelegate(g, this));
	setEditTriggers(QAbstractItemView::AllEditTriggers);
}
