static char rcsId[]="$Source: /usr/local/rcs/ForUtil/fflow/RCS/libflow.c,v $";
/*****
* libflow.c : common flowgraph generation routines.
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	Mon Aug 19 15:42:20 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:47:25 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog
* $Log: libflow.c,v $
* Revision 1.2  1996/08/28 17:47:25  koen
* Added add_default_program, print_file_stats; reset_scanner. 
* Changed sig_handler to print signal name on exit. 
* Changed new_entry so only 1 allocation is done for name, file and path 
* instead of three separate mallocs.
*
* Revision 1.1  1996/08/27 19:14:50  koen
* Initial Revision
*
*****/ 
/* needed to prevent multiple variable decls under MSDOS in sysdeps.h */
#define LIBRARY_OBJECT

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <signal.h>

#include "forutil.h"
#include "memmacros.h"
#include "libflow.h"

/**** Externally declared variables ****/
extern FILE *yyin, *yyout;

/**** Public Variables ****/
global_flow_data global_flow;
flowInfo *flow;
int action, Type, full_names, quiet, maxdepth, have_first_routine;
int num_ignore, num_dirs_to_visit, num_extensions, num_files;
int numroutine, numcall, numfunc;
char progname[32];
char curr_path[PATH_MAX], curr_file[NAME_MAX];
char *ignore_list[MAXIGNORE];
char *dirs_to_visit[MAXDIRS];
char *ext_table[MAXEXTS];
char **file_list;

#ifdef __MSDOS__
unsigned long dir_mem, ext_mem, file_mem, ign_mem;
int need_mem_info_for_msdos;
static unsigned long list_mem = 0;
#endif

/**** Private Function prototypes ****/
static inline void getParent(flowInfo *list, flowInfo *call);
static void add_default_program(void);
static inline flowInfo *new_entry(char *name, int callline, char *path,
	char *file, int num_args);
static RETSIGTYPE sig_handler(int signum);

/* Exit procedure */
#define SHUTUP(text,exitnum)	{if(yyin) { \
			fprintf(stderr, "%s: %s (%s).\n", progname, \
				text, #exitnum); \
			fprintf(stderr, "Stopping scanner\n"); \
			fclose(yyin); fclose(yyout); } \
			exit(exitnum+20); }

/****
* Signal handler for a nice and clean exit
****/
static RETSIGTYPE
sig_handler(int signum)
{
	switch(signum)
	{
		case SIGUSR1:
			SHUTUP("Memory allocation error", SIGUSR1);
		case SIGSEGV:
			SHUTUP("Segmentation violation", SIGSEGV);
#ifdef SIGQUIT
		case SIGQUIT:
			SHUTUP("Interrupted by user", SIGQUIT);
#endif
		case SIGINT:
			SHUTUP("Interrupted by user", SIGINT);
		default:
			SHUTUP("Terminated with unknown signal", signum);
	}
}

/*****
* Create a new flowgraph entry
* Args:
* name: name of (program,subroutine,function)
* callline: line where a call was made -or- where name was defined
* path: pathname of file
* file: name of file
*****/
static inline flowInfo
*new_entry(char *name, int callline, char *path, char *file, int num_args)
{
	static flowInfo *list = NULL;
	int buf_size = 0;

	/* allocate new entry, initialize every field to 0 */
	checked_calloc(list, 1, flowInfo); 

	/*
	* Due to our setup which attaches calls to a particular routine in a
	* given file, calls do not have a file nor a path associated with them.
	*/
	if(action != ADDCALL)
	{
		/* 
		* size of block to allocate 
		* each string needs room for a terminating \0 -> +3 (or 4) 
		*/
		buf_size = strlen(name) + strlen(file) + 
			(full_names ? strlen(path) + 4 : 3);
		/* allocate block, initialize to 0 */
		checked_calloc(list->name, buf_size, char);

		/* save routine name */
		strcpy(list->name,name);

		/* save file name */
		list->file = list->name + strlen(name)+2;
		strcpy(list->file, file);
		if(full_names)
		{
			/* save path name */
			list->path = list->file + strlen(file)+2;
			strcpy(list->path, path);
		}
	}
	else
	{
		buf_size = strlen(name)+1;
		checked_malloc(list->name, buf_size, char);
		strcpy(list->name,name);
	}
	list->Type = Type;
	list->defline = callline;
	list->num_args = num_args;
#ifdef __MSDOS__
	list_mem += sizeof(flowInfo);
	list_mem += len;
#endif
	return(list);
}

/****
* Add a routine definition
****/
inline void
add_new_func(char *name, int callline, char *path, char *file, int num_args)
{
	static flowInfo *new_func;

	new_func = new_entry(name, callline, path, file, num_args);

	/* indicate we have a routine definition for the current file */
	have_first_routine = True;

	/* Data initialisation */
	if(global_flow.head == NULL)
	{
		global_flow.head = new_func;
		global_flow.current = new_func;
		global_flow.call_head = NULL;
		global_flow.current_call = NULL;
	}
	else
	{
		global_flow.current->next = new_func;
		global_flow.current = new_func;
		global_flow.call_head = NULL;
		global_flow.current_call = NULL;
	}
}

/*****
* Add a default program definition at line 1 of the current file.
*****/
static void
add_default_program(void)
{
	char program[NAME_MAX];
	char *chPtr;

	/* reset to 0 */
	memset(program, '\0', NAME_MAX);

	/* check if the current file has an extension. If so, strip it of */
	if((chPtr = strchr(curr_file, '.')) != NULL)
		strncpy(program, curr_file, chPtr - curr_file);
	else
		strncpy(program, curr_file, NAME_MAX);

	/* print diagnostic message */
	if(!quiet)
	{
		fprintf(stderr, "WARNING: no program entry point found in file ");
		if(full_names)
			fprintf(stderr, "%s%s\n", curr_path, curr_file);
		else
			fprintf(stderr, "%s\n", curr_file);
		fprintf(stderr,"Using default program name %s at line 1\n", 
			program);
	}

	/* Set proper flags and create a new entry */
	action = ADDFUNC;
	Type   = PROGRAM;
	add_new_func(program, 1, curr_path, curr_file, 0);

	/* Reset flags */
	action = ADDCALL; 
	Type   = UNKNOWN;
}

/*****
* Add a new call definition
*****/
inline void
add_new_call(char *name, int callline, int num_args)
{
	static flowInfo *new_call;

	/* user forgot a program statement. Add one for him. */
	if(!have_first_routine)
		add_default_program();

	new_call = new_entry(name, callline, NULL, NULL, num_args);

	/* for data initialisation */
	if(global_flow.call_head == NULL)
	{
		global_flow.call_head = new_call;
		global_flow.current_call = new_call;
		global_flow.current->calls = new_call;
	}
	else
	{
		global_flow.current_call->next = new_call;
		global_flow.current_call = new_call;
	}
}

/*****
* Find in which file a function call is defined
* Very expensive (due to the strcasecmp call), but I don't see any other
* way...
*****/
static inline void
getParent(flowInfo *list, flowInfo *call)
{
	static flowInfo *tmp;
	for(tmp = list; tmp != NULL; tmp = tmp->next) 
	{ 
		if(!(strcasecmp(tmp->name, call->name)))
			break;
	}
	if(tmp == NULL) 
		return;
	call->parent = tmp;
	call->Type = KNOWN;
	/* This one has been used, mark it */
	tmp->called++;
} 

/*****
* Figure out where the calls made in each function are defined
*****/
flowInfo 
*sortAll(flowInfo *list)
{ 
	flowInfo *tmp, *tmp1; 
	for(tmp = list; tmp != NULL; tmp = tmp->next) 
	{ 
		for(tmp1 = tmp->calls; tmp1 != NULL; tmp1 = tmp1->next) 
		{ 
			getParent(list, tmp1);
			/* 
			* sanity check: if the parent of the current routine
			* or function is the routine/function itself, we are 
			* recursing on ourselves..
			*/
			if(tmp1->parent == tmp) 
			{
				tmp->recursive = 1; 
				fprintf(stderr,"ERROR: function %s recurses "
					"into itself\n", tmp->name); 
				fprintf(stderr,"line %i in file %s%s.\n", 
					tmp1->defline, tmp->path, tmp->file); 
				exit(8);
			} 
		} 
	} 
	return(list); 
}
 
/*****
* Show what functions are never invoked
*****/
void
print_unused_routines(FILE *file, flowInfo *list)
{
	flowInfo *tmp;

	for(tmp = list; tmp != NULL; tmp = tmp->next)
	{
		if(tmp->called == 0 && tmp->Type == SUBROUTINE)
		{
			fprintf(file, "SUBROUTINE %s is never invoked.\n\t(",
				tmp->name);
			if(full_names)
				fprintf(file, "%s%s, line %i)\n",tmp->path, 
					tmp->file, tmp->defline);
			else
				fprintf(file, "%s, line %i)\n", tmp->file,
					tmp->defline);
		}
	}
}

/*****
* Return 0 if the file given is to be ignored, something else otherwise
*****/
inline int
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
		return(0);
	}
	return(1);
}

/*****
* Reset necessary vars to start scanning the next file.
* Returns a FILE handle upon success, NULL on failure.
*****/
FILE
*reset_scanner(char *file_name)
{
	FILE *file;

	file = fopen(file_name, "r"); 
	if(!file) 
	{
		perror(file_name); 
		return NULL;
	}
	ParseFilename(file_name, curr_file, curr_path);
#ifdef __MSDOS__
	downcase(curr_file);
	downcase(curr_path);
#endif /* __MSDOS__ */
	if(!quiet) 
		printf("Scanning %s%s\n", curr_path, curr_file);
	numroutine = numfunc = numcall= 0;
	have_first_routine = False;
	action = DONOTHING; 
	return(file);
}

/*****
* Initialise all necessary global variables
*****/
void
initialise(char *argv_nul)
{
	/* Initialise global flow to NULL. */
	global_flow.head = NULL;
	global_flow.current = NULL;
	global_flow.call_head = NULL;
	global_flow.current_call = NULL;

	flow = NULL;
	full_names = quiet = 0;
	num_ignore = num_dirs_to_visit = num_extensions = num_files = 0;

	/* get program name */
	get_filename_part(argv_nul, progname);

#ifdef __MSDOS__
	dir_mem = ext_mem = file_mem = ign_mem = 0;
	need_mem_info_for_msdos = 0;
#endif
}

/*****
* Install various signal handlers
*****/
void
install_sig_handlers(void)
{
	/* install signal handlers */
	signal(SIGUSR1, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGSEGV, sig_handler);
#ifdef SIGQUIT
	signal(SIGQUIT, sig_handler);
#endif
}

/*****
* Show initial settings 
*****/
void
print_settings(void)
{
	int i;
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
			printf("%s\n", ext_table[i]); 
	}
	if(num_ignore != 0)
	{
		printf("List of files to ignore:\n");
		for(i = 0 ; i < num_ignore; i++)
			printf("%s\n", ignore_list[i]); 
	}

	printf("List of files to scan:\n");
	for(i = 0 ; i < num_files; i++)
		printf("%s\n", file_list[i]);
	fflush(stdout);
}

/*****
* Print scan statistics for this file
*****/
void
print_file_stats(FILE *file, char *file_name, int num_lines)
{ 
	fprintf(file, "----------------------\n");
	fprintf(file, "Scan statistics for   : %s\n", file_name);
	fprintf(file, "Total lines           : %i\n", num_lines);
	fprintf(file, "Function definitions  : %i\n", numfunc); 
	fprintf(file, "Subroutine definitions: %i\n", numroutine); 
	fprintf(file, "Total number of calls : %i\n", numcall); 
	fprintf(file, "----------------------\n");
} 

/*****
* Report msdos memory usage
*****/
void
print_dos_memory_usage(void)
{
#ifdef __MSDOS__
	int i;
	if(need_mem_info_for_msdos)
	{
		printf("\n------------------\n");
		printf("MSDOS Memory Usage\n\n");
		printf("Heap size is %u bytes\n", _heaplen);
		printf("Stack size is %u bytes\n", _stklen);
		printf("Memory used by flowInfo list: %lu bytes\n", list_mem);
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
			list_mem + dir_mem + ext_mem + ign_mem + file_mem);
	}
#endif
}
