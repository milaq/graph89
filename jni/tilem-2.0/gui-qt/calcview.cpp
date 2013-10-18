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

#include "calcview.h"

/*!
	\file calcview.cpp
	\brief Implementation of the CalcView class
*/

#include <QDir>
#include <QUrl>
#include <QMenu>
#include <QDebug>
#include <QImage>
#include <QThread>
#include <QBitmap>
#include <QPainter>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QFileDialog>
#include <QApplication>
#include <QImageWriter>

#include <scancodes.h>

#include "calc.h"
#include "keymap.h"
#include "calclink.h"
#include "settings.h"
#include "keymaploader.h"

extern const KeySpace ti_space, pc_space;

class CalcViewKeyMap : public KeyMap, public KeyLoadCallback
{
	public:
		CalcViewKeyMap(CalcView *v)
		 : KeyMap(v), m_view(v)
		{
			m_combos << KM;
		}
		
		virtual void operator () (QList<int>& in, QList<int>& out)
		{
			setKeys(in, m_combos.count());
			m_combos << out;
		}
		
	protected:
		virtual void activate(quintptr id)
		{
			KeyMap::activate(id);
			
			if ( id >= (unsigned)m_combos.count() )
				return;
			
			QList<int> keys = m_combos.at(id);
			
			foreach ( int k, keys )
			{
				if ( m_pressed.value(k, 0) )
					++m_pressed[k];
				else {
					m_pressed[k] = 1;
					m_view->calc()->keyPress(k);
				}
			}
		}
		
		virtual void deactivate(quintptr id)
		{
			KeyMap::deactivate(id);
			
			if ( id >= (unsigned)m_combos.count() )
				return;
			
			QList<int> keys = m_combos.at(id);
			
			foreach ( int k, keys )
			{
				if ( !(--m_pressed[k]) )
					m_view->calc()->keyRelease(k);
			}
		}
	private:
		CalcView *m_view;
		
		QHash<int, int> m_pressed;
		QList< QList<int> > m_combos;
};

/*!
	\class CalcThread
	\brief Utility class to manage calc emulation
	
	Perform the emulation in a separate thread to reduce latency in both GUI and emulator.
*/
class CalcThread : public QThread
{
	Q_OBJECT
	
	public:
		CalcThread(Calc *c, QObject * p = 0)
		: QThread(p), m_calc(c), exiting(0)
		{
			
		}
		
		void step()
		{
			if ( isRunning() )
				return;
			
			exiting = 1;
			start();
		}
		
		void stop()
		{
			// stop emulator thread before loading a new ROM
			exiting = 1;
			
			// wait for thread to terminate
			while ( isRunning() )
				usleep(1000);
			
		}
		
	signals:
		void runningChanged(bool y);
		
	protected:
		virtual void run()
		{
			int res;
			
			emit runningChanged(false);
			
			forever
			{
				if ( (res = (exiting ? m_calc->run_cc(1) : m_calc->run_us(10000))) )
				{
// 					if ( res & TILEM_STOP_BREAKPOINT )
// 					{
// 						qDebug("breakpoint hit");
// 					} else {
// 						qDebug("stop:%i", res);
// 					}
					break;
				}
				
				if ( exiting )
					break;
				
				// slightly slow down emulation (TODO : make delay adjustable)
				usleep(10000);
			}
			
			exiting = 0;
			
			emit runningChanged(true);
		}
		
	private:
		Calc *m_calc;
		volatile int exiting;
};

#include "calcview.moc"

/*!
	\class CalcView
	\brief Main UI class for emulation
	
	Displays calculator skin and forwards user interaction (mouse/keyboard) to emulator core.
	
	Accepts drops to send files to emulated calc.
*/

CalcView::CalcView(const QString& file, QWidget *p)
: QFrame(p), m_link(0), m_thread(0), m_hovered(-1), m_scale(1.0), m_skinLess(false), m_skin(0), m_screen(0), m_keymask(0)
{
	setFocusPolicy(Qt::StrongFocus);
	setFrameShape(QFrame::StyledPanel);
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	
	// accept drops for easy file download
	setAcceptDrops(true);
	
	setMouseTracking(true);
	
	m_keymap = new CalcViewKeyMap(this);
	
	// setup core objects
	m_calc = new Calc(this);
	
	connect(m_calc, SIGNAL( nameChanged(QString) ), this, SLOT( setWindowTitle(QString) ) );
	
	// load ROM
	if ( QFile::exists(file) )
		load(file);
	else
		load();
	
	QAction *a;
	// setup context menu
	m_cxt = new QMenu(this);
	
	m_cxt->addAction(tr("Load ROM/state..."), this, SLOT( load() ));
	m_cxt->addAction(tr("Save ROM/state..."), this, SLOT( save() ));
	m_cxt->addSeparator();
	a = m_cxt->addAction(tr("Pause"), this, SLOT( pause() ) );
	a->connect(this, SIGNAL( paused(bool) ), SLOT( setDisabled(bool) ) );
	a = m_cxt->addAction(tr("Resume"), this, SLOT( resume() ) );
	a->setEnabled(false);
	a->connect(this, SIGNAL( paused(bool) ), SLOT( setEnabled(bool) ) );
	a = m_cxt->addAction(tr("Reset"), this, SLOT( reset() ) );
	m_cxt->addSeparator();
	a = m_cxt->addAction(tr("Take screenshot"), this, SLOT( takeScreenshot() ) );
	m_cxt->addSeparator();
	a = m_cxt->addAction(tr("Send file..."), this, SLOT( sendFile() ) );
	a = m_cxt->addAction(tr("Grab external link"), this, SLOT( grabExternalLink() ) );
	a->connect(this, SIGNAL( externalLinkGrabbed(bool) ), SLOT( setDisabled(bool) ) );
	m_cxt->addSeparator();
	m_cxt->addAction(tr("Change skin..."), this, SLOT( selectSkin() ));
	m_cxt->addAction(tr("Change keymap..."), this, SLOT( selectKeymap() ));
	a = m_cxt->addAction(tr("Skinless"));
	a->setCheckable(true);
	a->setChecked(false);
	connect(a, SIGNAL( toggled(bool) ), SLOT( setSkinLess(bool) ) );
	a->connect(this, SIGNAL( skinLessChanged(bool) ), SLOT( setChecked(bool) ) );
	m_cxt->addSeparator();
	m_dock = m_cxt->addAction(tr("Float"), this, SIGNAL( toggleDocking() ) );
	m_cxt->addAction(tr("Close"), this, SLOT( close() ));
	
	// launch emulator thread
	resume();
	
	// start LCD update timer
	m_lcdTimerId = startTimer(10);
}

CalcView::~CalcView()
{
	delete m_link;
	
	m_thread->stop();
	
	// cleanup
	delete m_screen;
	delete m_keymask;
	delete m_skin;
}

bool CalcView::isPaused() const
{
	return !m_thread->isRunning();
}

void CalcView::step()
{
	m_thread->step();
}

void CalcView::pause()
{
	if ( m_thread->isRunning() )
		m_thread->stop();
}

void CalcView::resume()
{
	if ( !m_thread->isRunning() )
		m_thread->start();
}

void CalcView::reset()
{
	m_calc->reset();
}

void CalcView::takeScreenshot()
{
	const int w = m_calc->lcdWidth();
	const int h = m_calc->lcdHeight();
	const void *cd = m_calc->lcdData();
	
	QImage cpy(static_cast<const unsigned char*>(cd), w, h, QImage::Format_RGB32);
	
	QList<QByteArray> fmts = QImageWriter::supportedImageFormats();
	
	QString filters, all_filters("(");
	
	foreach ( QByteArray fmt, fmts )
	{
		filters += tr("%1 files (*%1);;").arg(QString::fromLocal8Bit(fmt));
		
		all_filters += '*';
		all_filters += fmt;
		all_filters += ' ';
	}
	
	all_filters += ')';
	
	QString file = QFileDialog::getSaveFileName(
											this,
											tr("Save screenshot as..."),
											QString(),
											tr("%1All image files%2").arg(filters).arg(all_filters)
										);
	
	if ( file.count() )
	{
		if ( !cpy.save(file) )
			QMessageBox::warning(
							this,
							tr("Screenshot taking failed"),
							tr(
								"Unable to write screenshot image :"
								"unsupported format or insufficient permissions"
							)
						);
	}
}

void CalcView::sendFile()
{
	if ( m_link )
	{
		QString file = QFileDialog::getOpenFileName(
											this,
											tr(""),
											QString(),
											tr(
												"Programs (*.73p *.83p *.8xp *.85p *.86p);;"
												"Groups (*.73g *.82g *.83g *.8xg *.85g *.86g);;"
												"Flash apps (*.73k *.8xk);;"
												"OS upgrades (*.73u *.8xu);;"
												"All files (*)"
											)
										);
		
		m_link->send(file);
	}
}

void CalcView::grabExternalLink()
{
	if ( m_link )
		m_link->grabExternalLink();
}

void CalcView::undocked()
{
	m_dock->setText(tr("Dock"));
}

void CalcView::docked()
{
	m_dock->setText(tr("Float"));
}

void CalcView::setSkinLess(bool y)
{
	if ( y != m_skinLess )
	{
		m_skinLess = y;
		
		updateView();
		
		emit skinLessChanged(y);
	}
}

void CalcView::updateView()
{
	if ( m_skinLess )
	{
		QBitmap msk(m_screen->size() * m_scale);
		msk.fill(Qt::color1);
		setFixedSize(m_screen->size() * m_scale);
		setMask(msk);
	} else {
		QSize sz = m_skin->size() * m_scale;
		setFixedSize(sz);
		setMask(m_skin->mask().scaled(sz));
	}
	
	updateGeometry();
	repaint();
}

Calc* CalcView::calc() const
{
	return m_calc;
}

CalcLink* CalcView::link() const
{
	return m_link;
}

void CalcView::load()
{
	QString romfile = QFileDialog::getOpenFileName(
										this,
										tr("Select ROM file"),
										QString(),
										tr("ROM files (*.rom);;All files (*)")
									);
	
	if ( QFile::exists(romfile)	)
		load(romfile);
	else if ( m_calc->romFile().isEmpty() )
		load();
}

void CalcView::save()
{
	save(m_calc->romFile());
}

void CalcView::load(const QString& file)
{
	/// 1) stop/cleanup phase
	startTimer(m_lcdTimerId);
	
	if ( m_thread )
		m_thread->stop();
	
	/// 2) load phase
	m_calc->load(file);
	
	if ( !m_link )
	{
		m_link = new CalcLink(m_calc, this);
		
		connect(m_link, SIGNAL( externalLinkGrabbed(bool) ), this, SIGNAL( externalLinkGrabbed(bool) ));
	} else
		m_link->setCalc(m_calc);
	
	if ( !m_thread )
	{
		m_thread = new CalcThread(m_calc, this);
		connect(m_thread, SIGNAL( started() ), this, SIGNAL( resumed() ) );
		connect(m_thread, SIGNAL( finished() ), this, SIGNAL( paused() ) );
		connect(m_thread, SIGNAL( runningChanged(bool) ), this, SIGNAL( paused(bool) ) );
	}
	
	m_model = m_calc->modelName();
	
	// setup skin and keyboard layout
	setupSkin();
	setupKeymap();
	
	/// 3) restart phase
	
	// launch emulator thread
	m_thread->start();
	
	// start LCD update timer
	m_lcdTimerId = startTimer(10);
	
	setWindowTitle(file);
}

void CalcView::save(const QString& file)
{
	m_calc->save(file);
}

void CalcView::quit()
{
	QApplication::quit();
}

QString CalcView::getSkinFileName()
{
	return QFileDialog::getOpenFileName(
									this,
									tr("Select skin for %1").arg(m_model),
									QApplication::applicationDirPath() + "/skins",
									"Skin files (*.skin)"
								);
}

QString CalcView::getKeymapFileName()
{
	return QFileDialog::getOpenFileName(
									this,
									tr("Select keymap for %1").arg(m_model),
									QApplication::applicationDirPath() + "/skins",
									"Keymap files (*.map)"
								);
}

void CalcView::selectSkin()
{
	Settings s;
	
	if ( s.load(getSkinFileName()) )
		loadSkin(s);
}

void CalcView::selectKeymap()
{
	QFile f(getKeymapFileName());
	
	m_keymap->clear();
	
	if ( f.open(QFile::ReadOnly | QFile::Text) )
		KeyMapLoader(&pc_space, &ti_space).load(f.readAll(), *m_keymap);
}

void CalcView::setupSkin()
{
	// cleanup previous images
	delete m_keymask;
	delete m_skin;
	delete m_screen;
	
	QString fn = QDir("skins").filePath(m_model + ".skin");
	
	Settings s;
	
	while ( !s.load(fn) )
		fn = getSkinFileName();
	
	loadSkin(s);
}

void CalcView::loadSkin(Settings& s)
{
	Settings::Entry *e = s.entry("lcd-coords");
	
	QList<int> l;
	
	if ( e )
		l = e->integerItems();
	
	if ( l.count() == 4 )
	{
		m_lcdX = l.at(0);
		m_lcdY = l.at(1);
		m_lcdW = l.at(2);
		m_lcdH = l.at(3);
	} else {
		qWarning("skin: Missing or malformed lcd-coords field.");
	}
	
	m_skin = new QPixmap(s.resource(s.value("skin")));
	m_screen = new QImage(m_lcdW, m_lcdH, QImage::Format_RGB32);
	m_keymask = new QImage(QImage(s.resource(s.value("keymask"))).createHeuristicMask());
	
	updateView();
	
	e = s.entry("key-coords");
	
	m_kCenter.clear();
	m_kScanCode.clear();
	m_kBoundaries.clear();
	
	if ( e )
	{
		for ( int i = 0; i < e->children.count(); ++i )
		{
			Settings::Entry *m = e->children.at(i);
			QList<int> coords = m->integerItems();
			
			if ( coords.count() == 3 )
			{
				m_kCenter << QPoint(coords.at(0), coords.at(1));
				m_kScanCode << coords.at(2);
				m_kBoundaries << keyBoundaries(m_kCenter.last());
			} else {
				qWarning("skin: Malformed key-coords entry.");
			}
		}
	} else {
		qWarning("skin: Missing key-coords field.");
	}
}

void CalcView::setupKeymap()
{
	QString fn = QDir("skins").filePath(m_model + ".map");
	
	QFile f(fn);
	
	m_keymap->clear();
	
	while ( !f.open(QFile::ReadOnly | QFile::Text) )
		f.setFileName(getKeymapFileName());
	
	KeyMapLoader loader(&pc_space, &ti_space);
	loader.load(f.readAll(), *m_keymap);
}

QSize CalcView::sizeHint() const
{
	return m_skin->size() * m_scale;
}

float CalcView::scale() const
{
	return m_scale;
}

void CalcView::setScale(float s)
{
	float ns = qBound(0.1f, s, 2.0f);
	
	if ( ns != m_scale )
	{
		m_scale = ns;
		
		updateView();
	}
}

void CalcView::updateLCD()
{
	// update QImage and schedule widget repaint
	if ( m_calc->lcdUpdate() )
	{
		const int w = m_calc->lcdWidth();
		const int h = m_calc->lcdHeight();
		
		QRgb *d = reinterpret_cast<QRgb*>(m_screen->bits());
		const unsigned int *cd = m_calc->lcdData();
		
		// write LCD into skin image
		for ( int i = 0; i < m_lcdH; ++i )
		{
			for ( int j = 0; j < m_lcdW; ++j )
			{
				int y = (h * i) / m_lcdH;
				int x = (w * j) / m_lcdW;
				
				d[i * m_lcdW + j] = cd[y * w + x];
			}
		}
		
		if ( m_skinLess )
			repaint(0, 0, m_lcdW * m_scale, m_lcdH * m_scale);
		else
			repaint(m_lcdX * m_scale, m_lcdY * m_scale, m_lcdW * m_scale, m_lcdH * m_scale);
	}
}

int CalcView::mappedKey(const QPoint& pos) const
{
	return qGray(m_keymask->pixel(pos)) ? 0 : closestKey(pos);
}

int CalcView::closestKey(const QPoint& p) const
{
	if ( m_skinLess )
		return -1;
	
	unsigned int sk_min = 0, d_min = 0x7fffffff;
	
	for ( int i = 0; i < m_kCenter.count(); ++i )
	{
		unsigned int d = (p - m_kCenter.at(i)).manhattanLength();
		
		//qDebug("?%i", d);
		if ( d < d_min )
		{
			sk_min = m_kScanCode.at(i);
			d_min = d;
		}
	}
	
	return sk_min;
}

int CalcView::keyIndex(const QPoint& p) const
{
	if ( m_skinLess )
		return -1;
	
	for ( int i = 0; i < m_kBoundaries.count(); ++i )
	{
		if ( m_kBoundaries.at(i).containsPoint(p, Qt::WindingFill) )
		{
			return i;
		}
	}
	
	return -1;
}

enum ExpandDirection
{
	D_H	= 1,
	D_H_L	= 0,
	D_H_R	= 2,
	
	D_V	= 4,
	D_V_U	= 0,
	D_V_D	= 8,
	
	Left		= D_H | D_H_L,
	Right		= D_H | D_H_R,
	Up			= D_V | D_V_U,
	Down		= D_V | D_V_D,
};

/*
	note : clockwise sorting of points
*/
void boundaries(const QImage *img, QPolygon& poly, int idx, int dir)
{
	int xoff = (dir & D_H) ? ((dir & D_H_R) ? 1 : -1) : 0;
	int yoff = (dir & D_V) ? ((dir & D_V_D) ? 1 : -1) : 0;
	
	QPoint p = poly.at(idx);
	
	if ( p.x() + xoff < 0 || p.x() + xoff >= img->width() )
		return;
	
	if ( p.y() + yoff < 0 || p.y() + yoff >= img->height() )
		return;
	
	if ( xoff )
	{
		QPoint n = p + QPoint(xoff, 0);
		
		if ( img->pixel(p) == img->pixel(n) )
		{
			if ( xoff && yoff )
			{
				if ( xoff == yoff )
				{
					poly.insert(idx, n);
					boundaries(img, poly, idx, xoff < 0 ? Left : Right);
					++idx;
				} else {
					poly.insert(idx + 1, n);
					boundaries(img, poly, idx + 1, xoff < 0 ? Left : Right);
				}
			} else {
				poly[idx] = n;
				boundaries(img, poly, idx, xoff < 0 ? Left : Right);
			}
		}
	}
	
	if ( yoff )
	{
		QPoint n = p + QPoint(0, yoff);
		
		if ( img->pixel(p) == img->pixel(n) )
		{
			if ( xoff && yoff )
			{
				if ( xoff != yoff )
				{
					poly.insert(idx, n);
					boundaries(img, poly, idx, yoff < 0 ? Up : Down);
					++idx;
				} else {
					poly.insert(idx + 1, n);
					boundaries(img, poly, idx + 1, yoff < 0 ? Up : Down);
				}
			} else {
				poly[idx] = n;
				boundaries(img, poly, idx, yoff < 0 ? Up : Down);
			}
		}
	}
	
	if ( xoff && yoff )
	{
		QPoint n = p + QPoint(xoff, yoff);
		
		if ( img->pixel(p) == img->pixel(n) )
		{
			poly[idx] = n;
			
			boundaries(img, poly, idx, dir);
		}
	}
}

QPolygon boundaries(const QImage *img, const QPoint& p)
{
	QPolygon r, t;
	
	t = QPolygon() << QPoint(p.x() - 1, p.y() - 1);
	boundaries(img, t, 0, Left | Up);
	r += t;
	
	t = QPolygon() << QPoint(p.x() + 0, p.y() - 1);
	boundaries(img, t, 0, Up);
	r += t;
	
	t = QPolygon() << QPoint(p.x() + 1, p.y() - 1);
	boundaries(img, t, 0, Right | Up);
	r += t;
	
	t = QPolygon() << QPoint(p.x() + 1, p.y() + 0);
	boundaries(img, t, 0, Right);
	r += t;
	
	t = QPolygon() << QPoint(p.x() + 1, p.y() + 1);
	boundaries(img, t, 0, Right | Down);
	r += t;
	
	t = QPolygon() << QPoint(p.x() + 0, p.y() + 1);
	boundaries(img, t, 0, Down);
	r += t;
	
	t = QPolygon() << QPoint(p.x() - 1, p.y() + 1);
	boundaries(img, t, 0, Left | Down);
	r += t;
	
	t = QPolygon() << QPoint(p.x() - 1, p.y() + 0);
	boundaries(img, t, 0, Left);
	r += t;
	
	return r;
}

QPolygon CalcView::keyBoundaries(const QPoint& p) const
{
	return boundaries(m_keymask, p);
}

void CalcView::keyPressEvent(QKeyEvent *e)
{
	QFrame::keyPressEvent(e);
}

void CalcView::keyReleaseEvent(QKeyEvent *e)
{
	QFrame::keyReleaseEvent(e);
}

void CalcView::mousePressEvent(QMouseEvent *e)
{
	int k = keyIndex(e->pos() / m_scale);
	
	if ( k != -1 )
	{
		e->accept();
		
		if ( !m_pressed.contains(k) )
		{
			m_pressed << k;
		}
		
		m_calc->keyPress(m_kScanCode.at(k));
	} else {
		QFrame::mousePressEvent(e);
	}
}

void CalcView::mouseReleaseEvent(QMouseEvent *e)
{
	int k = keyIndex(e->pos() / m_scale);
	
	if ( (k != -1) && !(e->modifiers() & Qt::ControlModifier) )
	{
		e->accept();
		
		if ( m_pressed.removeAll(k) )
		{
			
		}
		
		m_calc->keyRelease(m_kScanCode.at(k));
	} else {
		QFrame::mouseReleaseEvent(e);
	}
}

void CalcView::mouseMoveEvent(QMouseEvent *e)
{
	int k = keyIndex(e->pos() / m_scale);
	
	if ( k != m_hovered )
	{
		if ( m_hovered != -1 )
		{
			int old = m_hovered;
			m_hovered = -1;
			QRect r = m_kBoundaries.at(old).boundingRect();
			
			update(r.x() * m_scale, r.y() * m_scale, r.width() * m_scale, r.height() * m_scale);
		}
		
		m_hovered = k;
		
		if ( m_hovered != -1 )
		{
			QRect r = m_kBoundaries.at(m_hovered).boundingRect();
			
			update(r.x() * m_scale, r.y() * m_scale, r.width() * m_scale, r.height() * m_scale);
			//qDebug() << r << m_clip;
		}
	}
	
	return QFrame::mouseMoveEvent(e);
}

void CalcView::wheelEvent(QWheelEvent *e)
{
	if ( e->modifiers() & Qt::ControlModifier )
	{
		e->accept();
		
		setScale(m_scale + float(e->delta()) / 1200.0);
	} else {
		QFrame::wheelEvent(e);
	}
}

void CalcView::contextMenuEvent(QContextMenuEvent *e)
{
	e->accept();
	
	m_cxt->exec(e->globalPos());
}

void CalcView::timerEvent(QTimerEvent *e)
{
	if ( e->timerId() == m_lcdTimerId )
		updateLCD();
	else
		QFrame::timerEvent(e);
}

void CalcView::paintEvent(QPaintEvent *e)
{
	QFrame::paintEvent(e);
	
	QPainter p(this);
	
	// reduce opacity for unfocused calc (to notify which one has kbd focus)
	if ( !hasFocus() )
		p.setOpacity(0.5);
	
	p.scale(m_scale, m_scale);
	
	if ( m_skinLess )
	{
		p.drawImage(0, 0, *m_screen);
		return;
	}
	
	// smart update to reduce repaint overhead
	if ( e->rect() != QRect(m_lcdX * m_scale, m_lcdY * m_scale, m_lcdW * m_scale, m_lcdH * m_scale) )
	{
		p.drawPixmap(0, 0, *m_skin);
	} else {
		// screen repaint
		p.drawImage(m_lcdX, m_lcdY, *m_screen);
		return;
	}
	
	// hover marker repaint
	if ( m_hovered != -1 )
	{
		QRect r = m_kBoundaries.at(m_hovered).boundingRect();
		r = QRect(r.x() * m_scale, r.y() * m_scale, r.width() * m_scale, r.height() * m_scale);
		
		if ( e->rect().intersects(r) )
		{
			p.setBrush(QColor(0xff, 0x00, 0x00, 0x3f));
			p.drawPolygon(m_kBoundaries.at(m_hovered), Qt::WindingFill);
		}
	}
	
	p.setBrush(QColor(0x00, 0xff, 0x00, 0x3f));
	
	foreach ( int k, m_pressed )
		p.drawPolygon(m_kBoundaries.at(k), Qt::WindingFill);
	
	p.drawImage(m_lcdX, m_lcdY, *m_screen);
}

void CalcView::focusInEvent(QFocusEvent *e)
{
	setFrameShadow(QFrame::Raised);
	setBackgroundRole(QPalette::AlternateBase);
	
	QFrame::focusInEvent(e);
	
	update();
}

void CalcView::focusOutEvent(QFocusEvent *e)
{
	setFrameShadow(QFrame::Plain);
	setBackgroundRole(QPalette::Base);
	
	QFrame::focusOutEvent(e);
	
	update();
}

static bool isSupported(const QMimeData *md, CalcLink *m_link)
{
	if ( md->hasUrls() )
	{
		QList<QUrl> l = md->urls();
		
		foreach ( QUrl url, l )
			if ( m_link->isSupportedFile(url.toLocalFile()) )
				return true;
	}
	
	return false;
}

void CalcView::dropEvent(QDropEvent *e)
{
	QList<QUrl> l = e->mimeData()->urls();
	
	foreach ( QUrl url, l )
	{
		QString file = url.toLocalFile();
		
		if ( m_link->isSupportedFile(file) )
			m_link->send(file);
	}
	
	QFrame::dropEvent(e);
}

void CalcView::dragEnterEvent(QDragEnterEvent *e)
{
	if ( isSupported(e->mimeData(), m_link) )
		e->acceptProposedAction();
	else
		QFrame::dragEnterEvent(e);
}

void CalcView::dragMoveEvent(QDragMoveEvent *e)
{
	QFrame::dragMoveEvent(e);
}

void CalcView::dragLeaveEvent(QDragLeaveEvent *e)
{
	QFrame::dragLeaveEvent(e);
}
