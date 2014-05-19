static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winfflow/RCS/settings.c,v 1.2 1996/08/27 19:32:08 koen Exp koen $";
/*****
* settings.c: Window procedures for fflow settings. MS-Windows version
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	08-08-1996
* Last modification: 	$Date: 1996/08/27 19:32:08 $
*
* Author:		Koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Log: settings.c,v $
* Revision 1.2  1996/08/27 19:32:08  koen
* Changes related to new setting: dynamic updating.
*
* Revision 1.1  1996/08/18 18:34:32  koen
* Initial Revision
*
*****/
#include "autoconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#ifdef WIN32
#include <bwcc.h>
#endif

#include "version.h"
#include "forutil.h"
#include "windows\lib\winutil.h"
#include "windows\src\winfflow.h"			/* contains the flowInfo struct */
#include "windows\src\resource.h"

/**** Global Variables ****/
char default_editor[MAXPATHLEN];	/* editor to use */
char inifile[MAXPATHLEN];		/* name of the winfflow inifile */
char initial_dir[MAXPATHLEN];		/* initial dir in filesel dialog */
short editor_order;			/* order in which to serve arguments */
BOOL keep_files;				/* save filelist on exit */
BOOL dynamic_update;		/* allow dynamic flowgraph generation */
short maxdepth;				/* max. recursion depth */
short cutoff_depth;			/* flowgraph cutoff level */

/**** Private Functions ****/
static void parse_editor_command(char *string);

/*****
* This routine scans the editor command entered or loaded
* and composes a string used composing the WinExec command line.
*
* The user can enter a editor command of the form:
* <editor> @FILE <options> @LINE
* where @FILE is the name of the file to edit, and
* @LINE is the linenumber where the editor should start.
*
* This routine sets a global variable editor_order which can have the
* following values (where %s indicates FILE and %i indicates LINE):
*
* Value		Command			has_file_cmd	has_line_cmd
* ------------------------------------------------------------------
* 1		<editor> %s		1 or 0		0
* 2		<editor> %s %i		1		2
* 3		<editor> %i %s		2 or 0		1
*****/
static void
parse_editor_command(char *string)
{
	char *outPtr = string;
	short has_file_cmd = 0;
	short has_line_cmd = 0;
	short i;
	char szBuf[MAXPATHLEN];

	/* clear buffer */
	memset(szBuf, '\0', MAXPATHLEN);

	for(i = 0, outPtr; *outPtr != '\0'; outPtr++, i++)
	{
		switch(*outPtr)
		{
			case '@':
				outPtr++;
				if(!strncmpi(outPtr, "file", 4))
				{
					/* outPtr also gets incremented above */
				   	outPtr += 3;
					if(!has_line_cmd)
						has_file_cmd = 1;
					else
						has_file_cmd = 2;
					strcat(szBuf, "%s");
					i += 1;	
				}
				else
				{
					if(!strncmpi(outPtr, "line", 4))
					{
						outPtr += 3;
						if(!has_file_cmd)
							has_line_cmd = 1;
						else
							has_line_cmd = 2;
						strcat(szBuf, "%i");
						i+=1;
					}
					else
					{
						ErrHandler(NULL, R_MESSAGE, 
							RB_OK1, "Unknown editor"
							" variable %s detected,"
							" ignoring\n", outPtr);
						for(; *outPtr != '\0' && 
							*outPtr != ' '; 
							outPtr++, i++);
					}
				}
				break;
			default:
				szBuf[i] = *outPtr;
		}
	}
	szBuf[i] = '\0';
	/* file = 0, line = 0, see table above */
	if(!has_file_cmd && !has_line_cmd)
	{
		has_file_cmd = 1;	/* file comes first */
		strcat(szBuf, " %s");
	}
	/* file = 0, line = 1, see table above */
	if(!has_file_cmd && has_line_cmd)
	{
		editor_order = 3;	/* file comes last */
		strcat(szBuf, " %s");
		has_file_cmd = 2;
	}

	if(has_file_cmd == 1 && has_line_cmd == 2)
		editor_order = 2;
	if(has_file_cmd == 2 && has_line_cmd == 1)
		editor_order = 3;
	if(has_file_cmd == 1 && has_line_cmd == 0)
		editor_order = 1;

	strncpy(default_editor, szBuf, strlen(szBuf));
	default_editor[strlen(szBuf)] = '\0';
}

/*****
* Load initial settings from winfflow.ini
*****/
void
load_settings(void)
{
	GetExePath(hInst, inifile);

	strcat(inifile, "winfflow.ini");

	if(!check_if_file_exists(inifile))
	{
		char string[5];
		/* version information */
		WritePrivateProfileString("fflow", "scanner", VERSION, inifile);
		WritePrivateProfileString("fflow", "winfflow", WINFFLOW_VERSION,
			inifile);
		/* default settings */
		/* editor command */
		WritePrivateProfileString("Settings", "Editor",
			"POPPAD.EXE @FILE -l @LINE", inifile);
		/* save filelist on exit */
		WritePrivateProfileString("Settings","KeepFiles", "1", inifile);
		/* collapse consecutive calls to the same routine */
		WritePrivateProfileString("Settings", "CollapseMultiple", "1",
			inifile);
		/* allow dynamic updating of flowgraphs */
		WritePrivateProfileString("Settings", "DynamicUpdate", "1",
			inifile);
		/* don't hide unresolved calls */
		WritePrivateProfileString("Settings", "HideUnresolved", "0",
			inifile);
		/* cutoff depth for flowgraphs */
		WritePrivateProfileString("Settings", "CutOff", "0", inifile);
		itoa(MAXDEPTH, string, 10);
		/* maximum allowable recursion while generating flowgraph */
		WritePrivateProfileString("Settings", "MaxDepth", string,
			inifile);
	}
	/* default editor */
	GetPrivateProfileString("Settings", "Editor", "NOTEPAD.EXE @FILE",
		default_editor, MAXPATHLEN, inifile);

	collapse_consecutive = GetPrivateProfileInt("Settings",
		"CollapseMultiple", 1, inifile);
	hide_unresolved = GetPrivateProfileInt("Settings", "HideUnresolved",
		0, inifile);
	keep_files = GetPrivateProfileInt("Settings", "KeepFiles", 1, inifile);
	cutoff_depth = GetPrivateProfileInt("Settings", "CutOff", 0, inifile);
	dynamic_update = GetPrivateProfileInt("Settings", "DynamicUpdate", 1,
		inifile);
	maxdepth = GetPrivateProfileInt("Settings", "MaxDepth", MAXDEPTH,
		inifile);

	parse_editor_command(default_editor);

	/* Get initial directory */
	GetPrivateProfileString("Files", "CurrentDirectory", NULL, initial_dir,
		MAXPATHLEN, inifile);

	/* Set to current directory if not present */
	if(strlen(initial_dir) == 0)
		getcwd(initial_dir, MAXPATHLEN);

	/* cut off trailing slash */
	if(initial_dir[strlen(initial_dir) - 1] == '\\')
		initial_dir[strlen(initial_dir)-1] = '\0';
}

/*****
* Load files from the inifile and put them in the main window
* file listbox.
*****/
int
load_ini_file_data(HWND hDlg)
{
	short i, j;
	char szBuf[MAXPATHLEN];

	/* get no of items in the list */
	j = GetPrivateProfileInt("Files", "Total", 0, inifile);

	/* return if no files in list */
	if(j == 0)
		return(0);

	/* get the items in the list */
	for(i = 0; i < j; i++)
	{
		memset(szBuf, '\0', MAXPATHLEN);
		/* compose entry string */
		sprintf(szBuf, "%i", i);
		/* get string from file */
		GetPrivateProfileString("Files", szBuf, "<unknown>", szBuf,
			MAXPATHLEN, inifile);
		SendDlgItemMessage(hDlg, IDC_MAIN_LISTBOX, LB_ADDSTRING, 0,
			(LPARAM) szBuf);
	}
	return(1);
}

/*****
* Save all files in the main window file listbox to
* winfflow inifile.
*****/
void
save_ini_file_data(HWND hDlg)
{
	short i,j;
	char szBuf[MAXPATHLEN], linebuf[4];
	HWND hwndList = GetDlgItem(hDlg, IDC_MAIN_LISTBOX);

	/* don't save unless we're asked to do so */
	if(!keep_files)
		return;
	/* Clear any existing list of files */
	WritePrivateProfileString("Files", NULL, NULL, inifile);

	/* get no of items in the list */
	j = (int)SendMessage(hwndList, LB_GETCOUNT, 0, 0L);

	/* return if no files in list */
	if(j == 0)
		return;

	/* Save initial directory */
	WritePrivateProfileString("Files", "CurrentDirectory", initial_dir,
		inifile);

	/* get the items in the list */
	for(i = 0; i < j; i++)
	{
		SendMessage(hwndList, LB_GETTEXT, i, (LONG)(LPSTR)szBuf);
		sprintf(linebuf, "%i", i);
		WritePrivateProfileString("Files", linebuf, szBuf, inifile);
	}
	sprintf(linebuf, "%i", i);
	/* Total no of files */
	WritePrivateProfileString("Files", "Total", linebuf, inifile);
}

/*****
* Window procedure for the output selector dialog box
*****/
#pragma argsused
BOOL FAR PASCAL _export
SettingsProc(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
	char szBuf[MAXPATHLEN];

	switch(message)
	{
		case WM_INITDIALOG:
			GetPrivateProfileString("Settings", "Editor", 
				"POPPAD.EXE @FILE -l @LINE",
				szBuf, MAXPATHLEN, inifile);
			SetDlgItemText(hDlg, IDC_SETTINGS_EDITOR, szBuf);
			SetDlgItemInt(hDlg, IDC_SETTINGS_MAX_DEPTH, maxdepth, 
				FALSE);
			SetDlgItemInt(hDlg, IDC_SETTINGS_CUTOFF_DEPTH, 
				cutoff_depth, FALSE);
			CheckDlgButton(hDlg, IDC_SETTINGS_SAVE_FILELIST, 
				keep_files);
			CheckDlgButton(hDlg, IDC_SETTINGS_COLLAPSE, 
				collapse_consecutive);
			CheckDlgButton(hDlg, IDC_SETTINGS_HIDE, 
				hide_unresolved);
			CheckDlgButton(hDlg, IDC_SETTINGS_DYNAMIC,
            	dynamic_update);
			return TRUE;
		case WM_COMMAND:
		switch(GET_COMMAND_ID)
		{
			/* move to next control. */
			case IDC_DEFAULT_BUTTON:
				PostMessage(hDlg, WM_NEXTDLGCTL, 0, FALSE);
				return(TRUE);
			case IDOK:
			{
				char string[5];
				BOOL no_err = TRUE;/* Get..Int error flag */

				/* pick up editor command */
				GetDlgItemText(hDlg, IDC_SETTINGS_EDITOR,
					szBuf, MAXPATHLEN);
				if(strlen(szBuf))
				{
					memset(default_editor, '\0',
						sizeof(default_editor));
					strcpy(default_editor, szBuf);
					WritePrivateProfileString("Settings",
						"Editor", default_editor,
						inifile);
					parse_editor_command(default_editor);
				}
				/* get maximum recursion depth */
				maxdepth = GetDlgItemInt(hDlg,
					IDC_SETTINGS_MAX_DEPTH, &no_err, FALSE);
				if(!no_err)
					maxdepth = MAXDEPTH;
				/* get cutoff depth */
				cutoff_depth = GetDlgItemInt(hDlg,
					IDC_SETTINGS_CUTOFF_DEPTH, &no_err,
					FALSE);
				if(!no_err)
					cutoff_depth = 0;

				keep_files = IsDlgButtonChecked(hDlg,
					IDC_SETTINGS_SAVE_FILELIST);

				collapse_consecutive = IsDlgButtonChecked(hDlg,
					IDC_SETTINGS_COLLAPSE);
				hide_unresolved = IsDlgButtonChecked(hDlg,
					IDC_SETTINGS_HIDE);
				dynamic_update = IsDlgButtonChecked(hDlg,
					IDC_SETTINGS_DYNAMIC);

				/* save filelist on exit */
				WritePrivateProfileString("Settings",
					"KeepFiles",
					(keep_files ? "1" : "0"), inifile);
				/* collapse consecutive calls to same routine */
				WritePrivateProfileString("Settings",
					"CollapseMultiple",
					(collapse_consecutive ? "1" : "0"),
					inifile);
				/* hide unresolved calls ? */
				WritePrivateProfileString("Settings",
					"HideUnresolved",
					(hide_unresolved ? "1" : "0" ),
					inifile);
				/* allow dynamic flowgraph updating ? */
				WritePrivateProfileString("Settings",
					"DynamicUpdate",
					(dynamic_update ? "1" : "0" ),
					inifile);
				/* cutoff depth for flowgraphs */
				itoa(cutoff_depth, string, 10);
				WritePrivateProfileString("Settings", "CutOff",
					string, inifile);
				itoa(maxdepth, string, 10);
				/* maximum allowable recursion depth */
				WritePrivateProfileString("Settings", 
					"MaxDepth", string, inifile);
				EndDialog(hDlg,0);
				return(TRUE);
			}
			case IDCANCEL:
				EndDialog(hDlg,0);
				return(TRUE);
			case IDHELP:
				WinHelp(hDlg, "winfflow.hlp", HELP_CONTEXT,
					WINFFLOW_SETTINGS_DIALOG);
                return(TRUE);
		}
		break;
	}
	return(FALSE);
}
