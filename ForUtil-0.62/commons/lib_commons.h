/*****
* lib_commons.h : public header file for f77 common scanning/searching stuff
*
* This file Version	$Revision: 1.5 $
*
* Creation date:	Fri Mar 15 03:02:11 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:13:14 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/Commons/RCS/lib_commons.h,v 1.5 1996/08/27 19:13:14 koen Exp koen $
*****/
/*****
* ChangeLog 
* $Log: lib_commons.h,v $
* Revision 1.5  1996/08/27 19:13:14  koen
* ForUtil V0.62 update
*
* Revision 1.4  1996/07/30 01:57:03  koen
* checkin for ForUtil 0.57
*
* Revision 1.3  1996/05/06 00:32:26  koen
* Adapted for MSDOS.
*
* Revision 1.2  1996/04/25 02:27:20  koen
* Cleanup and comments added.
*
* Revision 1.1  1996/03/15 04:16:39  koen
* Initial Revision
*
*****/ 

#ifndef _lib_commons_h_
#define _lib_commons_h_

/* standard needed includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <gdbm.h>
#include <sys/stat.h>
#include <signal.h>

/* database file permissions */
#ifndef __MSDOS__
#define DBM_PERM (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#else
#define DBM_PERM 0644
#endif

/* keys for database information */
#define DBM_ID_TOKEN		"__cmmndbId"
#define DBM_UPDATE_TOKEN	"__cmmndbUpd"
#define DBM_INFO_TOKEN		"__cmmndbInfo"

#ifndef __MSDOS__
#ifdef SIGBUS 
#define ATTACH_SIGNALS { \
		signal(SIGBUS, sig_handler); \
		signal(SIGSEGV, sig_handler); \
		signal(SIGALRM, sig_handler); \
		signal(SIGUSR1, sig_handler); \
		signal(SIGQUIT, sig_handler); \
		signal(SIGINT, sig_handler); }
#else
#define ATTACH_SIGNALS { \
		signal(SIGSEGV, sig_handler); \
		signal(SIGALRM, sig_handler); \
		signal(SIGUSR1, sig_handler); \
		signal(SIGQUIT, sig_handler); \
		signal(SIGINT, sig_handler); }
#endif	/* SIGBUS */
#else
#define ATTACH_SIGNALS { \
		signal(SIGSEGV, sig_handler); \
		signal(SIGUSR1, sig_handler); \
		signal(SIGINT, sig_handler); }
#endif /* __MSDOS__ */

/* default database name */
#define DEF_DB	"commons.gdbm"
#define COMMON_LOCATION	"COMMON_DATABASE"
/* actually obsolete, but can be used to create separate databases */
#define COMMON_PRIVATE_LOCATION "COMMON_PRIVATE_DATABASE"
#define COMMON_MACHINE_LOCATION "COMMON_MACHINE_DATABASE"

/* scans a the block text for variable names */
extern int scan_block(char *text, int line_num);

/* find out where the database files are located */
extern char *get_database_location(char *db_env);

/* write and print the header of the file given */
extern void print_commondb_header(GDBM_FILE db_file);
extern void write_commondb_header(GDBM_FILE db_file, char *outfile);
extern void write_commondb_dirinfo(GDBM_FILE db_file, char *dir);
extern void write_commondb_update_info(GDBM_FILE db_file, int entries);

/* Don't add anything after this endif! */
#endif /* _lib_commons_h_ */
