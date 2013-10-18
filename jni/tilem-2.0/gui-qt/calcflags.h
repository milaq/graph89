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

#ifndef _CALC_FLAGS_H_
#define _CALC_FLAGS_H_

/*!
	\file calcflags.h
	\brief Definition of the CalcHexValue class
*/

#include <QLabel>

class CalcFlags : public QLabel
{
	Q_OBJECT
	
	public:
		CalcFlags(QWidget *p = 0)
		 : QLabel(p), ptr(0)
		{
			QFont f("Monospace");
			f.setStyleHint(QFont::Courier);
			
			setFont(f);
		}
		
		void setPointer(void *v)
		{
			ptr = v;
		}
		
	protected:
		virtual void paintEvent(QPaintEvent *e)
		{
			if ( ptr && !hasFocus() )
			{
				// check whether watched value changed since last update
				quint8 n = *(static_cast<quint8*>(ptr));
				
				if ( old != n )
				{
					// update text if needed
					old = n;
					QString t("--------");
					
					if ( old & 0x80 )
						t[0] = 'S';
					
					if ( old & 0x40 )
						t[1] = 'Z';
					
					if ( old & 0x20 )
						t[2] = '5';
					
					if ( old & 0x10 )
						t[3] = 'H';
					
					if ( old & 0x08 )
						t[4] = '3';
					
					if ( old & 0x04 )
						t[5] = 'P';
					
					if ( old & 0x02 )
						t[6] = 'N';
					
					if ( old & 0x01 )
						t[7] = 'C';
					
					setText(t);
				}
			}
			
			QLabel::paintEvent(e);
		}
		
	private:
		void *ptr;
		quint8 old;
};

#endif
