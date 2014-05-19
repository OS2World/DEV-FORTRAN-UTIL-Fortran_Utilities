/*****
* memmacros.h : iEdit memory allocation macros
*
* This file Version	$Revision: 1.6 $
*
* Creation date:	Wed Mar  6 22:15:20 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:20:45 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Source: /usr/local/rcs/ForUtil/lib/RCS/memmacros.h,v $
*****/
/*****
* ChangeLog 
* $Log: memmacros.h,v $
* Revision 1.6  1996/08/27 19:20:45  koen
* msdos related changes
*
* Revision 1.5  1996/08/07 21:19:29  koen
* Added the sysdeps header file
*
* Revision 1.4  1996/08/02 14:55:02  koen
* Added an #include <unistd.h> when HAVE_RAISE isn't defined.
*
* Revision 1.3  1996/07/16 09:20:57  koen
* added a define for raise if raise is not supported by the system
*
* Revision 1.2  1996/05/06 00:36:57  koen
* Adapted for MSDOS
*
* Revision 1.1  1996/03/08 15:35:30  koen
* Initial Revision
*
*****/

#ifndef _memmacros_h_
#define _memmacros_h_

/* AIX requires this to be the first thing in the file */
#ifdef __GNUC__
# ifndef alloca
# define alloca __builtin_alloca
# endif /* !alloca */
# else
#  if HAVE_ALLOCA_H
#   include <alloca.h>
#   else
#    ifdef _AIX
 #pragma alloca
#   else
#    ifndef alloca
#     ifndef __MSDOS__	/* gets defined in sysdeps.h */
char *alloca();
#     endif /* __MSDOS__ */
#    endif /* !alloca */
#   endif /* !_AIX */
# endif /* !HAVE_ALLOCA_H */
#endif /* !__GNUC__ */

#include <stdio.h>
#include <signal.h>
#include "sysdeps.h"

#ifndef HAVE_RAISE
#include <unistd.h>		/* for getpid() */
#define raise(SIG) kill(getpid(), SIG)
#endif

#define checked_malloc(var,size,type) \
	{if((var = (type*)MALLOC((size)*sizeof(type))) == NULL) \
	{ fprintf(stderr, "Internal error: malloc failed for %i bytes\n" \
		"file: %s, line %i\n", (size)*sizeof(type), __FILE__ , \
		__LINE__); \
	raise(SIGUSR1); \
	exit(7); } }

#define checked_calloc(var,size,type) \
	{if((var = (type*)CALLOC((size), sizeof(type))) == NULL) \
	{ fprintf(stderr, "Internal error: calloc failed for %i bytes\n" \
		"file: %s, line %i\n", (size)*sizeof(type), __FILE__ , \
		__LINE__); \
	raise(SIGUSR1); \
	exit(7); } }

#define checked_realloc(var,size,type) \
	{if((var = (type*)REALLOC((var), ((size)*sizeof(type)))) == NULL) \
	{ fprintf(stderr, "Internal error: realloc failed for %i bytes\n" \
		"file: %s, line %i\n", ((size)*sizeof(type)), __FILE__ , \
		__LINE__); \
	raise(SIGUSR1); \
	exit(7); } }

#define checked_alloca(var,size,type) \
	{if((var = (type*)alloca((size)*sizeof(type))) == NULL) \
	{ fprintf(stderr, "Internal error: alloca failed for %i bytes\n" \
		"file: %s, line %i\n", (size)*sizeof(type), __FILE__ , \
		__LINE__); \
	raise(SIGUSR1); \
	exit(7); } }

/* Don't add anything after this endif! */
#endif /* _memmacros_h_ */
