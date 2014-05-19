#ifndef lint
static char rcsId[]="$Header: /usr/local/rcs/ForUtil/Commons/RCS/lib_commons.c,v 1.9 1996/08/28 17:43:55 koen Exp koen $";
#endif
/*****
* lib_commons.c : functions used by all commons programs
*
* This file Version	$Revision: 1.9 $
*
* Creation date:	Fri Mar 15 04:51:43 GMT+0100 1996
* Last modification: 	$Date: 1996/08/28 17:43:55 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: lib_commons.c,v $
* Revision 1.9  1996/08/28 17:43:55  koen
* Changed uname define for __MSDOS__ only to #ifdef HAVE_UNAME;#else;#endif
*
* Revision 1.8  1996/08/27 19:11:46  koen
* Added support for DOUBLE, COMPLEX. linecount added to check if less than 
* 19 cont. lines
*
* Revision 1.7  1996/08/07 21:12:01  koen
* Fixed a bug in remove_tokens for the PARAMETER case.
*
* Revision 1.6  1996/08/02 14:49:21  koen
* moved all system dependencies to lib/sysdeps.h
*
* Revision 1.5  1996/07/30 01:56:01  koen
* re-ordered include files.
*
* Revision 1.4  1996/07/16 00:36:20  koen
* added warning and error values upon return from scan_block. Finally got
* compress_common_block correct with respect to continuation characters.
*
* Revision 1.3  1996/05/06 00:32:19  koen
* Adapted for MSDOS.
*
* Revision 1.2  1996/04/25 02:27:12  koen
* Cleanup and comments added.
*
* Revision 1.1  1996/03/15 04:16:32  koen
* Initial Revision
*
*****/ 
/* needed to prevent multiple variable decls under MSDOS in lib/sysdeps.h */
#define LIBRARY_OBJECT

/* include this before anything else */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <ctype.h>
#include <time.h>

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#else
struct utsname{
		char *nodename;
		char *sysname;
		char *machine;
};
#define uname(INFO) { (INFO)->nodename="<unknown nodename>"; \
	(INFO)->sysname="<unknown sysname>"; \
	(INFO)->machine="<unknown machine>";}
#endif /* !HAVE_UNAME */

#include "lib_commons.h"
#include "forutil.h"
#include "memmacros.h"

#ifdef TEST
#define VERBOSE

static char *text={"\n"
"      COMMON /ISUBW/ IB0020 , \n"                                           
"*     This is a comment containing a COMMON   IDTEST                     \n"
"     +               NMSBW, NMSBU , LNDFDS ,                             \n"
"     +               lnisbw(lmsbw), lnxsbw(lmsbw), lncsbw(lmsBW),        \n"
"     X               KBISBW(LMSBW), KBXSBW(LMSBW), KBCSBW(LMSBW),        \n"
"     x               LISDAS( LMSBW ) , IACTDS( LMSBW ) ,                 \n"
"     x               Intdas( lmsbu ) , nhgint ,                          \n"
"     +               IE0020                                              \n"
"      COMMON /XSUBW/ XB0020,XE0020                                       \n"
"  *    Intendent COMMON IDTES2 which should not show up in the output    \n"
"      CHARACTER*60   NAMSBW                                              \n"
"      CHARACTER*31   CSUBDF                                              \n"
"      CHARACTER*8    CB0020 , CE0020                                     \n"
"      CHARACTER*(LEN) CB0020 , CE0020                                    \n"
"      PARAMETER (MAXVEC=128)                                             \n"
"      PARAMETER ( LEN=128 )                                              \n"
"      PARAMETER ( LEN12 = 12 )                                           \n"
"      COMMON /CSUBW/ CB0020 , NAMSBW(LMSBW)  , CSUBDF(LMSBW) ,           \n"
"     #               CE0020\n"}; 

static char *wrong_text={"\n"
"*      This is a common block with some errors in it \n"
"  *    This is a comment starting with an ident. \n"
"*      This piece contains an unbalaced brace.   \n"
"      COMMON         IDWRONG(UNCLOSED            \n"
"\n"};

static int num_errors;
#endif /* TEST */

/* structure for holding token definitions */
typedef struct {
	int id;		/* id of token */
	char *token;	/* name of token */
	int len;	/* length of token */
}token;

#define COMMON		0
#define CHARACTER	1
#define LOGICAL		2
#define PARAMETER	3
#define REAL		4
#define INTEGER		5
#define DOUBLE		6
#define COMPLEX		7
#define NUM_TOKEN	8		/* number of tokens */
#define NUM_DELIMITER	4		/* number of possible delimiters */

/* token lookup table */
token token_words[NUM_TOKEN] = {
	{COMMON, "COMMON", 6},
	{CHARACTER, "CHARACTER*", 10},
	{LOGICAL, "LOGICAL", 7},
	{PARAMETER, "PARAMETER", 9},
	{REAL, "REAL", 4},
	{INTEGER, "INTEGER", 7},
	{DOUBLE, "DOUBLE PRECISION", 16},
	{COMPLEX, "COMPLEX", 7}};

static char delimit_table[NUM_DELIMITER] = {'/','/', '(', ')'};
static int num_warnings;

/****
* Find out what our database is called
****/
char
*get_database_location(char *db_env)
{
	static char *ptr;
	ptr = getenv(db_env);
	return(ptr);
}

/****
* get & print the header of the database given
****/
void
print_commondb_header(GDBM_FILE db_file)
{
	datum key, content;
	char *chPtr;

	key.dptr = DBM_ID_TOKEN;
	key.dsize = strlen(DBM_ID_TOKEN)+1;

	/* original database, contains platform, user, date & time */
	content = gdbm_fetch(db_file, key);
	if(content.dptr != NULL)
	{
		printf(content.dptr);
		free(content.dptr);
	}

	/* update info, contains platform, user, data & time */
	key.dptr = DBM_UPDATE_TOKEN;
	key.dsize = strlen(DBM_UPDATE_TOKEN)+1;

	content = gdbm_fetch(db_file, key);
	if(content.dptr != NULL)
	{
		printf(content.dptr);
		free(content.dptr);
	}

	/* searched directories */
	key.dptr = DBM_INFO_TOKEN;
	key.dsize = strlen(DBM_INFO_TOKEN)+1;
	content = gdbm_fetch(db_file, key);
	if(content.dptr != NULL)
	{
		printf("Contains the following directories:\n");
		for(chPtr = strtok(content.dptr, ","); chPtr != NULL; 
			chPtr = strtok(NULL, ","))
			printf("\t%s\n", chPtr);
	}
}

/***** 
* Add a section identifying what paths are stored in this database. 
*****/
void
write_commondb_dirinfo(GDBM_FILE db_file, char *dir)
{
	datum key, prev;

	key.dptr = DBM_INFO_TOKEN;
	key.dsize = strlen(DBM_INFO_TOKEN)+1;

	if(gdbm_exists(db_file, key))
	{
		prev = gdbm_fetch(db_file, key);
		/* 
		* If the stringcompare matches the current dir, it's already 
		* here so we don't want to update it again. 
		*/
		if(strstr(prev.dptr, dir))
			return;
		/*
		* Resize the returned item so it will contain the next also 
		* but separated by a comma 
		*/
		prev.dsize += strlen(dir) + 2;
		checked_realloc(prev.dptr, prev.dsize, char);

		/* update data */
		strcat(prev.dptr, ",");
		strcat(prev.dptr, dir);

		gdbm_store(db_file, key, prev, GDBM_REPLACE);

		free(prev.dptr);
	}
	else
	{
		prev.dsize += strlen(dir) + 1;
		prev.dptr = dir;
		gdbm_store(db_file, key, prev, GDBM_INSERT);
	}
}


/***** 
* Add a section identifying how many entries are stored in this database. 
*****/
/*ARGSUSED*/
void
write_commondb_update_info(GDBM_FILE db_file, int entries)
{
	datum content, update_key;
	struct utsname sysinfo;
	time_t cr_time;
	char *upd_time, fullname[256];

	uname(&sysinfo);
	time(&cr_time);
	upd_time = ctime(&cr_time);

	update_key.dptr = DBM_UPDATE_TOKEN;
	update_key.dsize = strlen(DBM_UPDATE_TOKEN)+1;
#if 0
	if(gdbm_exists(db_file, update_key))
	{
		datum prev;
		char *chPtr;
		int num_entry, num_files;

		prev = gdbm_fetch(db_file, update_key);
		chPtr = strstr(prev.dptr, "Contains ");
fprintf(stderr,"write_commondb_update_info, chPtr is %s\n", chPtr);
		sscanf(chPtr, "%i entries.", &num_entry);
fprintf(stderr,"write_commondb_update_info, num_entry is %i\n", num_entry); 
		entries +=num_entry;
	}
	sprintf(fullname, "Last updated by %s\nHost %s, platform %s (%s) "
		"on %sContains %i entries.\n", get_user_login(), 
		sysinfo.nodename, sysinfo.sysname, sysinfo.machine,
		upd_time, entries);
#endif

	sprintf(fullname, "Last updated by %s, host %s, platform %s (%s)\n"
		"on %s", get_user_login(), 
		sysinfo.nodename, sysinfo.sysname, sysinfo.machine,
		upd_time);

	content.dptr = fullname;
	content.dsize = strlen(fullname)+1;

	gdbm_store(db_file, update_key, content, GDBM_REPLACE);

}

/***** 
* Add a header identifying the user, date, system and name of the 
* database file 
*****/
void
write_commondb_header(GDBM_FILE db_file, char *outfile)
{
	struct utsname sysinfo;
	time_t cr_time;
	char fullname[256];
	datum key, content;
	char *upd_time;

	uname(&sysinfo);		/* get system information */
	time(&cr_time);			/* get the system time */
	upd_time = ctime(&cr_time);	/* convert to ASCII string */
	sprintf(fullname, "Database %s created by %s (%s).\n"
		"Host %s, platform %s (%s) on %s", 
		outfile, get_user_login(), get_user_name(), 
		sysinfo.nodename, sysinfo.sysname, sysinfo.machine,
		upd_time);

	content.dptr = fullname;
	content.dsize = strlen(fullname)+1;
	key.dptr = DBM_ID_TOKEN;
	key.dsize = strlen(DBM_ID_TOKEN)+1;

	if(!gdbm_exists(db_file, key))
		gdbm_store(db_file, key, content, GDBM_INSERT);
}

/*****
* Remove all unnecessary characters from the given string. Compresses white
* space, replaces newlines with commas and does a little syntax checking on
* intended commons and the no of continuation lines allowed by the f77 standard.
*****/
static inline void
compress_common_block(char *string, int curr_line_num)
{
	register char *outPtr = string;
	register int sp_count = 0;
	int numline = 0;
	while (1)
	{
		switch(*string)
		{
			case ' ':	/* a space, increment the space cnt. */
				string++;
				sp_count++;
				break;
			case '\n':	/* special case, replace with a comma */
				sp_count = 0;	/* reset space counter */
				numline++;
				*outPtr++=',';
				string++;
				break;
			/*
			* C is a comment char if it's at the beginning of a 
			* line (space count equals zero), so delete until we 
			* reach end of line or \0. 
			*/
			case 'C':
			case '*':
				if(sp_count == 0)
				{
					while(*string != '\0' 
						&& *string != '\n')
						string++;
					break;
				}
				else
					if(sp_count < 5)
					{
						num_warnings++;
						fprintf(stderr, "WARNING: "
							"intended comment "
							"found near line %i\n",
							curr_line_num);
						while(*string != '\0' 
							&& *string != '\n')
							string++;
						break;
					} /* else fall through */
			/* 
			* any character but a space or a 0 appearing on the 
			* sixth column is a continuation char 
			*/
			default:
				if(sp_count == 5 && *string != '\0' 
					&& *string != ' ' && *string != '0')
				{
					string++;
					sp_count++;
				}
				else
					*(outPtr++) = *(string++);
		}
		if (*string == 0)
		{
			/* 19 continuation lines allowed by f77 standard */
			if(numline > 18)
			{
				num_warnings++;
				fprintf(stderr, "WARNING: more than 19 "
					"continuation lines near line %i\n",
					curr_line_num);
			}
			*outPtr = '\0';
			return;
		}
	}
}

/****
* Remove excess commas in the output put there by compress_common_block.
****/
static inline void
remove_excess_commas(char *string)
{
	register char *outPtr = string;

	/* what is this 'R' doing here ? */
	if(*string == 'R' && *(string+1) == ',')
		string++;
	while(1)
	{
		/*
		* Skip commas as long as the next character is a comma and we 
		* haven't reached the end of the line
		*/
		if(*string == ',')
			while(*(string+1) != '\0' && *(string+1) == ',')
				string++;
		if(*string == 0)
		{
			*outPtr = '\0';
			return;
		}
		*(outPtr++) = *(string++);
	}
}

/*****
* remove the tokens in the token table from the given string
*****/
static inline int
remove_tokens(char *string)
{
	register char *outPtr = string;
	register int tok_id;

	while(1)
	{
		/* scan for any tokens appearing in token_table */
		for(tok_id = 0 ; tok_id < NUM_TOKEN && 
			strncmp(string, token_words[tok_id].token, 
				token_words[tok_id].len); tok_id++); 
		switch(tok_id)
		{
			/* This is again a special case. We only want to have
			* the name of the param, not it's value 
			*/
			case PARAMETER:
				string = string + token_words[tok_id].len;
				/* remove anything not a character */
				while(!isalnum(*string))
					string++;
				/* copy the word found */
				while(isalnum(*string))
					*(outPtr++) = *(string++);
				/* remove anything else until the end of line */
				while(*string != '\0' && *string != '\n')
					*string++; 
				break;
			case COMMON:
			case LOGICAL:
				/* skip past the token found */
				string = string + token_words[tok_id].len;
				break;
			case REAL:
			case INTEGER:
			case DOUBLE:
			case COMPLEX:
				/* skip past the token found */
				string = string + token_words[tok_id].len;
				/* numbers params must start with a letter */
				while(!isalpha(*string))
					string++;
				break;
			case CHARACTER:
				/* 
				* skip past the token found and any numbers 
				* found in here 
				*/
				string = string + token_words[tok_id].len;

				/* 
				* can't use isdigit(*string++), CHARACTER can
				* given as CHARACTER*(LEN) also 
				*/
				while(!isspace(*string))
					string++;
				break;
			case NUM_TOKEN:
				/* no token found */
				*(outPtr++) = *(string++);
				break;
			default:	
				/* should never happen */
				fprintf(stderr, "Internal Parse Error: invalid" 
					"token range!\n");
				exit(7);
		}
		/* terminate if at end of current block of text */
		if (*string == 0)
		{
			*outPtr = '\0';
			return(1);
		}
	}
}

/*****
* remove all text between the delimiters in the delimit_table
*****/ 
static inline int 
remove_delimiter(char *string)
{
	register char *outPtr = string;
	register int i;

	while(1)
	{
		for(i = 0 ; i < NUM_DELIMITER && *string != delimit_table[i]; 
			i++);
		switch(i)
		{
			case 0:	/* a / was found */
			case 2:	/* a ( was found */
				string++; /* move past the delimiter */
				while(*string != '\0' && 
					*string != delimit_table[i+1])
					string++;
				if (*string == 0)
				{
					*outPtr = '\0';
					return(0);
				}
				else /* move past the delimiter */
					string++; 
				break;
			default:
				*(outPtr++) = *(string++);
				break;
		}
		if (*string == 0)
		{
			*outPtr = '\0';
			return(1);
		}
	}
}

/**** 
* scan a block for common names 
* Returns 0 or the number of warnings outputted on success,
* -1 on failure.
****/ 
int 
scan_block(char *text, int line_num)
{
	num_warnings = 0;
	if(remove_tokens(text))
	{
		compress_common_block(text, line_num);
		if(remove_delimiter(text))
		{
			remove_excess_commas(text);
			return(num_warnings);
		}
		else
		{
			fprintf(stderr, "WARNING:"
				"unbalanced brace or delimiter near line %i.\n",
				line_num);
#ifdef TEST
			num_errors++;
#endif 
			return(-1);
		}
	}
	else	/* should never happen */
	{
		fprintf(stderr, "Internal Parse Error: Failed to remove common"
			"keywords!\n");
		exit(8);
	}
	return(-1);
}
			
#ifdef TEST
int main()
{
	char *chPtr;
	char path[256], file[256];
	int i;
	printf("Name of current user: %s\n", get_user_name());
	printf("Login name of current user: %s\n", get_user_login());
	fflush(stdout);
	printf("Testing ParseFilename on lib_comm.h\n");
	fflush(stdout);
	ParseFilename("lib_comm.h", file, path);        
	printf("lib_comm.h -> %s%s\n", path, file);
	fflush(stdout);
	printf("Test 1, you should get 1 warning.\n");
	printf("Block to scan: \n%s\n", text);
	if(scan_block(text))
	{
		printf("Commons found: \n");
		for(i=0, chPtr = strtok(text, ","); 
			chPtr != NULL; chPtr = strtok(NULL, ","), i++)
			printf("%s ", chPtr);
		printf("%i in total\n",i);
	}
	else
		printf("scan failed...\n"); 
	printf("\nTest 2, you should get 1 warning, 1 error and no output.\n");
	printf("Block to scan: \n%s\n", wrong_text);
	if((scan_block(wrong_text)) != -1)
	{
		printf("Oops, some errors should have been detected.\n");
		printf("Commons found: \n");
		for(i=0, chPtr = strtok(text, ","); 
			chPtr != NULL; chPtr = strtok(NULL, ","), i++)
			printf("%s\n", chPtr);
		printf("%i in total\n",i);
	}
	else
		printf("scan failed (as it should with this garbage)\n"); 
	switch(num_warnings)
	{
		case 0 :
			printf("Something was seriously wrong, no warnings "
				"found, should have found one\n");
			break;
		case 1:
			printf("Found one warning, should have found one. "
				"Test passed\n");
			break;
		default:
			printf("Serious trouble, found more than one warning: "
				"%i in total\n", num_warnings);
	}
	switch(num_errors)
	{
		case 0 :
			printf("Something was wrong, no errors found, should "
				"have found 1\n");
			break;
		case 1:
			printf("Found 1 error, should have found 1. "
				"Test passed\n");
			break;
		default:
			printf("Internal scanner error: found more than 1 "
				"error: %i in total\n", num_errors);
	}
	return(0);
}
#endif
