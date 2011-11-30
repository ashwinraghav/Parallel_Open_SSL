/* crypto/cryptlib.c */
/* ====================================================================
 * Copyright (c) 1998-2006 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 * ECDH support in OpenSSL originally developed by 
 * SUN MICROSYSTEMS, INC., and contributed to the OpenSSL project.
 */

#include "cryptlib.h"
#include <openssl/safestack.h>

#if defined(OPENSSL_SYS_WIN32) || defined(OPENSSL_SYS_WIN16)
static double SSLeay_MSVC5_hack=0.0; /* and for VC1.5 */
#endif

#if	defined(__i386)   || defined(__i386__)   || defined(_M_IX86) || \
	defined(__INTEL__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)

unsigned int  OPENSSL_ia32cap_P[2];
unsigned int *OPENSSL_ia32cap_loc(void) { return OPENSSL_ia32cap_P; }

#if defined(OPENSSL_CPUID_OBJ) && !defined(OPENSSL_NO_ASM) && !defined(I386_ONLY)
#define OPENSSL_CPUID_SETUP
#if defined(_WIN32)
typedef unsigned __int64 IA32CAP;
#else
typedef unsigned long long IA32CAP;
#endif
void OPENSSL_cpuid_setup(void)
{ static int trigger=0;
  IA32CAP OPENSSL_ia32_cpuid(void);
  IA32CAP vec;
  char *env;

    if (trigger)	return;

    trigger=1;
    if ((env=getenv("OPENSSL_ia32cap"))) {
	int off = (env[0]=='~')?1:0;
#if defined(_WIN32)
    	if (!sscanf(env+off,"%I64i",&vec)) vec = strtoul(env+off,NULL,0);
#else
	vec = strtoull(env+off,NULL,0);
#endif
	if (off) vec = OPENSSL_ia32_cpuid()&~vec;
    }
    else
	vec = OPENSSL_ia32_cpuid();

    /*
     * |(1<<10) sets a reserved bit to signal that variable
     * was initialized already... This is to avoid interference
     * with cpuid snippets in ELF .init segment.
     */
    OPENSSL_ia32cap_P[0] = (unsigned int)vec|(1<<10);
    OPENSSL_ia32cap_P[1] = (unsigned int)(vec>>32);
}
#endif

#else
unsigned int *OPENSSL_ia32cap_loc(void) { return NULL; }
#endif
int OPENSSL_NONPIC_relocated = 0;
#if !defined(OPENSSL_CPUID_SETUP) && !defined(OPENSSL_CPUID_OBJ)
void OPENSSL_cpuid_setup(void) {}
#endif

#if (defined(_WIN32) || defined(__CYGWIN__)) && defined(_WINDLL)
#ifdef __CYGWIN__
/* pick DLL_[PROCESS|THREAD]_[ATTACH|DETACH] definitions */
#include <windows.h>
/* this has side-effect of _WIN32 getting defined, which otherwise
 * is mutually exclusive with __CYGWIN__... */
#endif

/* All we really need to do is remove the 'error' state when a thread
 * detaches */

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
	     LPVOID lpvReserved)
	{
	switch(fdwReason)
		{
	case DLL_PROCESS_ATTACH:
		OPENSSL_cpuid_setup();
#if defined(_WIN32_WINNT)
		{
		IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)hinstDLL;
		IMAGE_NT_HEADERS *nt_headers;

		if (dos_header->e_magic==IMAGE_DOS_SIGNATURE)
			{
			nt_headers = (IMAGE_NT_HEADERS *)((char *)dos_header
						+ dos_header->e_lfanew);
			if (nt_headers->Signature==IMAGE_NT_SIGNATURE &&
			    hinstDLL!=(HINSTANCE)(nt_headers->OptionalHeader.ImageBase))
				OPENSSL_NONPIC_relocated=1;
			}
		}
#endif
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
		}
	return(TRUE);
	}
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <tchar.h>
#include <signal.h>
#ifdef __WATCOMC__
#if defined(_UNICODE) || defined(__UNICODE__)
#define _vsntprintf _vsnwprintf
#else
#define _vsntprintf _vsnprintf
#endif
#endif
#ifdef _MSC_VER
#define alloca _alloca
#endif

#if defined(_WIN32_WINNT) && _WIN32_WINNT>=0x0333
int OPENSSL_isservice(void)
{ HWINSTA h;
  DWORD len;
  WCHAR *name;
  static union { void *p; int (*f)(void); } _OPENSSL_isservice = { NULL };

    if (_OPENSSL_isservice.p == NULL) {
	HANDLE h = GetModuleHandle(NULL);
	if (h != NULL)
	    _OPENSSL_isservice.p = GetProcAddress(h,"_OPENSSL_isservice");
	if (_OPENSSL_isservice.p == NULL)
	    _OPENSSL_isservice.p = (void *)-1;
    }

    if (_OPENSSL_isservice.p != (void *)-1)
	return (*_OPENSSL_isservice.f)();

    (void)GetDesktopWindow(); /* return value is ignored */

    h = GetProcessWindowStation();
    if (h==NULL) return -1;

    if (GetUserObjectInformationW (h,UOI_NAME,NULL,0,&len) ||
	GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	return -1;

    if (len>512) return -1;		/* paranoia */
    len++,len&=~1;			/* paranoia */
    name=(WCHAR *)alloca(len+sizeof(WCHAR));
    if (!GetUserObjectInformationW (h,UOI_NAME,name,len,&len))
	return -1;

    len++,len&=~1;			/* paranoia */
    name[len/sizeof(WCHAR)]=L'\0';	/* paranoia */
#if 1
    /* This doesn't cover "interactive" services [working with real
     * WinSta0's] nor programs started non-interactively by Task
     * Scheduler [those are working with SAWinSta]. */
    if (wcsstr(name,L"Service-0x"))	return 1;
#else
    /* This covers all non-interactive programs such as services. */
    if (!wcsstr(name,L"WinSta0"))	return 1;
#endif
    else				return 0;
}
#else
int OPENSSL_isservice(void) { return 0; }
#endif

void OPENSSL_showfatal (const char *fmta,...)
{ va_list ap;
  TCHAR buf[256];
  const TCHAR *fmt;
#ifdef STD_ERROR_HANDLE	/* what a dirty trick! */
  HANDLE h;

    if ((h=GetStdHandle(STD_ERROR_HANDLE)) != NULL &&
	GetFileType(h)!=FILE_TYPE_UNKNOWN)
    {	/* must be console application */
	int   len;
	DWORD out;

	va_start (ap,fmta);
	len=_vsnprintf((char *)buf,sizeof(buf),fmt,ap);
	WriteFile(h,buf,len<0?sizeof(buf):(DWORD)len,&out,NULL);
	va_end (ap);
	return;
    }
#endif

    if (sizeof(TCHAR)==sizeof(char))
	fmt=(const TCHAR *)fmta;
    else do
    { int    keepgoing;
      size_t len_0=strlen(fmta)+1,i;
      WCHAR *fmtw;

	fmtw = (WCHAR *)alloca(len_0*sizeof(WCHAR));
	if (fmtw == NULL) { fmt=(const TCHAR *)L"no stack?"; break; }

#ifndef OPENSSL_NO_MULTIBYTE
	if (!MultiByteToWideChar(CP_ACP,0,fmta,len_0,fmtw,len_0))
#endif
	    for (i=0;i<len_0;i++) fmtw[i]=(WCHAR)fmta[i];

	for (i=0;i<len_0;i++)
	{   if (fmtw[i]==L'%') do
	    {	keepgoing=0;
		switch (fmtw[i+1])
		{   case L'0': case L'1': case L'2': case L'3': case L'4':
		    case L'5': case L'6': case L'7': case L'8': case L'9':
		    case L'.': case L'*':
		    case L'-':	i++; keepgoing=1; break;
		    case L's':	fmtw[i+1]=L'S';   break;
		    case L'S':	fmtw[i+1]=L's';   break;
		    case L'c':	fmtw[i+1]=L'C';   break;
		    case L'C':	fmtw[i+1]=L'c';   break;
		}
	    } while (keepgoing);
	}
	fmt = (const TCHAR *)fmtw;
    } while (0);

    va_start (ap,fmta);
    _vsntprintf (buf,sizeof(buf)/sizeof(TCHAR)-1,fmt,ap);
    buf [sizeof(buf)/sizeof(TCHAR)-1] = _T('\0');
    va_end (ap);

#if defined(_WIN32_WINNT) && _WIN32_WINNT>=0x0333
    /* this -------------v--- guards NT-specific calls */
    if (GetVersion() < 0x80000000 && OPENSSL_isservice() > 0)
    {	HANDLE h = RegisterEventSource(0,_T("OPENSSL"));
	const TCHAR *pmsg=buf;
	ReportEvent(h,EVENTLOG_ERROR_TYPE,0,0,0,1,0,&pmsg,0);
	DeregisterEventSource(h);
    }
    else
#endif
	MessageBox (NULL,buf,_T("OpenSSL: FATAL"),MB_OK|MB_ICONSTOP);
}
#else
void OPENSSL_showfatal (const char *fmta,...)
{ va_list ap;

    va_start (ap,fmta);
    vfprintf (stderr,fmta,ap);
    va_end (ap);
}
int OPENSSL_isservice (void) { return 0; }
#endif

void OpenSSLDie(const char *file,int line,const char *assertion)
	{
	OPENSSL_showfatal(
		"%s(%d): OpenSSL internal error, assertion failed: %s\n",
		file,line,assertion);
#if !defined(_WIN32) || defined(__CYGWIN__)
	abort();
#else
	/* Win32 abort() customarily shows a dialog, but we just did that... */
	raise(SIGABRT);
	_exit(3);
#endif
	}

#ifndef OPENSSL_FIPSCANISTER
void *OPENSSL_stderr(void)	{ return stderr; }
#endif
