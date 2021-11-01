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

#include "connectionmanager.h"

/*!
	\file connectionmanager.cpp
	\brief Implementation of the ConnectionManager class
*/

#include "calc.h"

ConnectionManager* ConnectionManager::instance()
{
	static ConnectionManager _i;
	
	return &_i;
}

/*!
	\class ConnectionManager
	\brief Manages virtual connections among Calc objects
*/

ConnectionManager::ConnectionManager(QObject *p)
 : QObject(p)
{
	
}

ConnectionManager::~ConnectionManager()
{
	
}

int ConnectionManager::connectionCount() const
{
	return m_connections.count() / 2;
}

Calc* ConnectionManager::connection(Calc *c) const
{
	return m_connections.value(c, 0);
}

void ConnectionManager::addConnection(Calc *c1, Calc *c2)
{
	// clear any previous connection
	removeConnection(c1);
	removeConnection(c2);
	
	//qDebug("adding connection : 0x%x <-> 0x%x", c1, c2);
	
	// add new connection
	m_connections[c1] = c2;
	m_connections[c2] = c1;
	
	connect(c1, SIGNAL( bytesAvailable() ), this, SLOT( bytesAvailable() ));
	connect(c2, SIGNAL( bytesAvailable() ), this, SLOT( bytesAvailable() ));
	
	emit connectionAdded(c1, c2);
}

void ConnectionManager::removeConnection(Calc *c)
{
	QHash<Calc*, Calc*>::iterator it = m_connections.find(c);
	
	if ( it != m_connections.end() )
	{
		disconnect(c, SIGNAL( bytesAvailable() ), this, SLOT( bytesAvailable() ));
		disconnect(*it, SIGNAL( bytesAvailable() ), this, SLOT( bytesAvailable() ));
		
		emit connectionRemoved(it.key(), *it);
		
		m_connections.remove(*it);
		m_connections.erase(it);
	}
}

void ConnectionManager::bytesAvailable()
{
	Calc *c = qobject_cast<Calc*>(sender());
	
	if ( !c )
		return;
	
	uint32_t n = c->byteCount();
	
	if ( !n )
		return;
	
	QHash<Calc*, Calc*>::iterator it = m_connections.find(c);
	
	if ( it != m_connections.end() )
	{
		//qDebug("sending %i byte(s) through connection", n);
		
		if ( n > 1 )
			(*it)->sendBytes(c->getBytes(n));
		else
			(*it)->sendByte(c->getByte());
	} else {
		qWarning("calc 0x%x sending bytes without connection (or could it be a PC link?)", c);
	}
}
