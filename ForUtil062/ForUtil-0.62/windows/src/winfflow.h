/*****
* winfflow.h : flowgraph struct definitions, MS Windows version
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	Thu Aug  8 04:03:18 GMT+0100 1996
* Last modification: 	$Date: 1996/08/27 19:29:50 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/winfflow/RCS/winfflow.h,v 1.2 1996/08/27 19:29:50 koen Exp koen $
*****/
/*****
* ChangeLog 
* $Log: winfflow.h,v $
* Revision 1.2  1996/08/27 19:29:50  koen
* lots of changes: added proto's from all routines.
*
* Revision 1.1  1996/08/18 18:34:52  koen
* Initial Revision
*
*****/

#ifndef _winfflow_h_
#define _winfflow_h_

#define WINFFLOW_VERSION	"0.12"

#define PROGRAM		1
#define FUNCTION	2
#define SUBROUTINE	3
#define CALL		4
#define KNOWN		5
#define UNKNOWN		6
#define UNDEF		"<unknown>"

#define DEFAULT_FILE	"All Files"

#define MAXDEPTH	64
#define MAXDIRS		64
#define MAXEXTS		8
#define MAXIGNORE	64

/* 
* Data structure containing flow graph information. 
* Note: routine is used as an indicator for both subroutines and functions 
*/
typedef struct _flowInfo{
	HGLOBAL hglb;
	LPSTR path;			/* path of file */
	LPSTR file;			/* name of file */
	LPSTR name;			/* name of routine */
	int  defline;			/* line name defined / call is made */
	short called; 			/* number of times called */
	short Type; 			/* type of record */
	short recursive;		/* true if routine is recursive */
	struct _flowInfo *calls;	/* list of calls made in routine */
	struct _flowInfo *parent;	/* parent of call */
	struct _flowInfo *next;		/* ptr to next routine */
}flowInfo;

/* global variables */
extern HINSTANCE hInst;			/* application instance handle */
extern HWND hWndMain;			/* main dialog window handle. */
extern HWND progress_dialog;		/* scan progress window handle */
extern HWND output_dialog;		/* flowgraph window handle */
extern flowInfo *flow;			/* data resulting from file scan */
extern int num_files;			/* total no of files to scan */
extern char **file_list;		/* files to scan */

/* overall settings */
extern char default_editor[MAXPATHLEN];	/* editor to use */
extern short editor_order;		/* type of args to default_editor */
extern char inifile[MAXPATHLEN];	/* name of the winfflow inifile */
extern char initial_dir[MAXPATHLEN];	/* initial dir in filesel dialog */
extern short maxdepth;			/* max. recursion depth */
extern short cutoff_depth;		/* flowgraph cutoff level */
extern BOOL keep_files;			/* save filelist on exit */
extern BOOL collapse_consecutive;	/* collapse calls to the same routine */
extern BOOL hide_unresolved;		/* don't show unresolved calls */
extern BOOL hide_resolved;		/* don't show resolved calls */
extern BOOL dynamic_update;		/* allow dynamic flowgraph generation */

/**** Function prototypes ****/

/* From winfflow.l */
extern void generate_flowgraph(void);
extern void free_flowgraph(flowInfo *list);

/* From output.c */
extern BOOL CALLBACK _export OutputProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam);
extern void display_output_box(void);
extern void file_flow_graph(flowInfo *list, char *file);
extern void func_flow_graph(flowInfo *list, char *func);
extern void show_refs(flowInfo *list, flowInfo *func);

/* from selector.c */
extern BOOL FAR PASCAL _export SelectorProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam);

/* from settings.c */
extern BOOL FAR PASCAL _export SettingsProc(HWND hDlg, UINT message,
	UINT wParam, LONG lParam);
extern void load_settings(void);
extern void save_ini_file_data(HWND hDlg);
extern int load_ini_file_data(HWND hDlg);

/* Edit file_name using the default editor, possibly starting at line_num */
#define edit_file(file_name,line_num) \
{ \
	char cmdline[MAXPATHLEN]; \
	switch(editor_order) { \
	case 1:	sprintf(cmdline, default_editor, file_name); break; \
	case 2: sprintf(cmdline, default_editor, file_name, line_num); break; \
	case 3: sprintf(cmdline, default_editor, line_num, file_name); break; \
} if((WinExec(cmdline, SW_SHOWNORMAL)) < 32) \
	ErrHandler(NULL, R_MESSAGE, RB_OK1, "Could not start the editor.\n" \
		"Are the path and name of the editor correct?"); }

/* Don't add anything after this endif! */
#endif /* _winfflow_h_ */
