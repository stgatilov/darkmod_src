#pragma hdrstop
#include "../darkmod/KeyboardHook.h"

#ifdef _WINDOWS_
#include "../darkmod/KeyboardHookWindows.h"
#endif
// We will add additional ones for other OS here later

static bool init_version = FileVersionList("$Source$  $Revision$   $Date$", init_version);

CKeyCode::CKeyCode():
KeyState(KS_FREE),
Impulse(0),
VirtualKeyCode(0),
RepeatCount(0),
ScanCode(0),
KeyMask(0),
KeyPressCount(0)
{
}

CKeyCode::~CKeyCode()
{
}

CKeyCode::CKeyCode( const CKeyCode& Copy ):
KeyState(KS_FREE),
Impulse(0),
VirtualKeyCode(0),
RepeatCount(0),
ScanCode(0),
KeyMask(0),
KeyPressCount(0)
{
	Clone( Copy );
}

CKeyCode::CKeyCode( const CKeyCode* Copy ):
KeyState(KS_FREE),
Impulse(0),
VirtualKeyCode(0),
RepeatCount(0),
ScanCode(0),
KeyMask(0),
KeyPressCount(0)
{
	Clone( *Copy );
}

const CKeyCode& CKeyCode::Clone( const CKeyCode& Clone )
{
	KeyState = Clone.KeyState;
	Impulse  = Clone.Impulse;
	VirtualKeyCode = Clone.VirtualKeyCode;
	RepeatCount = Clone.RepeatCount;
	ScanCode = Clone.ScanCode;
	KeyMask = Clone.KeyMask;
	KeyPressCount = Clone.KeyPressCount;
	return *this;
}

const CKeyCode& CKeyCode::operator=( const CKeyCode& Copy )
{
	return Clone( Copy );
}

const CKeyCode* CKeyCode::operator=( const CKeyCode* Copy )
{
	return & Clone( Copy );
}

KeyState_t CKeyCode::GetKeyState() const
{
	return KeyState;
}

bool CKeyCode::GetAlt( void ) const 
{
	bool rc = false;
	if( GetAltRight() || GetAltLeft() )
	{
		rc = true;
	}
	return rc;
}

bool CKeyCode::GetAltRight( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_ALT_RIGHT ) == true );
}

bool CKeyCode::GetAltLeft( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_ALT_LEFT ) == true );
}

bool CKeyCode::GetCtrl( void ) const
{
	bool rc = false;
	if( GetCtrlRight() || GetCtrlLeft() )
	{
		rc = true;
	}
	return rc;
}

bool CKeyCode::GetCtrlRight( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_CTRL_RIGHT ) == true );
}

bool CKeyCode::GetCtrlLeft( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_CTRL_LFFT ) == true );
}

bool CKeyCode::GetShift( void ) const
{
	bool rc = false;
	if( GetShiftRight() || GetShiftLeft() )
	{
		rc = true;
	}
	return rc;
}

bool CKeyCode::GetShiftRight( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_SHIFT_RIGHT ) == true );
}

bool CKeyCode::GetShiftLeft( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_SHIFT_LEFT ) == true );
}

bool CKeyCode::GetExtended( void ) const
{
	return ( BITCHK( KeyMask, KEYSTATE_EXTENDED ) == true );
}

bool CKeyCode::GetPressed( void ) const//currently pressed - False if just released
{
	return ( BITCHK( KeyMask, KEYSTATE_PRESSED ) == true );
}

bool CKeyCode::GetWasPressed( void ) const// Was it pressed last message
{
	return ( BITCHK( KeyMask, KEYSTATE_PRESSED_LAST ) == true );
}

bool CKeyboardHook::m_instanceFlag = false;
CKeyboardHook* CKeyboardHook::m_single = NULL;

CKeyboardHook* CKeyboardHook::getInstance()
{
    if(! m_instanceFlag)
    {
        m_single = new CKeyboardHook();
        m_instanceFlag = true;
    }
	return m_single;
}
CKeyboardHook::CKeyboardHook(void):
	m_hook(NULL),
	m_KeyPressCount(0),
	m_bKeyCapActive(false),
	m_KeyCapImpulse(IR_COUNT),
	m_KeyCapStartCount(0)
{
	for( int i = 0; i < IR_COUNT; i++)
	{
		m_KeyData[i].KeyState = KS_FREE;
		m_KeyData[i].Impulse = -1;
	}
#ifdef _WINDOWS_
	m_hook = CKeyboardHookWindows::getInstance( this );
#endif // #ifdef _WINDOWS_
}

CKeyboardHook::~CKeyboardHook(void)
{
	delete m_hook;
}

const CKeyCode& CKeyboardHook::GetCurrentKey() const
{
	return m_KeyPress;
}

bool CKeyboardHook::KeyCapture( void )
{
	bool bReturnVal( false );

	// make sure key press happened since we started listening
	if( m_KeyPress.KeyPressCount >= m_KeyCapStartCount )
	{
		ImpulseInit( m_KeyCapImpulse );
		m_bKeyCapActive = false;
		bReturnVal = true;
		
		DM_LOG(LC_SYSTEM,LT_DEBUG)LOGSTRING("KeyCap: Grabbed virtual keycode %d for impulse function %d \r", m_KeyData[ m_KeyCapImpulse ].VirtualKeyCode, m_KeyCapImpulse );
	}
	return bReturnVal;
}

void CKeyboardHook::KeyCaptureStart( ImpulseFunction_t action )
{
	if( action < IR_COUNT && action >= 0 )
	{
		// Clear the old data
		ImpulseFree( action );
		m_bKeyCapActive = true;
		// When using the console command for testing, we need to add a few more keypresses
		// so that it doesn't pick up the still held down return key
		// TODO: Remove this for the GUI-called version
		m_KeyCapStartCount = m_KeyPressCount + 2;
		m_KeyCapImpulse = action;
	}
	else
	{
		DM_LOG(LC_SYSTEM,LT_WARNING)LOGSTRING("WARNING: KeyCaptureStart called with invalid TDM impulse index: %d \r", action);
	}
	return;
}

void CKeyboardHook::ImpulseFree(ImpulseFunction_t Function)
{
	m_KeyData[Function].KeyState = KS_FREE;
	m_KeyData[Function].Impulse = -1;
}

bool CKeyboardHook::ImpulseInit(ImpulseFunction_t Function )
{
	bool rc = true;

	if(m_KeyData[Function].KeyState == KS_FREE)
	{
		m_KeyData[Function] = m_KeyPress;
		m_KeyData[Function].Impulse = -1;
		m_KeyData[Function].KeyState = KS_UPDATED;
		rc = false;
	}
	return rc;
}

bool CKeyboardHook::ImpulseIsUpdated(ImpulseFunction_t Function)
{
	return ( KS_UPDATED == m_KeyData[Function].KeyState );
}


void CKeyboardHook::ImpulseProcessed(ImpulseFunction_t Function)
{
	m_KeyData[Function].KeyState = KS_PROCESSED;
}