#ifndef lint
static char rcsId[]="$Source: /usr/local/rcs/ForUtil/lib/RCS/filelist.c,v $";
#endif
/*****
* filelist.c : routine for creating a filelist of a directory
*
* This file Version	$Revision: 1.5 $
*
* Creation date:	Mon Feb  4 03:59:03 GMT+0100 1980
* Last modification: 	$Date: 1996/08/27 19:20:11 $
* By:			$Author: koen $
* Current State:	$State: Exp $
*
* Author:		koen
* (C)Copyright 1995 Ripley Software Development
* All Rights Reserved
*****/
/*****
* ChangeLog 
* $Log: filelist.c,v $
* Revision 1.5  1996/08/27 19:20:11  koen
* msdos related changes
*
* Revision 1.4  1996/08/02 14:53:41  koen
* Changed the extension scanning stuff. It now really scans for the 
* extensions only. 
* Removed the verbose stuff. It made the libForUtil not really a lib since 
* it dependent on an extern variable.
*
* Revision 1.3  1996/07/16 09:20:09  koen
* corrected a bug for num_extensions == 0
*
* Revision 1.2  1996/05/06 00:36:48  koen
* Adapted for MSDOS
*
* Revision 1.1  1996/04/25 02:30:15  koen
* Initial Revision
*
*****/ 
/* needed to prevent multiple variable decls under MSDOS in sysdeps.h */
#define LIBRARY_OBJECT

/* include this before anything else */
#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
# define direct dirent
# else
# ifdef HAVE_SYS_DIR_H
# include <sys/dir.h>
# endif
#endif

#include "forutil.h"
#include "memmacros.h"

/*****
* Build a filelist of the directory path. The files found are stored in
* file_list, the number of files in the list in num_files.
* ext_table is used to check the filenames read agains extensions specified
* in ext_table. This check is only performed if num_extensions is non-zero
*****/
void
build_file_list(char **file_list[], int *num_files, char *path, 
	char *ext_table[], int num_extensions)
{
	DIR *dirp;
	struct direct *dp;
	struct stat buf;
	char s_path[MAXPATHLEN];
	int i, sub_files= *num_files, count=0;

	sprintf(s_path, "%s%s.", path, CHAR_SLASH);
/* try to open the given dir. If we fail, don't bother */
	dirp = opendir(s_path);
	if (dirp == NULL)
	    perror(path);
	else 
	{ 
/* walk each entry in this directory */
		for(dp=readdir(dirp); dp != NULL;)
		{
/* skip self and parent */
			if (strcmp(dp->d_name,".")!=0 && strcmp(dp->d_name,"..")!= 0)
			{
				sprintf(s_path, "%s%s%s", path, CHAR_SLASH, dp->d_name);
#ifdef VERBOSE
				printf("build_file_list, testing file %s\n", s_path);
#endif 
/* try if we can read this file */
				if((stat(s_path, &buf)) != 0)
				{
					perror(s_path);
				}
/*****
* if we can read it, see if it is a directory. If it is, skip it, else
* store it. 
*****/
				else if (!(buf.st_mode & S_IFDIR))
				{
/* check against given extensions (if any). */
					for(i = 0 ; i < num_extensions; i++)
					{ 
						if(ext_table[i][0] == '-' && 
							!(file_has_ext(dp->d_name)))
							break;
						if((strend(dp->d_name, ext_table[i]))) 
							break;
					}
					if( num_extensions == 0 || i != num_extensions)
					{
						checked_realloc(*file_list, sub_files+1, char*);
/* +2 for the / and a terminating \0 */
						checked_malloc(*(*file_list+sub_files), 
							(strlen(path)+2+strlen(dp->d_name)), char);

						sprintf(*(*file_list+sub_files), "%s%s%s", path, 
							CHAR_SLASH, dp->d_name); 
						(*num_files)++;
						sub_files++;
						count++;
					} 
				}
			}
/* get the next entry */
			dp = readdir(dirp);
		}
#ifdef CLOSEDIR_VOID
		closedir(dirp);
#else 
		if((closedir(dirp)) != 0)
			perror(path);
#endif
	}
	return;
} 
