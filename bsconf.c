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
#include <sys/stat.h>

/*- The configuration header and its types ---------------------------*/

#define VectorSize(v)	(sizeof(v) / sizeof(*v))

/*#define const*/
/*typedef unsigned int	uint;*/
typedef char*		pchar_t;
typedef const char*	cpchar_t;

typedef struct {
    int		m_bDefaultOn;
    cpchar_t	m_Description;
} SComponentInfo;

#include "bsconf.h"
#if !defined(BSCONF_VERSION) || BSCONF_VERSION < 0x03
#error Your bsconf.h file is too old; please update its format.
#endif

/*- Internal types and macros ----------------------------------------*/

#define foreachN(i,v,n)	for (i = 0; i != VectorSize(v) / n; ++ i)
#define foreach(i,v)	foreachN(i,v,1)

typedef struct {
    pchar_t	data;
    uint	allocated;
    uint	used;
    uint	last;
    uint	page;
} SBuf, *pbuf_t;
#define NULL_BUF	{ NULL, 0, 0, 0, 0 }

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

static char g_ConfigV [vv_last][16] = {
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

static SBuf g_Subs = NULL_BUF;
static SBuf g_ConfigVV [vv_last], g_ProgLocs [VectorSize (g_ProgVars) / 4];
static SBuf g_CustomLibDirs [16], g_CustomIncDirs [16];
static int g_nCustomLibDirs = 0, g_nCustomIncDirs = 0;
static struct utsname g_Uname;
static uint g_CpuCapBits = 0;

typedef enum {
    sys_Unknown,
    sys_Linux,
    sys_Mac,
    sys_Bsd,
    sys_Sun,
    sys_Alpha
} ESysType;

static ESysType g_SysType = sys_Unknown;

/*- Libc-like functions that might not exist -------------------------*/

static int StrLen (cpchar_t str)
{
    int l;
    for (l = 0; *str; ++ l, ++ str);
    return (l);
}

static pchar_t copy_n (cpchar_t src, pchar_t dest, int n)
{
    while (n--)
	*dest++ = *src++;
    return (dest);
}

static void fill_n (pchar_t str, int n, char v)
{
    while (n--)
	*str++ = v;
}

static void copy_backward (cpchar_t src, pchar_t dest, uint n)
{
    dest += n; src += n;
    while (n--)
	*--dest = *--src;
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

static void FatalError (cpchar_t errortext)
{
    perror (errortext);
    exit(-1);
}

/*- Malloc buffer object ---------------------------------------------*/

static pchar_t buf_addspace (pbuf_t p, uint n)
{
    if ((p->used += n) > p->allocated) {
	p->allocated = p->used;
	if (!(p->data = (pchar_t) realloc (p->data, p->allocated)))
	    FatalError ("out of memory");
    }
    return (p->data + p->used - n);
}

static void buf_clear (pbuf_t p)		{ p->used = p->last; }
static void append (cpchar_t s, pbuf_t p)	{ copy_n (s, buf_addspace (p, StrLen(s)), StrLen(s)); }
static void copy (cpchar_t s, pbuf_t p)	{ buf_clear (p); append (s, p); }

static void buf_copy_n (cpchar_t s, pbuf_t p, uint n)
{
    buf_clear (p);
    copy_n (s, buf_addspace (p, n), n);
}

static void append2 (cpchar_t s1, cpchar_t s2, pbuf_t p)
{
    uint l1 = StrLen(s1), l2 = StrLen(s2);
    copy_n (s2, copy_n (s1, buf_addspace (p, l1+l2), l1), l2);
}

static void buf_cap (pbuf_t p)
{
    if (p->used >= p->allocated || p->data [p->used]) {
	*buf_addspace(p,1) = 0;
	--p->used;
    }
}

static pchar_t buf_resize_fragment (pbuf_t p, uint fo, uint os, uint ns)
{
    int d = ns - os;
    if (d <= 0) {
	copy_n (p->data + fo - d, p->data + fo, p->used - (fo - d));
	p->used += d;
    } else {
	buf_addspace (p, d);
	copy_backward (p->data + fo, p->data + fo + d, p->used - (fo + d));
    }
    buf_cap (p);
    return (p->data + fo);
}

static void pushstring (pbuf_t p)	{ buf_cap (p); p->last = p->used + 1; }
static pchar_t buf_str (pbuf_t p)	{ buf_cap (p); return (p->data + p->last); }
#define S(buf)	buf_str(&buf)

static void buf_free (pbuf_t p)
{
    if (p->data)
	free (p->data);
    p->data = NULL;
    p->allocated = p->used = p->last = 0;
}

/*- File I/O with buffer objects -------------------------------------*/

static void ReadFile (cpchar_t filename, pbuf_t buf)
{
    struct stat st;
    FILE* fp = fopen (filename, "r");
    if (!fp)
	FatalError ("open");
    if (fstat (fileno(fp), &st))
	FatalError ("stat");
    buf_clear (buf);
    buf->used = fread (buf_addspace (buf, st.st_size + 1), 1, st.st_size, fp);
    if (((int) buf->used) < 0)
	FatalError ("read");
    buf_cap (buf);
    fclose (fp);
}

static void WriteFile (cpchar_t filename, pbuf_t buf)
{
    uint bw;
    FILE* fp = fopen (filename, "w");
    if (!fp)
	FatalError ("open");
    bw = fwrite (buf->data, 1, buf->used, fp);
    if (bw != buf->used)
	FatalError ("write");
    if (fclose (fp))
	FatalError ("close");
}

/*- Substitution engine ----------------------------------------------*/

static void MakeSubstString (cpchar_t str, pbuf_t pbuf)
{
    copy ("@", pbuf);
    append2 (str, "@", pbuf);
}

static void Substitute (cpchar_t matchStr, cpchar_t replaceStr)
{
    copy (matchStr, &g_Subs);
    pushstring (&g_Subs);
    copy (replaceStr, &g_Subs);
    pushstring (&g_Subs);
}

static void ExecuteSubstitutionList (pbuf_t buf)
{
    uint ml, rl;
    cpchar_t m = g_Subs.data, r;
    pchar_t cp, fbfirst = S(*buf);

    while (m < g_Subs.data + g_Subs.used) {
	ml = StrLen (m);
	r = m + ml + 1;
	rl = StrLen (r);
	for (cp = fbfirst; cp < fbfirst + buf->used; ++ cp) {
	    if (!compare (cp, m))
		continue;
	    cp = buf_resize_fragment (buf, cp - fbfirst, ml, rl);
	    fbfirst = S(*buf);
	    cp = copy_n (r, cp, rl);
	}
	m = r + rl + 1;
    }
}

static void ApplySubstitutions (void)
{
    uint f;
    SBuf srcFile = NULL_BUF, fileBuf = NULL_BUF;
    foreach (f, g_Files) {
	copy (g_Files[f], &srcFile);
	append (".in", &srcFile);
	ReadFile (S(srcFile), &fileBuf);
	ExecuteSubstitutionList (&fileBuf);
	WriteFile (g_Files[f], &fileBuf);
    }
    buf_free (&fileBuf);
    buf_free (&srcFile);
}

/*- Housekeeping -----------------------------------------------------*/

static void InitGlobals (void)
{
    uint i;
    SBuf nullBuf = NULL_BUF;
    g_Subs = nullBuf;
    foreach (i, g_ConfigVV)
	g_ConfigVV[i] = nullBuf;
    foreach (i, g_CustomLibDirs)
	g_CustomLibDirs[i] = nullBuf;
    foreach (i, g_CustomIncDirs)
	g_CustomIncDirs[i] = nullBuf;
    foreach (i, g_ProgLocs)
	g_ProgLocs[i] = nullBuf;
}

static void FreeGlobals (void)
{
    uint i;
    buf_free (&g_Subs);
    foreach (i, g_ConfigVV)
	buf_free (&g_ConfigVV[i]);
    foreach (i, g_CustomLibDirs)
	buf_free (&g_CustomLibDirs[i]);
    foreach (i, g_CustomIncDirs)
	buf_free (&g_CustomIncDirs[i]);
    foreach (i, g_ProgLocs)
	buf_free (&g_ProgLocs[i]);
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
);
    if (VectorSize(g_Components)) {
	printf ("\nOptions:\n");
	foreach (i, g_ComponentInfos) {
	    if (!g_ComponentInfos[i].m_Description[0])
		continue;
	    if (g_ComponentInfos[i].m_bDefaultOn)
		printf ("  --without-%-12s%s\n", g_Components[i * 3], g_ComponentInfos[i].m_Description);
	    else
		printf ("  --with-%-15s%s\n", g_Components[i * 3], g_ComponentInfos[i].m_Description);
	}
    }
    printf (
"\nSome influential environment variables:\n"
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
	    "\nUsing bsconf package version 0.3\n"
	    "Copyright (c) 2003-2005, Mike Sharov <msharov@users.sourceforge.net>\n"
	    "This configure script and the bsconf package are free software.\n"
	    "Unlimited permission to copy, distribute, and modify is granted.\n");
    exit (0);
}

/*- Configuration routines -------------------------------------------*/

static void GetConfigVarValues (int argc, cpchar_t const* argv)
{
    int a, apos, cvl;
    uint cv;
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
	    foreach (cv, g_ComponentInfos)
		if (compare (argv[a] + apos, g_Components[cv * 3]))
		    g_ComponentInfos[cv].m_bDefaultOn = (apos == 7);
	} else {
	    foreach (cv, g_ConfigV)
		if (compare (argv[a] + apos, g_ConfigV[cv]))
		    break;
	    if (cv == vv_last)
		continue;
	    apos += StrLen (g_ConfigV[cv]) + 1;
	    cvl = StrLen (argv[a]) - apos + 1;
	    if (cvl > 1) {
		buf_copy_n (argv[a] + apos, &g_ConfigVV[cv], cvl);
		if (cv == vv_customlib)
		    buf_copy_n (argv[a] + apos, &g_CustomLibDirs [g_nCustomLibDirs++], cvl);
		else if (cv == vv_custominclude)
		    buf_copy_n (argv[a] + apos, &g_CustomIncDirs [g_nCustomIncDirs++], cvl);
	    }
	    if (cv == vv_prefix && compare (S(g_ConfigVV[cv]), "/home")) {
		copy (S(g_ConfigVV[cv]), &g_CustomLibDirs [g_nCustomLibDirs]);
		append ("/lib", &g_CustomLibDirs [g_nCustomLibDirs++]);
		copy (S(g_ConfigVV[cv]), &g_CustomIncDirs [g_nCustomIncDirs]);
		append ("/include", &g_CustomIncDirs [g_nCustomIncDirs++]);
	    }
	}
    }
}

static void DefaultConfigVarValue (EVV v, EVV root, cpchar_t suffix)
{
    if (!*S(g_ConfigVV[v])) {
	copy (S(g_ConfigVV [root]), &g_ConfigVV [v]);
	append (suffix, &g_ConfigVV [v]);
    }
}

static void DetermineHost (void)
{
    typedef struct {
	char		sysname[8];
	char		type;
    } SHostType;
    static const SHostType g_HostTypes[] = {
	{ "linux",	sys_Linux },
	{ "sun",	sys_Sun },
	{ "solaris",	sys_Sun },
	{ "openbsd",	sys_Bsd },
	{ "netbsd",	sys_Bsd },
	{ "freebsd",	sys_Bsd },
	{ "osx",	sys_Mac },
	{ "darwin",	sys_Mac },
	{ "alpha",	sys_Alpha }
    };
    uint i;
    fill_n ((pchar_t) &g_Uname, sizeof(struct utsname), 0);
    uname (&g_Uname);
    Lowercase (g_Uname.machine);
    Lowercase (g_Uname.sysname);
    copy (g_Uname.machine, &g_ConfigVV [vv_host]);
    append ("-", &g_ConfigVV [vv_host]);
#ifdef __GNUC__
    append ("gnu", &g_ConfigVV [vv_host]);
#else
    append ("unknown", &g_ConfigVV [vv_host]);
#endif
    append2 ("-", g_Uname.sysname, &g_ConfigVV [vv_host]);
    foreach (i, g_HostTypes)
	if (compare (g_Uname.sysname, g_HostTypes[i].sysname))
	    g_SysType = g_HostTypes[i].type;
    if (compare (g_Uname.machine, "alpha"))
	g_SysType = sys_Alpha;
}

static void FillInDefaultConfigVarValues (void)
{
    typedef struct _SDefaultPathMap {
	unsigned char	var;
	unsigned char	base;
	char		path[10];
    } SDefaultPathMap;
    static const SDefaultPathMap c_Defaults[] = {
	{ vv_bindir,		vv_exec_prefix,	"/bin" },
	{ vv_sbindir,		vv_exec_prefix,	"/sbin" },
	{ vv_libexecdir,	vv_prefix,	"/libexec" },
	{ vv_datadir,		vv_prefix,	"/share" },
	{ vv_sysconfdir,	vv_prefix,	"/etc" },
	{ vv_sharedstatedir,	vv_prefix,	"/com" },
	{ vv_localstatedir,	vv_prefix,	"/var" },
	{ vv_libdir,		vv_exec_prefix,	"/lib" },
	{ vv_gcclibdir,		vv_exec_prefix,	"/lib" },
	{ vv_includedir,	vv_prefix,	"/include" },
	{ vv_gccincludedir,	vv_prefix,	"/include" },
	{ vv_infodir,		vv_prefix,	"/info" },
	{ vv_mandir,		vv_prefix,	"/man" }
    };
    uint i;

    if (!*S(g_ConfigVV [vv_prefix]))
	copy ("/usr/local", &g_ConfigVV [vv_prefix]);
    else if (S(g_ConfigVV[vv_prefix])[0] == '/' && !S(g_ConfigVV[vv_prefix])[1])
	g_ConfigVV [vv_prefix].data[0] = 0;
    if (!*S(g_ConfigVV [vv_exec_prefix]))
	DefaultConfigVarValue (vv_exec_prefix,	vv_prefix,	"");
    else if (S(g_ConfigVV[vv_exec_prefix])[0] == '/' && !S(g_ConfigVV[vv_exec_prefix])[1])
	g_ConfigVV [vv_exec_prefix].data[0] = 0;
    if (!*S(g_ConfigVV [vv_oldincludedir]))
	copy ("/usr/include", &g_ConfigVV [vv_oldincludedir]);

    foreach (i, c_Defaults)
	DefaultConfigVarValue (c_Defaults[i].var, c_Defaults[i].base, c_Defaults[i].path);

    if (!*S(g_ConfigVV [vv_prefix]))
	copy ("/usr", &g_ConfigVV [vv_prefix]);
    if (!*S(g_ConfigVV [vv_exec_prefix]))
	copy ("/usr", &g_ConfigVV [vv_exec_prefix]);

    if (!*S(g_ConfigVV [vv_host]))
	DetermineHost();
    if (!*S(g_ConfigVV [vv_build]))
	copy (S(g_ConfigVV [vv_host]), &g_ConfigVV [vv_build]);
}

static cpchar_t CopyPathEntry (cpchar_t pi, pbuf_t dest)
{
    uint pil = 0;
    while (pi[pil] && pi[pil] != ':') ++ pil;
    buf_copy_n (pi, dest, pil);
    return (*(pi += pil) ? ++pi : NULL);
}

static int IsBadInstallDir (cpchar_t match)
{
    static const char c_BadDirs[][10] = { "/etc", "/c", "/C", "/usr/etc", "/sbin", "/usr/sbin", "/usr/ucb" };
    uint i;
    foreach (i, c_BadDirs)
	if (compare (match, c_BadDirs[i]))
	    return (1);
    return (0);
}

static void FindPrograms (void)
{
    uint i, count;
    cpchar_t path, pi;
    SBuf match = NULL_BUF;

    path = getenv ("PATH");
    if (!path)
	path = "";

    foreach (i, g_ProgLocs) {
	copy (".", &match);
	count = 0;
	for (pi = path; pi; pi = CopyPathEntry (pi, &match)) {
	    /* Ignore "bad" versions of install, like autoconf does. */
	    if (compare (g_ProgVars[i * 4 + 1], "install") && IsBadInstallDir (S(match)))
		continue;
	    append2 ("/", g_ProgVars[i * 4 + 1], &match);
	    if (access (S(match), X_OK) == 0) {
		++ count;
		break;
	    }
	}
	if (count && compare (g_ProgVars[i * 4 + 1], "install"))
	    copy (S(match), &g_ProgLocs[i]);
	else
	    copy (g_ProgVars[i * 4 + 2 + !count], &g_ProgLocs[i]);
    }
    buf_free (&match);
}

static void SubstitutePaths (void)
{
    SBuf match = NULL_BUF;
    int cv;
    foreach (cv, g_ConfigV) {
	MakeSubstString (g_ConfigV [cv], &match);
	Substitute (S(match), S(g_ConfigVV [cv]));
    }
    buf_free (&match);
}

static void SubstituteCFlags (void)
{
    SBuf buf = NULL_BUF;
    int j;

    for (j = 0; j < g_nCustomIncDirs; ++ j)
	append2 (" -I", S(g_CustomIncDirs[j]), &buf);
    Substitute ("@CUSTOMINCDIR@", S(buf));

    buf_clear (&buf);
    for (j = 0; j < g_nCustomLibDirs; ++ j)
	append2 (" -L", S(g_CustomLibDirs[j]), &buf);
    Substitute ("@CUSTOMLIBDIR@", S(buf));

    buf_clear (&buf);
    #if __GNUC__ >= 3
	if (g_CpuCapBits & (1 << 23))
	    append (" -mmmx", &buf);
	if (g_SysType == sys_Linux)
	    if (g_CpuCapBits & ((1 << 22) | (1 << 25)))
		append (" -msse -mfpmath=sse", &buf);
	if (g_CpuCapBits & (1 << 26))
	    append (" -msse2", &buf);
	if (g_CpuCapBits & ((1 << 30) | (1 << 31)))
	    append (" -m3dnow", &buf);
    #endif
    Substitute ("@PROCESSOR_OPTS@", S(buf));

    #if __GNUC__ >= 3
	#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
	    copy (" --param max-inline-insns-single=1024", &buf);
	    append (" \\\n\t\t--param large-function-growth=65535", &buf);
	    append (" \\\n\t\t--param inline-unit-growth=1024", &buf);
	#else
	    copy ("-finline-limit=65535", &buf);
	#endif
	#if __GNUC__ >= 4
	    append (" \\\n\t\t-fvisibility-inlines-hidden", &buf);
	#endif
    #endif
    Substitute ("@INLINE_OPTS@", S(buf));

    #if __i386__
	Substitute ("-fPIC", "");
    #endif
    buf_free (&buf);
}

static void SubstituteEnvironment (int bForce)
{
    SBuf match = NULL_BUF;
    uint i;
    cpchar_t envval;

    foreach (i, g_EnvVars) {
	envval = getenv (g_EnvVars[i]);
	if (!envval) {
	    if (!bForce)
		continue;
	    envval = "";
	}
	MakeSubstString (g_EnvVars[i], &match);
	Substitute (S(match), envval);
    }
    buf_free (&match);
}

static void SubstitutePrograms (void)
{
    uint i;
    SBuf match = NULL_BUF;
    foreach (i, g_ProgLocs) {
	MakeSubstString (g_ProgVars [i * 4], &match);
	Substitute (S(match), S(g_ProgLocs [i]));
    }
    buf_free (&match);
}

#if defined(__GNUC__) && (__i386__ || __x86_64) && !defined(__PIC__)
static uint cpuid_supported (void)
{
    unsigned long forig, fnew;
    /* Pop flags, toggle ID bit, push, pop. cpuid supported if changed. */
    asm ("pushf\n\tpop\t%0\n\t"
	"mov\t%0, %1\n\txor\t$0x200000, %0\n\t"
	"push\t%0\n\tpopf\n\tpushf\n\tpop\t%0"
	: "=r"(fnew), "=r"(forig));
    return (fnew != forig);
}

static uint cpuid (void)
{
    #define i_cpuid(a,r,c,d)	asm("cpuid":"=a"(r),"=c"(c),"=d"(d):"0"(a):"ebx")
    const uint amdBits = 0xC9480000, extFeatures = 0x80000000, amdExtensions = 0x80000001;
    uint r, c, d, caps;
    if (!cpuid_supported()) return (0);
    i_cpuid (0, r, c, d);
    if (!r) return (0);
    i_cpuid (1, r, c, d);			/* Ask for feature list */
    caps = (d & ~amdBits);
    i_cpuid (extFeatures, r, c, d);
    if (r != extFeatures) {
	i_cpuid (amdExtensions, r, c, d);
	caps |= d & amdBits;
    }
    return (caps);
}
#else
static uint cpuid (void) { return (0); }
#endif

static void SubstituteCpuCaps (void)
{
    typedef struct {
	char	m_Bit;
	char	m_Disabled [26];
	char	m_Enabled [29];
    } SCpuCaps;
    static const SCpuCaps s_CpuCaps [] = {
	{  0, "#undef CPU_HAS_FPU",		"#define CPU_HAS_FPU 1"		},
	{  2, "#undef CPU_HAS_EXT_DEBUG",	"#define CPU_HAS_EXT_DEBUG 1"	},
	{  4, "#undef CPU_HAS_TIMESTAMPC",	"#define CPU_HAS_TIMESTAMPC 1"	},
	{  5, "#undef CPU_HAS_MSR",		"#define CPU_HAS_MSR 1"		},
	{  8, "#undef CPU_HAS_CMPXCHG8",	"#define CPU_HAS_CMPXCHG8 1"	},
	{  9, "#undef CPU_HAS_APIC",		"#define CPU_HAS_APIC 1"	},
	{ 11, "#undef CPU_HAS_SYSCALL",		"#define CPU_HAS_SYSCALL 1"	},
	{ 12, "#undef CPU_HAS_MTRR",		"#define CPU_HAS_MTRR 1"	},
	{ 15, "#undef CPU_HAS_CMOV",		"#define CPU_HAS_CMOV 1"	},
	{ 16, "#undef CPU_HAS_FCMOV",		"#define CPU_HAS_FCMOV 1"	},
	{ 22, "#undef CPU_HAS_SSE ",		"#define CPU_HAS_SSE 1"		},
	{ 23, "#undef CPU_HAS_MMX",		"#define CPU_HAS_MMX 1"		},
	{ 24, "#undef CPU_HAS_FXSAVE",		"#define CPU_HAS_FXSAVE 1"	},
	{ 25, "#undef CPU_HAS_SSE ",		"#define CPU_HAS_SSE 1"		},
	{ 26, "#undef CPU_HAS_SSE2",		"#define CPU_HAS_SSE2 1"	},
	{ 30, "#undef CPU_HAS_EXT_3DNOW",	"#define CPU_HAS_EXT_3DNOW 1"	},
	{ 31, "#undef CPU_HAS_3DNOW",		"#define CPU_HAS_3DNOW 1"	}
    };
    uint i;
    g_CpuCapBits = cpuid();
    foreach (i, s_CpuCaps)
	if (g_CpuCapBits & (1 << s_CpuCaps[i].m_Bit))
	    Substitute (s_CpuCaps[i].m_Disabled, s_CpuCaps[i].m_Enabled);
}

static void SubstituteHostOptions (void)
{
    static const short int boCheck = 0x0001;
    static const char boNames[2][16] = { "BIG_ENDIAN", "LITTLE_ENDIAN" };
    char buf [128];
    #if __GNUC__ >= 3
    if (g_SysType == sys_Sun)
    #endif
	Substitute ("-Wredundant-decls", "-Wno-redundant-decls");

    if (g_SysType == sys_Bsd) {
	Substitute (" @libgcc_eh@", "");
	Substitute ("#define WITHOUT_LIBSTDCPP 1", "#undef WITHOUT_LIBSTDCPP");
	Substitute ("NOLIBSTDCPP\t= -nodefaultlibs ", "#NOLIBSTDCPP\t= -nodefaultlibs");
	Substitute ("-Wredundant-decls", "-Wno-redundant-decls");
	Substitute ("-Winline", "-Wno-inline");
    }

    if ((g_SysType == sys_Linux) | (g_SysType == sys_Bsd))
	Substitute ("@SHBLDFL@", "-shared -Wl,-soname=${LIBSOLNK}");
    else if (g_SysType == sys_Mac)
	Substitute ("@SHBLDFL@", "-Wl,-single_module -compatibility_version 1 -current_version 1 -install_name ${LIBSOLNK} -Wl,-Y,1455 -dynamiclib -mmacosx-version-min=10.4");
    else if (g_SysType == sys_Sun)
	Substitute ("@SHBLDFL@", "-G");
    else {
	Substitute ("BUILD_SHARED\t= 1 ", "#BUILD_SHARED\t= 1");
	Substitute ("#BUILD_STATIC\t= 1", "BUILD_STATIC\t= 1 ");
    }

    if (g_SysType == sys_Mac) {
	Substitute (" @libgcc_eh@", "");
	Substitute ("@libgcc@", "@libsupc++@ @libgcc@");
	Substitute ("@SYSWARNS@", "-Wno-long-double");
	Substitute ("lib${LIBNAME}.so", "lib${LIBNAME}.dylib");
	Substitute ("${LIBSO}.${MAJOR}.${MINOR}.${BUILD}", "lib${LIBNAME}.${MAJOR}.${MINOR}.${BUILD}.dylib");
	Substitute ("${LIBSO}.${MAJOR}.${MINOR}", "lib${LIBNAME}.${MAJOR}.${MINOR}.dylib");
	Substitute ("${LIBSO}.${MAJOR}", "lib${LIBNAME}.${MAJOR}.dylib");
    } else
	Substitute ("@SYSWARNS@", "");

    if (g_SysType != sys_Sun)
	Substitute ("#undef HAVE_THREE_CHAR_TYPES", "#define HAVE_THREE_CHAR_TYPES 1");

    Substitute ("#undef RETSIGTYPE", "#define RETSIGTYPE void");
    Substitute ("#undef const", "/* #define const */");
    Substitute ("#undef inline", "/* #define inline __inline */");
    Substitute ("#undef off_t", "/* typedef long off_t; */");
    Substitute ("#undef size_t", "/* typedef long size_t; */");

    snprintf (buf, VectorSize(buf), "#define SIZE_OF_CHAR %zd", sizeof(char));
    Substitute ("#undef SIZE_OF_CHAR", buf);
    snprintf (buf, VectorSize(buf), "#define SIZE_OF_SHORT %zd", sizeof(short));
    Substitute ("#undef SIZE_OF_SHORT", buf);
    snprintf (buf, VectorSize(buf), "#define SIZE_OF_INT %zd", sizeof(int));
    Substitute ("#undef SIZE_OF_INT", buf);
    snprintf (buf, VectorSize(buf), "#define SIZE_OF_LONG %zd", sizeof(long));
    Substitute ("#undef SIZE_OF_LONG ", buf);
    snprintf (buf, VectorSize(buf), "#define SIZE_OF_POINTER %zd", sizeof(void*));
    Substitute ("#undef SIZE_OF_POINTER ", buf);
    snprintf (buf, VectorSize(buf), "#define SIZE_OF_SIZE_T %zd", sizeof(size_t));
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
    snprintf (buf, VectorSize(buf), "#define SIZE_OF_LONG_LONG %zd", sizeof(long long));
    Substitute ("#undef SIZE_OF_LONG_LONG", buf);
#endif
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
    Substitute ("#undef HAVE_VECTOR_EXTENSIONS", "#define HAVE_VECTOR_EXTENSIONS 1");
#else
    Substitute ("-Wshadow ", "");
#endif

    Substitute ("#undef LSTAT_FOLLOWS_SLASHED_SYMLINK", "#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1");
    Substitute ("#undef HAVE_STAT_EMPTY_STRING_BUG", "/* #undef HAVE_STAT_EMPTY_STRING_BUG */");

    if (g_SysType == sys_Linux)
	Substitute ("#undef HAVE_RINTF", "#define HAVE_RINTF 1");

    Substitute ("@BYTE_ORDER@", boNames [(uint)(*((const char*)&boCheck))]);
}

static void SubstituteCustomVars (void)
{
    uint i;
    SBuf match = NULL_BUF;
    foreachN (i, g_CustomVars, 2) {
	MakeSubstString (g_CustomVars [i * 2], &match);
	Substitute (S(match), g_CustomVars [i * 2 + 1]);
    }
    buf_free (&match);
}

static void SubstituteHeaders (void)
{
    uint i;
    cpchar_t pi;
    SBuf defaultPath = NULL_BUF, match = NULL_BUF;

    copy (S(g_ConfigVV [vv_includedir]), &defaultPath);
    append2 (":", S(g_ConfigVV [vv_oldincludedir]), &defaultPath);
    append2 (":", S(g_ConfigVV [vv_gccincludedir]), &defaultPath);
    for (i = 0; i != (uint) g_nCustomIncDirs; ++ i)
	append2 (":", S(g_CustomIncDirs [i]), &defaultPath);
    foreachN (i, g_Headers, 3) {
	for (pi = S(defaultPath); pi; pi = CopyPathEntry (pi, &match)) {
	    append2 ("/", g_Headers [i * 3], &match);
	    if (access (S(match), R_OK) == 0)
		Substitute (g_Headers [i * 3 + 1], g_Headers [i * 3 + 2]);
	}
    }
    buf_free (&match);
    buf_free (&defaultPath);
}

static void SubstituteLibs (void)
{
    static const char g_LibSuffixes[][8] = { ".a", ".so", ".la", ".dylib" };
    uint i, k, ok;
    cpchar_t pi;
    SBuf defaultPath = NULL_BUF, match = NULL_BUF;

    copy ("/lib:/usr/lib:/usr/local/lib", &defaultPath);
    pi = getenv ("LD_LIBRARY_PATH");
    if (pi)
	append2 (":", pi, &defaultPath);
    append2 (":", S(g_ConfigVV [vv_libdir]), &defaultPath);
    append2 (":", S(g_ConfigVV [vv_gcclibdir]), &defaultPath);
    for (i = 0; i != (uint) g_nCustomLibDirs; ++ i)
	append2 (":", S(g_CustomLibDirs [i]), &defaultPath);

    foreachN (i, g_Libs, 3) {
	ok = 0;
	for (pi = S(defaultPath); pi; pi = CopyPathEntry (pi, &match)) {
	    foreach (k, g_LibSuffixes) {
		CopyPathEntry (pi, &match);
		append2 ("/lib", g_Libs [i * 3], &match);
		append (g_LibSuffixes [k], &match);
		if (access (S(match), R_OK) == 0)
		    ok = 1;
	    }
	}
	copy ("@lib", &match);
	append2 (g_Libs[i * 3], "@", &match);
	Substitute (S(match), g_Libs [i * 3 + 1 + ok]);
    }
    buf_free (&match);
    buf_free (&defaultPath);
}

static void SubstituteFunctions (void)
{
    uint i;
    foreachN (i, g_Functions, 3)
	Substitute (g_Functions [i * 3 + 1], g_Functions [i * 3 + 2]);
    if ((g_SysType == sys_Mac) | (g_SysType == sys_Bsd))
	Substitute ("#define HAVE_STRSIGNAL 1", "#undef HAVE_STRSIGNAL");
    if (g_SysType == sys_Bsd)
	Substitute ("#define HAVE_VA_COPY 1", "#undef HAVE_VA_COPY");
}

static void SubstituteComponents (void)
{
    uint i, isOn;
    foreachN (i, g_Components, 3) {
	isOn = g_ComponentInfos[i].m_bDefaultOn;
	Substitute (g_Components [i * 3 + 1 + !isOn], g_Components [i * 3 + 1 + isOn]);
    }
}

/*--------------------------------------------------------------------*/

int main (int argc, const char* const* argv)
{
    InitGlobals();
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

    ApplySubstitutions();
    FreeGlobals();
    return (0);
}

