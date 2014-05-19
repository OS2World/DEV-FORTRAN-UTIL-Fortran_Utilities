#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/lib/RCS/environment.c,v 1.21 1996/08/27 19:19:48 koen Exp koen $";
#endif
/*****
* environment.c : environment scanning routines for libforUtil
*
* This file Version: 	$Revision: 1.21 $
*
* Creation date: 	03/18/1996 
* Last Modification:	$Date: 1996/08/27 19:19:48 $ 
* By:			$Author: koen $
* Current State:	$State: Exp $
* 
* Author:		koen
* (C)1995 Ripley Software Development
* All Rights Reserved
*****/ 
/*****
* ChangeLog
* $Log: environment.c,v $
* Revision 1.21  1996/08/27 19:19:48  koen
* msdos related updates
*
* Revision 1.20  1996/08/07 21:17:40  koen
* moved all msdos passwd emulation routines to msdospwd.c
*
* Revision 1.19  1996/08/02 14:52:52  koen
* Replaced forLibP.h with forutil.h
*
* Revision 1.18  1996/07/30 02:02:37  koen
* Reordered include files.
*
* Revision 1.17  1996/07/16 09:19:02  koen
* added an extra LOGIN if for msdos implementation of getpwuid. 
* Corrected a bug in make_directory
*
* Revision 1.16  1996/05/06 00:36:43  koen
* Adapted for MSDOS: added bogus getpwent & getpwnam functions.
*
* Revision 1.15  1996/03/08 15:32:11  koen
* Moved all Open Include database stuff to database.c
*
* Revision 1.14  1996/03/06 17:18:04  koen
* Working on open_selected stuff: a version which uses dbm, and a simple one 
* not using dbm. Removed the previously existing XXXXSearch routines 
* completely, the new routines are much faster.
*
* Revision 1.13  1996/02/28 02:55:52  koen
* Added make_directory and string_to_file routines.
*
* Revision 1.12  1996/02/19 01:29:38  koen
* Added lots of #ifdef VERBOSE fprintf... #endif lines.
*
* Revision 1.11  1996/02/14 01:52:58  koen
* Added the scan_for_(home,env_var,environment) routines + stuff to compile 
* misc.c standalone if TEST is defined.
*
* Revision 1.10  1996/02/13 19:21:00  koen
* Removed all x-related stuff to NeditUtils.c and GenUtils.c
* Moved the file-handling routines from GenUtils.c to here.
*
* Revision 1.8  1995/11/24  03:26:17  koen
* Added the XPromptMessage function: a wrapper for the DialogF of type 
* DF_PROMPT.
* Corrected return values for CheckIfSaved.
*
* Revision 1.7  1995/11/22  03:26:09  koen
* Changed CheckIfSaved to use DialogF instead of PostWarningBox. The latter
* doesn't seem to work under Linux.
*
* Revision 1.6  1995/11/17  01:44:45  koen
* Corrected a number of small bugs occuring in lint. Removed all runtime
* tracing information.
*
* Revision 1.5  1995/11/14  04:08:10  koen
* Code clean up.
*
* Revision 1.4  1995/11/12  23:02:41  koen
* modified get_home_dir, user's home directory is now returned in the
* argument **home_dir. Previous form returned a char*, which gave
* a casting warning when used.
*
* Revision 1.2  1995/10/31  17:32:46  koen
* new source file Id record
*
* Revision 1.1  1995/10/09  01:49:43  root
* Initial revision
*
*****/
/* needed to prevent multiple variable decls under MSDOS in sysdeps.h */
#define LIBRARY_OBJECT

/* include this before anything else */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h> 
#include <stdlib.h>
#include <ctype.h> 
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "memmacros.h"
#include "forutil.h"
#include "msdospwd.h"

/*
* permissions to use for directory creation, rwx for user, 
* rx for group and others. Change if you do not like this.
*/
#ifndef __MSDOS__
#define DIR_PERM (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#endif

/****
* Private data and function prototypes
****/
static struct passwd *user_pwd = NULL;

/*****
* get the user id of the current user
* If root is using this, it's gonna get called every time, but what
* the heck, root shouldn't be doing any development as root anyway...
*****/
static uid_t 
get_user_id(void)
{
	static uid_t user_id;
	if(user_id == 0)
		user_id = getuid();
	return(user_id);
}

/*****
* return a passwd for the current user.
* If it fails, default to name <unknown> and \tmp as home dir and
* tell the user what will happen.
*****/
static void 
get_user_info(void)
{
	if(user_pwd == NULL)
	{
		/* get the user data from the passwd file */
		user_pwd = getpwuid(get_user_id());
		if(user_pwd == NULL)
		{
			static char *temp_path;
			/*
			* no passwd info found, set name fields to <unknown> and try
			* to figure out what to use as pw_dir: try TMP, TEMP or /tmp 
			*/
			if((temp_path = getenv("TMP")) == NULL)
				if((temp_path = getenv("TEMP")) == NULL)
#ifdef __MSDOS__	/* current directory */
					temp_path = ".\\";
#else
					temp_path = "/tmp";
#endif
			fprintf(stderr, "Hmm, I cannot figure out who you are. "
				"Using %s for temporary storage.\n",
				temp_path);
			checked_malloc(user_pwd,1,struct passwd);
			user_pwd->pw_name = "<unknown>";
			user_pwd->pw_gecos = "<unknown>";
			user_pwd->pw_dir = temp_path;
		}
	}
}

/****
* get the login name of the user
****/
char 
*get_user_login(void)
{
	get_user_info();
	return(user_pwd->pw_name);
}

/****
* get the real name of the user
****/
char 
*get_user_name(void)
{
	get_user_info();
	return(user_pwd->pw_gecos);
}

/****
* get the home directory of the user
****/
void 
get_home_dir(char **home_dir)
{
	get_user_info();
	*home_dir = user_pwd->pw_dir;
	return;
}

/****
* Scan for referenced home directories by the char ~ in a string
****/
static void
scan_for_home(char *file)
{
	char *newfile;
	struct passwd *user_info = NULL;
	char *chPtr = NULL;
	int i = 0;

	/* scratch memory */
	checked_alloca(newfile, MAXPATHLEN, char);

	/*
	* Get the name of the user referred to. This scan stops when a separation
	* character is encounterd.
	*/
	for(chPtr = file+1, i = 0 ; 
		*chPtr != '\0' && *chPtr != SLASH && *chPtr != ':' ; *chPtr++)
		newfile[i++] = *chPtr;
	newfile[i] = '\0';

	if(newfile[0] == '\0')	/* reference to the current user */
	{
		get_user_info();
		sprintf(newfile, "%s%s", user_pwd->pw_dir, chPtr);
	}
	else 	/* reference to another user. Get his home directory */
	{
		if((user_info = getpwnam(newfile)) == NULL)
			return;
		sprintf(newfile, "%s%s", user_info->pw_dir, chPtr);
	}
	strcpy(file, newfile);
}

/****
* Scan for an environment variable in the given string
****/
static void
scan_for_env_var(char *file)
{
	char *newfile;
	char *chPtr = NULL, *envPtr = NULL;
	int i;

	/* scratch space */
	checked_alloca(newfile, MAXPATHLEN, char);

	/*
	* Get the name of the env. var referred to. This scan stops when a 
	* separation character or EOL is encounterd.
	*/
	for(chPtr = file+1, i = 0 ; 
		*chPtr != '\0' && *chPtr != SLASH && *chPtr != ':' ; *chPtr++)
		newfile[i++] = *chPtr;
	newfile[i] = '\0';

	/* no env. var of this name found, return and don't modify anything */
	if((envPtr = getenv(newfile)) == NULL)
		return;

	sprintf(newfile, "%s%s", envPtr, chPtr);
	/* substitute the real value and return the modified string */
	strcpy(file, newfile);
}

/****
* Scan for any environment variables or user's home directories
* and replace them with their corresponding value.
* Returns 1 if substitution took place, 0 if not.
****/
int
scan_for_environment(char *file)
{
	char *chPtr; 
	int changed = 0;

	/* first strip out any whitespace */
	stripspaces(file);

	for(chPtr = file ; *chPtr != '\0' ; *chPtr++)
	{
		switch(*chPtr)
		{
			case '~':
				/* home directory of current or other user */
				scan_for_home(chPtr);
				changed = 1;
				break;
#ifdef __MSDOS__	/* msdos environment vars begin with a percent sign */
			case '%':
#else
			case '$':
#endif
				/* environment variable */
				scan_for_env_var(chPtr);
				changed = 1;
				break;
		}
	}
	return(changed);
}

/*****
* make_directory: create a directory dir_to_make
*****/
int
make_directory(char *dir_to_make)
{
	char *temp, *ldir, *chPtr;
	char *curr_dir;
	int r_val;

	curr_dir = getcwd(NULL, MAXPATHLEN);	/* getcwd allocates this for us */

	checked_alloca(temp, MAXPATHLEN, char);
	checked_alloca(ldir, MAXPATHLEN, char);

	/* make a local copy of the given string, we do not want to alter it */
	strncpy(temp, dir_to_make, strlen(dir_to_make)+1);
	/* scan for any environment stuff inside */
	scan_for_environment(temp);
	/* check for a leading directory. If not prepend current path*/
	if(temp[0] != '.' && temp[0] != SLASH)
	{
		sprintf(ldir,"%s%s%s",curr_dir, CHAR_SLASH, temp);
		strncpy(temp,ldir,strlen(ldir)+1);
		strcpy(ldir,"");
	}
	/* test if we can cd to this dir */
	if(((r_val = chdir(temp)) == -1) && errno == ENOENT)
	{
		/* initialise the directory tree, subdirectory by subdirectory. */
		if(temp[0] == SLASH)
			strcpy(ldir, CHAR_SLASH);
		else
			strcpy(ldir,"");
		chPtr = strtok(temp, CHAR_SLASH);
		strcat(ldir, chPtr);
		strcat(ldir, CHAR_SLASH);
		/*
		* recurse through the directories to make. strtok always gives the first
		* part of the directory to create.
		*/
		for(chPtr = strtok(NULL, CHAR_SLASH);
			chPtr != NULL; chPtr = strtok(NULL, CHAR_SLASH))
		{
			strcat(ldir, chPtr);
			/* first try to cd to the given dir. */
			if((r_val = chdir(ldir)) == -1 && errno == ENOENT)
			{
				/* if we fail, create it. */
#ifdef __MSDOS__
				r_val = mkdir(ldir);
#else
				r_val = mkdir(ldir, DIR_PERM);
#endif
				if(r_val)
				{
					chdir(curr_dir);
					free(curr_dir);
					return(r_val);
				}
			}
			strcat(ldir, CHAR_SLASH);
		}
	}
	else
	/* directory already exists */
	{
		chdir(curr_dir);
		free(curr_dir);
		return(0); 
	}
	chdir(curr_dir);
	free(curr_dir);
	printf("Created directory %s\n", dir_to_make);
	return(r_val);
}

/****
* string_to_file: save the given string to a given file name
* In:
*	text: text to save to file
*	filename: file to save to
* Returns:
*	0 on success, -1 on failure and errno is set 
****/
int
string_to_file(char *text, char *filename)
{
	int len;
	FILE *fp;

	/* scan for any environment variables in here */
	scan_for_environment(filename);

	/* try to open a file with the given name */
#ifndef __MSDOS__
	if ((fp = fopen(filename,"w")) == NULL) 
#else
	if ((fp = fopen(filename,"wb")) == NULL) 
#endif
		return(-1);
	/* add a newline to the text if it isn't already there */
	len = strlen(text); 
	if(text[len-1] != '\n')
		text[len++]= '\n'; 
	/* flush the contents of text to the given file */
	fwrite(text, sizeof(char), len, fp);
	/* close the file */
	if (fclose(fp))
		return(-1);
	return(0);
}

/****
* Check if file exists by statting it.
* returns 0 on failure, size of file on success.
* This function does not test if the file is writable.
****/
unsigned long
check_if_file_exists(char *file)
{
	struct stat buf;
	if(stat(file, &buf))
		return(0);
	return((unsigned long)buf.st_size);
}
