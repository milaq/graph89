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

#ifndef _CALC_HEX_VALUE_H_
#define _CALC_HEX_VALUE_H_

/*!
	\file calchexvalue.h
	\brief Definition of the CalcHexValue class
*/

#include <QLineEdit>

class CalcHexValue : public QLineEdit
{
	Q_OBJECT
	
	public:
		CalcHexValue(QWidget *p = 0)
		 : QLineEdit(p), ptr(0)
		{
			connect(this, SIGNAL( textEdited(QString) ), this, SLOT( update(QString) ));
		}
		
		CalcHexValue(const QString& s, QWidget *p = 0)
		 : QLineEdit(s, p), ptr(0)
		{
			
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
				quint32 n = old;
				
				switch ( maxLength() )
				{
					case 1:
					case 2:
						n = *(static_cast<quint8*>(ptr));
						break;
						
					case 3:
					case 4:
						n = *(static_cast<quint16*>(ptr));
						break;
						
					default:
						break;
				}
				
				if ( old != n )
				{
					// update text if needed
					old = n;
					QString t = QString::number(n, 16).toUpper();
					
					while ( t.length() < maxLength() )
						t.prepend('0');
					
					setText(t);
				}
			}
			
			QLineEdit::paintEvent(e);
		}
		
	protected slots:
		void update(const QString& hex)
		{
			if ( !ptr )
				return;
			
			int n = hex.toUInt(0, 16);
			
			switch ( maxLength() )
			{
				case 1:
				case 2:
					*(static_cast<quint8*>(ptr)) = n;
					break;
					
				case 3:
				case 4:
					*(static_cast<quint16*>(ptr)) = n;
					break;
					
				default:
					break;
			}
		}
		
	private:
		void *ptr;
		quint32 old;
};

#endif
