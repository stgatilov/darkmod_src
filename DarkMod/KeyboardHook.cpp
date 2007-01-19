#pragma hdrstop
#include "../darkmod/KeyboardHook.h"

#ifdef _WINDOWS_
#include "../darkmod/KeyboardHookWindows.h"
#endif
// We will add additional ones for other OS here later

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
	memset( &m_KeyPress, 0, sizeof(KeyCode_t) );
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
		memcpy(&m_KeyData[Function], &m_KeyPress, sizeof(KeyCode_t));
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