/*
 * nstate_button.h - declaration of class nStateButton
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _NSTATE_BUTTON_H
#define _NSTATE_BUTTON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QPushButton>
#include <QPixmap>
#include <QVector>
#include <QPair>

#else

#include <qpushbutton.h>
#include <qpixmap.h>
#include <qvaluevector.h>
#include <qpair.h>

#endif


class nStateButton : public QPushButton
{
	Q_OBJECT
public:
	nStateButton( QWidget * _parent );
	virtual ~nStateButton();
	void addState( const QPixmap & _pixmap, const QString & _tooltip = "" );

	inline void setGeneralToolTip( const QString & _tooltip )
	{
		m_generalToolTip = _tooltip;
	}

	inline int state( void ) const
	{
		return( m_curState );
	}


public slots:
	void changeState( int _n );


signals:
	void changedState( int _n );


protected:
/*	virtual void paintEvent( QPaintEvent * _pe );*/
	virtual void mousePressEvent( QMouseEvent * _me );


private:
	vvector<QPair<QPixmap *, QString> > m_states;
	QString m_generalToolTip;

	int m_curState;

} ;

#endif
