/*****
* resource.h: resource identifiers for winfflow.rc
*
* This file Version	$Revision: 1.2 $
*
* Creation date:	08-08-1996
* Last modification: 	$Date: 1996/08/27 19:29:00 $
*
* Author:		Koen
* (C)1995-1996 Ripley Software Development
* All Rights Reserved
*****/
/*****
* $Header: /usr/local/rcs/ForUtil/winfflow/RCS/resource.h,v 1.2 1996/08/27 19:29:00 koen Exp koen $
*****/

#ifndef _resource_h_
#define _resource_h_

/* FileDialog filterstring for fortran files (*.f;*.f77) */
#define IDS_FILTERSTRING		1
#define IDS_FILTERSTRING1		2

/* main window resource defines. occupies 100-199 */
#define WINFFLOW_MAIN_DIALOG		100
#define IDC_MAIN_REMOVE_ALL		101
#define IDC_MAIN_SETTINGS		102
#define IDC_MAIN_LISTBOX		103
#define IDC_MAIN_REMOVE			104
#define IDC_MAIN_EDIT			105
#define IDC_MAIN_ADD			106
#define IDC_MAIN_ABOUT			107
/****
* Also used:
* IDOK:	generate flowgraph
* IDCANCEL: exit
* IDHELP: show help
****/

/* Progress dialog resource defines. Occupies 200-299 */
#define WINFFLOW_GENERATING_DIALOG	200
#define IDC_GENERATING2			201
#define IDC_GENERATING1			202

/* Output Selector resource defines. Occupies 300-399 */
#define WINFFLOW_OUTPUTSELECTOR_DIALOG	300
#define IDC_SELECTOR_VIEW		301
#define IDC_SELECTOR_LISTBOX		302
#define IDC_SELECTOR_CUTOFF_DEPTH	303
#define IDC_SELECTOR_CUTOFF		304
#define IDC_SELECTOR_HIDE		305
#define IDC_SELECTOR_COLLAPSE		306
#define IDC_SELECTOR_SUBROUTINE		307
#define IDC_SELECTOR_FUNCTION		308
#define IDC_SELECTOR_COMBOBOX		309
#define IDC_DEFAULT_BUTTON		310
#define IDC_SELECTOR_HIDE_KNOWN		311
#define IDC_SELECTOR_SHOW_UNUSED	312
#define IDC_SELECTOR_BROWSE		313
/****
* Also used:
* IDOK: close dialog
****/

/* Flowgraph dialog resource defines. Occupies 400-499 */
#define WINFFLOW_OUTPUT_DIALOG		400
#define IDC_OUTPUT_RESOLVED		401
#define IDC_OUTPUT_UNRESOLVED		402
#define IDC_OUTPUT_TOTAL_CALLS		403
#define IDC_OUTPUT_ROUTINE_NAME		404
#define IDC_OUTPUT_ROUTINE_TYPE		405
#define IDC_OUTPUT_FILENAME		406
#define IDC_OUTPUT_EDIT			407
#define IDC_OUTPUT_SAVE			408
#define IDC_OUTPUT_LISTBOX		409
#define IDC_OUTPUT_GROUPBOX		410
/****
* Also used:
* IDOK: close dialog
****/

/* Flowgraph Settings dialog defines. Occupies 500-599 */
#define WINFFLOW_SETTINGS_DIALOG	500
#define IDC_SETTINGS_MAX_DEPTH		501
#define IDC_SETTINGS_SAVE_FILELIST	502
#define IDC_SETTINGS_EDITOR		503
#define IDC_SETTINGS_CUTOFF_DEPTH	504
#define IDC_SETTINGS_HIDE		505
#define IDC_SETTINGS_COLLAPSE		506
#define IDC_SETTINGS_DYNAMIC		507
/****
* Also used:
* IDOK: accept settings
* IDCANCEL: reject settings
* IDHELP: show help
****/

/* Winfflow About dialog defines. Occupies 600-699 */
#define WINFFLOW_ABOUT_DIALOG		600
/****
* Also used:
* IDOK: close dialog
****/

/* Icon defines. Occupies 1000-1099 */
#define WINFFLOW_ICON_1			1000

/* Cursor Defines. Occupies 1100-1199 */
#define CURSOR_WAIT_1			1100
#define CURSOR_WAIT_2			1101
#define CURSOR_WAIT_3			1102

/* Don't add anything after this endif! */
#endif /* _resource_h_ */
