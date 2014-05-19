#ifndef lint
static char rcsId[]="$Source: /usr/local/rcs/ForUtil/lib/RCS/msdospwd.c,v $";
#endif
/*****
* msdospwd.c : fake passwd routines for msdos
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	Wed Aug  7 15:24:32 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:20:49 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: msdospwd.c,v $
* Revision 1.2  1996/08/27 19:20:49  koen
* msdos related changes
*
* Revision 1.1  1996/08/07 21:16:52  koen
* Initial Revision
*
*****/ 
/* needed to prevent multiple variable decls under MSDOS in sysdeps.h */
#define LIBRARY_OBJECT

/* include this before anything else */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdlib.h>
#include <sys/types.h>

#include "forutil.h"
#include "memmacros.h"
#include "msdospwd.h"

struct passwd
*getpwuid(uid_t uid)
{
	static struct passwd *pwd;

	if(pwd == NULL)
	{
		char *chPtr;

		checked_malloc(pwd, 1, struct passwd);
		/* get user identity */
		if((chPtr = getenv("USER")) == NULL)
			if((chPtr = getenv("LOGNAME")) == NULL)
				if((chPtr = getenv("LOGIN")) == NULL)
					chPtr = "nobody";
		pwd->pw_name = chPtr;
		pwd->pw_gecos = chPtr;

		/* set user and group id */
		pwd->pw_uid = uid;
		pwd->pw_gid = 0;

		/* get shell */
		if((chPtr = getenv("COMSPEC")) == NULL)
			chPtr = "COMMAND.COM";
		pwd->pw_shell = chPtr;
		pwd->pw_dir = "C:\\";	/* root directory of c drive */
	}
	return(pwd);
}

struct passwd
*getpwnam(char *name)
{
	static struct passwd *pwd;

	if(pwd == NULL)
	{
		char *chPtr;

		checked_malloc(pwd, 1, struct passwd);
		/* get user identity */
		if((chPtr = getenv("USER")) == NULL)
			if((chPtr = getenv("LOGNAME")) == NULL)
				if((chPtr = getenv("LOGIN")) == NULL)
					chPtr = name;
		pwd->pw_name = chPtr;
		pwd->pw_gecos = chPtr;

		/* set user and group id */
		pwd->pw_uid = 0;
		pwd->pw_gid = 0;

		/* get shell */
		if((chPtr = getenv("COMSPEC")) == NULL)
			chPtr = "COMMAND.COM";
		pwd->pw_shell = chPtr;
		pwd->pw_dir = "C:\\";	/* root directory of c drive */
	}
	return(pwd);
}

struct passwd
*getpwent(void)
{
	static struct passwd *pwd;

	if(pwd == NULL)
		pwd = getpwuid(0);
	return(pwd);
}
