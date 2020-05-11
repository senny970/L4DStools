//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef TIER1_H
#define TIER1_H

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ICvar;
class IProcessUtils;


//-----------------------------------------------------------------------------
// These tier1 libraries must be set by any users of this library.
// They can be set by calling ConnectTier1Libraries.
// It is hoped that setting this, and using this library will be the common mechanism for
// allowing link libraries to access tier1 library interfaces
//-----------------------------------------------------------------------------

// These are marked DLL_EXPORT for Linux.
DLL_EXPORT ICvar *cvar;
DLL_EXPORT ICvar *g_pCVar;
extern IProcessUtils *g_pProcessUtils;


//-----------------------------------------------------------------------------
// Call this to connect to/disconnect from all tier 1 libraries.
// It's up to the caller to check the globals it cares about to see if ones are missing
//-----------------------------------------------------------------------------
void ConnectTier1Libraries( CreateInterfaceFn *pFactoryList, int nFactoryCount );
void DisconnectTier1Libraries();


//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem for tier2 libraries
//-----------------------------------------------------------------------------


#endif // TIER1_H

