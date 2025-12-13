//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ipc_channel_interface.cpp
// Provides MOC implementation for IIPCChannel Qt interface
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "ipc_channel_interface.h"

#if defined(QT_CORE_LIB)
// This file exists solely to trigger MOC generation for IIPCChannel
// which is declared with Q_OBJECT in the header.
// Without this, targets that use IPCChannelNull would fail to link
// due to missing vtable entries for IIPCChannel's Qt meta-object system.
#endif
