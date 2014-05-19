/*****
* forutil.h : public header file for libforUtil.a
*
* This file Version	$Revision: 1.7 $
*
* Creation date:	Mon Feb  4 04:03:18 GMT+0100 1980
* Last modification: 	$Date: 1996/08/28 17:41:58 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Source: /usr/local/rcs/ForUtil/lib/RCS/forutil.h,v $
*****/
/*****
* ChangeLog 
* $Log: forutil.h,v $
* Revision 1.7  1996/08/28 17:41:58  koen
* Added proto for print_version_id
*
* Revision 1.6  1996/08/27 19:20:41  koen
* msdos related changes
*
* Revision 1.5  1996/08/07 21:19:06  koen
* Added the sysdeps header file
*
* Revision 1.4  1996/08/02 14:54:25  koen
* Added prototypes for downcase, file_has_ext and strend. 
* Moved system dependent stuff to sysdeps.h
*
* Revision 1.3  1996/07/30 02:03:05  koen
* Added a few default values if autoconf.h is not found. 
* Added a prototype for get_filename_part.
*
* Revision 1.2  1996/07/16 09:20:41  koen
* minor changes
*
* Revision 1.1  1996/04/25 02:30:40  koen
* Initial Revision
*
*****/ 

#ifndef _forutil_h_
#define _forutil_h_

#include "sysdeps.h"

#define SCAN_ARG(ARG) {if(arg[1] == '\0') \
	{ fprintf(stderr, "%s requires an argument\n", ARG); exit(1); }}

/***** Proto's from filelist.c *****/

/* create a filelist of path, store in **file_list, no of files stored
* is returned in num_files. Table of extensions to use is in ext_table,
* total no of extensions is in num_extensions */
extern void build_file_list(char **file_list[], int *num_files, char *path, 
	char *ext_table[], int num_extensions);

/***** Proto's from stringutil.c *****/

/* print program version number; p_minor is an rcs $Revision: 1.7 $ keyword */
extern void print_version_id(char *p_name, char *p_major, char *p_minor);

/* makes text in string all lowercase */
inline void downcase(char *string);

/* makes text in string all uppercase */
extern inline void upcase(char *string);

/* return true if file s1 has an extension */
extern inline int file_has_ext(char *s1);

/* return true if s2 is at the end of s1 */
extern inline int strend(char *s1, char *s2);

/* remove all spaces from a string */
extern void stripspaces(char *string);

#if !defined (HAVE_STRSTR)
#ifndef WINDOWS
extern char *strstr(char *s1, char *s2);
#endif
#endif

/* returns the last known error according to the value of errno */
extern char *ErrorString(void);

/***** Proto's from Splitpath.c *****/

/* split filename into a fully qualified file & and pathname */
extern int ParseFilename(char *fullname, char *filename, char *pathname);

/* get the filename part of any given path + filespec */
extern void get_filename_part(char *fullname, char *filename);

/***** Proto's from environment.c *****/

/* scan & substitute any environment variables in file */
extern int scan_for_environment(char *file);

/* create a directory */
extern int make_directory(char *dir_to_make);

/* write text to file filename */
extern int string_to_file(char *text, char *filename);

/* see if a file exists. If it does, return size of file */
extern unsigned long check_if_file_exists(char *file);

/* return the login name, true name and home directory of a user */
extern char *get_user_login(void);
extern char *get_user_name(void);
extern void get_home_dir(char **home_dir);

/* Don't add anything after this endif! */
#endif /* _forutil_h_ */
