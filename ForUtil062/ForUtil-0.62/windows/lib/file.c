static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winutil/RCS/file.c,v 1.12 1996/08/27 19:23:33 koen Exp koen $";
/******
* file.c: file open/read/write routines for Hicks.
*
* Hicks Version 	1.00
* This file Version	$Revision: 1.12 $
*
* Creation date:	06-06-1995
* Last modification: 	$Date: 1996/08/27 19:23:33 $
*
* Author:		Koen
* (C)1995 Ripley Software Development
* All Rights Reserved
******/
/*****
* ChangeLog
* $Log: file.c,v $
* Revision 1.12  1996/08/27 19:23:33  koen
* Update for Winfflow
*
* Revision 1.11  1996/05/23  03:20:30  koen
* cleaned up include files.
*
* Revision 1.10  1996/05/10  08:10:08  koen
* Added support for user database file open/save dialogs.
* Added error messages that use hMessage when bad things happen.
*
* Revision 1.9  1995/11/29  06:31:52  koen
* removed readDbValues function. It's now in db_utils.c
*
* Revision 1.8  1995/10/20  06:35:07  koen
* moved from admin source tree to common.lib. A little change in open_file
* has been made which allows this function to be called without a username
* supplied.
*
* Revision 1.8  1995/10/17  23:06:18  koen
* changed the user-database loading routines so they now use the routines
* from db_utils.dll
*
*****/

#include <errno.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include <dir.h>
#include <errno.h>

#include "windows\lib\winutil.h"

/*****
* Open a binary file. Return a file handle on succesfull open,
* display an error message and return NULL on failure.
*****/
FILE *open_file(char *filename)
{
	FILE *file;

	file = fopen(filename, "rb");
	if (!file)
	{
		ErrHandler(NULL, R_ERROR, RB_OK1, "Failed to open %s: %s", filename,
        	_sys_errlist[errno]);
		return (NULL);
	}
	return(file);
}

/*****
* Create a file in binary mode. Return file handle on success. Display a
* message and return NULL in case of failure.
*****/
FILE *create_file(char *filename)
{
	FILE *file;
	file = fopen(filename, "wb");
	if (!file)
	{
    	ErrHandler(NULL, R_ERROR, RB_OK1, "Failed to create %s: %s", filename,
			_sys_errlist[errno]);
		return (NULL);
	}
	return(file);
}

/*****
* Display a file save as fileselection dialog.
* This routine remembers the selected directory
*
* In:
*	hDlg: window of caller
*	initial_name: name to initialise filename text
*	filter: filter string to use. If not given, the default All Files is
*		used.
*
* Returns: the name of the selected file on success, NULL on failure.
*****/
char *display_save_dialog(HWND hDlg, char *initial_name, char *filter)
{
	OPENFILENAME ofn;
	static char szDirName[MAXPATH];
	static char szFile[MAXPATH];
	static char szFilter[MAXPATH]; /* Filter used by fileselection dialog*/
	static char  chReplace;    /* string separator for szFilter */
	short i = 0;

	/*
	* If szDirName has not been initialised before, set it to the current
	* directory.
	*/
	if(szDirName == NULL)
	{
		getcwd(szDirName, sizeof(szDirName));
		/* cut off a trailing slash */
		if(szDirName[strlen(szDirName) - 1] == '\\')
			szDirName[strlen(szDirName)-1] = '\0';
	}

	memset(szFilter, '\0', sizeof(szFilter));

	if(filter != NULL)
		strcpy(szFilter, filter);
	else
		strcpy(szFilter, "All Files (*.*)|*.*|");

	chReplace = szFilter[strlen(szFilter) - 1]; /* retrieve wildcard */
	for (i = 0; szFilter[i] != '\0'; i++)
	{
		if (szFilter[i] == chReplace)
		szFilter[i] = '\0';
	}
	/* initialise szFile */
	memset(szFile, '\0', MAXPATH);
	if(initial_name != NULL)
		strcpy(szFile, initial_name);

/* Set all structure members to zero. */
	memset(&ofn, 0, sizeof(OPENFILENAME));
/* Fill appropriate fields */
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = MAXPATH;
	ofn.lpstrDefExt = ".out";
	ofn.lpstrInitialDir = szDirName;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	if (GetSaveFileName(&ofn))
	{
		/* save szDirName for next run */
		memset(szDirName, '\0', MAXPATH);
		strncpy(szDirName, ofn.lpstrFile, ofn.nFileOffset);
		/* cut off a trailing slash */
		if(szDirName[strlen(szDirName) - 1] == '\\')
			szDirName[strlen(szDirName) - 1] = '\0';
		/* return filename */
		return(ofn.lpstrFile);
	}
	else
		return NULL;
}

/*****
* Display a file open fileselection dialog.
* This routine remembers the selected directory
*
* In:
*	hDlg: window of caller
*	initial_name: name to initialise filename text
*	filter: filter string to use. If not given, the default All Files is
*		used.
*
* Returns: the name of the selected file on success, NULL on failure.
*****/
char *display_open_dialog(HWND hDlg, char *initial_name, char *filter)
{
	OPENFILENAME ofn;
	static char szDirName[MAXPATH];
	static char szFile[MAXPATH];
	static char szFilter[MAXPATH]; /* Filter used by fileselection dialog*/
	static char  chReplace;    /* string separator for szFilter */
	short i = 0;

	/*
	* If szDirName has not been initialised before, set it to the current
	* directory.
	*/
	if(szDirName == NULL)
	{
		getcwd(szDirName, sizeof(szDirName));
		/* cut off a trailing slash */
		if(szDirName[strlen(szDirName) - 1] == '\\')
			szDirName[strlen(szDirName)-1] = '\0';
	}
	memset(szFilter, '\0', sizeof(szFilter));

	if(filter != NULL)
		strcpy(szFilter, filter);
	else
		strcpy(szFilter, "All Files (*.*)|*.*|");

	chReplace = szFilter[strlen(szFilter)- 1]; /* retrieve wildcard */
	for (i = 0; szFilter[i] != '\0'; i++)
	{
		if (szFilter[i] == chReplace)
		szFilter[i] = '\0';
	}
	/* initialise szFile */
	memset(szFile, '\0', MAXPATH);
	if(initial_name != NULL)
		strncpy(szFile, initial_name, 8);

/* Set all structure members to zero. */
	memset(&ofn, 0, sizeof(OPENFILENAME));
/* Fill appropriate fields */
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile= szFile;
	ofn.nMaxFile = MAXPATH;
	ofn.lpstrDefExt = ".out";
	ofn.lpstrInitialDir = szDirName;

	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileName(&ofn))
	{
		/* save szDirName for next run */
		memset(szDirName, '\0', MAXPATH);
		strncpy(szDirName, ofn.lpstrFile, ofn.nFileOffset);
		/* cut off a trailing slash */
		if(szDirName[strlen(szDirName) - 1] == '\\')
			szDirName[strlen(szDirName) - 1] = '\0';
		/* return filename */
		return(ofn.lpstrFile);
	}
	else
		return NULL;
}
