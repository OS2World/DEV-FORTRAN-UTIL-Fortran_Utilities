/*****
* sysdeps.h : public header file for system specific stuff
*
* This file Version	$Revision: 1.4 $
*
* Creation date:	Fri Aug  2 13:28:06 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:42:46 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Source: /usr/local/rcs/ForUtil/lib/RCS/sysdeps.h,v $
*****/
/*****
* ChangeLog 
* $Log: sysdeps.h,v $
* Revision 1.4  1996/08/28 17:42:46  koen
* Added #ifndef HAVE_ATEXIT for systems that do not have the atexit() function
*
* Revision 1.3  1996/08/27 19:20:58  koen
* msdos related changes
*
* Revision 1.2  1996/08/07 21:20:07  koen
* MSDOS related changes: added _heaplen, _stklen and all far*alloc defines
*
* Revision 1.1  1996/08/02 14:52:15  koen
* Initial Revision. Partly copied from forlibP.h
*
*****/ 

#ifndef _sysdeps_h_
#define _sysdeps_h_

#ifndef __MSDOS__	/* non-msdos default values */
# include <stdlib.h>
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif /* HAVE_UNISTD_H */
# include <sys/param.h>
# define SLASH	'/'
# define CHAR_SLASH	"/"
# ifndef NAME_MAX
#  define NAME_MAX	256
# endif
# ifndef PATH_MAX
#  define PATH_MAX	1024
# endif
# ifndef MAXPATHLEN
#  define MAXPATHLEN	1024
# endif
# define YYOUT_DEVICE "/dev/null"
# define MALLOC  malloc
# define CALLOC  calloc
# define REALLOC realloc

#else 		/* msdos/ms-windows default values */

# include <limits.h>
# include <dir.h>
# include <malloc.h>
# include <dos.h>
/* 
* to prevent a stack overflow set heap and stack to 32kb each. 
* IMPORTANT: this may only be defined ONCE.
*/
# ifndef LIBRARY_OBJECT
#ifndef _Windows
extern unsigned int _heaplen = (unsigned)32768L;
extern unsigned int _stklen = (unsigned)32768L;
#endif
# endif /* LIBRARY_OBJECT */
# define SLASH '\\'
# define CHAR_SLASH "\\"
# ifndef NAME_MAX
#  define NAME_MAX	13	/* +1 for terminating '\0' */
# endif
# ifndef PATH_MAX
#  define PATH_MAX	128
# endif
# ifndef MAXPATHLEN
#  define MAXPATHLEN 128
# endif
# define YYOUT_DEVICE "NUL"	/* NUL device of current directory */
# define strcasecmp strcmpi
# define SIGUSR1 SIGFPE
# ifndef _Windows
# define MALLOC  farmalloc
# define CALLOC  farcalloc
# define REALLOC farrealloc
# define free farfree
# else	/* Use regular functions for MS Windows */
# define MALLOC malloc
# define CALLOC calloc
# define REALLOC realloc
# endif /* _Windows */
#endif  /* __MSDOS__ */

/* define inline to be empty if it isn't defined in autoconf.h */
#ifndef inline
#define inline
#endif /* inline */

/* default return type from signal handlers if not defined in autoconf.h */
#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif

/* Bad luck. Get a decent system. */
#ifndef HAVE_ATEXIT
#define atexit(arg)
#endif

/* Don't add anything after this endif! */
#endif /* _sysdeps_h_ */
