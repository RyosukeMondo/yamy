//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// engine_input.cpp


#include "misc.h"

#include "engine.h"
#include "errormessage.h"
#include "hook.h"
#include "mayurc.h"
#include "windowstool.h"

#include <iomanip>
#include <process.h>


unsigned int Engine::injectInput(const KEYBOARD_INPUT_DATA *i_kid, const KBDLLHOOKSTRUCT *i_kidRaw)
{
	InjectionContext ctx;
	ctx.isDragging = false; // TODO: Query from InputHook if necessary
	ctx.dragStartPos.x = 0;
	ctx.dragStartPos.y = 0;
	
	m_inputInjector->inject(i_kid, ctx, i_kidRaw);
	return 1;
}


// pop all pressed key on win32
void Engine::keyboardResetOnWin32()
{
	for (Keyboard::KeyIterator
			i = m_setting->m_keyboard.getKeyIterator();  *i; ++ i) {
		if ((*i)->m_isPressedOnWin32)
			generateKeyEvent((*i), false, true);
	}
}
