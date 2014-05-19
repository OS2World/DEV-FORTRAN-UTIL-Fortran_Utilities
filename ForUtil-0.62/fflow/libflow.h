/*****
* libflow.h : libflow header file.
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	Mon Aug 19 15:49:49 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:48:35 $
*
* Author:		koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/fflow/RCS/libflow.h,v 1.2 1996/08/28 17:48:35 koen Exp koen $
*****/
/*****
* ChangeLog 
* $Log: libflow.h,v $
* Revision 1.2  1996/08/28 17:48:35  koen
* Added proto's for print_file_stats and reset_scanner. 
* Changed type from output_line in flowInfo struct from int to unsigned long.
*
* Revision 1.1  1996/08/27 19:14:53  koen
* Initial Revision
*
*****/ 

#ifndef _libflow_h_
#define _libflow_h_

#define DONOTHING	0 
#define ADDFUNC		1
#define ADDCALL		2
#define True		1
#define False		0 
#define PROGRAM		1
#define FUNCTION	2
#define SUBROUTINE	3 
#define CALL		4 
#define KNOWN		5
#define UNKNOWN		6 
#define UNDEF		"<unknown>"

#define MAXDEPTH	64
#define MAXDIRS		64
#define MAXEXTS		8
#define MAXIGNORE	64

/* data structure containing flow graph information */
typedef struct _flowInfo{
	char *path;			/* path of file */
	char *file;			/* name of file */
	char *name;			/* name of function */
	int  defline;			/* line name is def'd/call is made */
	int called; 			/* number of times called */
	unsigned long output_line;	/* line no in output file */
	int Type; 			/* type of record */
	int num_args;			/* no of args */
	int recursive;			/* true if this function is recursive */
	struct _flowInfo *calls;	/* list of calls made in function */
	struct _flowInfo *parent;	/* parent of call */
	struct _flowInfo *next;		/* ptr to next record */
}flowInfo; 

/* Bookkeeping data for flowgraph data */
typedef struct _global_flow_data{
	flowInfo *head;
	flowInfo *current;
	flowInfo *call_head;
	flowInfo *current_call;
}global_flow_data;

/**** Public Variables defined in libflow.c ****/
extern global_flow_data global_flow;
extern flowInfo *flow;
extern int action, Type, full_names, quiet, maxdepth, have_first_routine;
extern int num_ignore, num_dirs_to_visit, num_extensions, num_files;
extern int numroutine, numcall, numfunc;
extern char progname[32];
extern char *ignore_list[MAXIGNORE];
extern char *dirs_to_visit[MAXDIRS];
extern char *ext_table[MAXEXTS];
extern char **file_list;
extern char curr_path[PATH_MAX], curr_file[NAME_MAX];

#ifdef __MSDOS__
extern unsigned long dir_mem, ext_mem, file_mem, ign_mem;
extern int need_mem_info_for_msdos;
#endif

/**** Public Routines defined in libflow.c ****/
extern flowInfo *sortAll(flowInfo *list);
extern inline int ignore_this_file(char *file);
extern inline void add_new_call(char *name, int callline, int num_args);
extern inline void add_new_func(char *name, int callline, char *path, 
	char *file, int num_args);
extern FILE *reset_scanner(char *file_name);
extern void print_unused_routines(FILE *file, flowInfo *list);
extern void print_dos_memory_usage(void);
extern void initialise(char *argv_nul);
extern void install_sig_handlers(void);
extern void print_settings(void);
extern void print_file_stats(FILE *file, char *file_name, int num_lines);

/* Don't add anything after this endif! */
#endif /* _libflow_h_ */
