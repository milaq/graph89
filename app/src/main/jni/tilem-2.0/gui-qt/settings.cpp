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

#include "settings.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QFileInfo>

/*!
	\file settings.cpp
	\brief Implementation of the Settings class
*/

/*!
	\class Settings
	\brief A small utility class to read settings
	
	Basically a parser that produces a settings tree from a string of a custom
	format and a couple of methods to access the data in that tree.
	
	The main (only) purpose of this class is to load skin files so do not expect
	it to be general purpose.
*/

Settings::Settings()
{
	m_root = new Entry;
}

Settings::~Settings()
{
	delete m_root;
}

QString Settings::resource(const QString& r) const
{
	return QDir(m_resourceDir).filePath(r);
}

Settings::Entry* Settings::entry(const QString& name) const
{
	return m_root->entry(name);
}

QString Settings::value(const QString& key, const QString& defaultValue) const
{
	return m_root->value(key, defaultValue);
}

Settings::Entry* Settings::Entry::entry(const QString& name) const
{
	for ( int i = 0; i < children.count(); ++i )
		if ( children.at(i)->name == name )
			return children.at(i);
	
	return 0;
}

QString Settings::Entry::value(const QString& key, const QString& defaultValue) const
{
	Entry *c = entry(key);
	
	QString v = (c->children.count() == 1 && c->children.first()->children.isEmpty())
				? c->children.first()->name
				: defaultValue;
	
	return v;
}

QStringList Settings::Entry::stringItems() const
{
	QStringList l;
	
	for ( int i = 0; i < children.count(); ++i )
		if ( children.at(i)->children.isEmpty() )
			l << children.at(i)->name;
	
	return l;
}

QList<int> Settings::Entry::integerItems() const
{
	QStringList l = stringItems();
	
	QList<int> r;
	
	foreach ( QString item, l )
	{
		bool ok = false;
		int n = item.toInt(&ok, 0);
		if ( ok )
			r << n;
	}
	
	return r;
}

bool Settings::load(const QString& file)
{
	QFile f(file);
	
	if ( f.open(QFile::ReadOnly | QFile::Text) )
	{
		m_resourceDir = QFileInfo(file).path();
		
		return read(QString::fromLocal8Bit(f.readAll()));
	} else
		return false;
}

static QStringList tokenize(const QString& d)
{
	QStringList l;
	
	int i = 0, last = 0;
	
	while ( i < d.length() )
	{
		if ( d.at(i) == '{' || d.at(i) == '}' )
		{
			if ( last != i )
				l << d.mid(last, i - last);
			
			l << QString(1, d.at(i));
			
			last = i + 1;
		} else if ( d.at(i).isSpace() || d.at(i) == ',' ) {
			if ( last != i )
				l << d.mid(last, i - last);
			
			last = i + 1;
		} else if ( d.at(i) == '#' ) {
			do
			{
				++i;
			} while ( d.at(i) != '\n' );
			
			last = i + 1;
		}
		
		++i;
	}
	
	return l;
}

bool Settings::read(const QString& data)
{
	/*
		very simple settings parser (mostly for skin files)
		
		Shell-like comments : signle line, started by #
		Tree structure : curly braces delimit blocks
		List structure : comma/whitespace delimit items
		Item can be made of alphanum chars plus a couple of others
		Spaces around item separators are discarded
	*/
	
	QStringList l = tokenize(data);
	
	Entry *e = m_root;
	
	foreach ( QString t, l )
	{
		if ( t == "{" )
		{
			if ( e->children.isEmpty() || e->children.last()->children.count() )
			{
				e->addChild(new Entry());
			}
			
			e = e->children.last();
		} else if ( t == "}" ) {
			if ( !e->parent )
			{
				qWarning("Malformed settings data : overdose of block close");
				return false;
			}
			
			e = e->parent;
		} else {
			e->addChild(new Entry(t));
		}
	}
	
	if ( e != m_root )
	{
		qWarning("Malformed settings data : missing block close");
		return false;
	}
	
	return true;
}
