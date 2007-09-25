#ifndef SINGLE_SOURCE_COMPILE

/*
 * visualization_widget.cpp - widget for visualization of sound-data
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#else

#include <qpainter.h>
#include <qpixmap.h>
#include <qtimer.h>

#endif


#include "visualization_widget.h"
#include "embed.h"
#include "engine.h"
#include "templates.h"
#include "tooltip.h"


const int UPDATE_TIME = 1000 / 20;	// 20 fps



visualizationWidget::visualizationWidget( const QPixmap & _bg, QWidget * _p,
						visualizationTypes _vtype ) :
	QWidget( _p ),
	s_background( _bg ),
	m_enabled( TRUE )
{
#ifndef QT4
	setBackgroundMode( Qt::NoBackground );
#endif
	setFixedSize( s_background.width(), s_background.height() );


	const fpp_t frames = engine::getMixer()->framesPerPeriod();
	m_buffer = new surroundSampleFrame[frames];

	engine::getMixer()->clearAudioBuffer( m_buffer, frames );


	m_updateTimer = new QTimer( this );
	connect( m_updateTimer, SIGNAL( timeout() ), this, SLOT( update() ) );
	if( m_enabled == TRUE )
	{
		m_updateTimer->start( UPDATE_TIME );
	}

	connect( engine::getMixer(), SIGNAL( nextAudioBuffer() ),
					this, SLOT( updateAudioBuffer() ) );

	toolTip::add( this, tr( "click to enable/disable visualization of "
							"master-output" ) );
}




visualizationWidget::~visualizationWidget()
{
	delete[] m_buffer;
}




void visualizationWidget::updateAudioBuffer( void )
{
	if( m_enabled == TRUE )
	{
		engine::getMixer()->lock();
		memcpy( m_buffer, engine::getMixer()->currentReadBuffer(),
				engine::getMixer()->framesPerPeriod() *
						BYTES_PER_SURROUND_FRAME );
		engine::getMixer()->unlock();
	}
}




void visualizationWidget::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap draw_pm( rect().size() );

	QPainter p( &draw_pm, this );
#endif
	p.drawPixmap( 0, 0, s_background );

	if( m_enabled == TRUE )
	{
		float master_output = engine::getMixer()->masterGain();
		Uint16 w = width()-4;
		float half_h = -( height() - 6 ) / 3.0 * master_output - 1;
		Uint16 x_base = 2;
		Uint16 y_base = height()/2 - 1;
		Uint16 old_y[DEFAULT_CHANNELS] = { y_base + (int)(
							m_buffer[0][0]*half_h ),
							y_base + (int)(
							m_buffer[0][1]*half_h )
						} ;

		p.setClipRect( 2, 2, w, height()-4 );

		float max_level = 0.0;

		const fpp_t frames =
				engine::getMixer()->framesPerPeriod();

		// analyse wave-stream for max-level
		for( fpp_t frame = 0; frame < frames; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < SURROUND_CHANNELS;
									++chnl )
			{
				if( tAbs<float>( m_buffer[frame][chnl] ) >
								max_level )
				{
					max_level = tAbs<float>(
							m_buffer[frame][chnl] );
				}
			}
		}
		// and set color according to that...
		if( max_level * master_output < 0.9 )
		{
			p.setPen( QColor( 96, 255, 96 ) );
		}
		else if( max_level * master_output < 1.1 )
		{
			p.setPen( QColor( 255, 192, 64 ) );
		}
		else
		{
			p.setPen( QColor( 255, 64, 64 ) );
		}

		const int xd = w / frames;
		// now draw all that stuff
		for( fpp_t frame = 0; frame < frames; ++frame )
		{
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				Uint16 cur_y = y_base +
					(Uint16)( m_buffer[frame][ch] *
								half_h );
				Uint16 xp = x_base + frame * w / frames;
				p.drawLine( xp, old_y[ch], xp+xd, cur_y );
				old_y[ch] = cur_y;
			}
		}

	}

#ifndef QT4
	// and blit all drawn stuff on the screen...
	bitBlt( this, rect().topLeft(), &draw_pm );
#endif
}




void visualizationWidget::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton )
	{
		m_enabled = !m_enabled;
		if( m_enabled == TRUE )
		{
			m_updateTimer->start( UPDATE_TIME );
		}
		else
		{
			m_updateTimer->stop();
			// we have to update (remove last waves),
			// because timer doesn't do that anymore
			update();
		}
	}
}



#include "visualization_widget.moc"


#endif
