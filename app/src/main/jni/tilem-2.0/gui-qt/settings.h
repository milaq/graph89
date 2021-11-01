/****************************************************************************
**
** Copyright (C) 2009 Hugues Luc BRUANT aka fullmetalcoder 
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

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

/*!
	\file settings.h
	\brief Definition of the Settings class
*/

#include <QStringList>

class Settings
{
	public:
		struct Entry
		{
			inline Entry() : parent(0) {}
			inline Entry(const QString& n) : name(n), parent(0) {}
			
			~Entry()
			{
				detach();
				
				qDeleteAll(children);
			}
			
			inline void addChild(Entry *e)
			{
				if ( !e )
					return;
				
				e->parent = this;
				children << e;
			}
			
			inline void removeChild(Entry *e)
			{
				if ( !e )
					return;
				
				e->parent = 0;
				children.removeAll(e);
			}
			
			inline void attach(Entry *p)
			{
				if ( !p )
					return;
				
				parent = p;
				p->children << this;
			}
			
			inline void detach()
			{
				if ( parent )
					parent->children.removeAll(this);
				
				parent = 0;
			}
			
			Entry* entry(const QString& name) const;
			QString value(const QString& key, const QString& defaultValue = QString()) const;
			
			QStringList stringItems() const;
			QList<int> integerItems() const;
			
			QString name;
			
			Entry *parent;
			QList<Entry*> children;
		};
		
		Settings();
		~Settings();
		
		QString resource(const QString& r) const;
		
		bool load(const QString& file);
		bool read(const QString& data);
		
		Entry* entry(const QString& name) const;
		QString value(const QString& key, const QString& defaultValue = QString()) const;
		
	private:
		Entry *m_root;
		QString m_resourceDir;
};

#endif
