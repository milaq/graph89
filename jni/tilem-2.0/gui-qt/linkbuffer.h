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

#ifndef _LINK_BUFFER_H_
#define _LINK_BUFFER_H_

/*!
	\file linkbuffer.h
	\brief Definition of the LinkBuffer class
*/

#include <stdint.h>

#include <QByteArray>
#include <QReadWriteLock>

/*!
	\class LinkBuffer
	\brief A small container class tailored for internal use in Calc
	
	It is basically a circular queue of bytes which offers maximum
	performance when working with a number of items inferior to the
	nominal value of 1024 and will slow down beyond that because it
	falls back on a dynamic container to hold the "overflowing" data.
	
	\note This container is thread-safe
*/

class LinkBuffer
{
	public:
		inline LinkBuffer()
		{
			m_base = m_count = 0;
		}
		
		~LinkBuffer()
		{
			
		}
		
		inline bool isEmpty() const
		{
			QReadLocker l(&m_lock);
			return !m_count;
		}
		
		inline uint32_t count() const
		{
			QReadLocker l(&m_lock);
			return m_count;
		}
		
		QByteArray take(uint32_t count)
		{
			QByteArray b;
			b.resize(count);
			
			for ( uint32_t i = 0; i < count; ++i )
				b[i] = at(i);
			
			remove(count);
			
			return b;
		}
		
		void take(uint32_t count, char *d)
		{
			for ( uint32_t i = 0; i < count; ++i )
				d[i] = at(i);
			
			remove(count);
		}
		
		void remove(uint32_t count)
		{
			QWriteLocker l(&m_lock);
			
			m_base = (m_base + count) & 1023;
			
			if ( m_count >= count )
			{
				m_count -= count;
			} else {
				m_overflow.remove(0, count - m_count);
				m_count = 0;
			}
			
			uint32_t n = qMin(1024 - m_count, uint32_t(m_overflow.count()));
			
			if ( n )
			{
				// bring back overflow into data
				
				for ( uint32_t i = 0; i < n; ++i )
					append(m_overflow.at(i));
				
				m_overflow.remove(0, n);
			}
		}
		
		inline char at(uint32_t idx) const
		{
			QReadLocker l(&m_lock);
			return idx < 1024 ? m_d[(m_base + idx) & 1023] : m_overflow.at(idx - 1024);
		}
		
		inline LinkBuffer& operator += (char c)
		{
			QWriteLocker l(&m_lock);
			
			append(c);
			
			return *this;
		}
		
		inline LinkBuffer& operator += (const QByteArray& ba)
		{
			QWriteLocker l(&m_lock);
			
			const int c = ba.count();
			for ( int i = 0; i < c; ++i )
				append(ba.at(i));
			
			return *this;
		}
		
	private:
		inline void append(char c)
		{
			if ( m_count == 1024 )
			{
				// keep overflowing data
				m_overflow += c;
			} else {
				m_d[(m_base + m_count) & 1023] = c;
				++m_count;
			}
		}
		
		uint32_t m_base, m_count;
		char m_d[1024];
		QByteArray m_overflow;
		
		mutable QReadWriteLock m_lock;
};

#endif
