// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// Standard program main with error handling.
//
// The reason for using the singleton and all this error handling is that it
// is very important for CKeyboard object to be able to clean up on exit. If
// it fails to do so, the keyboard remains in the UI state and the shell will
// not be very usable. There will be no echo, no cursor, and no scrollback.
// (You can manually fix it with "stty sane" and "echo ^v ESC c")
//
// So main installs various cleanup handlers to catch fatal conditions, like
// signals, exceptions, or terminates, and exit normally, calling destructors
// on all objects. The singleton pattern is required because if the program
// exits from a signal handler, destructors of local objects in main will not
// be called, resulting in a garbled console. Hence the static Instance()
// function, whose static member object is destructed on exit.
//
// This cleanup problem is common to all UI environments, be it console, the
// framebuffer, or X. Crashing the UI environment without cleanup is a very
// bad idea and you will have to write something like the code in this file
// for every UI framework you choose to implement on top of utio.

#pragma once
#include <utio.h>
using namespace utio;
using namespace utio::gdt;
using namespace ustl;

extern "C" void InstallCleanupHandlers (void) noexcept;

/// Declares singleton Instance() call in \p TestClass
#define DECLARE_SINGLETON(TestClass)	\
    static TestClass& Instance (void) { static TestClass obj; return obj; }

/// Exception handling harness for demos
template <typename T>
int RunTest (void) noexcept
{
    int rv = EXIT_FAILURE;
    try {
	T::Instance().Run();
	rv = EXIT_SUCCESS;
    } catch (ustl::exception& e) {
	cout.flush();
	cerr << "Error: " << e << endl;
    } catch (...) {
	cout.flush();
	cerr << "Unexpected error." << endl;
    }
    return rv;
}

/// Standard main with error handling
#define StdTestMain(TestClass)	\
int main (void)			\
{				\
    InstallCleanupHandlers();	\
    return RunTest<TestClass>();\
}
