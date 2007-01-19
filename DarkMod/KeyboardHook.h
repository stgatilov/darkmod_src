#ifndef CKEYBOARDHOOK_HPP
#define CKEYBOARDHOOK_HPP

#pragma once
#include "../idlib/precompiled.h"

class CKeyboardHook;
class CKeyboardHookWindows;

class CKeyboardHookBase // Base class for all Keyboard hook classes
{
public:
	CKeyboardHookBase( CKeyboardHook* pParent = NULL ){}
	virtual ~CKeyboardHookBase() = 0 {}
};

class CKeyboardHook
{
	friend class CKeyboardHookWindows;// Handles the actual hooking under windows
public:
	virtual ~CKeyboardHook(void);
	static CKeyboardHook* getInstance();

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
	KeyCode_t				*ImpulseData(ImpulseFunction_t Function) { return &m_KeyData[Function]; };

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

protected:

	/********************************************************************************************************
	*
	*   Key and Impulse Variables
	*
	********************************************************************************************************/

	/**
	* Key press count.  Used by the keyboard callback to count up key presses
	* starting from the first.  Used to test up-to-dateness.  
	* Incremented on each recorded keypress with nCode > 0.
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
	KeyCode_t				m_KeyPress;				// Current keypress
	KeyCode_t				m_KeyData[IR_COUNT];	// Keypress associated with an IMPULSEprivate:

private:
	// this is a base class* for the OS dependant hook class
	CKeyboardHookBase* m_hook;

	// singleton stuff goes here
	CKeyboardHook(void);
    static bool m_instanceFlag;
    static CKeyboardHook* m_single;
};

#endif // #ifndef CKEYBOARDHOOK_HPP