/*****
* msdospwd.h : passwd fake for msdos
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	Wed Aug  7 15:24:34 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:20:52 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Source: /usr/local/rcs/ForUtil/lib/RCS/msdospwd.h,v $
*****/
/*****
* ChangeLog 
* $Log: msdospwd.h,v $
* Revision 1.2  1996/08/27 19:20:52  koen
* msdos related changes
*
* Revision 1.1  1996/08/07 21:16:56  koen
* Initial Revision
*
*****/ 

#ifndef _msdospwd_h_
#define _msdospwd_h_

#ifndef __MSDOS__
#include <pwd.h>
#else

#include <sys/types.h>

struct passwd{
	char *pw_name;		/* Username.  */
	char *pw_passwd;	/* Password.  */
	uid_t pw_uid;		/* User ID.  */
	gid_t pw_gid;		/* Group ID.  */
	char *pw_gecos;		/* Real name.  */
	char *pw_dir;		/* Home directory.  */
	char *pw_shell;		/* Shell program.  */
};

#define getuid()	0

extern struct passwd *getpwuid(uid_t uid);
extern struct passwd *getpwnam(char *name);
extern struct passwd *getpwent(void);

#endif /* __MSDOS__ */
/* Don't add anything after this endif! */
#endif /* _msdospwd_h_ */
