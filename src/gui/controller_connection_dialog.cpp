#ifndef SINGLE_SOURCE_COMPILE

/*
 * controller_connection_dialog.cpp - dialog allowing the user to create and
 *	modify links between controllers and models
 *
 * Copyright (c) 2008  Paul Giblock <drfaygo/at/gmail.com>
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


#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>

#include "controller_connection_dialog.h"
#include "controller_connection.h"
#include "lcd_spinbox.h"
#include "led_checkbox.h"
#include "combobox.h"
#include "group_box.h"
#include "midi_controller.h"
#include "midi_client.h"
#include "midi_port.h"
#include "midi.h"
#include "song.h"
#include "tool_button.h"

#include "gui_templates.h"
#include "embed.h"

class autoDetectMidiController : public midiController
{
public:
	autoDetectMidiController( model * _parent ) :
		midiController( _parent ),
		m_detectedMidiChannel( 0 ),
		m_detectedMidiController( 0 )
	{
		updateMidiPort();
	}


	virtual ~autoDetectMidiController()
	{
	}


	virtual void processInEvent( const midiEvent & _me,
					const midiTime & _time, bool _lock )
	{
		if( _me.m_type == CONTROL_CHANGE &&
				( m_midiChannel.value() == _me.m_channel + 1 ||
				m_midiChannel.value() == 0 ) )
		{
			m_detectedMidiChannel = _me.m_channel + 1;
			m_detectedMidiController = ( _me.m_data.m_bytes[0] & 0x7F ) + 1;

			emit valueChanged();
		}
	}


	int m_detectedMidiChannel;
	int m_detectedMidiController;

	// Would be a nice copy ctor, but too hard to add copy ctor because
	// model has none.
	midiController * copyToMidiController( model * _parent )
	{
		midiController * c = new midiController( _parent );
		c->midiChannelModel()->setValue( m_midiChannel.value() );
		c->midiControllerModel()->setValue( m_midiController.value() );
		c->setReadablePorts( m_readablePorts );

		return c;
	}


	void useDetected( void )
	{
		m_midiChannel.setValue( m_detectedMidiChannel );
		m_midiController.setValue( m_detectedMidiController );
	}


public slots:
	void reset( void )
	{
		m_midiChannel.setValue( 0 );
		m_midiController.setValue( 0 );
	}

};




controllerConnectionDialog::controllerConnectionDialog( QWidget * _parent, 
		const automatableModel * _target_model
	) :
	QDialog( _parent ),
	m_controller( NULL ),
	m_midiController( NULL ),
	m_readablePorts( NULL ),
	m_midiAutoDetect( FALSE ),
	m_targetModel( _target_model )
{
	setWindowIcon( embed::getIconPixmap( "setup_audio" ) );
	setWindowTitle( tr( "Connection Settings" ) );
	setModal( TRUE );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 10 );
	vlayout->setMargin( 10 );


	// Midi stuff
	m_midiGroupBox = new groupBox( tr( "MIDI CONTROLLER" ), this );
	m_midiGroupBox->setGeometry( 2, 2, 240, 64 );
	connect( m_midiGroupBox->model(), SIGNAL( dataChanged() ),
			this, SLOT( midiToggled() ) );
	
	m_midiChannelSpinBox = new lcdSpinBox( 2, m_midiGroupBox,
			tr( "Input channel" ) );
	m_midiChannelSpinBox->addTextForValue( 0, "--" );
	m_midiChannelSpinBox->setLabel( tr( "CHANNEL" ) );
	m_midiChannelSpinBox->move( 8, 24 );

	m_midiControllerSpinBox = new lcdSpinBox( 3, m_midiGroupBox,
			tr( "Input controller" ) );
	m_midiControllerSpinBox->addTextForValue( 0, "---" );
	m_midiControllerSpinBox->setLabel( tr( "CONTROLLER" ) );
	m_midiControllerSpinBox->move( 72, 24 );
	

	m_midiAutoDetectCheckBox =
			new ledCheckBox( tr("Auto Detect"),
				m_midiGroupBox, tr("Auto Detect") );
	m_midiAutoDetectCheckBox->setModel( &m_midiAutoDetect );
	m_midiAutoDetectCheckBox->move( 136, 30 );
	connect( &m_midiAutoDetect, SIGNAL( dataChanged() ),
			this, SLOT( autoDetectToggled() ) );

	// when using with non-raw-clients we can provide buttons showing
	// our port-menus when being clicked
	midiClient * mc = engine::getMixer()->getMIDIClient();
	if( mc->isRaw() == FALSE )
	{
		m_readablePorts = new QMenu( this );
		m_readablePorts->setFont( pointSize<9>(
						m_readablePorts->font() ) );
		connect( m_readablePorts, SIGNAL( triggered( QAction * ) ),
			this, SLOT( activatedReadablePort( QAction * ) ) );

		toolButton * rp_btn = new toolButton( m_midiGroupBox );
		rp_btn->setText( tr( "MIDI-devices to receive "
						"MIDI-events from" ) );
		rp_btn->setIcon( embed::getIconPixmap( "midi_in" ) );
		rp_btn->setGeometry( 200, 24, 40, 40 );
		rp_btn->setMenu( m_readablePorts );
		rp_btn->setPopupMode( QToolButton::InstantPopup );
	}


	// User stuff
	m_userGroupBox = new groupBox( tr( "USER CONTROLLER" ), this );
	m_userGroupBox->setGeometry( 2, 70, 240, 64 );
	connect( m_userGroupBox->model(), SIGNAL( dataChanged() ),
			this, SLOT( userToggled() ) );

	m_userController = new comboBox( m_userGroupBox, "Controller" );
	m_userController->setGeometry( 10, 20, 200, 22 );

	m_mappingFunction = new QLineEdit( this );
	m_mappingFunction->setGeometry( 2, 140, 240, 16 );
	m_mappingFunction->setText( "input" );

	QWidget * buttons = new QWidget( this );
	buttons->setGeometry( 2, 160, 240, 32 );

	resize( 256, 196 );

	for( int i = 0; i < engine::getSong()->controllers().size(); ++i )
	{
		controller * c = engine::getSong()->controllers().at( i );
		m_userController->model()->addItem( c->publicName() );
	}
	

	QHBoxLayout * btn_layout = new QHBoxLayout( buttons );
	btn_layout->setSpacing( 0 );
	btn_layout->setMargin( 0 );


	
	QPushButton * select_btn = new QPushButton( 
					embed::getIconPixmap( "add" ),
					tr( "OK" ), buttons );
	connect( select_btn, SIGNAL( clicked() ), 
				this, SLOT( selectController() ) );
	
	QPushButton * cancel_btn = new QPushButton( 
					embed::getIconPixmap( "cancel" ),
					tr( "Cancel" ), buttons );
	//connect( cancel_btn, SIGNAL( clicked() ),
	//			this, SLOT( reject() ) );

	btn_layout->addStretch();
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( select_btn );
	btn_layout->addSpacing( 10 );
	btn_layout->addWidget( cancel_btn );
	btn_layout->addSpacing( 10 );

	
	// TODO, handle by making this a model for the Dialog "view"
	controllerConnection * cc = NULL;
	if( m_targetModel )
	{
		cc = m_targetModel->getControllerConnection();

		if( cc && cc->getController()->type() != 
				controller::DummyController && engine::getSong() )
		{
			if ( cc->getController()->type() == 
					controller::MidiController )
			{
				m_midiGroupBox->model()->setValue( TRUE );
				// ensure controller is created
				midiToggled();
			
				midiController * cont = 
					(midiController*)( cc->getController() );
				m_midiChannelSpinBox->model()->setValue(
						cont->midiChannelModel()->value() );
				m_midiControllerSpinBox->model()->setValue(
						cont->midiControllerModel()->value() );

				// update menuupdateReadablePortsMenu
				m_midiController->setReadablePorts( static_cast<midiController*>( cc->getController() )->m_readablePorts );
				updateReadablePortsMenu();
			}
			else
			{
				int idx = engine::getSong()->controllers().indexOf(
						cc->getController() );

				if( idx >= 0 )
				{
					m_userGroupBox->model()->setValue( TRUE );
					m_userController->model()->setValue( idx );
				}
			}
		}
	}

	if( !cc )
	{
		m_midiGroupBox->model()->setValue( TRUE );
	}

	show();
}




controllerConnectionDialog::~controllerConnectionDialog()
{
	if( m_midiController )
		delete m_midiController;
}




void controllerConnectionDialog::selectController( void )
{
	if( m_midiGroupBox->model()->value() > 0 )
	{
		if( m_midiControllerSpinBox->model()->value() > 0 )
		{
			midiController * mc;
			mc = m_midiController->copyToMidiController(
					engine::getSong() );
			mc->getMidiPort()->setName( m_targetModel->displayName() );
			m_controller = mc;
		}
	}
	else 
	{
		if( m_userGroupBox->model()->value() > 0 && 
				engine::getSong()->controllers().size() )
		{
			m_controller = engine::getSong()->controllers().at( 
					m_userController->model()->value() );
		}
	}
	accept();
}




void controllerConnectionDialog::midiToggled( void )
{
	int enabled = m_midiGroupBox->model()->value();
	if( enabled != 0 )
	{
		if( m_userGroupBox->model()->value() != 0 )
		{
			m_userGroupBox->model()->setValue( 0 );
		}

		if( !m_midiController )
		{
			m_midiController = new autoDetectMidiController( engine::getSong() );
			m_midiChannelSpinBox->setModel( 
					m_midiController->midiChannelModel() );
			m_midiControllerSpinBox->setModel( 
					m_midiController->midiControllerModel() );


			connect( m_midiController, SIGNAL( valueChanged() ), 
				this, SLOT( midiValueChanged() ) );

			updateReadablePortsMenu();
		}
	}
	else
	{
		m_midiAutoDetect.setValue( FALSE );
	}

	m_midiChannelSpinBox->setEnabled( enabled );
	m_midiControllerSpinBox->setEnabled( enabled );
	m_midiAutoDetectCheckBox->setEnabled( enabled );
}




void controllerConnectionDialog::userToggled( void )
{
	int enabled = m_userGroupBox->model()->value();
	if( enabled != 0 && m_midiGroupBox->model()->value() != 0 )
	{
		m_midiGroupBox->model()->setValue( 0 );
	}

	m_userController->setEnabled( enabled );
}




void controllerConnectionDialog::autoDetectToggled( void )
{
	if( m_midiAutoDetect.value() )
	{
		m_midiController->reset();
	}
}




void controllerConnectionDialog::midiValueChanged( void )
{
	if( m_midiAutoDetect.value() )
	{
		m_midiController->useDetected();

		m_midiAutoDetect.setValue( FALSE );
	}
}




void controllerConnectionDialog::activatedReadablePort( QAction * _item )
{
	// make sure, MIDI-port is configured for input
	if( _item->isChecked() == TRUE &&
		m_midiController->m_midiPort->mode() != midiPort::Input &&
		m_midiController->m_midiPort->mode() != midiPort::Duplex )
	{
		//mio->m_receiveEnabledModel.setValue( TRUE );
	}
	engine::getMixer()->getMIDIClient()->subscribeReadablePort(
			m_midiController->m_midiPort, _item->text(), !_item->isChecked() );
	m_midiController->m_readablePorts[_item->text()] = _item->isChecked();
}




void controllerConnectionDialog::updateReadablePortsMenu( void )
{
	if( m_readablePorts )
	{
		m_readablePorts->clear();
		for( midiController::midiPortMap::const_iterator it =
					m_midiController->m_readablePorts.begin();
				it != m_midiController->m_readablePorts.end(); ++it )
		{
			QAction * a = m_readablePorts->addAction( it.key() );
			a->setCheckable( TRUE );
			a->setChecked( *it );
		}
	}
}

#include "controller_connection_dialog.moc"

#endif
