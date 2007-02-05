#ifndef CKEYBOARDHOOK_HPP
#define CKEYBOARDHOOK_HPP

#pragma once
#include "../idlib/precompiled.h"

class CKeyboard;
class CKeyboardWindows;// do NOT include headers for OS specific classes here!

// Any key that is to be changed from an impulse to a button behaviour
// has to be listed here. The id gives the index in the array which slot
// is reserved for that function.
typedef enum {
	IR_FROB,
	IR_INVENTORY_NEXT,
	IR_INVENTORY_PREV,
	IR_LEAN_FORWARD,
	IR_LEAN_LEFT,
	IR_LEAN_RIGHT,
	IR_COUNT
} ImpulseFunction_t;

typedef enum {
	KS_UPDATED,			// Keyinfo has been updated by the hook
	KS_PROCESSED,		// Key has been processed by the gameengine
	KS_FREE,			// Keyslot is currently free.
	KS_COUNT
} KeyState_t;

/**
 * KeyCode is a structure that contains the information for a key which is related
 * to an IMPULSE.
 */

#define KEYSTATE_ALT_RIGHT    BIT(1)
#define KEYSTATE_ALT_LEFT     BIT(2)
#define KEYSTATE_SHIFT_RIGHT  BIT(3)
#define KEYSTATE_SHIFT_LEFT   BIT(4)
#define KEYSTATE_CTRL_RIGHT   BIT(5)
#define KEYSTATE_CTRL_LFFT    BIT(6)
#define KEYSTATE_PRESSED_LAST BIT(7)// pressed last message as well
#define KEYSTATE_PRESSED      BIT(8)// pressed now
#define KEYSTATE_EXTENDED     BIT(9)//is this an extended key

class CKeyCode
{
	friend class CKeyboard;
	friend class CKeyboardWindows;
public:
	CKeyCode();
	~CKeyCode();
	CKeyCode( const CKeyCode& );
	CKeyCode( const CKeyCode* );
	const CKeyCode& Clone( const CKeyCode& );
	const CKeyCode& operator=( const CKeyCode& );
	const CKeyCode* operator=( const CKeyCode* );
	bool GetAlt( void ) const;
	bool GetAltRight( void ) const;
	bool GetAltLeft( void ) const;
	bool GetCtrl( void ) const;
	bool GetCtrlRight( void ) const;
	bool GetCtrlLeft( void ) const;
	bool GetShift( void ) const;
	bool GetShiftRight( void ) const;
	bool GetShiftLeft( void ) const;
	bool GetExtended( void ) const;
	bool GetPressed( void ) const;//currently pressed - False if just released
	bool GetWasPressed( void ) const;// Was it pressed last message
	KeyState_t GetKeyState() const;
protected:
	KeyState_t	KeyState;		// protocoll state for the interface with the gameengine
	int		Impulse;			// Impulsevalue this is associated with.
	int		VirtualKeyCode;
	int		RepeatCount;
	int		ScanCode;			// The value depends on the OEM.
	unsigned short KeyMask;       // See Above
	int		KeyPressCount;		// Count of this keypress (starts counting up from first key pressed)
};
//typedef CKeyCode KeyCode_t;

class CKeyboardHookBase // Base class for all Keyboard hook classes
{
public:
	CKeyboardHookBase( CKeyboard* pParent = NULL ){}
	virtual ~CKeyboardHookBase() = 0 {}
};

class CKeyboard
{
	friend class CKeyboardWindows;// Handles the actual hooking under windows
public:
	virtual ~CKeyboard(void);
	static CKeyboard* getInstance();

	/********************************************************************************************************
	*
	*  Impulse Functions
	*
	********************************************************************************************************/
	
	/**
	 * ImpulseInit will initialize a slot with the current keypress if it is empty.
	 * The function returns true if the slot has already been initialized for this 
	 * keypress. If false is returned the slot was free before and is now ready to use
	 * This should always be the first function to be called in order to determine wether
	 * an impulse has been triggered already for continous use.
	 */
	bool					ImpulseInit( ImpulseFunction_t Function );

	/**
	 * ImpulseIsUpdated checks wether the slot has been updated since the last time the impulse
	 * has been processed.
	 */
	bool					ImpulseIsUpdated(ImpulseFunction_t Function);

	/**
	 * ImpulseProcessed has to be called whenever the impulse function has processed it's
	 * keystate, but is not finished yet.
	 */
	void					ImpulseProcessed(ImpulseFunction_t Function);

	/**
	 * ImpulseFree is called when the processing of the impulse is finished and no further
	 * reporting should be done. This would usually be when the key is released.
	 */
	void					ImpulseFree(ImpulseFunction_t Function);

	/**
	 * ImpulseData returns the pointer to the keyinfo structure. The state should not be modified 
	 * via this pointer.
	 */
	const CKeyCode *ImpulseData(ImpulseFunction_t Function) { return &m_KeyData[Function]; };

	/********************************************************************************************************
	*
	*    Key capture functions
	*
	********************************************************************************************************/

	/**
	*	Called in the frame loop when we are binding keys to TDM buttons.  Attempts to capture
	* the currently pressed key to the desired impulse.  Returns true if key has been
	* captured, otherwise false.
	**/
	bool					KeyCapture( void );

	/**
	*	Called to activate KeyCapture and initialize the impulse we are binding
	* It takes the impulse ID for the action we want to bind the key to.
	**/
	void					KeyCaptureStart( ImpulseFunction_t action );


	const CKeyCode& GetCurrentKey() const;

protected:
	/********************************************************************************************************
	*
	*   Key and Impulse Variables
	*
	********************************************************************************************************/

	/**
	* Key press count.  Used by the keyboard callback to count up key presses
	* starting from the first.  Used to test up-to-dateness.  
	* Incremented on each recorded keypress with nCode == HC_ACTION.
	**/
	int						m_KeyPressCount;
	
	/**
	* Set to true if key capture is active, for identifying impulse
	* keys for the keyboard handler.
	**/
	bool					m_bKeyCapActive;
	
	/**
	* Impulse ID to associate with the key we're currently capturing
	**/
	ImpulseFunction_t		m_KeyCapImpulse;
	
	/**
	* The value of the current key press count when the key capture started, 
	* used to tell if keypress is current or from before.
	**/
	int						m_KeyCapStartCount;
	CKeyCode				m_KeyPress;				// Current keypress
	CKeyCode				m_KeyData[IR_COUNT];	// Keypress associated with an IMPULSEprivate:

private:
	// this is a base class* for the OS dependant hook class
	CKeyboardHookBase* m_hook;

	// singleton stuff goes here
	CKeyboard(void);
    static bool m_instanceFlag;
    static CKeyboard* m_single;
public:
	void ImpuleUpdate(void);
};

#endif // #ifndef CKEYBOARDHOOK_HPP