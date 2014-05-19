#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/Commons/RCS/list_commons.c,v 1.9 1996/08/28 17:44:15 koen Exp koen $";
#endif
/*****
* list_commons.c : list the contents of the commons database
*
* This file Version	$Revision: 1.9 $
*
* Creation date:	Fri Mar 15 03:55:32 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:44:15 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: list_commons.c,v $
* Revision 1.9  1996/08/28 17:44:15  koen
* Added print_version_id
*
* Revision 1.8  1996/08/27 19:12:29  koen
* Added total no of commons and unique no of commons reporting.
*
* Revision 1.7  1996/08/07 21:12:41  koen
* Added a few upcase statements for MSDOS
*
* Revision 1.6  1996/08/02 14:49:07  koen
* moved all system dependencies to lib/sysdeps.h
*
* Revision 1.5  1996/07/30 01:55:34  koen
* Updated command line options.
*
* Revision 1.4  1996/07/16 00:37:59  koen
* added lists sorted per file stored in the database. Also added option to 
* only show the contents of a single file in the database.
*
* Revision 1.3  1996/05/06 00:32:10  koen
* Adapted for MSDOS. Also added signal handling.
*
* Revision 1.2  1996/04/25 02:26:49  koen
* Cleanup and comments added.
*
* Revision 1.1  1996/03/15 04:16:35  koen
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

typedef struct _file_info{
	char *filename;
	char *commons;
	int num_commons;
	int common_size;
	int curr_size;
	struct _file_info *next;
}file_info;

static char progname[32];

static GDBM_FILE db_file;	/* database file type */
static int showinfo;
static int num_files;		/* total number of files in database */
static int sort_per_file;	/* True for sorted output */
char *database;			/* name of database */
char *outfile;			/* name of output file */
char *incfile;			/* commons in file incfile, set sort_per_file */

file_info *common_file_contents;/* for sorted output */

#define BUFLEN	64 	/* allocation length for a block of commons */
			/* room for 9 commons. */

static char *usage = {"Usage: %s [-sv] [-ddatabase] [-fincfile] [-ooutfile]\n"
"\n Options: \n"
"\t-s: sort commons per file\n"
"\t-h, --help: print this help\n"
"\t-v: show database version information\n"
"\t--version: display version number\n"
"\t-ddatabase: database to list. If not given, the value of \n"
"\t            %s is checked.\n"
"\t-fincfile: output only the commons defined in file incfile\n"
"\t-ooutfile: place output in file outfile. Default is stdout\n\n"
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
* This is needed because the database must be closed properly whenever
* an error occurs. If this is not done, corruption of the database might
* occur.
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
* Add a common to the contents of an include file or create an entry for
* a include file.
* Args:
* info: ptr to file_info to update
* filename: name of file to store info for
* common: common to add to information of file filename
*****/
file_info
*add_file_info(file_info *info, char *filename, char *common)
{
	if(info == NULL)
	{
		/* allocate memory */
		checked_malloc(info, 1, file_info);	
		/* size of this common */
		info->common_size = strlen(common)+1;	
		info->num_commons = 1;
		info->curr_size = BUFLEN;
		checked_malloc(info->filename, strlen(filename)+1, char);

		/* allocate space for the commons in blocks of BUFLEN bytes */
		checked_malloc(info->commons, BUFLEN , char);
		strcpy(info->filename, filename);
		strcpy(info->commons, common);
		info->next = NULL;
		num_files++;
	}
	else
	{
	/*
	* if an entry for this file exists, append the name of the common to the
	* list for this file. If the buffer we use for storing the names of the
	* commons found in this file is about to overflow, we extend the 
	* buffer for this file by BUFLEN bytes.
	*/
		if(!strcmp(info->filename, filename))
		{
			/* ptr to location where new common is to be stored */
			int oldLen = info->common_size + 1;
			info->common_size += strlen(common)+2;
			/* check if we have space left in the buffer */
			if(info->common_size > info->curr_size)
			{
				info->curr_size += BUFLEN;
				checked_realloc(info->commons, info->curr_size,
					char);
			}
			/* store the common read */
			strcpy(info->commons + oldLen, common);
			info->num_commons++;
		}
		else
			info->next = add_file_info(info->next,filename,common);
	}
	return(info);
}

/*****
* Print the given file information only for a single file
*****/
static
void print_sorted_info_for_file(file_info *info, FILE *outF)
{
	file_info *tmpFile;
	char *chPtr;
	int i,k;

	for(tmpFile = info; tmpFile != NULL && 
		(strstr(tmpFile->filename, incfile)) == NULL; 
		tmpFile = tmpFile->next);
	if(tmpFile)
	{
		fprintf(outF, "\nFile: %s, %i commons\n", tmpFile->filename, 
			tmpFile->num_commons);
		for ( i = 0, k = 0, chPtr = tmpFile->commons; 
			i < tmpFile->num_commons; 
			i++, chPtr += strlen(chPtr)+2 )
		{
			/* allow 8 commons per line of output */
			fprintf(outF, "%s\t", chPtr);
			k++;
			if((k/8. - (int)(k/8.))==0. )
			{
				k = 0;
				fprintf(outF, "\n");
			}
		}
		fprintf(outF, "\n");
	}
	else
		fprintf(stderr, "%s: not found in %s\n", incfile, database);
}
/*****
* Print the given file information
*****/
static
void print_sorted_info(file_info *info, FILE *outF)
{
	file_info *tmpFile;
	char *chPtr;
	int i,k;

	for(tmpFile = info; tmpFile != NULL; tmpFile = tmpFile->next)
	{
		fprintf(outF, "\nFile: %s, %i commons\n", tmpFile->filename, 
			tmpFile->num_commons);
		for ( i = 0, k = 0, chPtr = tmpFile->commons; 
			i < tmpFile->num_commons; 
			i++, chPtr += strlen(chPtr)+2 )
		{
			/* 8 commons per line of output */
			fprintf(outF, "%s\t", chPtr);
			k++;
			if((k/8. - (int)(k/8.))==0. )
			{
				k = 0;
				fprintf(outF, "\n");
			}
		}
		fprintf(outF, "\n");
	}
}

/*****
* Scan and set command line options
*****/
int set_global_option(char *arg)
{ 
	switch(arg[0])
	{
		case 'h' :	/* help requested */
			printf(usage, progname, COMMON_LOCATION); 
			exit(2); 
		case 's' :	/* sort contents of database per include file */
			sort_per_file = 1;
			break;
		case 'v' :	/* only adminstritivia for the given database */
			showinfo = 1;
			break;
		case 'd' :	/* database to use */
			SCAN_ARG("-d")
			database = arg + 1;
			return(1);
		case 'f' :	/* include file to list */
			SCAN_ARG("-f")
			incfile = arg + 1;
#ifdef __MSDOS__
			upcase(incfile);
#endif
			sort_per_file = 1; /* must be true for this */
			return(1);
		case 'o' :	/* output file specified */
			SCAN_ARG("-o");
			outfile = arg + 1;
			return(1);
		case '-' :	/* secondary options */
			if(!strcmp("help", arg+1))	/* help wanted */
			{
				printf(usage, progname, COMMON_LOCATION); 
				exit(2); 
			}
			if(!strcmp("version", arg+1))	/* version number */
			{
				print_version_id(progname, VERSION, "$Revision: 1.9 $");
				exit(2);
			}		/* fall through */
		default:
			fprintf(stderr,"unknown option -%s\n", arg);
			exit(3);
	}
	return(0); 
}

/*****
* Main driver routine for sorting contents of a common database
* on a per include file basis.
****/
static
void list_and_sort(FILE *outF)
{
	datum key, nextkey, content;
	int i = 0, num_stored = 0, num_unique = 0;
	char *chPtr;

	print_commondb_header(db_file); /* print the header of the database */
	printf("Scanning Database. Be patient, this may take a while...\n");
	fflush(stdout);

	/* get first entry of the database file */
	key = gdbm_firstkey(db_file);	
	while(key.dptr)
	{
		/* get contents of this entry */
		content = gdbm_fetch(db_file, key);
		/* skip our own keywords (they begin with a __ ) */
		if(content.dptr && key.dptr[0] != '_' )
		{
			/* scan all files where this common is defined */
			for(chPtr = strtok(content.dptr, ","); chPtr != NULL; 
				chPtr = strtok(NULL, ","))
			{
				common_file_contents = add_file_info(common_file_contents, 
						chPtr, key.dptr);
				num_stored++;
			}
			num_unique++;
			free(content.dptr);
			/* print a dot for every 100 commons in the file */
			if( ++i == 100)
			{
				printf(".");
				fflush(stdout);
				i = 0;
			}
		}
		else
			if(key.dptr[0] != '_' )
				printf("WARNING: no data found for common %s\n",
					key.dptr);

		/* get the next key */
		nextkey = gdbm_nextkey ( db_file, key );
		free ( key.dptr );
		key = nextkey;
	}
	printf("\n");
	fflush(stdout);
	if(incfile != NULL)
		print_sorted_info_for_file(common_file_contents, outF);
	else
		print_sorted_info(common_file_contents, outF);
	printf("\nTotal commons stored: %i\n", num_stored);
	printf("Total unique commons stored: %i\n", num_unique);
	printf("Total files in database: %i\n", num_files);
}

/*****
* List the complete contents of a database, unsorted
*****/
static
void list_database(FILE *outF)
{
	datum key, nextkey, content;
	int num_stored = 0;

	key = gdbm_firstkey(db_file);
	while(key.dptr)
	{
		content = gdbm_fetch(db_file, key);
		if(content.dptr)
		{
			fprintf(outF, "%s, file %s\n", key.dptr, content.dptr);
			num_stored++;
			free(content.dptr);
		}
		else
			fprintf(stderr,"WARNING: no data found for common %s\n",
				key.dptr);
		nextkey = gdbm_nextkey ( db_file, key );
		free ( key.dptr );
		key = nextkey;
	}
	printf("Total unique commons stored: %i\n", num_stored);
} 

int main(int argc, char **argv)
{
	char *arg;
	int narg, i;
	FILE *outF = NULL;

	database = NULL;
	outfile = NULL;
	incfile = NULL;

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
	/*
	* Try to get a database name from the environment specification if no
	* database is given on the command line 
	*/
	if(database == NULL)
		database = get_database_location(COMMON_LOCATION);
	if(database == NULL)
	{
		fprintf(stderr, "%s: no database found/specified\n", argv[0]);
		printf(usage, progname, COMMON_LOCATION); 
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

	if(outfile)
	{
		outF = fopen(outfile, "w");
		if(outF == NULL)
		{
			perror(outfile);
			fprintf(stderr, "fopen failed for %s, resetting to "
				"stdout\n", outfile);
		}
	}
	if(outF == NULL)
		outF = stdout;
	if(!showinfo)
	{
		if(sort_per_file)
			list_and_sort(outF);
		else
			list_database(outF);
	}
	if(showinfo)
		print_commondb_header(db_file);
	gdbm_close(db_file);
	if(outF)
		fclose(outF);
	return(0);
}
