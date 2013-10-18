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

#ifndef _CALC_H_
#define _CALC_H_

/*!
	\file calc.h
	\brief Definition of the Calc class
*/

#include "linkbuffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <tilem.h>

#include <QHash>
#include <QObject>
#include <QMutex>
#include <QByteArray>
#include <QScriptValue>
#include <QReadWriteLock>

class QScriptEngine;

class Calc : public QObject
{
	friend class CalcLink;
	friend class CalcDebugger;
	
	friend void tilem_message(TilemCalc* calc, const char* msg, ...);
	friend void tilem_warning(TilemCalc* calc, const char* msg, ...);
	friend void tilem_internal(TilemCalc* calc, const char* msg, ...);
	
	Q_OBJECT
	
	public:
		enum LogLevel
		{
			Message,
			Warning,
			Internal
		};
		
		typedef TilemZ80BreakpointFunc BreakCallback;
		
		Calc(QObject *p = 0);
		~Calc();
		
		QString name() const;
		
		QString romFile() const;
		
		int model() const;
		QString modelName() const;
		QString modelDescription() const;
		
		bool lcdUpdate();
		int lcdWidth() const;
		int lcdHeight() const;
		const unsigned int* lcdData() const;
		
		void resetLink();
		
		bool isBroadcasting() const;
		void setBroadcasting(bool y);
		
		bool isSending() const;
		bool isReceiving() const;
		
		uint32_t byteCount() const;
		
		char topByte();
		char getByte();
		void sendByte(char c);
		
		QByteArray getBytes(int n);
		int getBytes(int n, char *d);
		void sendBytes(const QByteArray& d);
		
		int breakpointCount() const;
		
		void addBreakpoint(BreakCallback cb);
		void removeBreakpoint(int n);
		
		int breakpointType(int n) const;
		void setBreakpointType(int n, int type);

		bool isBreakpointPhysical(int n) const;
		void setBreakpointPhysical(int n, bool y);

		dword breakpointStartAddress(int n) const;
		void setBreakpointStartAddress(int n, dword a);
		
		dword breakpointEndAddress(int n) const;
		void setBreakpointEndAddress(int n, dword a);
		
		dword breakpointMaskAddress(int n) const;
		void setBreakpointMaskAddress(int n, dword a);
		
	public slots:
		void load(const QString& file);
		void save(const QString& file);
		
		dword run_us(int usec);
		dword run_cc(int clock);
		
		void reset();
		
		void stop(int reason = 0);
		
		void keyPress(int sk);
		void keyRelease(int sk);
		
		void setName(const QString& n);
		
	signals:
		void nameChanged(const QString& n);
		
		void bytesAvailable();
		
		void breakpoint(int id);
		void log(const QString& message, int type, dword addr);
		
	private:
		typedef dword (*emulator)(TilemCalc *c, int amount, int *remaining);
		dword run(int amount, emulator emu);
		
		QString m_romFile, m_name;
		
		QMutex m_run;
		TilemCalc *m_calc;
		
		QList<int> m_breakIds;
		
		unsigned char *m_lcd;
		unsigned int *m_lcd_comp;
		
		volatile bool m_load_lock, m_link_lock, m_broadcast;
		
		LinkBuffer m_input, m_output;
		
		static QHash<TilemCalc*, Calc*> m_table;
};

#endif
