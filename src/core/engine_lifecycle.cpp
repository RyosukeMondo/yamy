//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_lifecycle.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


Engine::Engine(tomsgstream &i_log)
		: m_hwndAssocWindow(NULL),
		m_setting(NULL),
		m_buttonPressed(false),
		m_dragging(false),
		m_keyboardHandler(installKeyboardHook, Engine::keyboardDetour),
		m_mouseHandler(installMouseHook, Engine::mouseDetour),
		m_inputQueue(NULL),
		m_readEvent(NULL),
		m_queueMutex(NULL),
		m_sts4mayu(NULL),
		m_cts4mayu(NULL),
		m_isLogMode(false),
		m_isEnabled(true),
		m_isSynchronizing(false),
		m_eSync(NULL),
		m_generateKeyboardEventsRecursionGuard(0),
		m_currentKeyPressCount(0),
		m_currentKeyPressCountOnWin32(0),
		m_lastGeneratedKey(NULL),
		m_oneShotRepeatableRepeatCount(0),
		m_isPrefix(false),
		m_currentKeymap(NULL),
		m_currentFocusOfThread(NULL),
		m_hwndFocus(NULL),
		m_afShellExecute(NULL),
		m_variable(0),
		m_log(i_log) {
	BOOL (WINAPI *pChangeWindowMessageFilter)(UINT, DWORD) =
		reinterpret_cast<BOOL (WINAPI*)(UINT, DWORD)>(GetProcAddress(GetModuleHandle(_T("user32.dll")), "ChangeWindowMessageFilter"));

	if(pChangeWindowMessageFilter != NULL) {
		pChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
	}

	for (size_t i = 0; i < NUMBER_OF(m_lastPressedKey); ++ i)
		m_lastPressedKey[i] = NULL;

	// set default lock state
	for (int i = 0; i < Modifier::Type_end; ++ i)
		m_currentLock.dontcare(static_cast<Modifier::Type>(i));
	for (int i = Modifier::Type_Lock0; i <= Modifier::Type_Lock9; ++ i)
		m_currentLock.release(static_cast<Modifier::Type>(i));

	// create event for sync
	CHECK_TRUE( m_eSync = CreateEvent(NULL, FALSE, FALSE, NULL) );
	// create named pipe for &SetImeString
	m_hookPipe = CreateNamedPipe(addSessionId(HOOK_PIPE_NAME).c_str(),
								 PIPE_ACCESS_OUTBOUND,
								 PIPE_TYPE_BYTE, 1,
								 0, 0, 0, NULL);
	StrExprArg::setEngine(this);

	m_msllHookCurrent.pt.x = 0;
	m_msllHookCurrent.pt.y = 0;
	m_msllHookCurrent.mouseData = 0;
	m_msllHookCurrent.flags = 0;
	m_msllHookCurrent.time = 0;
	m_msllHookCurrent.dwExtraInfo = 0;
}


Engine::~Engine() {
	CHECK_TRUE( CloseHandle(m_eSync) );

	// destroy named pipe for &SetImeString
	if (m_hookPipe && m_hookPipe != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(m_hookPipe);
		CHECK_TRUE( CloseHandle(m_hookPipe) );
	}
}


// start keyboard handler thread
void Engine::start() {
	m_keyboardHandler.start(this);
	m_mouseHandler.start(this);

	CHECK_TRUE( m_inputQueue = new std::deque<KEYBOARD_INPUT_DATA> );
	CHECK_TRUE( m_queueMutex = CreateMutex(NULL, FALSE, NULL) );
	CHECK_TRUE( m_readEvent = CreateEvent(NULL, TRUE, FALSE, NULL) );
	m_ol.Offset = 0;
	m_ol.OffsetHigh = 0;
	m_ol.hEvent = m_readEvent;

	CHECK_TRUE( m_threadHandle = (HANDLE)_beginthreadex(NULL, 0, keyboardHandler, this, 0, &m_threadId) );
}


// stop keyboard handler thread
void Engine::stop() {
	m_mouseHandler.stop();
	m_keyboardHandler.stop();

	WaitForSingleObject(m_queueMutex, INFINITE);
	delete m_inputQueue;
	m_inputQueue = NULL;
	SetEvent(m_readEvent);
	ReleaseMutex(m_queueMutex);

	WaitForSingleObject(m_threadHandle, 2000);
	CHECK_TRUE( CloseHandle(m_threadHandle) );
	m_threadHandle = NULL;

	CHECK_TRUE( CloseHandle(m_readEvent) );
	m_readEvent = NULL;

	for (ThreadIds::iterator i = m_attachedThreadIds.begin();
		 i != m_attachedThreadIds.end(); i++) {
		 PostThreadMessage(*i, WM_NULL, 0, 0);
	}
}


bool Engine::prepairQuit() {
	// terminate and unload DLL for ThumbSense support if loaded
	manageTs4mayu(_T("sts4mayu.dll"), _T("SynCOM.dll"),
				  false, &m_sts4mayu);
	manageTs4mayu(_T("cts4mayu.dll"), _T("TouchPad.dll"),
				  false, &m_cts4mayu);
	return true;
}


// sync
bool Engine::syncNotify() {
	Acquire a(&m_cs);
	if (!m_isSynchronizing)
		return false;
	CHECK_TRUE( SetEvent(m_eSync) );
	return true;
}


// update m_lastPressedKey
void Engine::updateLastPressedKey(Key *i_key)
{
	m_lastPressedKey[1] = m_lastPressedKey[0];
	m_lastPressedKey[0] = i_key;
}


// set current keymap
void Engine::setCurrentKeymap(const Keymap *i_keymap, bool i_doesAddToHistory)
{
	if (i_doesAddToHistory) {
		m_keymapPrefixHistory.push_back(const_cast<Keymap *>(m_currentKeymap));
		if (MAX_KEYMAP_PREFIX_HISTORY < m_keymapPrefixHistory.size())
			m_keymapPrefixHistory.pop_front();
	} else
		m_keymapPrefixHistory.clear();
	m_currentKeymap = i_keymap;
}


unsigned int WINAPI Engine::InputHandler::run(void *i_this)
{
	reinterpret_cast<InputHandler*>(i_this)->run();
	_endthreadex(0);
	return 0;
}

Engine::InputHandler::InputHandler(INSTALL_HOOK i_installHook, INPUT_DETOUR i_inputDetour)
	: m_installHook(i_installHook), m_inputDetour(i_inputDetour)
{
	CHECK_TRUE(m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL));
	CHECK_TRUE(m_hThread = (HANDLE)_beginthreadex(NULL, 0, run, this, CREATE_SUSPENDED, &m_threadId));
}

Engine::InputHandler::~InputHandler()
{
	CloseHandle(m_hEvent);
}

void Engine::InputHandler::run()
{
	MSG msg;

	CHECK_FALSE(m_installHook(m_inputDetour, m_engine, true));
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(m_hEvent);

	while (GetMessage(&msg, NULL, 0, 0)) {
		// nothing to do...
	}

	CHECK_FALSE(m_installHook(m_inputDetour, m_engine, false));

	return;
}

int Engine::InputHandler::start(Engine *i_engine)
{
	m_engine = i_engine;
	ResumeThread(m_hThread);
	WaitForSingleObject(m_hEvent, INFINITE);
	return 0;
}

int Engine::InputHandler::stop()
{
	PostThreadMessage(m_threadId, WM_QUIT, 0, 0);
	WaitForSingleObject(m_hThread, INFINITE);
	return 0;
}
