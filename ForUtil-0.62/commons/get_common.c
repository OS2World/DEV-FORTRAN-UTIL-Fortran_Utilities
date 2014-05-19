#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/Commons/RCS/get_common.c,v 1.7 1996/08/28 17:44:10 koen Exp koen $";
#endif
/*****
* get_common.c : lookup the definition of a common in a database
*
* This file Version	$Revision: 1.7 $
*
* Creation date:	Fri Mar 15 02:50:55 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:44:10 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: get_common.c,v $
* Revision 1.7  1996/08/28 17:44:10  koen
* Added print_version_id and corrected usage message.
*
* Revision 1.6  1996/08/27 19:10:27  koen
* Added get_filename_part and updated usage reporting.
*
* Revision 1.5  1996/08/02 14:48:56  koen
* moved all system dependencies to lib/sysdeps.h
*
* Revision 1.4  1996/07/30 01:55:23  koen
* updated command line options.
*
* Revision 1.3  1996/05/06 00:32:32  koen
* Adapted for MSDOS
*
* Revision 1.2  1996/04/25 02:26:55  koen
* Cleanup and comments added.
*
* Revision 1.1  1996/03/15 04:16:28  koen
* Initial Revision
*
*****/ 
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include "lib_commons.h"
#include "forutil.h"
#include "memmacros.h"
#include "version.h"

static char progname[32];

static GDBM_FILE db_file;
static int showinfo;
static int senscase;
char *database;

static char *usage = {"Usage: %s [-cv] [-ddatabase] <name of common>\n"
"\n Options: \n"
"\t-c: case sensitive search, default is off.\n"
"\t-h, --help: print this help\n"
"\t-v : show database version information\n"
"\t--version: display version number\n"
"\t-d[database]: database to search. If not given, the value of \n"
"\t              %s is checked.\n\n"
"(C)Copyright 1995-1996 by Ripley Software Development\n"};

#define SHUTUP(text,exitnum)	{  \
	fprintf(stderr, "\n%s: %s\n", progname, text); \
	if(db_file) { \
		fprintf(stderr, "Syncing and closing database\n");\
		gdbm_close(db_file); \
	} \
	fprintf(stderr, "(aborted with signal %s)\n", #exitnum); \
	exit(exitnum+20); }

/****
* Signal handler for a nice and clean exit 
****/
static void
sig_handler(int signum)
{
	switch(signum)
	{
#ifdef SIGBUS
		case SIGBUS:
			SHUTUP("Aaaiieee, a bus error.", SIGBUS);
#endif /* SIGBUS */
		case SIGSEGV:
			SHUTUP("Oops, Segmentation violation!", SIGSEGV);
#ifdef SIGALRM
		case SIGALRM:
			SHUTUP("Aborted.", SIGALRM); 
#endif
		case SIGUSR1:
			SHUTUP("Memory allocation error.", SIGUSR1);
#ifdef SIGQUIT
		case SIGQUIT:
			SHUTUP("Interrupted", SIGQUIT);
#endif
		case SIGINT:
			SHUTUP("Interrupted", SIGINT);
		default:
			SHUTUP("What did _you_ do? Stopping.", signum);
	}
}

/*****
* Scan and set command line options
*****/
int set_global_option(char *arg)
{ 
	switch(arg[0])
	{
		case 'c' :	/* case sensitive search */
			senscase = 1;
			break;
		case 'h' :	/* help requested */
			printf(usage, progname, COMMON_LOCATION); 
			exit(2); 
		case 'v' :	/* only adminstritivia for the given database */
			showinfo = 1;
			break;
		case 'd' :	/* database to use */
			SCAN_ARG("-d")
			database = arg + 1;
			return(1);
		case '-' :	/* secondary options */
			if(!strcmp("help", arg+1))	/* help wanted */
			{
				printf(usage, progname, COMMON_LOCATION); 
				exit(2); 
			}
			if(!strcmp("version", arg+1))	/* version number */
			{
				print_version_id(progname, VERSION, "$Revision: 1.7 $");
				exit(2);
			}		/* fall through */
		default:
			fprintf(stderr,"unknown option -%s\n", arg);
			exit(3);
	}
	return(0); 
}


int main(int argc, char *argv[])
{
	GDBM_FILE db_file;
	datum key, content;
	char common[12], *chPtr, *arg;
	int narg, i;

	database = NULL;

	get_filename_part(argv[0], progname);

	arg = argv[narg = 1];
	while ( narg < argc && arg[0] == '-' )
	{
		if (arg[0] == '-')
		{ 
			int num_opts = strlen(arg) ; 
			for(i = 1 ; i < num_opts ; i++ )
			if(set_global_option(++arg)) 
				break;
		} 
		arg = argv[++narg];
	}

	if(narg == argc && !showinfo)
	{
		fprintf(stderr, "%s: no key given\n", progname);
		printf(usage, progname, COMMON_LOCATION);
		exit(4);
	}
	/*
	* Try to get a database name from the environment specification if no
	* database is given on the command line 
	*/
	if(database == NULL)
		database = get_database_location(COMMON_LOCATION);
	if(database == NULL)
	{
		fprintf(stderr, "%s: no database found/specified\n", argv[0]);
		exit(5);
	}

	/* Try to open the database */
	db_file = gdbm_open(database, 256, GDBM_READER, DBM_PERM, NULL);
	if(db_file == NULL)
	{
		fprintf(stderr, "gdbm error: %s\n", gdbm_strerror(gdbm_errno));
		perror(database);
		exit(6);
	}

	/* 
	* Attach signals, we don't want our database messed up when something 
	* happens 
	*/
	ATTACH_SIGNALS;

	if(!showinfo)
	{
		strcpy(common, argv[narg]);

		/* make key uppercase when we must ignore case */
		if(senscase == 0)
			upcase(common); 
		key.dptr = common;
		key.dsize = strlen(common)+1;
		content = gdbm_fetch(db_file, key);
		if(content.dptr == NULL)
		{
			fprintf(stderr, "%s not found in database %s\n", common,
				database);
			return(-1);
		}
		if(strstr(content.dptr, ","))
		{
			printf("%s defined in multiple files:\n", common);
			for(chPtr = strtok(content.dptr, ","); chPtr != NULL; 
				chPtr = strtok(NULL, ","))
				printf("\t%s\n", chPtr);
		}
		else
			printf("%s defined in file %s\n", common, content.dptr);
		free(content.dptr);
	}
	else
		print_commondb_header(db_file);
	gdbm_close(db_file);
	return(0);
}
