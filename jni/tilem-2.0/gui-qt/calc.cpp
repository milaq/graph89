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

#include "calc.h"

/*!
	\file calc.cp
	\brief Implementation of the Calc class
*/

#include <scancodes.h>

#include <QDir>
#include <QColor>
#include <QFileInfo>
#include <QInputDialog>
#include <QScriptEngine>

#include <errno.h>

class RegisterDword
{
	public:
		RegisterDword()
		{
			qRegisterMetaType<dword>("dword");
		}
};

static RegisterDword rdw;

QHash<TilemCalc*, Calc*> Calc::m_table;

/*!
	\class Calc
	\brief Core class to manage calc emulation
	
	Abstracts away the use of libtilemcore and load/save of roms
*/

Calc::Calc(QObject *p)
 : QObject(p), m_calc(0), m_lcd(0), m_lcd_comp(0)
{
	
}

Calc::~Calc()
{
	QMutexLocker lock(&m_run);
	
	m_table.remove(m_calc);
	
	// release memory
	tilem_calc_free(m_calc);
	
	delete m_lcd_comp;
	delete m_lcd;
}

QString Calc::name() const
{
	return m_name;
}

void Calc::setName(const QString& n)
{
	if ( n == m_name )
		return;
	
	m_name = n;
	
	emit nameChanged(n);
}

QString Calc::romFile() const
{
	return m_romFile;
}

int Calc::model() const
{
	return m_calc ? m_calc->hw.model_id : 0;
}

QString Calc::modelName() const
{
	return QString(m_calc->hw.name);
}

QString Calc::modelDescription() const
{
	return QString(m_calc->hw.desc);
}

void Calc::resetLink()
{
	tilem_linkport_graylink_reset(m_calc);
}

bool Calc::isBroadcasting() const
{
	return m_broadcast;
}

void Calc::setBroadcasting(bool y)
{
	m_broadcast = y;
}

/*!
	\return whether the *calc* is writing data through the linkport
*/
bool Calc::isSending() const
{
	return m_output.count();
}

/*!
	\return whether the *calc* has data to read from the linkport
*/
bool Calc::isReceiving() const
{
	return m_input.count();
}

/*!
	\return amount of data written to the linkport by the *calc* that can be retrieved using getByte()
*/
uint32_t Calc::byteCount() const
{
	return m_output.count();
}

/*!
	\brief get a byte from the buffer in which calc linkport writes are stored but does not remove it from the buffer
*/
char Calc::topByte()
{
	if ( m_output.isEmpty() )
		return 0;
	
	char c = m_output.at(0);
	
	return c;
}

/*!
	\brief get a byte from the buffer in which calc linkport writes are stored
*/
char Calc::getByte()
{
	if ( m_output.isEmpty() )
		return 0;
	
	char c = m_output.at(0);
	
	m_output.remove(1);
	
	return c;
}

/*!
	\brief send a byte to the calc
*/
void Calc::sendByte(char c)
{
	m_input += c;
}

/*!
	\brief get up to n bytes from the buffer in which calc linkport writes are stored
*/
QByteArray Calc::getBytes(int n)
{
	if ( n < 0 || m_output.isEmpty() )
		return QByteArray();
	
	n = qMin(n, (int)m_output.count());
	
	return m_output.take(n);
}

/*!
	\brief get up to n bytes from the buffer in which calc linkport writes are stored
*/
int Calc::getBytes(int n, char *d)
{
	if ( n < 0 || m_output.isEmpty() )
		return 0;
	
	n = qMin(n, (int)m_output.count());
	
	m_output.take(n, d);
	
	return n;
}

/*!
	\brief send bytes to the calc
*/
void Calc::sendBytes(const QByteArray& d)
{
	m_input += d;
}

int Calc::breakpointCount() const
{
	return m_breakIds.count();
}

void Calc::addBreakpoint(BreakCallback cb)
{
	m_breakIds << tilem_z80_add_breakpoint(m_calc, TILEM_BREAK_MEM_EXEC, 0, 0, -1, cb, 0);
}

void Calc::removeBreakpoint(int n)
{
	if ( n >= 0 && n < m_breakIds.count() )
		tilem_z80_remove_breakpoint(m_calc, m_breakIds.takeAt(n));
}

int Calc::breakpointType(int n) const
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	return tilem_z80_get_breakpoint_type(m_calc, id) & TILEM_BREAK_TYPE_MASK;
}

void Calc::setBreakpointType(int n, int type)
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	int oldtype = tilem_z80_get_breakpoint_type(m_calc, id);
	type |= oldtype & ~TILEM_BREAK_TYPE_MASK;
	tilem_z80_set_breakpoint_type(m_calc, id, type);
}

bool Calc::isBreakpointPhysical(int n) const
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	return tilem_z80_get_breakpoint_type(m_calc, id) & TILEM_BREAK_PHYSICAL;
}

void Calc::setBreakpointPhysical(int n, bool y)
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	int type = tilem_z80_get_breakpoint_type(m_calc, id);

	if (y)
		type |= TILEM_BREAK_PHYSICAL;
	else
		type &= ~TILEM_BREAK_PHYSICAL;

	tilem_z80_set_breakpoint_type(m_calc, id, type);
}

dword Calc::breakpointStartAddress(int n) const
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	return tilem_z80_get_breakpoint_address_start(m_calc, id);
}

void Calc::setBreakpointStartAddress(int n, dword a)
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	tilem_z80_set_breakpoint_address_start(m_calc, id, a);
}

dword Calc::breakpointEndAddress(int n) const
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	return tilem_z80_get_breakpoint_address_end(m_calc, id);
}

void Calc::setBreakpointEndAddress(int n, dword a)
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	tilem_z80_set_breakpoint_address_end(m_calc, id, a);
}

dword Calc::breakpointMaskAddress(int n) const
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	return tilem_z80_get_breakpoint_address_mask(m_calc, id);
}

void Calc::setBreakpointMaskAddress(int n, dword a)
{
	int id = n >= 0 && n < m_breakIds.count() ? m_breakIds.at(n) : 0;
	
	tilem_z80_set_breakpoint_address_mask(m_calc, id, a);
}

/*!
	\brief Rest calc (simulate battery pull)
*/
void Calc::reset()
{
	QMutexLocker lock(&m_run);
	
	byte keys[8];
	
	memcpy(keys, m_calc->keypad.keysdown, 8 * sizeof(byte));
	
	tilem_calc_reset(m_calc);
	
	memcpy(m_calc->keypad.keysdown, keys, 8 * sizeof(byte));
}

void Calc::load(const QString& file)
{
	QMutexLocker lock(&m_run);
	
	QFileInfo info(file);
	
	FILE *romfile, *savefile;

	QString savefilename = QDir(info.path()).filePath(info.completeBaseName() + ".sav");
	
	romfile = fopen(qPrintable(file), "rb");
	
	if ( !romfile )
	{
		qWarning(qPrintable(tr("Unable to load ROM file \"%s\": %s")),
			 qPrintable(file), strerror(errno));
		return;
	} else {
		m_load_lock = true;
		//qDebug("successfully opened %s", qPrintable(file));
	}
	
	savefile = fopen(qPrintable(savefilename), "rt");
	
	if ( m_calc )
	{
		//qDebug("cleanin up previous state.");
		m_table.remove(m_calc);
		
		tilem_calc_free(m_calc);
		m_calc = 0;
		
		delete m_lcd_comp;
		m_lcd_comp = 0;
		
		delete m_lcd;
		m_lcd = 0;
	}
	
	m_romFile = file;
	
	char rom_type = tilem_guess_rom_type(romfile);
	
	QStringList options;
	int nmodels, selected;
	const TilemHardware** models;
	
	tilem_get_supported_hardware(&models, &nmodels);
	
	for ( int i = 0; i < nmodels; ++i )
	{
		options << QString::fromLatin1(models[i]->desc);
		
		if ( models[i]->model_id == rom_type )
			selected = i;
	}
	
	QString t =
	QInputDialog::getItem(
						0,
						tr("Select ROM type"),
						tr("Select ROM type (keep default if unsure)"),
						options,
						selected,
						false
					);

	selected = options.indexOf(t);
	
	if ( selected != -1 )
		rom_type = models[selected]->model_id;
	
	//qDebug("%c", rom_type);
	
	m_calc = tilem_calc_new(rom_type);
	m_table[m_calc] = this;
	tilem_calc_load_state(m_calc, romfile, savefile);
	
	// some link emulation magic...
	m_calc->linkport.linkemu = TILEM_LINK_EMULATOR_GRAY;
	m_calc->z80.stop_mask &= ~(TILEM_STOP_LINK_READ_BYTE | TILEM_STOP_LINK_WRITE_BYTE | TILEM_STOP_LINK_ERROR);
	
	m_broadcast = true;
	m_link_lock = false;
	
	// instant LCD state and "composite" LCD state (grayscale is a bitch...)
	m_lcd = new unsigned char[m_calc->hw.lcdwidth * m_calc->hw.lcdheight / 8];
	m_lcd_comp = new unsigned int[m_calc->hw.lcdwidth * m_calc->hw.lcdheight];
	
	m_calc->lcd.emuflags = TILEM_LCD_REQUIRE_DELAY;
	m_calc->flash.emuflags = TILEM_FLASH_REQUIRE_DELAY;
	
	fclose(romfile);
	
	if ( savefile )
		fclose(savefile);
	
	m_load_lock = false;
}

void Calc::save(const QString& file)
{
	QMutexLocker lock(&m_run);
	
	QFileInfo info(file);
	
	FILE *romfile, *savefile;

	QString savefilename = QDir(info.path()).filePath(info.completeBaseName() + ".sav");
	
	if ( !(m_calc->hw.flags & TILEM_CALC_HAS_FLASH) )
	{
		romfile = NULL;
	} else if ( !(romfile = fopen(qPrintable(file), "wb")) ) {
		qWarning(qPrintable(tr("Unable to save ROM file \"%s\": %s")),
			 qPrintable(file), strerror(errno));
	}

	if ( !(savefile = fopen(qPrintable(savefilename), "wt")) )
	{
		qWarning(qPrintable(tr("Unable to save state file \"%s\": %s")),
			 qPrintable(savefilename), strerror(errno));
	}

	if ( romfile || savefile )
	{
		// save state
		tilem_calc_save_state(m_calc, romfile, savefile);
	}

	if ( romfile )
		fclose(romfile);
	
	if ( savefile )
		fclose(savefile);
	
}

dword Calc::run_us(int usec)
{
	return run(usec, tilem_z80_run_time);
}

dword Calc::run_cc(int clock)
{
	return run(clock, tilem_z80_run);
}

dword Calc::run(int amount, emulator emu)
{
	QMutexLocker lock(&m_run);
	
	if ( !m_calc )
		return -1;
	
	int remaining = amount;
	
	do
	{
		// try to forward data written into input buffer to link port
		
		if ( !m_link_lock )
		{
			if ( m_input.count() )
			{
				m_calc->z80.stop_reason = 0;
				
				if ( !tilem_linkport_graylink_send_byte(m_calc, m_input.at(0)) )
				{
					#ifdef TILEM_QT_LINK_DEBUG
					printf("@> %02x", static_cast<unsigned char>(m_input.at(0)));
					#endif
					
					m_input.remove(1);
					
					if ( !(m_calc->z80.stop_reason & TILEM_STOP_LINK_WRITE_BYTE) )
					{
						m_link_lock = true;
					} else {
						// here's the trick to speed things up : batch processing whenever possible
						while ( m_input.count() && (m_calc->z80.stop_reason & TILEM_STOP_LINK_WRITE_BYTE) )
						{
							m_calc->z80.stop_reason = 0;
							
							if ( !tilem_linkport_graylink_send_byte(m_calc, m_input.at(0)) )
							{
								//qDebug("@byte successfully written");
								#ifdef TILEM_QT_LINK_DEBUG
								printf(" %02x", static_cast<unsigned char>(m_input.at(0)));
								#endif
								m_input.remove(1);
							}
						}
					}
					
					#ifdef TILEM_QT_LINK_DEBUG
					printf("\n");
					fflush(stdout);
					#endif
				}
			} else {
				int b = tilem_linkport_graylink_get_byte(m_calc);
				
				if ( b != -1 )
				{
					// one byte successfully read yay!
					
					m_output += b;
					
					#ifdef TILEM_QT_LINK_DEBUG
					qDebug("@< %02x [%i] [0x%x]", static_cast<unsigned char>(b), m_output.count(), this);
					#endif
					
					if ( m_broadcast )
						emit bytesAvailable();
				}
			}
		}
		
		dword res = emu(m_calc, remaining, &remaining);
		
		/*
			some link emulation magic : seamlessly transfer
			data from buffers to the calc using a virtual
			graylink to allow asynchronous transfers
		*/
		if ( res & TILEM_STOP_LINK_WRITE_BYTE )
		{
			//qDebug("@byte successfully written");
			m_link_lock = false;
			remaining = qMax(1, remaining);
		}
		
		if ( res & TILEM_STOP_LINK_ERROR )
		{
			qWarning("@link error.");
			tilem_linkport_graylink_reset(m_calc);
			break;
		}
		
// 		if ( res & TILEM_STOP_INVALID_INST )
// 		{
// 			break;
// 		}
// 		
// 		if ( res & TILEM_STOP_UNDOCUMENTED_INST )
// 		{
// 			break;
// 		}
// 		
		if ( res & TILEM_STOP_BREAKPOINT )
		{
			emit breakpoint(m_calc->z80.stop_breakpoint);
			break;
		}
	} while ( remaining > 0 );
	
// 	if ( m_calc->z80.stop_reason )
// 		qDebug("stop:%i", m_calc->z80.stop_reason);
	
	return m_calc->z80.stop_reason;
}

void Calc::stop(int reason)
{
	// QMutexLocker lock(&m_run);
	
	if ( !m_calc )
		return;
	
	tilem_z80_stop(m_calc, reason);
}

void Calc::keyPress(int sk)
{
	tilem_keypad_press_key(m_calc, sk);
}

void Calc::keyRelease(int sk)
{
	tilem_keypad_release_key(m_calc, sk);
}

bool Calc::lcdUpdate()
{
	if ( m_load_lock || !m_calc )
		return false;
	
	// low : black, high : white
	unsigned int low, high;
	const int cc = int(m_calc->lcd.contrast);
	const unsigned int end = m_calc->hw.lcdheight * m_calc->hw.lcdwidth;
	
	// contrast determination
	if ( m_calc->lcd.active && !(m_calc->z80.halted && !m_calc->poweronhalt) )
	{
		// update "plain" LCD data
		(*m_calc->hw.get_lcd)(m_calc, m_lcd);
		
		/*
			LCD behaves roughly as follows :
			
			0->31 : high is white, low goes from white to black
			32->63 : low is black, high goes from white to black
		*/
		const int c = 63 - cc;
		//low = qMin(31, c) / 8  + qMax(0, c - 31) * 4;
		//high = qMax(31, c) * 4 + qMin(0, c - 31) / 8;
		
		low = 0x00;
		high = 0xff;
		
// 		if ( c < 32 ) {
// 			low = 0;
// 			high = c * 8;
// 		} else {
// 			low = (c - 32) * 8;
// 			high = 255;
// 		}
	} else {
		low = high = 0xff;
	}
	
	// update "composite" LCD data
	bool changed = false;
	
	for ( unsigned int idx = 0; idx < end; ++idx )
	{
		unsigned int v = m_lcd[idx >> 3] & (0x80 >> (idx & 7)) ? low : high;
		
		// blending for grayscale
		unsigned int g = v + ((qRed(m_lcd_comp[idx]) - v) * 7) / 8;
		
		v = qRgb(g, g, g);
		
		if ( v != m_lcd_comp[idx] )
		{
			changed = true;
			m_lcd_comp[idx] = v;
		}
	}
	
	return changed;
}

int Calc::lcdWidth() const
{
	return m_calc ? m_calc->hw.lcdwidth : 0;
}

int Calc::lcdHeight() const
{
	return m_calc ? m_calc->hw.lcdheight : 0;
}

const unsigned int* Calc::lcdData() const
{
	return m_lcd_comp;
}

/*
	Implementation of libtilemcore memory allocation and debug output routines
*/

extern "C" {
void* tilem_realloc(void* p, size_t s)
{
	if (s) {
		if (p) {
			p = realloc(p, s);
		}
		else {
			p = malloc(s);
		}

		if (!p) {
			fprintf(stderr, "out of memory (need %lu bytes)\n",
				(unsigned long) s);
			abort();
		}
	}
	else if (p) {
		free(p);
		p = 0;
	}

	return p;
}

void tilem_free(void* p)
{
	tilem_realloc(p, 0);
}

void* tilem_malloc(size_t s)
{
	return tilem_realloc(0, s);
}

void* tilem_try_malloc(size_t s)
{
	return malloc(s);
}

void* tilem_malloc0(size_t s)
{
	void* p = calloc(s, 1);

	if (!p) {
		fprintf(stderr, "out of memory (need %lu bytes)\n",
			(unsigned long) s);
		abort();
	}

	return p;
}

void* tilem_try_malloc0(size_t s)
{
	return calloc(s, 1);
}

void* tilem_malloc_atomic(size_t s)
{
	return tilem_malloc(s);
}

void* tilem_try_malloc_atomic(size_t s)
{
	return malloc(s);
}
}

/* Logging */

extern "C" void tilem_message(TilemCalc* calc, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	
	Calc *c = Calc::m_table.value(calc, 0);
	
	if ( c )
	{
		emit c->log(QString().vsprintf(msg, ap), Calc::Message, c->m_calc->z80.r.pc.w.l);
	} else {
		fprintf(stderr, "x%c: ", calc->hw.model_id);
		vfprintf(stderr, msg, ap);
		fputc('\n', stderr);
	}
	
	va_end(ap);
}

extern "C" void tilem_warning(TilemCalc* calc, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	
	Calc *c = Calc::m_table.value(calc, 0);
	
	if ( c )
	{
		emit c->log(QString().vsprintf(msg, ap), Calc::Warning, c->m_calc->z80.r.pc.w.l);
	} else {
		fprintf(stderr, "x%c: WARNING: ", calc->hw.model_id);
		vfprintf(stderr, msg, ap);
		fputc('\n', stderr);
	}
	
	va_end(ap);
}

extern "C" void tilem_internal(TilemCalc* calc, const char* msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	
	Calc *c = Calc::m_table.value(calc, 0);
	
	if ( c )
	{
		emit c->log(QString().vsprintf(msg, ap), Calc::Internal, c->m_calc->z80.r.pc.w.l);
	} else {
		fprintf(stderr, "x%c: INTERNAL ERROR: ", calc->hw.model_id);
		vfprintf(stderr, msg, ap);
		fputc('\n', stderr);
	}
	
	va_end(ap);
}
