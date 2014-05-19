%{
#ifndef lint
static char rcsId[]="$Source: /usr/local/rcs/ForUtil/fflow/RCS/fflow.l,v $";
#endif
/*****
* fflow.l : lexical analyzer for Fortran Source files
*
* This file Version	$Revision: 1.9 $
*
* Creation date:	Mon Feb  5 02:26:23 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:45:21 $
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
* $Log: fflow.l,v $
* Revision 1.9  1996/08/28 17:45:21  koen
* Added print_version_id; added the -llevel option and appropriate changes.
*
* Revision 1.8  1996/08/27 19:16:01  koen
* Heavy changes: updated scanner rules; optimized memory allocation routines; 
* moved a number of routines to libflow.c
*
* Revision 1.7  1996/08/07 21:14:24  koen
* Updated for MSDOS and added warning messages when -I, -E or -x is exceeded
*
* Revision 1.6  1996/08/02 14:50:29  koen
* Added the -n and -n- options. Moved all system dependencies to lib/sysdeps.h
*
* Revision 1.5  1996/07/30 02:00:57  koen
* Changed output stuff.
* Added missing things to command line options.
* Added version information.
* Changed the ignore_this_file routine to use strstr.
* Fixed a bug which left files open when a file was to be ignored.
*
* Revision 1.4  1996/07/16 09:05:44  koen
* cmd_line related bug fixed.
* Changed all fprintf(stderr,...) to printf(...) where output on stdout is
* expected
*
* Revision 1.3  1996/05/06 00:34:40  koen
* Adapted for MSDOS.
*
* Revision 1.2  1996/02/05 01:45:29  koen
* Added the inline define for systems without __inline__ (noticably HP-UX).
*
* Revision 1.1  1996/02/05 01:28:07  koen
* Initial revision
*
*****/ 
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h> 
#include <string.h>
#include <errno.h>

#include "forutil.h"
#include "memmacros.h"
#include "version.h"
#include "libflow.h"

/**** Private Variables ****/
static int skip_unknowns, ignore_empty_funcs;
static int start_at_program, ignore_unused_funcs, start_at_named_function;
static int complete_graph, never_invoked, no_graph; 
static int output_line, verbose, cutoff_depth;
static char *sep_char, *base_function_name, *outfile;
static char cmd_line[MAXPATHLEN];
static FILE *output_file;

static char *usage = {"Usage: %s [-cefhinpqtuv] [-Eext] [-Idir] [-ddepth] [-ofile] [-llevel] [-sname] [-xname] [files]\n\n"
" Generates a (partial) flowgraph of a collection of fortran files\n\n"
" Options\n"
"\t-c: generate a complete flowgraph for each subroutine or function\n"
"\t-e: ignore functions without calls\n"
"\t-f: print precise location of filenames. Otherwise print name\n"
"\t    of file only.\n"
"\t-h, --help: print this help\n"
"\t-i: ignore unknown function calls\n"
"\t-n: tell what subroutines are never invoked. Use -n- to only check for\n"
"\t    this and you do not want a flowgraph\n"
"\t-p: start flowgraph at PROGRAM definition\n"
"\t-q: be really quiet (usefull if called from a script)\n"
"\t-t: use tabs instead of | as depth indicator\n"
"\t-u: ignore unused subroutines\n"
"\t-v: be verbose\n"
"\t--version: display version number\n"
"\t-Eext: extensions to use for retrieving files. Multiple -E allowed.\n"
"\t-Idir: get files from directory dir. Multiple -I allowed\n"
"\t-ddepth: maximum recursion depth, default is %i\n"
"\t-llevel: Generate a flowgraph up to level deep\n"
"\t-ofile: file to write flowgraph to, default is stdout\n"
"\t-sname: use <name> as the topmost function\n"
"\t-xname.f: exclude file. Multiple -x allowed\n\n"
" Currently, fflow only looks at CALL, so although FUNCTION is found, these \n"
" do not show up in the flowgraph\n\n"
"(C)Copyright 1995-1996 by Ripley Software Development\n"};

/**** Private Function prototypes ****/
static void print_flow_graph(FILE *file, flowInfo *list);
static void print_flow_calls(FILE *file, flowInfo *calls, int depth);
static int set_global_option(char *arg);

%}

routine  ^([ \t]{6,})subroutine[ \t]
program  ^([ \t]{6,})program[ \t]
function ^([ \t]{6,})[ [:alpha:]\*[:digit:]]*function[ \t]
call	 [^('][ \t]{1,}call[ ]*
name [^\(\)'\n ]*[a-z0-9_]+
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
<FNAME>{name}?	{
		/*
		* Depending on the action taken,
		* add a new function to the flowgraph list or
		* add an entry for a call made in the current 
		* function
		*/
			if(action == ADDFUNC)
				add_new_func(yytext, yylineno, curr_path, 
					curr_file, 0);
			else
				add_new_call(yytext, yylineno, 0);
			BEGIN 0;
		}
{nl}		BEGIN 0;
.	 	; /* do nothing  for all other remaining chars */ 
%%

/*****
* Print the flowgraph to the given file. We follow up to depth levels
* deep. If this depth is reached, recursion might be happening. If this
* is the case, the program aborts with an appropriate message. The
* default value for depth is MAXDEPTH, defined to 64. 
* 
* This routine is recursive.
*****/
static void 
print_flow_calls(FILE *file, flowInfo *calls, int depth)
{
	flowInfo *tmp;
	int i;

	/* return if we are cutoff_depth levels deep in the calltree */
	if(cutoff_depth && depth >= cutoff_depth)
		return;

	if(depth >= maxdepth) 
	{
		fprintf(stderr,"ERROR: I'm %i levels deep. This might indicate "
			"undetected recursion.\n", depth);
		if(calls->name) 
			fprintf(stderr, "Current function is %s in file %s%s\n",
				calls->name, calls->path, calls->file);  
		fprintf(stderr,"If this is no recursion, increase maxdepth by "
				"using -d\n"); 
		exit(9);
	} 
	for(tmp = calls; tmp != NULL; tmp = tmp->next)
	{
		if(tmp->parent || (skip_unknowns == 0 && tmp->Type != KNOWN)) 
		{
			fprintf(file,"%-5i",depth+1);
			for(i = 0 ; i < depth; i++)
				fprintf(file," %s", sep_char);
			fprintf(file," %s -> %s, line %i ", sep_char, tmp->name,
				tmp->defline);
			if(tmp->parent)
			{
				if(complete_graph || 
					tmp->parent->output_line == 0) 
				{
					if(full_names)
						fprintf(file,"(%s", 
							tmp->parent->path);
					else
						fprintf(file, "(");
					fprintf(file, "%s, line %i)\n",
						tmp->parent->file, 
						tmp->parent->defline);
					if(errno)
					{
						fprintf(stderr, "Write failed:"
							" %s\n", ErrorString());
						exit(6);
					}
					tmp->output_line = ++output_line;
					if(!tmp->parent->recursive && 
						tmp->parent->calls)
						print_flow_calls(file, 
							tmp->parent->calls, 
							depth+1);
				} 
				else if(!complete_graph)
				{
					fprintf(file, "==>> line %i <<==\n", 
						tmp->parent->output_line);
					output_line++;
				}
			} 
			else 
			{
				fprintf(file, "(<unknown>)\n");
				tmp->output_line = ++output_line;
			}
		}
	}
}

/*****
* Main flow graph printing routine
*****/
static void 
print_flow_graph(FILE *file, flowInfo *list)
{
	flowInfo *tmp, *start;

	if(start_at_program)
	{
		for(tmp=list; tmp!=NULL && tmp->Type!=PROGRAM; tmp=tmp->next);
		if(tmp == NULL)
		{
			fprintf(stderr,"\nCould not find PROGRAM anywhere, "
				"I'm stymied\n");
			exit(10);
		}
		start = tmp;
	}	
	else
		if(start_at_named_function) 
		{ 
			for(tmp = list; tmp != NULL && 
				strcasecmp(base_function_name, tmp->name); 
				tmp = tmp->next);
			if(tmp == NULL)
			{
				fprintf(stderr,"\nCould not find function or "
					"subroutine %s anywhere, "
					"I'm stymied\n", base_function_name);
				exit(11);
			}
			start = tmp;
		} 
		else
			start = list;

	if(!quiet) 
		printf("Writing to %s...\n", outfile); 
	fflush(stdout);
	fprintf(file, "%%\n%% Fortran flowgraph generated by fflow\n");
	fprintf(file, "%% args used: %s\n%%\n", cmd_line); 
	output_line = 5;

	if(start_at_named_function) 
	{
		fprintf(file, "\n%%\n%% Generated for %s\n%%\n", 
			base_function_name); 
		output_line += 4;
	}
	/* neat loop termination trick */
	for(tmp = start; tmp != NULL; 
		tmp=(ignore_unused_funcs ? NULL : tmp->next))
	{
		if(verbose)
			printf("Generating flowgraph for: %s\n", tmp->name);
		if(tmp->calls || !ignore_empty_funcs)
		{
			tmp->output_line = output_line++;
			switch(tmp->Type)
			{
				case PROGRAM:
					fprintf(file,"program");
					break;
				case FUNCTION:
					fprintf(file,"function");
					break;
				case SUBROUTINE:
					fprintf(file,"subroutine");
					break;
				default:	/* should not be reached */
					fprintf(file,"(unknown type)");
			}
			if(full_names)
				fprintf(file," %s (%s",tmp->name,tmp->path);
			else
				fprintf(file," %s (", tmp->name);
			fprintf(file,"%s, line %i)\n\n",tmp->file,tmp->defline);
			output_line++;
			print_flow_calls(file, tmp->calls, 0);
			fprintf(file,"\n"); 
			output_line++;
			if(verbose)
				fflush(stdout);
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
		case 'c' :	/* generate a complete flowgraph */
			complete_graph = 1;
			break;
		case 'e' :	/* ignore empty functions */
			ignore_empty_funcs = 1;
			break;
		case 'f' :	/* use full names instead of filename only.*/
			full_names = 1 ;
			break;
		case 'h' :	/* help requested */
			printf(usage, progname, MAXDEPTH); 
			exit(2); 
		case 'i' :	/* don't print unresolved references */
			skip_unknowns = 1;
			break;
		case 'n' :	/* tell what subroutines are never invoked */
			never_invoked = 1;
			/* -n- will not generate a flowgraph */
			if(arg[1] != '\0' && arg[1] == '-')
			{
				no_graph = 1;
				return(1);
			}
			break;
#ifdef __MSDOS__
		case 'm' :	/* show memory usage under msdos */
			need_mem_info_for_msdos = 1;
			break;
#endif
		case 'p' :	/* start the flowgraph at PROGRAM */
			start_at_program = 1;
			break;
		case 'q' : 	/* be really quiet */
			quiet = 1; 
			verbose = 0; 
		case 't' :	/* use a tabs instead of | */
			sep_char = "\t";
			break;
		case 'u' :	/* ignore unreferenced functions */
			ignore_unused_funcs = 1;
			break;
		case 'v' : 	/* be verbose */
			verbose = 1; 
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
#ifdef __MSDOS__	/* dos returns uppercase when building a filelist */
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
		case 'd' : /* max. recursion depth */
			SCAN_ARG("-d");
			maxdepth = atoi(arg + 1);
			return(1);
		case 'l' : /* max. recursion depth */
			SCAN_ARG("-l");
			cutoff_depth = atoi(arg + 1);
			return(1);
		case 'o' :	/* name of the output file */
			SCAN_ARG("-o");
			outfile = arg + 1;
			return(1);
		case 's' :	/* start flowgraph at the given function name */
			start_at_named_function = 1;
			SCAN_ARG("-s");
			base_function_name = arg + 1;
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
#ifdef __MSDOS__	/* dos returns uppercase when building a filelist */
				upcase(ignore_list[num_ignore-1]);
#endif
			}
			return(1);
		case '-' :	/* secondary options */
			if(!strcmp("help", arg+1))	/* help wanted */
			{
				printf(usage, progname, MAXDEPTH); 
				exit(2); 
			}
			if(!strcmp("version", arg+1))	/* show version id */
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
* main for fflow
*****/
int 
main(int argc, char **argv)
{ 
	int i, narg; 
	char *arg; 

	/* set global default values */
	initialise(argv[0]);

	if(argc == 1)
	{
		fprintf(stderr, "%s: no files given\n",progname);
		printf(usage, progname, MAXDEPTH); 
		exit(4);
	}

	/* Default values */
	outfile = "stdout"; 
	sep_char = "|"; 
	maxdepth = MAXDEPTH; 
	cutoff_depth = 0;

	/* scan for any command line options and save a copy of it */
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
		strcat(cmd_line, argv[narg]);
		strcat(cmd_line, " ");
		arg = argv[++narg];
	}

	/* if no directories or files are given, exit */
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
	if(num_dirs_to_visit && verbose)
		printf("Scanning directories:\n");

	for(i = 0 ; i < num_dirs_to_visit; i++)
	{
		if(verbose)
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
		if(verbose && argc-narg !=0)
			printf("%i files given on command line.\n", argc - narg); 
	}

	/* show settings if we have to be verbose */
	if(verbose)
		print_settings();

	if(!quiet)
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

	/* print the flow graph if thus wanted */
	if(no_graph == 0)
		print_flow_graph(output_file, flow);

	/* print subroutines that are never invoked */
	if(never_invoked)
		print_unused_routines(output_file, flow);

	/* close the output file */
	if(!strcmp(outfile, "stdout"))
	{
		fflush(output_file);
		fclose(output_file); 
	}
	if(!quiet)
		printf("Done, total files scanned: %i\n", i);

	/* when wanted, print msdos memory usage summary */
	print_dos_memory_usage();

	return(0);
}
