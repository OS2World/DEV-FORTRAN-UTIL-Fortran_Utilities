%{
#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/Commons/RCS/scan_commons.l,v 1.9 1996/08/28 17:44:20 koen Exp koen $";
#endif
/*****
* scan_commons.l : database builder for stuff defined in common blocks
*
* This file Version	$Revision: 1.9 $
*
* Creation date:	Fri Mar 15 18:02:11 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:44:20 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*
* This program has three undocumented switches for debugging purposes:
* -c: dump core on errors
* -t: do not install signal traps, needed for debugging
* -m for memory information under msdos
*****/
/*****
* ChangeLog 
* $Log: scan_commons.l,v $
* Revision 1.9  1996/08/28 17:44:20  koen
* Added print_version_id
*
* Revision 1.8  1996/08/27 19:13:29  koen
* Changed scanner routines to support scanning of source files.
*
* Revision 1.7  1996/08/07 21:11:04  koen
* Updated the rules and added a rule for PARAMETER
*
* Revision 1.6  1996/08/02 14:49:29  koen
* moved all system dependencies to lib/sysdeps.h
*
* Revision 1.5  1996/07/30 01:58:16  koen
* Changed rule for common blocks to use every possible char as a 
* continuation character. 
* Implemented the ignore_this_file routine. 
* Reorganized output stuff and command line handling. 
* Added version information.
*
* Revision 1.4  1996/07/16 00:38:14  koen
* minor (cosmetic) changes.
*
* Revision 1.3  1996/05/06 00:33:02  koen
* Adapted for MSDOS. Added some showing of files to scan when -v is used.
*
* Revision 1.2  1996/04/25 02:27:04  koen
* Cleanup and comments added.
*
* Revision 1.1  1996/03/15 04:16:42  koen
* Initial Revision
*
*****/ 
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <signal.h>

#include "lib_commons.h"
#include "forutil.h"
#include "memmacros.h"
#include "version.h"

#define MAXDIRS		64
#define MAXIGNORE	64
#define MAXEXTS		8

#define YY_EXIT_FAILURE	13

/****
* Local variables
****/
datum content;
int verbose;
static char progname[32];
static int quiet, full_names, replace, dump_core, no_signal;
static int num_total, num_total_accepted, num_total_rejected;
static int num_total_replaced, num_total_stored;
static int num_common_total, num_common_rejected, num_common_accepted;
static int num_common_replaced, num_common_stored, fast;
static int reorganize, curr_file_num, ign_file_num;
static char *outfile;
static char curr_path[512], curr_file[128];
static int num_dirs_to_visit, num_extensions, num_ignore;
static char *dirs_to_visit[MAXDIRS];
static char *ext_table[MAXEXTS];
static char *ignore_list[MAXIGNORE];

int num_files;
static int num_warnings, num_errors;
char **file_list;

#ifdef __MSDOS__
unsigned long dir_mem, ext_mem, file_mem, ign_mem;
static int need_mem_info_for_msdos;
#endif

/* the database file */
static GDBM_FILE db_file;

/****
* Local function prototypes
****/
static void get_common_contents(char *text);

static void (*gdbm_proto_func)();	/* generic gdbm insertion proto */
static void replace_item(datum key);	/* replace & update an item */
static void insert_item(datum key);	/* insert an item */

static char *usage = {"Usage: %s [-afhqrsv] [-Eext] [-Idir] [-ddatabase] [-xname] [files]\n\n"
" Scan f77 include files for commons and stores them for use with get_common\n"
"\n Options:\n"
"\t-a: add instead of rejecting duplicate commons in different files\n"
"\t-f: Store filenames only. Default is full filenames.\n"
"\t-h, --help: print this help\n"
"\t-q: be really quiet (usefull if called from a script)\n"
"\t-r: re-organize after updating. This may take a _long_ time.\n"
"\t-s: safe operation, resync database after each update (safe but slower)\n"
"\t-v: verbose\n"
"\t--version: display version number\n"
"\t-E[ext] : extensions to use when searching files. Multiple -E allowed\n"
"\t          Use -E- to specify no extension.\n"
"\t-I[dir] : get files from directory dir. Multiple -I allowed\n"
"\t-d[database]: name of database to create/update\n"
"\t-x[name]: name of file to ignore when scanning directories\n"
" Using -f can help reduce the size of the created database.\n"
" To automate updates, scan_commons looks at the value of the environment\n"
" variable COMMON_DATABASE if no database is given.\n\n"
"(C)Copyright 1995-1996 by Ripley Software Development\n"};

/****
* A Note on the regular expressions used.
*
* We scan for a number of defined types in common include files.
* (common, character, real and logical)
* The scanning sequence is as follows:
* 1. A scan is made for lines containing the COMMON, LOGICAL, REAL,
*    DOUBLE, COMPLEX and CHARACTER keywords. These lines must begin with
*    at least 6 spaces and must be followed by characters defining that param.
*    The contents of the text found are scanned for ``real'' definitions by 
*    get_common_contents
* 
* 2. When step 1 is complete, a further scan is done to see if the
*    common block found has continuation lines, identified by the 
*    BEGIN GETBLOCK statement. 
*    All lines following any of the keywords mentioned above that begin
*    with five spaces and a continuation character contain additional
*    contents of that parameter block and are picked up by the scanner.
*
* The ``+'' after both regexp tells lex it must match one or more of these
* regular expressions. 
*****/
%}

common ^([ ]{6,})common[[:print:]]+
parameter ^([ ]{6,})parameter[[:print:]]+
logical ^([ ]{6,})logical[[:print:]]+
integer ^([ ]{6,})integer[[:print:]]+
real ^([ ]{6,})real[[:print:]]+
double ^([ ]{6,})"double precision"[[:print:]]+
complex ^([ ]{6,})complex[[:print:]]+
character ^([ ]{6,})character[[:print:]]+
blocklines ([ ]{5})[^ ]{1}[ \t]+[[:print:]]+\n
comment ^[*cC].*
nl \n

%option caseless yylineno

%x GETBLOCK

%%
{comment}.+{nl}  BEGIN(INITIAL);

{parameter} | 
{parameter}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{common} |
{common}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{character} |
{character}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{logical} |
{logical}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{integer} |
{integer}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{real} |
{real}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{double} |
{double}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
{complex} |
{complex}\n({blocklines}+) {
			get_common_contents(yytext);
			BEGIN 0;
		}
.	BEGIN(INITIAL);
<<EOF>>	{
		yyterminate();
		}
%%

/****
* macro used in the signal catcher routine below.
* aborts the scanner if yyin is still open and, if needed,
* closes the current database file.
****/
#define SHUTUP(text,exitnum)	{ \
	fprintf(stderr, "\n%s: %s\n", progname, text); \
	if(yyin) \
	{ \
		fprintf(stderr, "Stopping scanner\n"); \
		YY_FLUSH_BUFFER; \
		fclose(yyin); fclose(yyout); \
	} \
	if(db_file) \
	{ \
		fprintf(stderr, "Syncing and closing database\n");\
		gdbm_close(db_file); db_file = NULL; \
	} \
	fprintf(stderr, "Terminated with signal %s\n", #exitnum); \
	fprintf(stderr, "Total files scanned: %i\n", curr_file_num); \
	if(dump_core) abort(); \
	exit(exitnum+20); \
	}

/****
* Exit function for internal flex errors. This function will
* close the database file (if left open)
****/
void
scanner_fail(void)
{
#if 0
	if(db_file)
	{
		gdbm_close(db_file);
		db_file = NULL;
	}
	fclose(yyin);
	fclose(yyout);
#endif
}
/****
* Signal handler for a nice and clean exit 
****/
static RETSIGTYPE
sig_handler(int signum)
{
	switch(signum)
	{
#ifdef SIGBUS	/* SIGBUS is not defined on all systems */
		case SIGBUS:
			SHUTUP("Aaaiieee, a bus error!", SIGBUS);
#endif /* SIGBUS */
		case SIGSEGV:
			SHUTUP("Ooops, a segmentation violation.", SIGSEGV);
#ifdef SIGALRM	/* same for SIGALRM */
		case SIGALRM:
			SHUTUP("Aborted.", SIGALRM);
#endif
		case SIGUSR1:
			SHUTUP("Memory allocation error.", SIGUSR1);
#ifdef SIGQUIT	/* same for SIGQUIT */
		case SIGQUIT:
			SHUTUP("Interrupted.", SIGQUIT);
#endif
		case SIGINT:
			SHUTUP("Interrupted.", SIGINT);
		default:
			SHUTUP("What did _you_ do?  Stopping.", signum);
	}
}

/*****
* replace an existing entry in the current database with the data contained
* in key. Otherwise create a new entry in the database.
*****/
static void replace_item(datum key)
{
	if(gdbm_exists(db_file, key))
	{
		datum prev;

		prev = gdbm_fetch(db_file, key);
		/*
		* If the stringcompare matches the current file, it's already 
		* here so we don't want to update it again. 
		*/
		if(strstr(prev.dptr, content.dptr))
		{
			num_common_rejected++;
			return;
		}
		/*
		* Resize the returned item so it will contain the next also 
		* but separated by a comma 
		*/
		prev.dsize += content.dsize + 2;
		checked_realloc(prev.dptr, prev.dsize, char);

		/* update data */
		strcat(prev.dptr, ",");
		strcat(prev.dptr, content.dptr);

		/* store the new data */
		gdbm_store(db_file, key, prev, GDBM_REPLACE);

		free(prev.dptr);
		num_common_accepted++;
		num_common_stored++;
		num_common_replaced++;
		return;
	}	/* fall through */
	insert_item(key);
}

/*****
* Add a new entry to the current database
*****/
static void insert_item(datum key)
{
	if(gdbm_store(db_file, key, content, GDBM_INSERT))
	{
		if(!verbose)
			printf("already have %s, rejected.\n", key.dptr);
		num_common_rejected++;
		return;
	}	/* fall through */
	num_common_accepted++;
	num_common_stored++;
}

/*****
* retrieve all commons found in text
*****/
static void
get_common_contents(char *text)
{
	char *chPtr, *block;
	int i;
	datum key;

	/* allocate scratch memory */
	checked_malloc(block, strlen(text)+1, char);
	/* 
	* must copy, text is a ptr into a buffer of the scanner. 
	* Modifying this block will cause unpredictable results 
	*/
	strcpy(block, text);
	/* make all text upcase */
	upcase(block);

	/* 
	* forget blocks with FUNCTION in them, scanner picked up the
	* wrong line.
	*/
	if(strstr(block, "FUNCTION"))
		return;

	/* scan the block for commons. scan_block returns 0 on error. */
	if((num_warnings = scan_block(block, yylineno)) != -1)
	{
		/* store the commons returned */
		for(i=0, chPtr = strtok(block, ","); chPtr != NULL; 
			chPtr = strtok(NULL, ","), i++)
		{
			key.dptr = chPtr;
			key.dsize = strlen(chPtr)+1;
			/*
			* gdbm_proto_func is a ptr to the actual function used. 
			* It's real value depends on the -a command line option 
			*/
			gdbm_proto_func(key); 
			num_common_total++; 
		}
	}
	else
		num_errors++;
	/* if we got warnings, display in what file they are found */
	if(num_warnings)
		fprintf(stderr, "File: %s%s\n", curr_path, curr_file);
	free(block);
}

/*****
* Command line handling
*****/
int set_global_option(char *arg)
{ 
	switch(arg[0])
	{
		case 'a' :	/* add instead of reject commons */
			printf("adding instead of rejecting duplicate "
				"commons\n");
			replace = GDBM_REPLACE;
			gdbm_proto_func = replace_item;
			break;
		case 'c' :	/* dump core on errors */
			dump_core = 1;
			break;
		case 'f' :	/* use filenames instead of full paths */
			full_names = 0 ;
			break;
		case 'h' :	/* help requested */
			printf(usage, progname); 
			exit(2); 
#ifdef __MSDOS__
		case 'm' :	/* show memory usage under msdos */
			need_mem_info_for_msdos = 1;
			break;
#endif
		case 'q' : 	/* be really quiet */
			quiet = 1; 
			verbose = 0; 
			break; 
		case 'r' :	/* re-organize database */
			reorganize = 1;
			break;
		case 's' :	/* fast store. No update after each add */
			fast = 0;
			break;
		case 't' :	/* don't install signal handlers */
			no_signal = 1;
			break;
		case 'v' : 	/* be verbose */
			verbose = 1; 
			break; 
		case 'E' :	/* an extension specification */
			SCAN_ARG("-E");
			if(num_extensions == MAXEXTS)
			{
				fprintf(stderr, "Capacity exceeded, more than "
					"%i extensions given on command "
					"line.\n", MAXEXTS);
				fprintf(stderr, "Ignoring extension %s\n", 
					arg+1);
			}
			else
			{
				checked_malloc(ext_table[num_extensions], 
					strlen(arg+1)+1,char);
				strcpy(ext_table[num_extensions++], arg+1); 
#ifdef __MSDOS__ 	/* dos returns uppercase when building a filelist */
				upcase(ext_table[num_extensions-1]);
#endif
			}
			return(1); 
		case 'I' :	/* a directory specification */
			SCAN_ARG("-I");
			if(num_dirs_to_visit == MAXDIRS)
			{
				fprintf(stderr, "Capacity exceeded, more than "
					"%i directories given on command "
					"line.\n", MAXDIRS);
				fprintf(stderr, "Ignoring directory %s\n", 
					arg+1);
			}
			else
			{
				checked_malloc(dirs_to_visit[num_dirs_to_visit],
					strlen(arg+1)+1, char);
				strcpy(dirs_to_visit[num_dirs_to_visit++], 
					arg+1); 
			}
			return(1); 
		case 'd' :	/* name of database to create/update */
			SCAN_ARG("-d");
			outfile = arg + 1;
			return(1); 
		case 'x' :	/* ignore file specification */
			SCAN_ARG("-x");
			if(num_ignore == MAXIGNORE)
			{
				fprintf(stderr, "Capacity exceeded, more than "
					"%i ignores given on command line.\n", 
					MAXIGNORE);
				fprintf(stderr, "Ignoring option -x%s\n", 
					arg+1);
			}
			else
			{
				checked_malloc(ignore_list[num_ignore], 
					strlen(arg+1)+1, char);
				strcpy(ignore_list[num_ignore++], arg+1);
#ifdef __MSDOS__ /* dos returns uppercase when building a filelist */
				upcase(ignore_list[num_ignore-1]);
#endif
			}
			return(1);
		case '-' :	/* secondary options */
			if(!strcmp("help", arg+1))	/* help wanted */
			{
				printf(usage, progname); 
				exit(2); 
			}
			if(!strcmp("version", arg+1))	/* version number */
			{
				print_version_id(progname, VERSION, "$Revision: 1.9 $");
				exit(2);
			}	/* fall through */
		default:
			fprintf(stderr,"unknown option -%s\n", arg);
			exit(3);
	}
	return(0); 
}

/*****
* Return 0 if the file given is to be ignored, something else otherwise
*****/
static inline int 
ignore_this_file(char *file)
{
	register int i;
	char *ret_val = NULL;
	for(i = 0 ; i < num_ignore &&
		(ret_val = strstr(file, ignore_list[i])) == NULL; i++)
			/* do nothing */ ;
	if(ret_val != NULL)
	{
		if(!quiet)
			printf("Ignoring file %s\n", file);
		ign_file_num++;
		return(0);
	}
	return(1);
}

/*****
* main for scan_commons
*****/
int main (int argc, char **argv)
{ 
	FILE *file;
	int i, narg;
	char *arg;
	char fullname[MAXPATHLEN];

	get_filename_part(argv[0], progname);

	if(argc == 1)
	{
		printf(usage, progname);
		exit(2);
	}

	/*
	* Command line handling. Set default switches.
	*/
	full_names = 1;
	replace = GDBM_INSERT;
	fast = GDBM_FAST;
	/* default action is to insert a common if it does not yet exist */
	gdbm_proto_func = insert_item;

	/* scan the entire command line for any options */
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
	if(outfile == NULL)
		outfile = get_database_location(COMMON_LOCATION);
	if(outfile == NULL)
	{
		fprintf(stderr, "%s: no database found/specified\n", progname);
		exit(5);
	}

	/* attach signal handlers if not in debug mode. */
	if(!no_signal)
		ATTACH_SIGNALS;

	/* see if we have a dir spec or files on the command line */
	if(num_dirs_to_visit == 0 && narg == argc && reorganize == 0 )
	{
		fprintf(stderr,"%s: no files given.\n", progname);
		exit(4);
	}

	/* report initial settings if we have to be verbose */
	if(verbose)
	{
		if(num_dirs_to_visit != 0)
		{
			printf("List of directories to visit:\n");
			for(i = 0 ; i < num_dirs_to_visit; i++)
				printf("%s\n", dirs_to_visit[i]);
		}
		if(num_extensions != 0)
		{
			printf("List of extensions to use:\n");
			for(i = 0 ; i < num_extensions; i++)
				printf("%s ", ext_table[i]);
			printf("\n");
		}
		if(num_ignore != 0)
		{
			printf("List of files to ignore:\n");
			for(i = 0 ; i < num_ignore; i++)
				printf("%s\n", ignore_list[i]);
		}
		if(num_dirs_to_visit != 0)
			printf("Scanning directories\n");
		fflush(stdout);
	}

	/* go create a list of all files in the directories given (if any) */
	for(i = 0 ; i < num_dirs_to_visit; i++)
	{
		if(verbose)
			printf("%s\n", dirs_to_visit[i]); 
		build_file_list(&file_list, &num_files, dirs_to_visit[i],
			ext_table, num_extensions); 
	}

	/* add files given on the command line to the list of files to visit */
	if(narg != argc)
	{
		for(i = narg ; i < argc ; i++)
		{
			checked_realloc(file_list, num_files+1, char*);
			checked_malloc(file_list[num_files], strlen(argv[i])+1,
				char);
			sprintf(file_list[num_files++], "%s", argv[i]); 
		}
		if(verbose && argc-narg !=0)
			printf("%i files given on command line.", argc - narg); 
	}

	/* give the list of files to scan when we are being verbose */
	if(verbose)
	{
		printf("List of files to scan:\n");
		for(i = 0 ; i < num_files; i++)
			printf("%s\n", file_list[i]);
	}

	/* show number of files to scan */
	if(!quiet)
	{
		printf("Total Files to scan: %i\n", num_files); 
		printf("Database to create/update: %s\n", outfile);
	}
	fflush(stdout);

	/* open the database file for storage. Set to fast mode if requested */
	db_file = gdbm_open(outfile, 256, GDBM_WRCREAT | fast, DBM_PERM, NULL);
	if(db_file == NULL)
	{
		fprintf(stderr, "gdbm error: %s\n", gdbm_strerror(gdbm_errno));
		perror(outfile);
		exit(6);
	}

	/* write some administrative data to the database */
	write_commondb_header(db_file, outfile);
	for(i = 0 ; i < num_dirs_to_visit; i++)
		write_commondb_dirinfo(db_file, dirs_to_visit[i]);

	/* purge non-matching chars to /dev/null */
	yyout = fopen(YYOUT_DEVICE, "w+");

	num_files--;

	/* register exit function if the scanner fails */
	atexit(scanner_fail);

	/* loop through all files in the filelist */
	for(i = 0; i <= num_files; i++)
	{
		/* Check to see if we are to ignore this file */
		if((num_ignore == 0 ? 1 : ignore_this_file(file_list[i])))
		{
			file = fopen(file_list[i], "r");
			if(!file)
			{
				perror(file_list[i]); 
				continue;
			}
			num_common_total = num_common_rejected = 0;
			num_common_accepted = 0;
			num_common_replaced = num_common_stored = 0;
			num_errors = num_warnings = 0;

			/* get the complete path to this file and store it */
			ParseFilename(file_list[i], curr_file, curr_path);
			if(full_names)
				sprintf(fullname, "%s%s", curr_path, curr_file);
			else
				sprintf(fullname, "%s", curr_file);

			/* data belonging to each common found in this file */
			content.dptr = fullname;
			content.dsize = strlen(fullname)+1;
			yyin = file;  
			if(!quiet) 
				printf("\nScanning %s\n", file_list[i]);
			rewind(yyin);
			/* go scan the file */
			yylex();
			YY_FLUSH_BUFFER;	/* clear scanner buffer */ 
			fclose(yyin);		/* close the input file */
			if(verbose) 		/* report if wanted */
			{ 
				printf("\n---- Stats for file %s (%i/%i)\n", 
					file_list[i], i+1, num_files+1);
				printf("commons found      : %i\n", 
					num_common_total);
				printf("duplicate commons  : %i\n", 
					num_common_replaced);
				printf("commons rejected   : %i\n", 
					num_common_rejected);
				printf("commons accepted   : %i\n", 
					num_common_accepted);
				printf("commons stored     : %i\n", 
					num_common_stored);
				printf("Scan warnings      : %i\n", 
					num_warnings);
				printf("Scan errors        : %i\n", 
					num_errors);
			}
			/* flush messages to stdout */
			fflush(stdout);
			num_total += num_common_total;
			num_total_replaced += num_common_replaced;
			num_total_rejected += num_common_rejected;
			num_total_accepted += num_common_accepted;
			num_total_stored   += num_common_stored;
			curr_file_num++;
		}
	}
	/* close /dev/null */
	fclose(yyout);

	/* report some statistics */
	if(!quiet)
	{
		printf("\n--- Total Statistics ---\n");
		printf("Directories visited: %i\n", num_dirs_to_visit);
		printf("Files searched     : %i\n", num_files+1);
		printf("Files ignored      : %i\n\n", ign_file_num);
		printf("commons found      : %i\n", num_total);
		printf("duplicate commons  : %i\n", num_total_replaced);
		printf("commons rejected   : %i\n", num_total_rejected);
		printf("commons accepted   : %i\n", num_total_accepted);
		printf("commons stored     : %i\n", num_total_stored);
		printf("\n------------------------\n");
	}

	/* write some more administrative data */
	write_commondb_update_info(db_file, num_total - num_total_rejected );

	if(reorganize)
	{
		if(!quiet)
			printf("Re-organizing database\n");
		if(gdbm_reorganize(db_file))
		{
			fprintf(stderr, "gdbm error: %s\n",
				gdbm_strerror(gdbm_errno));
			perror(outfile);
		}
	}
	gdbm_close(db_file);	/* close the database */
	db_file = NULL;

#ifdef __MSDOS__
	if(need_mem_info_for_msdos)
	{
		printf("\n------------------\n");
		printf("MSDOS Memory Usage\n\n");
		printf("Heap size is %u bytes\n", _heaplen);
		printf("Stack size is %u\ bytesn", _stklen);
		dir_mem = sizeof(dirs_to_visit);
		for(i = 0 ; i < num_dirs_to_visit; i++)
			dir_mem += strlen(dirs_to_visit[i]);
		printf("Memory used by dir list: %lu bytes\n", dir_mem);
		ext_mem = sizeof(ext_table);
		for(i = 0 ; i < num_extensions; i++)
			ext_mem += strlen(ext_table[i]);
		printf("Memory used by ext list: %lu bytes\n", ext_mem);
		ign_mem = sizeof(ignore_list);
		for(i = 0 ; i < num_ignore; i++)
			ign_mem += strlen(ignore_list[i]);
		printf("Memory used by ignore list: %lu bytes\n", ign_mem);
		file_mem = sizeof(file_list);
		for(i = 0 ; i < num_files; i++)
			file_mem += strlen(file_list[i]);
	 	printf("Memory used by file list: %lu bytes\n", file_mem);

		printf("Total memory used: %lu bytes\n",
			dir_mem + ext_mem + ign_mem + file_mem);
	}
#endif
	return(0);
}
