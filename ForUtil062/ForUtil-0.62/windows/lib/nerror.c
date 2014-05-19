static char rcsId[]="$Header: /usr/local/rcs/ForUtil/winutil/RCS/nerror.c,v 1.12 1996/08/27 19:23:40 koen Exp koen $";
/******
* nerror.c : error handler for ripley.
*
* Common Version	1.00
* This file Version:	$Revision: 1.12 $
*
* Creation date:	04-03-1995
* Last modification $Date: 1996/08/27 19:23:40 $
*
* Author:		Koen
* (C)1995 Ripley Software Development
* All Rights Reserved
******/
/*****
* ChangeLog
* $Log: nerror.c,v $
* Revision 1.12  1996/08/27 19:23:40  koen
* Changes for Winfflow
*
* Revision 1.11  1995/11/29  06:32:55  koen
* Made the default titles english instead of dutch.
*
* Revision 1.10  1995/10/17  22:58:00  koen
* Adjusted file header
*
* Revision 1.9  1995/08/18  19:55:32  koen
* Has been revised to use common.h and the titles of the message boxes are
* now also language dependent.
*
*****/ 

/******
* ErrHandler: printf-like error handler.
* type indicates the type of error occured
* buttons is the number of buttons
* msg is the message string
* ... are args to msg.
******/

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

/*****
* Local includes 
*****/ 
#include "windows\lib\winutil.h" 

int ErrHandler(HWND hwnd, int type, int buttons, char *msg, ...)
{
	va_list args;
	int answer;
	char Message[256];
	char *Title; 
	static int error_types[] = {
		MB_ICONEXCLAMATION,
		MB_ICONSTOP,
		MB_ICONQUESTION,
		MB_ICONINFORMATION};
	static char *dialog_name_eng[]={
		"Warning",
		"Error",
		"Question",
		"Message"};
	static int error_buttons[]={
		MB_OK,
		MB_OKCANCEL,
		MB_YESNO,
		MB_YESNOCANCEL,
		MB_RETRYCANCEL,
		MB_ABORTRETRYIGNORE};

	if(type < R_WARNING && type > R_MESSAGE)
		return(0);
	if (buttons < RB_OK1 && buttons > RB_RETRY3)
		return(0);

	va_start(args, msg);	/* init args */
	vsprintf(Message, msg, args);
	va_end(args);
	Title = dialog_name_eng[type];
	answer = MessageBox(hwnd, Message, Title, error_types[type]
		| error_buttons[buttons] | MB_TASKMODAL);
	return(answer);
}
