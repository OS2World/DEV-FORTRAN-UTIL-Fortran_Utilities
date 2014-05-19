%{
#ifndef lint
static char rcsId[]="$Source: /usr/local/rcs/ForUtil/ffscan/RCS/ffscan.l,v $";
#endif
/*****
* fflow.l : f77 argument checker
*
* This file Version	$Revision: 1.5 $
*
* Creation date:	Wed Jul 17 22:38:31 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:50:18 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*
* This program has one undocumented switch for debugging purposes:
* -m for memory information under msdos
*****/
/*****
* ChangeLog 
* $Log: ffscan.l,v $
* Revision 1.5  1996/08/28 17:50:18  koen
* Changed verbose options and argument printing in count_args. 
* Added print_version_id.
*
* Revision 1.4  1996/08/27 19:16:48  koen
* Heavy changes: new scanner rules; optimized memory allocations; Moved
* a number of routines to libflow.c
*
* Revision 1.3  1996/08/07 21:14:56  koen
* Updated for MSDOS
*
* Revision 1.2  1996/08/02 14:50:47  koen
* moved all system dependencies to lib/sysdeps.h
*
* Revision 1.1  1996/07/30 02:04:03  koen
* Initial Revision. Based on fflow and scan_commons.
*
*****/
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "forutil.h"
#include "memmacros.h"
#include "version.h"
#include "libflow.h"

/**** Private Variables ****/
static char *outfile;
static int verbose, num_diff; 
static int total_routines, total_calls, total_funcs;
static int never_invoked;
static FILE *output_file;

static char *usage = {"Usage: %s [-fhnqv] [-Eext] [-Idir] [-ofile] [-xname] [files]\n\n"
" Scans the arguments of calls and compares them with the subroutine definition.\n\n"
" Options\n"
"\t-f: print precise location of filenames. Otherwise print name\n"
"\t    of file only.\n"
"\t-h, --help: print this help\n"
"\t-n: tell what subroutines are never invoked\n"
"\t-q: be really quiet (usefull if called from a script)\n"
"\t-v: be verbose. Additional -v increases verbosiness\n"
"\t--version: display version number\n"
"\t-Eext: extensions to use for retrieving files. Multiple -E allowed.\n"
"\t-Idir: get files from directory dir. Multiple -I allowed\n"
"\t-ofile: file to write flowgraph to, default is stdout\n"
"\t-xname.f: exclude file. Multiple -x allowed\n\n"
"(C)Copyright 1995-1996 by Ripley Software Development\n"};

/**** Private Functions ****/
static int set_global_option(char *arg);
static inline void count_args(char *text);
static void cross_check(FILE *file, flowInfo *list);

/*****
* On the regular expressions below:
* The routine and program regexps _must_ start with spaces (or tabs) (6 min.)
* at the beginning of a line followed by the name of keyword subroutine or 
* program and followed by a space (or a tab)
* The regexp for call and function is different in that it does not have to 
* start at the beginning of the line.
* It must start with at least one space and may be preceeded by any combination
* of whitespace, characters, digits and delimiters tokens.
* The name of the (function,subroutine,program) may _not_ start with a left
* or right brace or a single quote. The name itself may be made by any 
* combination of alpha-numeric characters and underscores and can be followed
* by an optional space or newline.
* The regexps for the argument blocks is split in two parts.
* The first part is the regexp for a single line of arguments: it may start 
* with up to six spaces, followed by any possible character (continuation char)
* any number of spaces and a left brace. After that any number of characters,
* followed by an optional right brace, an optional space and and optional
* newline.
* The second part is an argument list that may start right after the name
* of a (function,subroutine,program). It can have up to five spaces, any 
* possible character except a space (continuation char), optional spaces,
* an optional left brace. After that any number of chars followed by an
* optional right brace, space and newline.
* The regexp field <FNAME> contains three different regexps which will
* match (to my knowledge) any formulation of a function or subroutine
* definition with any sort of argument specification, including no arguments.
*****/
%}

routine  ^([ \t]{6,})subroutine[ \t]
program  ^([ \t]{6,})program[ \t]
function ^([ \t]{6,})[ [:alpha:]\*[:digit:]]*function[ \t]
call	 [^('][ \t]{1,}call[ ]
name [^\(\)' ]*[a-zA-Z0-9_]+[ \n]?
argline [ ]{0,6}.?[ ]*\(.+\)?[ ]?\n?
argblock [ ]{5}[^ ][ ]*\(?.+\)?[ ]?\n?
nl \n

%option caseless yylineno

%x FNAME

%%
^[*cC].* ; /* eat up comments entirely */  
{program}	{
			action = ADDFUNC; 
			Type   = PROGRAM;
			BEGIN FNAME;
		}  
{function}	{
			numfunc++;
			action = ADDFUNC; 
			Type   = FUNCTION; 
			BEGIN FNAME;
		}  
{routine}	{
			numroutine++;
			action = ADDFUNC; 
			Type   = SUBROUTINE; 
			BEGIN FNAME;
		} 
{call}		{
			numcall++;
			action = ADDCALL; 
			Type   = UNKNOWN; 
			BEGIN FNAME;
		} 
<FNAME>{name}{argline}* |
<FNAME>{name}{argblock}* |
<FNAME>{name}{argline}{argblock}* {
			count_args(yytext);
			BEGIN 0;
		}  
{nl}		BEGIN 0;
.		; /* do nothing  for all other remaining chars */
%%

/*****
* Count all arguments in the given text
*****/
static inline void
count_args(char *text)
{
	char *string = text;
	char func_name[64];
	char *args = NULL;
	int brace = 0, num_args = 0, num_line = 0, at_bol = 0;

	/* get function name */
	for(;*string !='\0' && *string != '(' && *string != '\n' && 
		!isspace(*string) ; string++);

	/* do not overflow name space */
	strncpy(func_name, text, string - text > 64 ? 64 : string - text);

	/* null-terminate */
	func_name[string-text] = '\0';
	/* only if we have to be very verbose */
	if(verbose > 2)
	{	
		char *chPtr;
		int i = 0;
		/* copy remaining args */
		checked_alloca(args, strlen(text), char);
		strncpy(args, string, strlen(string));
		args[strlen(string)] = '\0';
		printf("-------------------\n");
		/* 
		* print out the text preceeded with line numbers. 
		* This slows things seriously down!
		*/
		printf("line %i, text scanned:\n", yylineno);
		printf("%i: ", i);
		for(chPtr = text; *chPtr != '\0'; chPtr++)
		{
			putc(*chPtr, stdout);
			if(*chPtr == '\n' && *(chPtr+1)!='\0')
				printf("%i:", ++i);
		}
		putc('\n', stdout);
	}
	while(*string)
	{
		switch(*string)
		{
			case '(':	 /* initialize argcount. */
				if(num_args == 0)
					num_args++;
				brace++;
				break;
			case ')':
				brace--;
				break;
			case '*': /* eat up comments entirely */
			case 'c':
			case 'C':
				if(at_bol)
				{
					if(verbose > 2)
						printf("Rejecting comment on "
							"line %i\n", num_line); 
					while(*string != '\0' && 
						*string != '\n')
						string++;
					at_bol = 0;
				}
				break;
			case '\'':
				string++;
				while(*string != '\0' && *string != '\'')
				{
					if(*string == '\n')
						num_line++;
					string++;
				}
				break;
			case ',':	 /* only when single left brace open*/
				if(brace == 1)	
					num_args++;
				break;
		}
		if(*string == '\0')
			break;
		at_bol = 0;
		if(*string == '\n')
		{
			num_line++;
			at_bol = 1;
		}
		string++;
	}
	/* only if we have to be very verbose */
	if(verbose > 2)
	{
		if(action == ADDFUNC)
			printf("routine name is %s\nargs are: %s", func_name,
				num_args == 0 ? "<none>\n":args);
		if(action == ADDCALL)
			printf("call made: %s\nargs supplied are: %s",
				func_name, num_args == 0 ? "<none>\n":args);
		printf("brace count: %i\nargcount: %i\n", brace, num_args);
		printf("-------------------\n");
		fflush(stdout);
	}
	/* 19 continuation lines allowed by f77 standard */
	if(num_line > 18)	
	{
			fprintf(stderr, "WARNING: more than 19 continuation "
				"lines ");
			if(Type == ADDFUNC)
				fprintf(stderr, "in routine %s.\n", func_name); 
			else
				fprintf(stderr, "in call to %s.\n", func_name);
			fprintf(stderr,"near line %i in file %s%s\n", yylineno, 
				curr_path, curr_file);
	}
	if(brace != 0)
	{
		if(brace < 0)
			fprintf(stderr, "WARNING: unbalanced right brace ");
		else
			fprintf(stderr, "WARNING: unbalanced left brace ");
		fprintf(stderr, "on line %i of argument list ", num_line);
		if(Type == ADDFUNC)
			fprintf(stderr, "in routine %s.\n", func_name); 
		else
			fprintf(stderr, "in call to %s.\n", func_name);
		fprintf(stderr,"near line %i in file %s%s\n", yylineno, 
			curr_path, curr_file);
	}
	/*
	* Depending on the action taken,
	* add a new function to the flowgraph list or
	* add an entry for a call made in the current 
	* function
	*/
	if(action == ADDFUNC)
		add_new_func(func_name, yylineno, curr_path, curr_file, 
			num_args);
	else
		add_new_call(func_name, yylineno, num_args);
}

/*****
* Perform argument check
*****/
static void
cross_check(FILE *file, flowInfo *list)
{
	flowInfo *curr_func, *call; 
	for(curr_func = list; curr_func != NULL; curr_func = curr_func->next) 
	{
		for(call = curr_func->calls; call != NULL; call = call->next)
		{
			if(call->parent && 
				call->num_args != call->parent->num_args)
			{
				num_diff++;
				if(full_names)
				{
					fprintf(file, "---> %s called with %i "
						"args\n", 
						call->name, call->num_args);
					fprintf(file, "near line %i of %s%s\n",
						call->defline, curr_func->path,
						curr_func->file);
					fprintf(file, "Defined with %i args "
						"near line %i in \n",
						call->parent->num_args, 
						call->parent->defline);
					fprintf(file, "%s%s\n", 
						call->parent->path, 
						call->parent->file);
				}
				else
				{
					fprintf(file, "---> %s called with %i "
						"args ", 
						call->name, call->num_args);
					fprintf(file, "near line %i of %s\n", 
						call->defline, 
						curr_func->file);
					fprintf(file, "Defined with %i args ", 
						call->parent->num_args);
					fprintf(file, "near line %i in %s\n", 
						call->parent->defline, 
						call->parent->file);
				}
			}
		}
	}
}

/*****
* Scan command line options
*****/
static int 
set_global_option(char *arg)
{ 
	switch(arg[0])
	{
		case 'f' :	/* use full names instead of filename only.*/
			full_names = 1 ;
			break;
		case 'h' :	/* help requested */
			printf(usage, progname); 
			exit(2); 
		case 'n' :	/* tell what subroutines are never invoked */
			never_invoked = 1;
			break;
#ifdef __MSDOS__
		case 'm' :	/* show memory usage under msdos */
			need_mem_info_for_msdos = 1;
			break;
#endif
		case 'q' : 	/* be really quiet */
			quiet = 1; 
			verbose = 0; 
			break; 
		case 'v' : 	/* be verbose */
			verbose++;
			break; 
		case 'E' :	/* an extension specification */
			SCAN_ARG("-E");
			if(num_extensions == MAXEXTS)
			{
				fprintf(stderr, "Capacity exceeded, more than "
					"%i extensions given on command line\n",
					MAXEXTS);
				fprintf(stderr,"Ignoring extension %s\n",arg+1);
			}
			else
			{
				checked_malloc(ext_table[num_extensions], 
					strlen(arg+1)+1, char);
				strcpy(ext_table[num_extensions++], arg+1);
#ifdef __MSDOS__ /* dos returns uppercase when building a filelist */
				upcase(ext_table[num_extensions-1]);
#endif
			}
			return(1); 
		case 'I' :	/* a directory specification */
			SCAN_ARG("-I");
			if(num_dirs_to_visit == MAXDIRS)
			{
				fprintf(stderr,"Capacity exceeded, more than %i"
					" directories given on command line\n",
					MAXDIRS);
				fprintf(stderr,"Ignoring directory %s\n",arg+1);
			}
			else
			{
				checked_malloc(dirs_to_visit[num_dirs_to_visit],
					strlen(arg+1)+1, char);
				strcpy(dirs_to_visit[num_dirs_to_visit++], 
					arg+1); 
			}
			return(1); 
		case 'o' :	/* name of the output file */
			SCAN_ARG("-o");
			outfile = arg + 1;
			return(1); 
		case 'x' :	/* ignore the given file */
			SCAN_ARG("-x");
			if(num_ignore == MAXIGNORE)
			{
				fprintf(stderr, "Capacity exceeded, more than "
					"%i ignores given on command line\n",
					MAXIGNORE);
				fprintf(stderr, "Ignoring option -x%s\n",arg+1);
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
		case '-' :			/* secondary options */
			if(!strcmp("help", arg+1))	/* help wanted */
			{
				printf(usage, progname); 
				exit(2); 
			}
			if(!strcmp("version", arg+1))	/* show version number*/
			{
				print_version_id(progname, VERSION, "$Revision: 1.5 $");
				exit(2);
			}		/* fall through */
		default:
			fprintf(stderr,"unknown option -%s\n", arg);
			exit(3);
	}
	return(0); 
}

/*****
* main for ffscan
*****/
int 
main (int argc, char **argv)
{ 
	int i, narg;
	char *arg;

	/* set global default values */
	initialise(argv[0]);

	if(argc == 1) 
	{
		fprintf(stderr, "%s: no files given\n",progname);
		printf(usage, progname);
		exit(4);
	}

	/* Default values */
	outfile = "stdout";

	/* scan for any command line options */
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
	/* if no directories nor any files are given, exit */
	if(num_dirs_to_visit == 0 && narg == argc)
	{
		fprintf(stderr,"no files given.\n");
		exit(4);
	}

	/* install signal handlers */
	install_sig_handlers();

	/* open the output file */
	if(!strcmp(outfile, "stdout"))
		output_file = stdout;
	else if((output_file = fopen(outfile, "w")) == NULL)
	{
		perror(outfile);
		exit(6);
	}

	/*
	* always use this extension of none is given and directories are to be
	* searched. 3 equals the size of .f\0
	*/
	if(num_dirs_to_visit && !num_extensions)
	{
		checked_malloc(ext_table[num_extensions], 3, char);
#ifdef __MSDOS__	/* msdos returns uppercase when building a filelist */
		ext_table[num_extensions++] = ".F";
#else
		ext_table[num_extensions++] = ".f";
#endif
	}

	/* when directories have been given on the cmdline, build a filelist */
	if(num_dirs_to_visit && verbose > 1)
		printf("Scanning directories:\n");

	/* build a list of all files in the given directories (if any) */
	for(i = 0 ; i < num_dirs_to_visit; i++)
	{
		if(verbose > 1)
			printf("%s\n", dirs_to_visit[i]); 
		build_file_list(&file_list, &num_files, dirs_to_visit[i],
			ext_table, num_extensions); 
	}
	/* see if we have files left on the command line */
	if(narg != argc)
	{
		for(i = narg ; i < argc ; i++)
		{
			checked_realloc(file_list, num_files+1, char*);
			checked_malloc(file_list[num_files], strlen(argv[i])+1,
				char);
			sprintf(file_list[num_files++], "%s", argv[i]);
		}
		if(verbose > 1 && argc-narg !=0)
			printf("%i files given on command line.\n", argc - narg);
	}

	/* show settings if we have to be a little bit verbose */
	if(verbose > 1)
		print_settings();

	if(verbose > 1)
	{
		printf("Total Files to scan: %i\n", num_files);
		printf("File to write to: %s\n", outfile);
	}
	fflush(stdout);

	/* purge any non-matching output from yylex to /dev/null */
	yyout = fopen(YYOUT_DEVICE, "w+");

	/* scan the file list */
	for(i = 0; i < num_files; i++)
	{
		if ((num_ignore == 0 ? 1 : ignore_this_file(file_list[i])))
		{
			/* reset scanner for this file */
			if((yyin = reset_scanner(file_list[i])) == NULL)
				continue;
			yylineno = 0;
			/* scan the file */
			yylex();
			/*
			* flush the buffer of flex so we won't have
			* any trailing stuff from the previous file
			*/
			YY_FLUSH_BUFFER;
			/* close input file */
			fclose(yyin);
			/* report some statistics if we have to */
			if(verbose)
				print_file_stats(output_file, file_list[i], 
					yylineno);
			total_routines += numroutine;
			total_calls += numcall;
			total_funcs += numfunc;
		}
	}
	fclose(yyout);

	/* this is the final flowgraph data */
	flow = global_flow.head;

	if(!quiet) 
		printf("Sorting...\n"); 
	fflush(stdout);

	/* Sort the flowgraph */
	flow = sortAll(flow); 

	/* do argument check */
	cross_check(output_file, flow);

	/* print subroutines that are never invoked */
	if(never_invoked)
		print_unused_routines(output_file, flow);

	if(verbose)
	{
		fprintf(output_file, "Checked %i files.\n", num_files);
		fprintf(output_file, "Found %i subroutine definitions\n",
			total_routines);
		fprintf(output_file, "Found %i function definitions\n", 
			total_funcs);
		fprintf(output_file, "Found %i calls to subroutines\n", 
			total_calls);
		fprintf(output_file, "Checked %i calls in %i defined "
			"subroutines in %i files.\n", 
			total_calls, total_routines, num_files);
	}

	/* report final results */
	if(strcmp(outfile, "stdout") || !quiet)
	{
		if(num_diff)
			fprintf(output_file, "Found %i argument mismatches.\n", 
				num_diff);
		else
			fprintf(output_file, "No argument mismatches found.\n");
	}

	/* close output file */
	if(strcmp(outfile, "stdout"))
	{
		fflush(output_file);
		fclose(output_file);
	}

	/* when wanted, print msdos memory usage summary */
	print_dos_memory_usage();

	if(num_diff)
		return(12);
	else
		return(0);
}
