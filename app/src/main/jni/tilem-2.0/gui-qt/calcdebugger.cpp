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

#include "calcdebugger.h"

/*!
	\file calcdebugger.cpp
	\brief Implementation of the CalcDebugger class
*/

#include "calc.h"
#include "calcview.h"
#include "calcgrid.h"

#include <QMenu>
#include <QMessageBox>
#include <QAbstractListModel>

static inline QString hex(dword val, int ml)
{
	QString t = QString::number(val, 16).toUpper();
	
	while ( t.length() < ml )
		t.prepend('0');
	
	return t;
}

struct BreakData
{
	QString m_script;
	QScriptValue m_comp;
};

int bpDispatch(TilemCalc *tc, dword a, void *d)
{
// 	Calc::Breakpoint *bp = static_cast<Calc::Breakpoint*>(d);
// 	
// 	QScriptValueList args;
// 	args << QScriptValue(a);
// 	
// 	return bp->test.isFunction() ? bp->test.call(QScriptValue(), args).toInt32() : 1;
	return 1;
}

class BreakpointModel : public QAbstractListModel
{
	Q_OBJECT
	
	public:
		BreakpointModel(QObject *p = 0)
		 : QAbstractListModel(p), m_calc(0)
		{
			
		}
		
		void setCalc(Calc *c)
		{
			m_calc = c;
			reset();
		}
		
		virtual int rowCount(const QModelIndex& i) const
		{
			if ( !m_calc )
				return 0;
			
			return i.isValid() ? 0 : m_calc->breakpointCount();
		}
		
		virtual QVariant data(const QModelIndex& i, int r) const
		{
			if ( !m_calc )
				return QVariant();
			
			int n = i.row();
			
			if ( r == Qt::DisplayRole && n != -1 )
				return hex(m_calc->breakpointStartAddress(n), 6)+":"+hex(m_calc->breakpointEndAddress(n), 6);
			
			return QVariant();
		}
		
		void changed(int row)
		{
			QModelIndex idx = index(row, 0, QModelIndex());
			emit dataChanged(idx, idx);
		}
		
		void addBreakpoint()
		{
			if ( !m_calc )
				return;
			
			int i = m_calc->breakpointCount();
			beginInsertRows(QModelIndex(), i, i);
			m_calc->addBreakpoint(bpDispatch);
			endInsertRows();
		}
		
		void removeBreakpoint(int i)
		{
			if ( !m_calc )
				return;
			
			beginRemoveRows(QModelIndex(), i, i);
			m_calc->removeBreakpoint(i);
			endRemoveRows();
		}
		
	private:
		QPointer<Calc> m_calc;
};

class CalcListModel : public QAbstractListModel
{
	Q_OBJECT
	
	public:
		CalcListModel(CalcGrid *g, QObject *p = 0)
		 : QAbstractListModel(p), m_grid(g)
		{
			connect(g, SIGNAL( beginAddCalc(int) ), this, SLOT( beginAddCalc(int) ));
			connect(g, SIGNAL( endAddCalc() ), this, SLOT( endAddCalc() ));
			connect(g, SIGNAL( beginRemoveCalc(int) ), this, SLOT( beginRemoveCalc(int) ));
			connect(g, SIGNAL( endRemoveCalc() ), this, SLOT( endRemoveCalc() ));
		}
		
		virtual int rowCount(const QModelIndex& i) const
		{
			return i.isValid() ? 0 : m_grid->calcCount();
		}
		
		virtual QVariant data(const QModelIndex& i, int r) const
		{
			return r == Qt::DisplayRole ? m_grid->calc(i.row())->calc()->name() : QVariant();
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

#include "calcdebugger.moc"

/*!
	\class CalcDebugger
	\brief A widget offering debuging capabilities for an emulated calc
*/

CalcDebugger::CalcDebugger(CalcGrid *g, QWidget *p)
 : QWidget(p), m_calcGrid(g)
{
	setupUi(this);
	
	m_disasm = tilem_disasm_new();
	
	cb_target->setModel(new CalcListModel(g, this));
	
	lw_breakpoints->setModel(new BreakpointModel(this));
	lw_breakpoints->setContextMenuPolicy(Qt::CustomContextMenu);
	
	connect(b_step	, SIGNAL( clicked() ),
			this	, SIGNAL( step() ) );
	
	connect(lw_breakpoints->selectionModel(), SIGNAL( currentChanged(QModelIndex, QModelIndex) ),
			this							, SLOT  ( currentBreakpointChanged(QModelIndex) ) );
	
	QFont f("Monospace");
	f.setStyleHint(QFont::Courier);
	
	lv_disasm->setFont(f);
	
	tbPages->setEnabled(false);
	
	m_refreshId = startTimer(spn_refresh->value());
}

CalcDebugger::~CalcDebugger()
{
	tilem_disasm_free(m_disasm);
}

QSize CalcDebugger::sizeHint() const
{
	return QSize(300, 800);
}

void CalcDebugger::timerEvent(QTimerEvent *e)
{
	if ( isVisible() && m_calc && m_calc->m_calc && (e->timerId() == m_refreshId) )
	{
		const byte flags = m_calc->m_calc->z80.r.af.b.l;
		
		switch ( tbPages->currentIndex() )
		{
			case 1:
				// update flags
// 				chk_flag_S->setChecked(flags & FLAG_S);
// 				chk_flag_Z->setChecked(flags & FLAG_Z);
// 				chk_flag_5->setChecked(flags & FLAG_Y);
// 				chk_flag_H->setChecked(flags & FLAG_H);
// 				chk_flag_3->setChecked(flags & FLAG_X);
// 				chk_flag_P->setChecked(flags & FLAG_P);
// 				chk_flag_N->setChecked(flags & FLAG_N);
// 				chk_flag_C->setChecked(flags & FLAG_C);
				
				/*
					force repaint of all the watch widgets for changes to be visible
					
					optimization : directly repaint the parent widget to reduce overhead...
				*/
				pageCPU->repaint();
				
				break;
				
			case 2:
			{
				chk_flash_lock->setChecked(!m_calc->m_calc->flash.unlock);
				
				pageMemory->repaint();
				break;
			}
				
			default:
				break;
		}
	}
}

void CalcDebugger::on_cb_target_currentIndexChanged(int idx)
{
	CalcView *v = idx != -1 ? m_calcGrid->calc(idx) : 0;
	Calc *c = v ? v->calc() : 0;
	
	if ( c && m_calc == c )
		return;
	
	if ( m_calc )
	{
		disconnect(m_calc	, SIGNAL( paused(bool) ),
				   this		, SLOT  ( paused(bool) ) );
		
		disconnect(m_calc	, SIGNAL( breakpoint(int) ),
				   this		, SLOT  ( breakpoint(int) ) );
	}
	
	m_calc = c;
	
	tbPages->setEnabled(m_calc);
	
	if ( m_calc && m_calc->m_calc )
	{
		// breakpoints page
		qobject_cast<BreakpointModel*>(lw_breakpoints->model())->setCalc(m_calc);
		
		// CPU page
		
		TilemZ80Regs& regs = m_calc->m_calc->z80.r;
		
		le_af->setPointer(&regs.af.w.l);
		le_bc->setPointer(&regs.bc.w.l);
		le_de->setPointer(&regs.de.w.l);
		le_hl->setPointer(&regs.hl.w.l);
		
		le_af_shadow->setPointer(&regs.af2.w.l);
		le_bc_shadow->setPointer(&regs.bc2.w.l);
		le_de_shadow->setPointer(&regs.de2.w.l);
		le_hl_shadow->setPointer(&regs.hl2.w.l);
		
		le_ix->setPointer(&regs.ix.w.l);
		le_iy->setPointer(&regs.iy.w.l);
		
		le_pc->setPointer(&regs.pc.w.l);
		le_sp->setPointer(&regs.sp.w.l);
		
		le_ir->setPointer(&regs.ir.w.l);
		
		lbl_flags->setPointer(&regs.af.b.l);
		
		le_disasm_start->setText("0000");
		b_resume->setEnabled(true);
		paused(v->isPaused());
		
		connect(v	, SIGNAL( paused(bool) ),
				this, SLOT  ( paused(bool) ) );
		
		connect(this, SIGNAL( pause() ),
				v	, SLOT  ( pause() ) );
		
		connect(this, SIGNAL( resume() ),
				v	, SLOT  ( resume() ) );
		
		connect(this, SIGNAL( step() ),
				v	, SLOT  ( step() ) );
		
		connect(m_calc	, SIGNAL( breakpoint(int) ),
				this	, SLOT  ( breakpoint(int) ) );
		
		// memory page
		
		le_bankA->setPointer(m_calc->m_calc->mempagemap + 1);
		le_bankB->setPointer(m_calc->m_calc->mempagemap + 2);
		le_bankC->setPointer(m_calc->m_calc->mempagemap + 3);
		
		// ports page
		
	} else {
		
	}
}

void CalcDebugger::on_lw_breakpoints_customContextMenuRequested(const QPoint& p)
{
	const QModelIndex& idx = lw_breakpoints->indexAt(p);
	
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	if ( !bkpt )
		return;
	
	QMenu m;
	QAction *add = m.addAction(tr("New breakpoint"));
	QAction *rem = idx.isValid() ? m.addAction(tr("Remove breakpoint")) : 0;
	
	QAction *a = m.exec(lw_breakpoints->mapToGlobal(p));
	
	if ( a == add )
	{
		bkpt->addBreakpoint();
	} else if ( a == rem ) {
		bkpt->removeBreakpoint(idx.row());
	}
}

static inline void setHex(QLineEdit *e, dword val)
{
	e->setText(hex(val, e->maxLength()));
}

void CalcDebugger::currentBreakpointChanged(const QModelIndex& idx)
{
	if ( !m_calc )
		return;
	
	gb_break_trigger->setEnabled(idx.isValid());
	te_break_script->setEnabled(idx.isValid());
	
	if ( idx.isValid() )
	{
		int n = idx.row();
		
		cb_break_type->setCurrentIndex(m_calc->breakpointType(n) - 1);
		
		bool phy = m_calc->isBreakpointPhysical(n);
		rb_break_physical->setChecked(phy);
		/*
		QString m("HHHH;");
		
		if ( phy )
			m.prepend("HH");
		
		le_break_start_addr->setInputMask(m);
		le_break_end_addr->setInputMask(m);
		le_break_mask_addr->setInputMask(m);
		*/
		
		setHex(le_break_start_addr, m_calc->breakpointStartAddress(n));
		setHex(le_break_end_addr, m_calc->breakpointEndAddress(n));
		setHex(le_break_mask_addr, m_calc->breakpointMaskAddress(n));
	} else {
		cb_break_type->setCurrentIndex(0);
		
		le_break_start_addr->clear();
		le_break_end_addr->clear();
		le_break_mask_addr->clear();
	}
}

void CalcDebugger::on_cb_break_type_currentIndexChanged(int idx)
{
	int r = lw_breakpoints->selectionModel()->currentIndex().row();
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	m_calc->setBreakpointType(r, idx + 1);
	bkpt->changed(r);
}

void CalcDebugger::on_rb_break_physical_toggled(bool y)
{
	int r = lw_breakpoints->selectionModel()->currentIndex().row();
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	m_calc->setBreakpointPhysical(r, y);
	bkpt->changed(r);
}

void CalcDebugger::on_le_break_start_addr_textEdited(const QString& s)
{
	int r = lw_breakpoints->selectionModel()->currentIndex().row();
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	m_calc->setBreakpointStartAddress(r, s.toULong(0, 16));
	bkpt->changed(r);
}

void CalcDebugger::on_le_break_end_addr_textEdited(const QString& s)
{
	int r = lw_breakpoints->selectionModel()->currentIndex().row();
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	m_calc->setBreakpointEndAddress(r, s.toULong(0, 16));
	bkpt->changed(r);
}

void CalcDebugger::on_le_break_mask_addr_textEdited(const QString& s)
{
	int r = lw_breakpoints->selectionModel()->currentIndex().row();
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	m_calc->setBreakpointMaskAddress(r, s.toULong(0, 16));
	bkpt->changed(r);
}

void CalcDebugger::on_spn_refresh_valueChanged(int val)
{
	if ( m_refreshId != -1 )
		killTimer(m_refreshId);
	
	m_refreshId = val > 0 ? startTimer(val) : -1;
}

void CalcDebugger::on_le_disasm_start_textChanged(const QString& s)
{
	updateDisasm(s.toULong(0, 16), spn_disasm_length->value());
}

void CalcDebugger::on_spn_disasm_length_valueChanged(int val)
{
	updateDisasm(le_disasm_start->text().toULong(0, 16), val);
}

void CalcDebugger::updateDisasm(dword addr, int len)
{
	const int ml = 60;
	char inst_buffer[ml];
	
	lv_disasm->clear();
	
	for ( int i = 0; i < len; ++i )
	{
		QString l = QString("%1   %2").arg(addr, 6, 16, QChar('0')).toUpper();
		
		tilem_disasm_disassemble(m_disasm, m_calc->m_calc, 0, addr, &addr, inst_buffer, ml);
		
		lv_disasm->addItem(l.arg(QString(inst_buffer).toLower()));
	}
}

void CalcDebugger::paused(bool y)
{
	b_resume->setText(y ? tr("Resume") : tr("Pause"));
	b_step->setEnabled(y);
	
	static const char *s_resume = SIGNAL( resume() ), *s_pause = SIGNAL( pause() );
	
	connect(b_resume, SIGNAL( clicked() ),
			this	, y ? s_resume : s_pause);
	
	disconnect(b_resume	, SIGNAL( clicked() ),
			   this		, y ? s_pause : s_resume);
	
	if ( y && m_calc->m_calc )
	{
		le_disasm_start->setText(QString::number(m_calc->m_calc->z80.r.pc.d, 16));
		tbPages->setCurrentIndex(1);
	}
}

void CalcDebugger::breakpoint(int id)
{
	BreakpointModel *bkpt = qobject_cast<BreakpointModel*>(lw_breakpoints->model());
	
	//bkpt->(id);
	//QMessageBox::information(0, tr("Calc paused"), tr("Breakpoint hit : %1").arg(id));
	
// 	le_disasm_start->setText(QString::number(m_calc->m_calc->z80.r.pc.d, 16));
// 	
// 	tbPages->setCurrentIndex(1);
}
