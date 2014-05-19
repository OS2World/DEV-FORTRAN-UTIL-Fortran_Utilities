%{
#ifndef lint
static char rcsId[]="$Source: /usr/local/rcs/ForUtil/ftags/RCS/ftags.l,v $";
#endif
/*****
* ftags.c : tags file generator for Fortran source files
*
* This file Version	$Revision: 1.6 $
*
* Creation date:	Mon Feb  5 02:26:29 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:50:51 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: ftags.l,v $
* Revision 1.6  1996/08/28 17:50:51  koen
* Added print_version_id
*
* Revision 1.5  1996/08/07 21:16:02  koen
* Added the YYOUT_DEVICE for yyout
*
* Revision 1.4  1996/08/02 14:51:24  koen
* Moved all system dependencies to lib/sysdeps.h
*
* Revision 1.3  1996/07/30 02:02:03  koen
* Added version information and the autoconf.h include file.
*
* Revision 1.2  1996/05/06 00:34:57  koen
* Adapted for MSDOS
*
* Revision 1.1  1996/02/05 01:28:21  koen
* Initial revision
*
*****/ 
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h> 
#include <ctype.h>
#include <string.h>

#include "memmacros.h"
#include "forutil.h"
#include "version.h"

#define PROGRAM		1
#define FUNCTION	2
#define SUBROUTINE	3 

typedef struct _flowInfo{
	char file[128];
	char name[32];
	int Type; 
	struct _flowInfo *left;
	struct _flowInfo *right; 
}flowInfo; 

static int Type; 
flowInfo *flow; 
char *curFile; 
int verbose;

static flowInfo *addEntry(flowInfo *list, char *name); 
void printFlow(flowInfo *list);
%}

function " "{6}function[ ]
routine  " "{6}subroutine[ ]
program  " "{6}program[ ]
name [a-zA-Z0-9_]  
nl \n

%option caseless

%x FNAME

%%
^[*cC].* ; /* eat up comments entirely */  
{program}  {
				Type   = PROGRAM; 
				BEGIN FNAME;
				}  
{function} {
				Type   = FUNCTION; 
				BEGIN FNAME;
				}  
{routine}  {
				Type   = SUBROUTINE; 
				BEGIN FNAME;
				} 
<FNAME>{name}+ {
				char *chptr; 
				char text[512]; 
				strcpy(text, yytext); 
				for ( chptr = text; *chptr != '\0'; *chptr++) 
					*chptr = tolower(*chptr); 
				flow = addEntry(flow, text);  /* addFlow(flow);  */ 
				for ( chptr = text; *chptr != '\0'; *chptr++) 
					*chptr = toupper(*chptr); 
				flow = addEntry(flow, text);  /* addFlow(flow);  */ 
				BEGIN 0;
				}  
{nl}			{
				BEGIN 0;
				} /* increment line number */  
.     		; /* do nothing  for all other remaining chars */ 
%%

void PrintTree(FILE *file, flowInfo *Tree)  
{ 
	if(Tree != NULL) 
	{ 
		PrintTree(file, Tree->left); 
		fprintf(file,"%s\t%s\t", Tree->name, Tree->file); 
		switch(Tree->Type)
		{
			case PROGRAM:
				fprintf(file,"/PROGRAM");
				break;
			case FUNCTION:
				fprintf(file,"/FUNCTION");
				break; 
			case SUBROUTINE:
				fprintf(file,"/SUBROUTINE");
				break;
			default:
				break;
			
		} 
		fprintf(file, " %s/\n", Tree->name); 
		PrintTree(file, Tree->right); 
	} 
} 

void printFlow(flowInfo *list) 
{ 
	FILE *file; 
	file = fopen("tags", "w"); 
	if(!file)
	{
		perror("tags"); 
		exit(6);
	} 
	PrintTree(file, list); 
	fclose(file); 
}

static flowInfo *addEntry(flowInfo *list, char *name)
{
	int cmp; 
	if(list == NULL) 
	{ 
		checked_malloc(list, 1, flowInfo);
		strcpy(list->name,name); 
		strcpy(list->file, curFile); 
		list->Type = Type; 
		list->left = list->right = NULL; 
	} 
	else
	{
		cmp = strcmp(list->name, name); 
		if(cmp > 0 ) 
			list->left = addEntry(list->left, name); 
		else
			if(cmp < 0 ) 
				list->right = addEntry(list->right, name); 
		else
		{
			fprintf(stderr,"Warning: %s already defined in %s\n", name, 
				list->file); 
			list->left = addEntry(list->left, name); 
			return(list); 
		} 
	} 
	return(list); 
} 

/****
* main for ftags
****/
int main(int argc, char *argv[])
{ 
	FILE *file;
	int i; 
	static char progname[32];

	get_filename_part(argv[0], progname);

	verbose = 0;

	if (argc == 1)
	{
		printf("%s: no files given\n", progname);
		printf("\nUsage: %s fortran-files\n", progname);
		printf("\n Generates a case-insensitive tags file of"
			" given fortran files for use with vi\n\n");
		print_version_id(progname, VERSION, "$Revision: 1.6 $");
		printf("(C)Copyright 1995-1996 by Ripley Software Development\n");
		exit(2);
	}

	/* purge any non-matching output from yylex to /dev/null */
	yyout = fopen(YYOUT_DEVICE, "w+");

	for(i = 1; i <argc; i++)
	{
		file = fopen(argv[i], "r"); 
		if(!file) 
			perror(argv[i]); 
		else
		{ 
			yyin = file;  
			curFile = argv[i]; 
			printf("%s\n", argv[i]);
			fflush(stdout); 
			yylex();
			YY_FLUSH_BUFFER;
			fclose(yyin);
		}
	}
	fclose(yyout);
	printFlow(flow); 
	return(0);
}
