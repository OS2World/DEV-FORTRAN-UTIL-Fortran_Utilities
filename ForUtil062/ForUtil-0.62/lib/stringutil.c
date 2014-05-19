#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/lib/RCS/stringutil.c,v 1.7 1996/08/28 17:42:14 koen Exp koen $";
#endif
/*****
* stringutil.c : some usefull string routines
*
* This file Version	$Revision: 1.7 $
*
* Creation date:	Mon Feb  4 03:54:11 GMT+0100 1980
* Last modification: 	$Date: 1996/08/28 17:42:14 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: stringutil.c,v $
* Revision 1.7  1996/08/28 17:42:14  koen
* Added print_version_id routine: prints a version string on stdout.
*
* Revision 1.6  1996/08/07 21:18:40  koen
* Corrected a potential bug in strend.
*
* Revision 1.5  1996/08/02 14:53:58  koen
* Added the downcase, file_has_ext and strend routines.
*
* Revision 1.4  1996/07/30 02:02:42  koen
* Reordered include files.
*
* Revision 1.3  1996/07/16 09:21:27  koen
* cast in ErrorString if HAVE_STRERROR is defined
*
* Revision 1.2  1996/05/06 00:36:53  koen
* Adapted for MSDOS.
*
* Revision 1.1  1996/04/25 02:30:29  koen
* Initial Revision
*
*****/ 
/* needed to prevent multiple variable decls under MSDOS in sysdeps.h */
#define LIBRARY_OBJECT

/* include this before anything else */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <ctype.h>
#include <errno.h>

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
extern int errno;
#else
#include <string.h>
#endif

#include "sysdeps.h"

/****
* Print a version information
* p_name: progran name
* p_major: major version id, plain string
* p_minor: minor version id, rcs-style revision string.
*****/
void
print_version_id(char *p_name, char *p_major, char *p_minor)
{
	char *chPtr;
	char minor[8];

	/* move to the first space */
	for(chPtr = p_minor; *chPtr != ' ' && *chPtr != '\0'; chPtr++);
	/* sanity check */
	if(*chPtr == '\0')
		chPtr = " 0.0 $";
	chPtr++;	/* skip past leading space */
	memset(minor, '\0', 8);
	strncpy(minor, chPtr, strlen(chPtr)-2); /* remove trailing stuff */
	printf("%s version %s.%s\n", p_name, p_major, minor);
}

/****
* Make string downcase
****/
inline void
downcase(char *string)
{
	register char *outPtr = string;
	for(outPtr = string; *outPtr != '\0'; 
		*(outPtr++) = tolower(*(string++)));
}

/****
* Make string uppercase
****/
inline void
upcase(char *string)
{
	register char *outPtr = string;
	for(outPtr = string; *outPtr != '\0'; 
		*(outPtr++) = toupper(*(string++)));
}

/*****
* See if s1 does have an extension attached to it
* return 1 of it does, 0 if it does not
******/
inline int
file_has_ext(char *s1)
{
	int l1 = strlen(s1)-1;	/* don't include '\0' */
	char *p1 = s1+l1;

	for(; *p1 && *p1 != SLASH && *p1 != '.' ; p1--);

	if(*p1 && *p1 == '.')
		return(1);
	return(0);
}

/*****
* see if the string s2 is at the end of string s1
* returns 1 if it does, 0 if it does not
*****/
inline int
strend(char *s1, char *s2)
{
	register int i,j;

	/* -1 since we don't include '\0' */
	for(i = strlen(s1)-1, j = strlen(s2)-1; s1[i] == s2[j] && j >= 0; 
		i--, j--);

	if(j == -1)
		return(1);
	return(0);
}

#if !defined (HAVE_STRSTR)
/* Return the starting address of string S2 in S1;
   return 0 if it is not found. 
   (note: take from the gnu file utilities) */
inline char
*strstr (char *s1, char *s2)
{
	register int i;
	register char *p1;
	register char *p2;
	register char *s = s1;

	for (p2 = s2, i = 0; *s; p2 = s2, i++, s++)
	{
		for (p1 = s; *p1 && *p2 && *p1 == *p2; p1++, p2++)
			;
		if (!*p2)
			break;
	}
	if (!*p2)
		return s1 + i;
	return 0;
}
#endif

/*****
* Remove white spaces from a string
*****/
void
stripspaces(char *string)
{
        char *outPtr = string;
        while (1)
        {
                if (*string != ' ' && *string != '\t')
                        *(outPtr++) = *(string++);
                else
                        string++;
                if (*string == 0)
                {
                        *outPtr = '\0';
                        return;
                }
        }
}

/*****
* Generic error string routine
*****/
char 
*ErrorString(void)
{
#ifdef HAVE_STRERROR
	return((char*)strerror(errno));
#else
	return(sys_errlist[errno]); 
#endif 
}
