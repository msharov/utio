/* This file is part of bsconf - a configure replacement.
 *
 * Copyright (C) 2005 by Mike Sharov <msharov@users.sourceforge.net>
 * This file is free software, distributed under the MIT License.
 *
 * bsconf was written to replace autoconf-made configure scripts, which
 * through their prohibitively large size have fallen out of favor with
 * many developers. The configure script performs many time-consuming
 * tests, the results of which are usually unused because no progammer
 * likes to pollute his code with ifdefs. This program performs the
 * program and header lookup and substitutes @CONSTANTS@ in specified
 * files. bsconf.h is used to define which files, programs, and headers
 * to look for. config.h is generated, but the headers are only checked
 * for existence and the functions are assumed to exist (nobody is going
 * to ifdef around every memchr). Most compilers these days have all the
 * headers you want, and if yours does not, there is always gcc. There
 * is simply no excuse for using a braindead compiler when better ones
 * are freely available.
*/

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>

/*--------------------------------------------------------------------*/

#define BUFSIZE		0x10000
#define MAX_LIB_DIRS	16
#define MAX_INC_DIRS	16

/*--------------------------------------------------------------------*/

#define VectorSize(v)	(sizeof(v) / sizeof(*v))

/*#define const*/
/*typedef unsigned int	uint;*/
typedef char*		pchar_t;
typedef const char*	cpchar_t;
typedef const char	string_t [64];
typedef char		strbuf_t [128];

typedef enum {
    vv_prefix,
    vv_exec_prefix,
    vv_bindir,
    vv_sbindir,
    vv_libexecdir,
    vv_datadir,
    vv_sysconfdir,
    vv_sharedstatedir,
    vv_localstatedir,
    vv_libdir,
    vv_gcclibdir,
    vv_includedir,
    vv_oldincludedir,
    vv_gccincludedir,
    vv_custominclude,
    vv_customlib,
    vv_infodir,
    vv_mandir,
    vv_build,
    vv_host,
    vv_last
} EVV;

/*--------------------------------------------------------------------*/

static void GetConfigVarValues (int argc, cpchar_t const* argv);
static void FillInDefaultConfigVarValues (void);
static void FindPrograms (void);
static void SubstitutePaths (void);
static void SubstituteEnvironment (int bForce);
static void SubstitutePrograms (void);
static void SubstituteCpuCaps (void);
static void SubstituteCFlags (void);
static void SubstituteHostOptions (void);
static void SubstituteHeaders (void);
static void SubstituteLibs (void);
static void SubstituteFunctions (void);
static void SubstituteComponents (void);
static void SubstituteCustomVars (void);

static void DetermineHost (void);
static void DefaultConfigVarValue (EVV v, EVV root, cpchar_t suffix);
static void Substitute (cpchar_t matchStr, cpchar_t replaceStr);
static void ExecuteSubstitutionList (void);
static void MakeSubstString (cpchar_t str, pchar_t substString);
static cpchar_t CopyPathEntry (cpchar_t pi, pchar_t dest);
static int  IsBadInstallDir (cpchar_t match);

/* Unreliable (according to autoconf) libc stuff */
static int   StrLen (cpchar_t str);
static void  Lowercase (pchar_t str);
static int   compare (cpchar_t str1, cpchar_t str2);
static pchar_t copy (cpchar_t src, pchar_t dest);
static pchar_t copy_n (cpchar_t src, pchar_t dest, int n);
static pchar_t append (cpchar_t src, pchar_t dest);
static pchar_t append2 (cpchar_t src1, cpchar_t src2, pchar_t dest);
static void  fill_n (pchar_t str, int n, char v);
static pchar_t copy_backward (cpchar_t src, pchar_t dest, int n);
static void  ReadFile (cpchar_t filename);
static void  WriteFile (cpchar_t filename);
static void  FatalError (cpchar_t errortext);

/*--------------------------------------------------------------------*/

static int  g_BufSize;
static char g_Buf [BUFSIZE];

typedef struct {
    int		m_bDefaultOn;
    cpchar_t	m_Description;
} SComponentInfo;

#include "bsconf.h"

static cpchar_t g_ConfigV [vv_last] = {
    "prefix",
    "exec_prefix",
    "bindir",
    "sbindir",
    "libexecdir",
    "datadir",
    "sysconfdir",
    "sharedstatedir",
    "localstatedir",
    "libdir",
    "gcclibdir",
    "includedir",
    "oldincludedir",
    "gccincludedir",
    "custominclude",
    "customlib",
    "infodir",
    "mandir",
    "build",
    "host"
};

static strbuf_t g_ConfigVV [vv_last];
static strbuf_t g_ProgLocs [VectorSize (g_ProgVars) / 4];
static strbuf_t g_CustomLibDirs [MAX_LIB_DIRS];
static strbuf_t g_CustomIncDirs [MAX_INC_DIRS];
static int g_nCustomLibDirs = 0;
static int g_nCustomIncDirs = 0;

static struct utsname g_Uname;

typedef struct {
    uint	m_Bit;
    cpchar_t	m_Description;
    cpchar_t	m_Disabled;
    cpchar_t	m_Enabled;
} SCpuCaps;

static const SCpuCaps g_CpuCaps [] = {
    {  0, "FPU",	"#undef CPU_HAS_FPU",		"#define CPU_HAS_FPU 1"		},
    {  2, "DEBUG",	"#undef CPU_HAS_EXT_DEBUG",	"#define CPU_HAS_EXT_DEBUG 1"	},
    {  4, "TimeStamp",	"#undef CPU_HAS_TIMESTAMPC",	"#define CPU_HAS_TIMESTAMPC 1"	},
    {  5, "MSR",	"#undef CPU_HAS_MSR",		"#define CPU_HAS_MSR 1"		},
    {  8, "CMPXCHG8",	"#undef CPU_HAS_CMPXCHG8",	"#define CPU_HAS_CMPXCHG8 1"	},
    {  9, "APIC",	"#undef CPU_HAS_APIC",		"#define CPU_HAS_APIC 1"	},
    { 11, "SYSCALL",	"#undef CPU_HAS_SYSCALL",	"#define CPU_HAS_SYSCALL 1"	},
    { 12, "MTRR",	"#undef CPU_HAS_MTRR",		"#define CPU_HAS_MTRR 1"	},
    { 15, "CMOV",	"#undef CPU_HAS_CMOV",		"#define CPU_HAS_CMOV 1"	},
    { 16, "FCMOV",	"#undef CPU_HAS_FCMOV",		"#define CPU_HAS_FCMOV 1"	},
    { 22, "SSE",	"#undef CPU_HAS_SSE ",		"#define CPU_HAS_SSE 1"		},
    { 23, "MMX",	"#undef CPU_HAS_MMX",		"#define CPU_HAS_MMX 1"		},
    { 24, "FXSAVE",	"#undef CPU_HAS_FXSAVE",	"#define CPU_HAS_FXSAVE 1"	},
    { 25, "SSE",	"#undef CPU_HAS_SSE ",		"#define CPU_HAS_SSE 1"		},
    { 26, "SSE2",	"#undef CPU_HAS_SSE2",		"#define CPU_HAS_SSE2 1"	},
    { 27, "SSE3",	"#undef CPU_HAS_SSE3",		"#define CPU_HAS_SSE3 1"	},
    { 30, "3dNow!+",	"#undef CPU_HAS_EXT_3DNOW",	"#define CPU_HAS_EXT_3DNOW 1"	},
    { 31, "3dNow!",	"#undef CPU_HAS_3DNOW",		"#define CPU_HAS_3DNOW 1"	}
};
static uint g_CpuCapBits = 0;

typedef enum {
    sys_Unknown,
    sys_Linux,
    sys_Mac,
    sys_Bsd,
    sys_Sun,
    sys_Alpha
} ESysType;

typedef struct {
    cpchar_t	sysname;
    ESysType	type;
} SHostType;

static const SHostType g_HostTypes[] = {
    { "linux",		sys_Linux },
    { "sun",		sys_Sun },
    { "solaris",	sys_Sun },
    { "openbsd",	sys_Bsd },
    { "netbsd",		sys_Bsd },
    { "freebsd",	sys_Bsd },
    { "osx",		sys_Mac },
    { "darwin",		sys_Mac },
    { "alpha",		sys_Alpha }
};

static ESysType g_SysType = sys_Unknown;

static cpchar_t g_LibSuffixes[] = { ".a", ".so", ".la" };

static uint g_nSubs = 0;
static strbuf_t g_Subs [MAX_SUBSTITUTIONS * 2];

/*--------------------------------------------------------------------*/

int main (int argc, const char* const* argv)
{
    uint f;
    strbuf_t srcFile;

    GetConfigVarValues (--argc, ++argv);
    FillInDefaultConfigVarValues();

    FindPrograms();
    SubstituteComponents();
    SubstituteHostOptions();
    SubstituteCpuCaps();
    SubstituteCFlags();
    SubstitutePaths();
    SubstituteEnvironment (0);
    SubstitutePrograms();
    SubstituteHeaders();
    SubstituteLibs();
    SubstituteFunctions();
    SubstituteCustomVars();
    SubstituteEnvironment (1);

    for (f = 0; f < VectorSize(g_Files); ++ f) {
	copy (g_Files[f], srcFile);
	append (".in", srcFile);
	ReadFile (srcFile);
	ExecuteSubstitutionList();
	WriteFile (g_Files[f]);
    }
    return (0);
}

static void PrintHelp (void)
{
    uint i;
    printf (
"This program configures " PACKAGE_STRING " to adapt to many kinds of systems.\n"
"\n"
"Usage: configure [OPTION] ...\n"
"\n"
"Configuration:\n"
"  --help\t\tdisplay this help and exit\n"
"  --version\t\tdisplay version information and exit\n"
"\n"
"Installation directories:\n"
"  --prefix=PREFIX\tarchitecture-independent files [/usr/local]\n"
"  --exec-prefix=EPREFIX\tarchitecture-dependent files [PREFIX]\n"
"  --bindir=DIR\t\tuser executables [EPREFIX/bin]\n"
"  --sbindir=DIR\t\tsystem admin executables [EPREFIX/sbin]\n"
"  --libexecdir=DIR\tprogram executables [EPREFIX/libexec]\n"
"  --datadir=DIR\t\tread-only architecture-independent data [PREFIX/share]\n"
"  --sysconfdir=DIR\tread-only single-machine data [PREFIX/etc]\n"
"  --sharedstatedir=DIR\tmodifiable architecture-independent data [PREFIX/com]\n"
"  --localstatedir=DIR\tmodifiable single-machine data [PREFIX/var]\n"
"  --libdir=DIR\t\tobject code libraries [EPREFIX/lib]\n"
"  --includedir=DIR\tC header files [PREFIX/include]\n"
"  --oldincludedir=DIR\tC header files for non-gcc [/usr/include]\n"
"  --gccincludedir=DIR\tGCC internal header files [PREFIX/include]\n"
"  --custominclude=DIR\tNonstandard header file location (cumulative)\n"
"  --customlib=DIR\tNonstandard library file location (cumulative)\n"
"  --infodir=DIR\t\tinfo documentation [PREFIX/info]\n"
"  --mandir=DIR\t\tman documentation [PREFIX/man]\n"
"\n"
"System types:\n"
"  --build=BUILD\t\tconfigure for building on BUILD [guessed]\n"
"  --host=HOST\t\tcross-compile to build programs to run on HOST [BUILD]\n"
"\n");
    if (VectorSize(g_Components)) {
	printf ("Options:\n");
	for (i = 0; i < VectorSize(g_ComponentInfos); ++ i) {
	    if (!g_ComponentInfos[i].m_Description[0])
		continue;
	    if (g_ComponentInfos[i].m_bDefaultOn)
		printf ("  --without-%-12s%s\n", g_Components[i * 3], g_ComponentInfos[i].m_Description);
	    else
		printf ("  --with-%-15s%s\n", g_Components[i * 3], g_ComponentInfos[i].m_Description);
	}
	printf ("\n");
    }
    printf (
"Some influential environment variables:\n"
"  CC\t\tC compiler\t\tCFLAGS\n"
"  CPP\t\tC preprocessor\t\tCPPFLAGS\n"
"  CXX\t\tC++ compiler\t\tCXXFLAGS\n"
"  LD\t\tLinker\t\t\tLDFLAGS\n"
"\n"
"Report bugs to " PACKAGE_BUGREPORT ".\n");
    exit (0);
}

static void PrintVersion (void)
{
    printf (PACKAGE_NAME " configure " PACKAGE_VERSION "\n"
	    "\nUsing bsconf package version 0.1\n"
	    "Copyright (c) 2003-2005, Mike Sharov <msharov@users.sourceforge.net>\n"
	    "This configure script and the bsconf package are free software.\n"
	    "Unlimited permission to copy, distribute, and modify is granted.\n");
    exit (0);
}

/*--------------------------------------------------------------------*/

static void GetConfigVarValues (int argc, cpchar_t const* argv)
{
    int a, apos, cvl;
    uint cv;
    for (cv = 0; cv < vv_last; ++ cv)
	fill_n (g_ConfigVV[cv], sizeof(strbuf_t), 0);
    /* --var=VALUE */
    for (a = 0; a < argc; ++ a) {
	if (!compare (argv[a], "--"))
	    continue;
	apos = 2;
	if (compare (argv[a] + apos, "help"))
	    PrintHelp();
	else if (compare (argv[a] + apos, "version"))
	    PrintVersion();
	else if (compare (argv[a] + apos, "with")) {
	    apos += 4;
	    if (compare (argv[a] + apos, "out"))
		apos += 3;
	    ++ apos;
	    for (cv = 0; cv < VectorSize(g_ComponentInfos); ++ cv)
		if (compare (argv[a] + apos, g_Components[cv * 3]))
		    g_ComponentInfos[cv].m_bDefaultOn = (apos == 7);
	} else {
	    for (cv = 0; cv < vv_last; ++ cv)
		if (compare (argv[a] + apos, g_ConfigV[cv]))
		    break;
	    if (cv == vv_last)
		continue;
	    apos += StrLen (g_ConfigV[cv]) + 1;
	    cvl = StrLen (argv[a]) - apos + 1;
	    if (cvl > 1) {
		copy_n (argv[a] + apos, g_ConfigVV[cv], cvl);
		if (cv == vv_customlib)
		    copy_n (argv[a] + apos, g_CustomLibDirs [g_nCustomLibDirs++], cvl);
		else if (cv == vv_custominclude)
		    copy_n (argv[a] + apos, g_CustomIncDirs [g_nCustomIncDirs++], cvl);
	    }
	    if (cv == vv_prefix && compare (g_ConfigVV[cv], "/home")) {
		copy (g_ConfigVV[cv], g_CustomLibDirs [g_nCustomLibDirs]);
		append ("/lib", g_CustomLibDirs [g_nCustomLibDirs++]);
		copy (g_ConfigVV[cv], g_CustomIncDirs [g_nCustomIncDirs]);
		append ("/include", g_CustomIncDirs [g_nCustomIncDirs++]);
	    }
	}
    }
}

static void DefaultConfigVarValue (EVV v, EVV root, cpchar_t suffix)
{
    if (!*(g_ConfigVV [v])) {
	copy (g_ConfigVV [root], g_ConfigVV [v]);
	append (suffix, g_ConfigVV [v]);
    }
}

static void FillInDefaultConfigVarValues (void)
{
    if (!*(g_ConfigVV [vv_prefix]))
	copy ("/usr/local", g_ConfigVV [vv_prefix]);
    else if (g_ConfigVV [vv_prefix][0] == '/' && !g_ConfigVV [vv_prefix][1])
	g_ConfigVV [vv_prefix][0] = 0;
    if (!*(g_ConfigVV [vv_exec_prefix]))
	DefaultConfigVarValue (vv_exec_prefix,	vv_prefix,	"");
    else if (g_ConfigVV [vv_exec_prefix][0] == '/' && !g_ConfigVV [vv_exec_prefix][1])
	g_ConfigVV [vv_exec_prefix][0] = 0;
    if (!*(g_ConfigVV [vv_oldincludedir]))
	copy ("/usr/include", g_ConfigVV [vv_oldincludedir]);

    DefaultConfigVarValue (vv_bindir,		vv_exec_prefix,	"/bin");
    DefaultConfigVarValue (vv_sbindir,		vv_exec_prefix,	"/sbin");
    DefaultConfigVarValue (vv_libexecdir,	vv_prefix,	"/libexec");
    DefaultConfigVarValue (vv_datadir,		vv_prefix,	"/share");
    DefaultConfigVarValue (vv_sysconfdir,	vv_prefix,	"/etc");
    DefaultConfigVarValue (vv_sharedstatedir,	vv_prefix,	"/com");
    DefaultConfigVarValue (vv_localstatedir,	vv_prefix,	"/var");
    DefaultConfigVarValue (vv_libdir,		vv_exec_prefix,	"/lib");
    DefaultConfigVarValue (vv_gcclibdir,	vv_exec_prefix,	"/lib");
    DefaultConfigVarValue (vv_includedir,	vv_prefix,	"/include");
    DefaultConfigVarValue (vv_gccincludedir,	vv_prefix,	"/include");
    DefaultConfigVarValue (vv_infodir,		vv_prefix,	"/info");
    DefaultConfigVarValue (vv_mandir,		vv_prefix,	"/man");

    if (!*(g_ConfigVV [vv_prefix]))
	copy ("/", g_ConfigVV [vv_prefix]);
    if (!*(g_ConfigVV [vv_exec_prefix]))
	copy ("/", g_ConfigVV [vv_exec_prefix]);

    if (!*(g_ConfigVV [vv_host]))
	DetermineHost();
    if (!*(g_ConfigVV [vv_build]))
	copy (g_ConfigVV [vv_host], g_ConfigVV [vv_build]);
}

static void DetermineHost (void)
{
    int i;
    fill_n ((pchar_t) &g_Uname, sizeof(struct utsname), 0);
    uname (&g_Uname);
    Lowercase (g_Uname.machine);
    Lowercase (g_Uname.sysname);
    copy (g_Uname.machine, g_ConfigVV [vv_host]);
    append ("-", g_ConfigVV [vv_host]);
#ifdef __GNUC__
    append ("gnu", g_ConfigVV [vv_host]);
#else
    append ("unknown", g_ConfigVV [vv_host]);
#endif
    append2 ("-", g_Uname.sysname, g_ConfigVV [vv_host]);
    for (i = 0; i < VectorSize(g_HostTypes); ++ i)
	if (compare (g_Uname.sysname, g_HostTypes[i].sysname))
	    g_SysType = g_HostTypes[i].type;
    if (compare (g_Uname.machine, "alpha"))
	g_SysType = sys_Alpha;
}

static cpchar_t CopyPathEntry (cpchar_t pi, pchar_t dest)
{
    while (*pi && *pi != ':')
	*dest++ = *pi++;
    *dest = 0;
    return (*pi ? ++pi : NULL);
}

static int IsBadInstallDir (cpchar_t match)
{
    return (compare (match, "/etc") ||
	    compare (match, "/usr/sbin") ||
	    compare (match, "/c") ||
	    compare (match, "/C") ||
	    compare (match, "/usr/etc") ||
	    compare (match, "/sbin") ||
	    compare (match, "/usr/ucb") ||
	    compare (match, "/usr/afsws/bin"));
}

static void FindPrograms (void)
{
    uint i, count;
    cpchar_t path, pi;
    strbuf_t match;

    path = getenv ("PATH");
    if (!path)
	path = "";

    for (i = 0; i < VectorSize(g_ProgLocs); ++ i) {
	fill_n (g_ProgLocs[i], sizeof(strbuf_t), 0);
	fill_n (match, sizeof(strbuf_t), 0);
	count = 0;
	for (pi = path; pi; pi = CopyPathEntry (pi, match)) {
	    /* Ignore "bad" versions of install, like autoconf does. */
	    if (compare (g_ProgVars[i * 4 + 1], "install") && IsBadInstallDir (match))
		continue;
	    append2 ("/", g_ProgVars[i * 4 + 1], match);
	    if (access (match, X_OK) == 0) {
		++ count;
		break;
	    }
	}
	if (count && compare (g_ProgVars[i * 4 + 1], "install"))
	    copy (match, g_ProgLocs[i]);
	else
	    copy (g_ProgVars[i * 4 + 2 + !count], g_ProgLocs[i]);
    }
}

static void SubstitutePaths (void)
{
    strbuf_t match;
    int cv;
    for (cv = 0; cv < vv_last; ++ cv) {
	MakeSubstString (g_ConfigV [cv], match);
	Substitute (match, g_ConfigVV [cv]);
    }
}

static void SubstituteCFlags (void)
{
    strbuf_t buf;
    int j;

    buf[0] = 0;
    for (j = 0; j < g_nCustomIncDirs; ++ j)
	append2 (" -I", g_CustomIncDirs[j], buf);
    Substitute ("@CUSTOMINCDIR@", buf);

    buf[0] = 0;
    for (j = 0; j < g_nCustomLibDirs; ++ j)
	append2 (" -L", g_CustomLibDirs[j], buf);
    Substitute ("@CUSTOMLIBDIR@", buf);

    buf[0] = 0;
    #if __GNUC__ >= 3
	if (g_CpuCapBits & (1 << 23))
	    append (" -mmmx", buf);
	if (g_SysType == sys_Linux)
	    if (g_CpuCapBits & ((1 << 22) | (1 << 25)))
		append (" -msse -mfpmath=sse", buf);
	if (g_CpuCapBits & (1 << 26))
	    append (" -msse2", buf);
	if (g_CpuCapBits & (1 << 27))
	    append (" -msse3", buf);
	if (g_CpuCapBits & ((1 << 30) | (1 << 31)))
	    append (" -m3dnow", buf);
    #endif
    Substitute ("@PROCESSOR_OPTS@", buf);

    buf[0] = 0;
    #if __GNUC__ >= 3
	append (" -finline-limit=65535", buf);
	#if __GNUC__ > 3 || __GNUC_MINOR__ >= 4
	    append (" --param large-function-growth=65535", buf);
	    append (" --param inline-unit-growth=1024", buf);
	#endif
	#if __GNUC__ >= 4
	    append (" -fvisibility-inlines-hidden", buf);
	#endif
    #endif
    Substitute ("@INLINE_OPTS@", buf);

    #ifdef __i386__
	Substitute ("-fPIC", "");
    #endif
}

static void SubstituteEnvironment (int bForce)
{
    strbuf_t match;
    uint i;
    cpchar_t envval;

    for (i = 0; i < VectorSize(g_EnvVars); ++ i) {
	envval = getenv (g_EnvVars[i]);
	if (!envval) {
	    if (!bForce)
		continue;
	    envval = "";
	}
	MakeSubstString (g_EnvVars[i], match);
	Substitute (match, envval);
    }
}

static void SubstitutePrograms (void)
{
    uint i;
    strbuf_t match;
    for (i = 0; i < VectorSize(g_ProgLocs); ++ i) {
	MakeSubstString (g_ProgVars [i * 4], match);
	Substitute (match, g_ProgLocs [i]);
    }
}

static uint cpuid (void)
{
    uint caps = 0;
#if defined(__GNUC__) && defined(__i386__)
    const uint amdBits = 0xC9480000;
    asm (
	"pushf			\n\t"	/* First, test if cpuid instruction works */
	"popl %%eax		\n\t"
	"mov %%eax, %%ecx	\n\t"
	"xor $0x200000, %%eax	\n\t"	/* Toggle the ID bit in one copy of eflags */
	"pushl %%eax		\n\t"
	"popf			\n\t"
	"pushf			\n\t"
	"popl %%eax		\n\t"	/* If the the flags are unchanged, cpuid is not supported */
	"test %%eax, %%ecx	\n\t"
	"jz 0f			\n\t"	/* This is a gcc local label jump */
	"xor %%eax, %%eax	\n\t"	/* Ask whether feature list is supported */
	"cpuid			\n\t"
	"test %%eax,%%eax	\n\t"
	"jz 0f			\n\t"
	"mov $1, %%eax		\n\t"	/* Ask for feature list */
	"cpuid			\n\t"
	"and %1, %%edx		\n\t"	/* The inverse of the AMD bit constant below */
	"and $1, %%ecx		\n\t"	/* Extract SSE3 bit from Intel extensions */
	"shl $27, %%ecx		\n\t"
	"or %%ecx, %%edx	\n\t"	/* ... and put it at bit 27 */
	"mov %%edx, %0		\n\t"
	"mov $0x80000000, %%eax	\n\t"
	"cpuid			\n\t"
	"test $0x80000000, %%eax\n\t"	/* Test for extended feature support */
	"jz 0f			\n\t"
	"mov $0x80000001, %%eax	\n\t"	/* AMD extensions */
	"cpuid			\n\t"
	"and %2, %%edx		\n\t"	/* Take only AMD specific bits */
	"or %%edx, %0		\n\t"
	"0:"
	: "=m"(caps)
	: "g"(~amdBits), "g"(amdBits)
	: "cc", "%eax", "%ebx", "%ecx", "%edx");
#endif
    return (caps);
}

static void SubstituteCpuCaps (void)
{
    uint i;
    g_CpuCapBits = cpuid();
    for (i = 0; i < VectorSize(g_CpuCaps); ++ i)
	if (g_CpuCapBits & (1 << g_CpuCaps[i].m_Bit))
	    Substitute (g_CpuCaps[i].m_Disabled, g_CpuCaps[i].m_Enabled);
}

static void SubstituteHostOptions (void)
{
    strbuf_t buf;
    if (g_SysType == sys_Mac)
	Substitute ("@SYSWARNS@", "-Wno-long-double");
    else
	Substitute ("@SYSWARNS@", "");
    #if __GNUC__ >= 3
    if (g_SysType == sys_Mac) {
	Substitute ("@libgcc@", "@libsupc++@ @libgcc@");
	Substitute (" @libgcc_eh@", "");
    }
    if (g_SysType == sys_Sun)
    #endif
	Substitute ("-Wredundant-decls", "-Wno-redundant-decls");

    if (g_SysType == sys_Bsd) {
	Substitute ("-Wredundant-decls", "-Wno-redundant-decls");
	Substitute ("-Winline", "-Wno-inline");
    }

    if (g_SysType != sys_Linux && g_SysType != sys_Sun) {
	Substitute ("BUILD_SHARED\t= 1 ", "#BUILD_SHARED\t= 1");
	Substitute ("#BUILD_STATIC\t= 1", "BUILD_STATIC\t= 1 ");
    }

    if (g_SysType == sys_Linux)
	Substitute ("@SHBLDFL@", "-shared -Wl,-soname=${LIBSOLNK}");
    else
	Substitute ("@SHBLDFL@", "-G");

    if (g_SysType != sys_Sun)
	Substitute ("#undef HAVE_THREE_CHAR_TYPES", "#define HAVE_THREE_CHAR_TYPES 1");

    Substitute ("#undef RETSIGTYPE", "#define RETSIGTYPE void");
    Substitute ("#undef const", "/* #define const */");
    Substitute ("#undef inline", "/* #define inline __inline */");
    Substitute ("#undef off_t", "/* typedef long off_t; */");
    Substitute ("#undef size_t", "/* typedef long size_t; */");

    sprintf (buf, "#define SIZE_OF_CHAR %d", sizeof(char));
    Substitute ("#undef SIZE_OF_CHAR", buf);
    sprintf (buf, "#define SIZE_OF_SHORT %d", sizeof(short));
    Substitute ("#undef SIZE_OF_SHORT", buf);
    sprintf (buf, "#define SIZE_OF_INT %d", sizeof(int));
    Substitute ("#undef SIZE_OF_INT", buf);
    sprintf (buf, "#define SIZE_OF_LONG %d", sizeof(long));
    Substitute ("#undef SIZE_OF_LONG ", buf);
    sprintf (buf, "#define SIZE_OF_POINTER %d", sizeof(void*));
    Substitute ("#undef SIZE_OF_POINTER ", buf);
    sprintf (buf, "#define SIZE_OF_SIZE_T %d", sizeof(size_t));
    Substitute ("#undef SIZE_OF_SIZE_T ", buf);
    if (g_SysType == sys_Alpha || g_SysType == sys_Mac)
	Substitute ("#undef SIZE_OF_BOOL ", "#define SIZE_OF_BOOL SIZE_OF_LONG");
    else
	Substitute ("#undef SIZE_OF_BOOL ", "#define SIZE_OF_BOOL SIZE_OF_CHAR");
    if ((sizeof(size_t) == sizeof(unsigned long) &&
	 sizeof(size_t) != sizeof(uint)) || g_SysType == sys_Mac)
	Substitute ("#undef SIZE_T_IS_LONG", "#define SIZE_T_IS_LONG 1");
#if defined(__GNUC__) || (__WORDSIZE == 64) || defined(__ia64__)
    if (g_SysType != sys_Bsd)
	Substitute ("#undef HAVE_INT64_T", "#define HAVE_INT64_T 1");
#endif
#if defined(__GNUC__) || defined(__GLIBC_HAVE_LONG_LONG)
    Substitute ("#undef HAVE_LONG_LONG", "#define HAVE_LONG_LONG 1");
    sprintf (buf, "#define SIZE_OF_LONG_LONG %d", sizeof(long long));
    Substitute ("#undef SIZE_OF_LONG_LONG", buf);
#endif
#if __GNUC__ >= 3
    Substitute ("#undef HAVE_VECTOR_EXTENSIONS", "#define HAVE_VECTOR_EXTENSIONS 1");
#else
    Substitute ("-Wshadow ", "");
#endif

    Substitute ("#undef LSTAT_FOLLOWS_SLASHED_SYMLINK", "#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1");
    Substitute ("#undef HAVE_STAT_EMPTY_STRING_BUG", "/* #undef HAVE_STAT_EMPTY_STRING_BUG */");

    Substitute ("#undef PACKAGE_BUGREPORT",	"#define PACKAGE_BUGREPORT \"" PACKAGE_BUGREPORT "\"");
    Substitute ("#undef PACKAGE_NAME",		"#define PACKAGE_NAME \"" PACKAGE_NAME "\"");
    Substitute ("#undef PACKAGE_STRING",	"#define PACKAGE_STRING \"" PACKAGE_STRING "\"");
    Substitute ("#undef PACKAGE_TARNAME",	"#define PACKAGE_TARNAME \"" PACKAGE_TARNAME "\"");
    Substitute ("#undef PACKAGE_VERSION",	"#define PACKAGE_VERSION \"" PACKAGE_VERSION "\"");

    if (g_SysType == sys_Linux)
	Substitute ("#undef HAVE_RINTF", "#define HAVE_RINTF 1");
}

static void SubstituteCustomVars (void)
{
    uint i;
    strbuf_t match;
    for (i = 0; i < VectorSize(g_CustomVars) / 2; ++ i) {
	MakeSubstString (g_CustomVars [i * 2], match);
	Substitute (match, g_CustomVars [i * 2 + 1]);
    }
}

static void SubstituteHeaders (void)
{
    uint i;
    cpchar_t pi;
    strbuf_t defaultPath;
    strbuf_t match;

    copy (g_ConfigVV [vv_includedir], defaultPath);
    append2 (":", g_ConfigVV [vv_oldincludedir], defaultPath);
    append2 (":", g_ConfigVV [vv_gccincludedir], defaultPath);
    for (i = 0; i < (uint) g_nCustomIncDirs; ++ i)
	append2 (":", g_CustomIncDirs [i], defaultPath);
    for (i = 0; i < VectorSize(g_Headers) / 3; ++ i) {
	for (pi = defaultPath; pi; pi = CopyPathEntry (pi, match)) {
	    append2 ("/", g_Headers [i * 3], match);
	    if (access (match, R_OK) == 0)
		Substitute (g_Headers [i * 3 + 1], g_Headers [i * 3 + 2]);
	}
    }
}

static void SubstituteLibs (void)
{
    uint i, k, ok;
    cpchar_t pi;
    char defaultPath [4096];
    strbuf_t match;

    copy ("/lib:/usr/lib:/usr/local/lib", defaultPath);
    pi = getenv ("LD_LIBRARY_PATH");
    if (pi)
	append (pi, defaultPath);
    append2 (":", g_ConfigVV [vv_libdir], defaultPath);
    append2 (":", g_ConfigVV [vv_gcclibdir], defaultPath);
    for (i = 0; i < (uint) g_nCustomLibDirs; ++ i)
	append2 (":", g_CustomLibDirs [i], defaultPath);

    for (i = 0; i < VectorSize(g_Libs) / 3; ++ i) {
	ok = 0;
	for (pi = defaultPath; pi; pi = CopyPathEntry (pi, match)) {
	    for (k = 0; k < VectorSize(g_LibSuffixes); ++ k) {
		CopyPathEntry (pi, match);
		append2 ("/lib", g_Libs [i * 3], match);
		append (g_LibSuffixes [k], match);
		if (access (match, R_OK) == 0)
		    ok = 1;
	    }
	}
	copy ("@lib", match);
	append2 (g_Libs[i * 3], "@", match);
	Substitute (match, g_Libs [i * 3 + 1 + ok]);
    }
}

static void SubstituteFunctions (void)
{
    uint i;
    for (i = 0; i < VectorSize(g_Functions) / 3; ++ i)
	Substitute (g_Functions [i * 3 + 1], g_Functions [i * 3 + 2]);
    if (g_SysType == sys_Mac)
	Substitute ("#define HAVE_STRSIGNAL 1", "#undef HAVE_STRSIGNAL");
}

static void SubstituteComponents (void)
{
    uint i, isOn;
    for (i = 0; i < VectorSize(g_Components) / 3; ++ i) {
	isOn = g_ComponentInfos[i].m_bDefaultOn;
	Substitute (g_Components [i * 3 + 1 + !isOn], g_Components [i * 3 + 1 + isOn]);
    }
}

/*--------------------------------------------------------------------*/

static void Substitute (cpchar_t matchStr, cpchar_t replaceStr)
{
    if (g_nSubs >= MAX_SUBSTITUTIONS)
	FatalError ("substitution list is too long, increase MAX_SUBSTITUTIONS");
    copy (matchStr, g_Subs[g_nSubs * 2]);
    copy (replaceStr, g_Subs[g_nSubs * 2 + 1]);
    ++ g_nSubs;
}

static void ExecuteSubstitutionList (void)
{
    uint i;
    int rsl, taill, delta;
    pchar_t cp;

    for (i = 0; i < g_nSubs; ++ i) {
	rsl = StrLen (g_Subs[i * 2 + 1]);
	delta = rsl - StrLen (g_Subs[i * 2]);
	for (cp = g_Buf; cp < g_Buf + g_BufSize; ++ cp) {
	    if (!compare (cp, g_Subs[i * 2]))
		continue;
	    if (g_BufSize + delta >= BUFSIZE)
		FatalError ("buffer overflow");
	    g_BufSize += delta;
	    taill = g_BufSize - (cp - g_Buf);
	    if (delta > 0)
		copy_backward (cp, cp + delta, taill);
	    else if (delta < 0)
		copy_n (cp + (-delta), cp, taill);
	    cp = copy_n (g_Subs[i * 2 + 1], cp, rsl);
	}
    }
}

static void MakeSubstString (cpchar_t str, pchar_t substString)
{
    copy ("@", substString);
    append2 (str, "@", substString);
}

/*--------------------------------------------------------------------*/

static void FatalError (cpchar_t errortext)
{
    perror (errortext);
    exit(-1);
}

static int StrLen (cpchar_t str)
{
    int l;
    for (l = 0; *str; ++ l, ++ str);
    return (l);
}

static void Lowercase (pchar_t str)
{
    for (; *str; ++ str)
	if (*str >= 'A' && *str <= 'Z')
	    *str += 'a' - 'A';
}

static int compare (cpchar_t str1, cpchar_t str2)
{
    while (*str1 && *str2 && *str1 == *str2)
	++ str1, ++ str2;
    return (!*str2);
}

static pchar_t copy (cpchar_t src, pchar_t dest)
{
    while (*src) *dest++ = *src++;
    *dest = 0;
    return (dest);
}

static pchar_t copy_n (cpchar_t src, pchar_t dest, int n)
{
    while (n--)
	*dest++ = *src++;
    return (dest);
}

static pchar_t append (cpchar_t src, pchar_t dest)
{
    while (*dest) ++ dest;
    return (copy (src, dest));
}

static pchar_t append2 (cpchar_t src1, cpchar_t src2, pchar_t dest)
{
    return (append (src2, append (src1, dest)));
}

static void fill_n (pchar_t str, int n, char v)
{
    while (n--)
	*str++ = v;
}

static pchar_t copy_backward (cpchar_t src, pchar_t dest, int n)
{
    dest += n; src += n;
    while (n--)
	*--dest = *--src;
    return (dest + n);
}

static void ReadFile (cpchar_t filename)
{
    FILE* fp = fopen (filename, "r");
    if (!fp)
	FatalError ("fopen");
    fill_n (g_Buf, BUFSIZE, 0);
    g_BufSize = fread (g_Buf, 1, BUFSIZE - 1, fp);
    if (g_BufSize <= 0)
	FatalError ("fread");
    fclose (fp);
}

static void WriteFile (cpchar_t filename)
{
    int bw;
    FILE* fp = fopen (filename, "w");
    if (!fp)
	FatalError ("fopen");
    bw = fwrite (g_Buf, 1, g_BufSize, fp);
    if (bw != g_BufSize)
	FatalError ("fwrite");
    if (fclose (fp))
	FatalError ("fclose");
}

