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

#include "keymaploader.h"

#include <QDebug>

/*!
	\file keymaploader.h
	\brief Implementation of the KeyMapLoader class
*/

int KeySpace::sk(const char *s) const
{
	int i = 0, j = 0;
	const int sl = qstrlen(s);
	
	while ( prefixes[j] )
	{
		const int pl = qstrlen(prefixes[j]);
		
		if ( !qstrnicmp(prefixes[j], s, pl) )
		{
			while ( names[i].name )
			{
				if ( !qstricmp(names[i].name, s + pl) )
					return names[i].sk;
				
				++i;
			}
			
			return 0;
		}
		
		++j;
	}
	
	return 0;
}

/*!
	\class KeyMapLoader
	\brief A versatile keymap loader
*/

KeyMapLoader::KeyMapLoader(const KeySpace *in, const KeySpace *out)
 : m_spaceIn(in), m_spaceOut(out)
{
	// TIEmu compat...
// 	m_separators[Entry] = '\n';
// 	m_separators[Space] = ':';
// 	m_separators[Sequence] = 0;
// 	m_separators[Combo] = ',';
	
	m_separators[Entry] = '\n';
	m_separators[Space] = ':';
	m_separators[Sequence] = ',';
	m_separators[Combo] = '|';
}

KeyMapLoader::~KeyMapLoader()
{
	
}

char KeyMapLoader::separator(Separator sep) const
{
	return m_separators[sep];
}

void KeyMapLoader::setSeparator(Separator sep, char s)
{
	m_separators[sep] = s;
}

const KeySpace* KeyMapLoader::inSpace() const
{
	return m_spaceIn;
}

const KeySpace* KeyMapLoader::outSpace() const
{
	return m_spaceOut;
}

int KeyMapLoader::combo(const QByteArray& cmb, const KeySpace *ks) const
{
	int k = 0;
	QList<QByteArray> keys = cmb.split(m_separators[Combo]);
	
	foreach ( QByteArray key, keys )
		k |= ks->sk(key.constData());
	
	return k;
}

QList<int> KeyMapLoader::keys(const QByteArray& seq, const KeySpace *ks) const
{
	QList<int> l;
	QList<QByteArray> combos = seq.split(m_separators[Sequence]);
	
	foreach ( QByteArray c, combos )
	{
		int kc = combo(c, ks);
		
		if ( kc )
			l << kc;
	}
	
	return l;
}

void KeyMapLoader::load(const QByteArray& data, KeyLoadCallback& cb) const
{
	QList<QByteArray> entries = data.split(m_separators[Entry]);
	
	foreach ( QByteArray e, entries )
	{
		int cidx = e.indexOf("//");
		
		if ( cidx != -1 )
			e = e.left(cidx).trimmed();
		
		if ( e.isEmpty() )
			continue;
		
		//qDebug() << e;
		
		QList<QByteArray> spaces = e.split(m_separators[Space]);
		
		if ( spaces.count() != 2 )
		{
			qDebug("Invalid keymap entry : %s", e.constData());
			continue;
		}
		
		QList<int> keys_in = keys(spaces.at(0), m_spaceIn);
		QList<int> keys_out = keys(spaces.at(1), m_spaceOut);
		
		//qDebug() << keys_in << keys_out;
		
		if ( keys_in.count() && keys_out.count() )
			cb(keys_in, keys_out);
	}
}
