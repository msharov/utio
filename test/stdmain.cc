// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#define _GNU_SOURCE 1	// for strsignal
#include "stdmain.h"
#include <signal.h>

/// Called when a signal is received.
static void OnSignal (int sig)
{
    cout.flush();
    #if HAVE_STRSIGNAL
	cerr.format ("Fatal error: %s received.\n", strsignal(sig));
    #else
	cerr.format ("Fatal error: system signal %d received.\n", sig);
    #endif
    cerr.flush();
    exit (sig);
}

/// Called by the framework on unrecoverable exception handling errors.
static void Terminate (void)
{
    assert (!"Unrecoverable exception handling error detected.");
    raise (SIGABRT);
    exit (EXIT_FAILURE);
}

/// Called when an exception violates a throw specification.
static void OnUnexpected (void)
{
    cerr << "Fatal internal error: unexpected exception caught.\n";
    Terminate();
}

/// Installs OnSignal as handler for signals.
extern "C" void InstallCleanupHandlers (void)
{
    static const uint8_t c_Signals[] = {
	SIGINT, SIGQUIT, SIGILL,  SIGTRAP, SIGABRT,
	SIGIOT, SIGBUS,  SIGFPE,  SIGSEGV, SIGTERM,
	SIGIO,  SIGCHLD
    };
    for (size_t i = 0; i < VectorSize(c_Signals); ++ i)
	signal (c_Signals[i], OnSignal);
    std::set_terminate (Terminate);
    std::set_unexpected (OnUnexpected);
}
