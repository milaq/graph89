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

#ifndef _KEY_MAP_LOADER_H_
#define _KEY_MAP_LOADER_H_

/*!
	\file keymaploader.h
	\brief Definition of the KeyMapLoader class
*/

#include <QList>
#include <QByteArray>

struct KeyName
{
	const char *name;
	int sk;
};

struct KeySpace
{
	const char **prefixes;
	const KeyName *names;
	
	int sk(const char *s) const;
};

class KeyLoadCallback
{
	public:
		virtual ~KeyLoadCallback() {}
		virtual void operator () (QList<int>& in, QList<int>& out) = 0;
};

class KeyMapLoader
{
	public:
		enum Separator
		{
			Entry,
			Space,
			Sequence,
			Combo,
			
			SeparatorCount
		};
		
		KeyMapLoader(const KeySpace *in, const KeySpace *out);
		~KeyMapLoader();
		
		char separator(Separator sep) const;
		void setSeparator(Separator sep, char s);
		
		const KeySpace* inSpace() const;
		const KeySpace* outSpace() const;
		
		int combo(const QByteArray& cmb, const KeySpace *ks) const;
		QList<int> keys(const QByteArray& seq, const KeySpace *ks) const;
		void load(const QByteArray& data, KeyLoadCallback& cb) const;
		
	private:
		const KeySpace *m_spaceIn, *m_spaceOut;
		char m_separators[SeparatorCount];
};

#endif
