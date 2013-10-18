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

#ifndef _KEY_MAP_H_
#define _KEY_MAP_H_

/*!
	\file keymap.h
	\brief Definition of the KeyMap class
*/

#include <QHash>
#include <QObject>

class QShortcut;
class QKeySequence;

#define KM QList<int>()

class KeyMap : public QObject
{
	Q_OBJECT
	
	public:
		enum InputFlag
		{
			MergeModifiers		= 1,
			SkipAutoRepeat		= 2,
			SingleActivation	= 4,
			OrderedActivation	= 8
		};
		
		KeyMap(QWidget *w);
		KeyMap(QWidget *w, QObject *p);
		~KeyMap();
		
		int keyCount() const;
		
		QList<quintptr> ids() const;
		QList<int> keys(quintptr id) const;
		
	public slots:
		virtual void clear();
		virtual void setKeys(const QList<int>& keys, quintptr id);
		
	signals:
		void activated(quintptr id);
		void deactivated(quintptr id);
		
	protected:
		virtual bool eventFilter(QObject *o, QEvent *e);
		
		virtual void activate(quintptr id);
		virtual void deactivate(quintptr id);
		
	private:
		QWidget *m_widget;
		
		int m_mode;
		
		quintptr m_active;
		QHash<int, int> m_keyDown;
		
		int m_matchStep;
		QList<quintptr> m_curMatch;
		
		QHash< quintptr, QList<int> > m_map;
};

#endif
