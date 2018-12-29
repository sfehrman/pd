/**************************************************************************
pd.c v5.1.1
Copyright 1991-2005 (c) Scott L. Fehrman
All rights reserved, Use subject to license term

This program and it's related documents may be distributed and/or copied 
without charge. The related source code and executable files may be used 
freely providing that all copyright notices and information about the 
author and company are not removed or altered in any manner. The users
of this program and it's related documents understand that this material 
is provided "as is" with no warranty. The author and/or company are not 
responsible for any "side effects" and/or problems that this material may 
have on system and/or application files and data. The use of this program 
and it's related material implies that the user understands the terms and 
conditions for which it is provided.

Scott L. Fehrman, Systems Engineer
Sun Microsystems, Inc.
Two Pierce Place
Suite 1500
Itasca, IL 60143
(630) 285-7632
scott.fehrman@Sun.COM

Compile options:

   Solaris: gcc -o pd pd.c -O3 -lcurses `getconf LFS_CFLAGS` -D_SOLARIS_ACLS -D_DATE_YMD 
            cc -o pd pd.c -xO3 -lcurses `getconf LFS_CFLAGS` -D_SOLARIS_ACLS -D_DATE_YMD
     Linux: gcc -o pd pd.c -O3 [ -lcurses | -lncurses ]

***************************************************************************/

/*
 * include system header files 
 */

#include <stdio.h>                /* standard unix input/output facilities */
#include <time.h>                 /* file date/time data */
#include <sys/types.h>            /* system types used by <sys/stat.h> */
#include <sys/param.h>            /* machine dependant paramters */
#include <dirent.h>               /* directory entry information */
#include <sys/stat.h>             /* status information about files */
#include <malloc.h>               /* dyn memory allocation */
#include <errno.h>                /* error handling */
#include <pwd.h>                  /* passwd file data */
#include <grp.h>                  /* group file data */
#include <curses.h>               /* curses screen manipulation library */
#include <term.h>                 /* terminal information */
#include <stdlib.h>               /* getopt */

#ifdef _SOLARIS_ACL
#include <sys/acl.h>              /* Solaris access control lists */
#endif

#ifndef MAXNAMELEN
#define MAXNAMELEN 256
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef S_IFDOOR
#define S_IFDOOR 0xD000
#endif

/* 
 * define macros if needed 
 */

#ifndef S_ISDIR
#define    S_ISDIR(m)  (((m)&S_IFMT) == S_IFDIR) /* is a directory */
#endif

#ifndef S_ISLNK
#define    S_ISLNK(m)  (((m)&S_IFMT) == S_IFLNK) /* is symbolic link */
#endif

#ifndef S_ISBLK
#define    S_ISBLK(m)  (((m)&S_IFMT) == S_IFBLK) /* is block device */
#endif

#ifndef S_ISCHR
#define    S_ISCHR(m)  (((m)&S_IFMT) == S_IFCHR) /* is char device */
#endif

#ifndef S_ISDOOR
#define S_ISDOOR(m)    (((m)&0xF000) == S_IFDOOR)  /* is a Door */
#endif

#ifndef major
#define major(dev) (((dev) >> 8) & 0xff)         /* device major # */
#endif

#ifndef minor
#define minor(dev) ((dev) & 0xff)                /* device minor # */
#endif

#ifndef TRUE
#define          TRUE  1
#endif

#ifndef FALSE
#define         FALSE  0
#endif

#define OPT_NO_DOT  0             /* no files/dir that start with "." */
#define OPT_NO_HDR  1             /* no column headers */
#define OPT_NO_NAME 2             /* no user/group name, use uid/gid */
#define OPT_NO_PWD  3             /* no pathname of dir being listed */
#define OPT_NO_QTY  4             /* no quantity count for files/dirs */

#define OPT_DATE_ALL 0            /* show modified, accessed, status changed */
#define OPT_DATE_DTM 1            /* show modified */
#define OPT_DATE_DTA 2            /* show accessed */
#define OPT_DATE_DTS 3            /* show status changed */
#define OPT_DATE_MDY 4            /* format: month/day/year MM/DD/YYYY */
#define OPT_DATE_DMY 5            /* format: day/month/year DD/MM/YYYY */
#define OPT_DATE_YMD 6            /* format: year/month/day YYYY/MM/DD */

#define OPT_SORT_NONE  0          /* no sort */
#define OPT_SORT_REV   1          /* reverse any sort */
#define OPT_SORT_NAME  2          /* sort by file/dir name (default) */
#define OPT_SORT_DTM   3          /* sort by date/time modified */
#define OPT_SORT_DTA   4          /* sort by date/time accessed */
#define OPT_SORT_DTS   5          /* sort by date/time status changed */
#define OPT_SORT_USER  6          /* sort by user id */
#define OPT_SORT_GROUP 7          /* sort by group id */
#define OPT_SORT_INODE 8          /* sort by inode # */
#define OPT_SORT_LINK  9          /* sort by hard link count */
#define OPT_SORT_SIZE  10         /* sort by file size */
#define OPT_SORT_TYPE  11         /* sort by file type */

#define OPT_TYPE_FILE  0          /* show only files (non-directories) */
#define OPT_TYPE_DIR   1          /* show only directories */

/* 
 * Size units: byte, kilo, mega, giga
 */ 

#define OPT_UNIT_BYTE  0
#define OPT_UNIT_KILO  1
#define OPT_UNIT_MEGA  2
#define OPT_UNIT_GIGA  3

/*
 * define application constant variables 
 */

#define       VERSION  "5.1.1"    /* program revision stamp */
#define  MAX_NAME_LEN  MAXNAMELEN /* max character length of a file name */
#define  MAX_PATH_LEN  MAXPATHLEN /* max character length of a path name */
#define  SCREEN_WIDTH  80         /* default number of characters on screen */
#define     NUM_BLANK  2          /* blank characters between columns */
#define    ENTRY_FILE  0         
#define     ENTRY_DIR  1
#define       CMD_DIR  2
#ifdef _SOLARIS_ACL
#define    GETOPT_STR  "abcd:hilmn:ps:t:u:vABCDLRST"
#else
#define    GETOPT_STR   "bcd:hilmn:ps:t:u:vABCDLRST"
#endif

/*
 * declare global variables 
 */

extern char   *optarg;
extern int     optind, opterr, optopt;

typedef struct EntryTag {                     /* struct for entries */
              char szName[MAX_NAME_LEN];      /* entry name */
              struct stat Stat;               /* inode structure */
              int iLink;                      /* is symlink */
              int iValid;                     /* is symlink valid */
              char szLink[MAX_PATH_LEN];      /* pathname in link */
              } EntryData;

EntryData     *File,                          /* ptr to array, files */
              *Dir,                           /* ptr to array, directories */
              *Cmd;                           /* ptr to array, cmd line dirs */

typedef struct SrchTag {
              char szName[MAX_NAME_LEN];
              struct SrchTag *next;
              } SrchPath;

SrchPath      *PathFirst, *PathCurr, *PathInsert;

enum          eTagDateFormat {                /* date format options */
              MDY,                            /* MM/DD/YYYY */
              DMY,                            /* DD/MM/YYYY */
              YMD                             /* YYYY/MM/DD */
              } eDateFormat;

char          *szProgName,                    /* program name argv[0] */
              szDirName[MAX_NAME_LEN],        /* dir name argv[0+x] */
              cQtyChar = '.',                 /* qty terminating char */
              szError[MAX_PATH_LEN + 32];

int           iAllocFile = 100,               /* # files to alloc memory */
              iAllocDir = 100,                /* # dirs to alloc memory */
              iAllocCmd = 100,                /* # cmd dirs to alloc memory */
              iNumFiles,                      /* counter # files */
              iNumDirs,                       /* counter # dirs */
              iCmdDirs,
              iNumPaths,
              iNumEntries = 0,                /* counter total entries */
              iLongest = 0,                   /* length of longest name */
              iNumColumns,                    /* # of columns to display */
              iScrWidth = SCREEN_WIDTH,       /* Screen width # columns */
              iSortOption = OPT_SORT_NAME,    /* qty of sort options */
              iFlagAbort = FALSE,             /* flag, do not run -h, -v */
              iFlagAccessed = FALSE,          /* flag, date/time accessed */
              iFlagAcls = FALSE,              /* flag, show ACL information */
              iFlagAltOutput = FALSE,         /* flag, alternate output */
              iFlagBlocks = FALSE,            /* flag, blocks allocated */
              iFlagBrief = FALSE,             /* flag, brief output */
              iFlagCount = FALSE,             /* flag, count entries only */
              iFlagDirs = TRUE,               /* flag, show directories */
              iFlagDirAsFile = FALSE,
              iFlagDot = TRUE,                /* flag, show files with .*/
              iFlagFiles = TRUE,              /* flag, show files */
              iFlagHidden = FALSE,            /* flag, show "." & ".." */
              iFlagHeader = TRUE,             /* flag, show header extn dis */
              iFlagInode = FALSE,             /* flag, show file inode # */
              iFlagLink = FALSE,              /* flag, show link status */
              iFlagLinkCnt = FALSE,           /* flag, show file link count */
              iFlagMode = FALSE,              /* flag, show file perm mode */
              iFlagModified = FALSE,          /* flag, date/time modified */
              iFlagName = TRUE,               /* flag, show name & group */
              iFlagPath = TRUE,               /* flag, show pathname */
              iFlagPro = FALSE,               /* flag, protection data */
              iFlagQty = TRUE,                /* flag, show qty of entries */
              iFlagRecursive = FALSE,         /* flag, recursive search */
              iFlagReverse = FALSE,           /* flag, reverse sort */
              iFlagSingle = FALSE,            /* flag, single column */
              iFlagSize = FALSE,              /* flag, size & owner data */
              iFlagType = FALSE,              /* flag, file type */
              iFlagSortDefault = TRUE,        /* flag, sort files -ns */
              iFlagSortName = TRUE,
              iFlagStatus = FALSE,            /* flag, date/time stat mod */
	      iFlagSizeUnit = OPT_UNIT_BYTE,  /* flag, show size (default = bytes) */
              iFilesOnly = FALSE;             /* flag, cmd line, files only */

struct stat   *pStat, *pStatLink;             /* ptr to file status struct */

/*
 * function prototypes
 */

static int compare();
int  read_entry(char *,char *,int); 
void add_path(char *);          
void get_path(void);              
void sort_entries(void);            
void store_entry(int,char *,struct stat *,int,int,char *,int);
void process_entries(void);
void command_option(int);        
void command_date_option(char *); 
void command_no_option(char *);  
void command_sort_option(char *); 
void command_type_option(char *); 
void command_unit_option(char *); 
void process_directory(void);    
void display_header(void);      
void read_directory(DIR *);   
void display_file(void);
void display_directory(void);     
void display_dirfile(void);
void display_expanded(EntryData *,char *); 
void get_term_info(void);
void display_alternate(EntryData *,char *);
void build_perm_string(int,char *);
void output_alt_header(void);        
void output_timedate(struct tm *);
void align_column(int,int,int); 
void program_usage(void);        
void program_help(void);           
void program_version(void);

#ifdef _SOLARIS_ACL
void output_acl_info(char *);
#endif

/**********************************************************************/
int 
main(int argc, char *argv[])
/**********************************************************************/
{
   register int  t;
   int           iReturn = FALSE;
   int           iLink = FALSE;
   int           iValid = FALSE;
   int           i,c;
   char          szOption[MAX_NAME_LEN];
   static char   szLinkName[MAX_PATH_LEN];

   szProgName = argv[0];
   szLinkName[0] = '\0';
   iNumFiles = iNumDirs = iCmdDirs = iNumPaths = 0;
   PathFirst = PathCurr = PathInsert = NULL;

   /* 
    * the default date format may be set at compile-time -D_DATE_XXX
    */

#ifdef _DATE_MDY
   eDateFormat = MDY;
#elif _DATE_DMY
   eDateFormat = DMY;
#elif _DATE_YMD
   eDateFormat = YMD;
#else
   eDateFormat = MDY;
#endif

   /*
    * allocate some memory for the files and dirs
    */

   File = (EntryData *)malloc((unsigned int)(sizeof(EntryData)*iAllocFile));
   Dir  = (EntryData *)malloc((unsigned int)(sizeof(EntryData)*iAllocDir));
   Cmd  = (EntryData *)malloc((unsigned int)(sizeof(EntryData)*iAllocCmd));
   
   pStat     = (struct stat *)malloc((unsigned int)(sizeof(struct stat)));
   pStatLink = (struct stat *)malloc((unsigned int)(sizeof(struct stat)));

   if ( !File || !Dir || !Cmd || !pStat || !pStatLink )
   {
      (void)sprintf(szError,"%s: malloc()",szProgName);
      perror(szError);
      exit(errno);
   }

   get_term_info();

   while ( ( c=getopt(argc,argv,GETOPT_STR) ) != EOF ) command_option(c);

   /*
    * process all the files / dirs arguments
    */

   for ( ; optind < argc ; optind++ )
      if ( !iFlagAbort ) read_entry(argv[optind], argv[optind], TRUE);
   PathInsert = PathCurr;
   
   /*
    * if there are NO command line file names, use current dir "."
    */

   if ( !iNumFiles && !iCmdDirs && !iNumPaths && !iFlagAbort )
   {
      if ( stat(".",pStat) < 0 )
      {
         iFlagAbort = TRUE;
         iReturn = TRUE;
         (void)sprintf(szError,"%s: %s",szProgName,".");
         perror(szError);
      }
      else
      {
         add_path(".");
         iFlagDirAsFile = FALSE;
      }
   }
   else iNumEntries = iNumFiles + iCmdDirs;

   if ( !iFlagAbort ) process_entries();

#ifdef _DEBUG
   printf("DBG: main() Path Count = %d\n",iNumPaths);
   PathCurr = PathFirst;
   i=0;
   while ( PathCurr )
   {
      i++;
      printf("DBG: main() %4d: '%s'\n",i,PathCurr->szName);
      PathCurr = PathCurr->next;
   }
#endif

   return(iReturn);
}


/**********************************************************************/
void 
get_term_info()
/**********************************************************************/
{
   int iErrRet;
   setupterm((char *)0, 1, &iErrRet);

   /*
    * get the number of columns in the current window
    */

   if ( iErrRet == 1 )
   {
      if ( columns < 20 || columns > 200 ) iScrWidth = SCREEN_WIDTH;
      else                                 iScrWidth = columns;
   }
   else                                    iScrWidth = SCREEN_WIDTH;

   return;
}

/**********************************************************************/
void 
command_option(int c)
/**********************************************************************/
{
   /*
    * process the command line argument from getopt()
    */

   switch ( c )
   {
#ifdef _SOLARIS_ACL
      case 'a':         /*** Access Control Lists ACL's ***/
         iFlagAltOutput = TRUE;
         iFlagAcls = TRUE;
         break;
#endif
      case 'b':         /*** brief ***/
         iFlagBrief = TRUE;
         iFlagHeader = FALSE;
         iFlagPath = FALSE;
         iFlagQty = FALSE;
         break;
      case 'c':         /*** count only ***/
         iFlagCount = TRUE;
         iFlagSortDefault = FALSE;
         break;
      case 'd':         /*** date options ***/
         command_date_option(optarg);
         break;
      case 'h':         /*** help ***/
         program_help();
         iFlagAbort = TRUE;
         break;
      case 'i':         /*** inode ***/
         iFlagAltOutput = TRUE;
         iFlagInode = TRUE;
         break;
      case 'l':         /*** show sym link info not what it references ***/
         iFlagLink = TRUE;
         break;
      case 'm':         /*** show permission mode [4777] ***/
         iFlagAltOutput = TRUE;
         iFlagPro = TRUE;
         iFlagMode = TRUE;
         break;
      case 'n':         /*** "no" options ***/
         command_no_option(optarg);
         break;
      case 'p':         /*** permissions: 'rwx rwx rwx  user.group' ***/
         iFlagAltOutput = TRUE;
         iFlagPro = TRUE;
         break;
      case 's':         /*** sort options ***/
         command_sort_option(optarg);
         break;
      case 't':         /*** type selection: files or dirs or both ***/
         command_type_option(optarg);
         break;
      case 'u':         /*** unit (size) options ***/
         command_unit_option(optarg);
         break;
      case 'v':         /*** version ***/
         program_version();
         iFlagAbort = TRUE;
         break;
      case 'A':         /*** show all (including '.' and '..') ***/
         iFlagHidden = TRUE;
         iFlagDot = TRUE;
         break;
      case 'B':         /*** file blocks allocated ***/
         iFlagBlocks = TRUE;
         iFlagAltOutput = TRUE;
         break;
      case 'C':         /*** output names in a single column ***/
         iFlagSingle = TRUE;
         break;
      case 'D':         /*** process directory itself not its content ***/
         iFlagDirAsFile = TRUE;
         break;
      case 'L':         /*** hard link count ***/
         iFlagAltOutput = TRUE;
         iFlagLinkCnt = TRUE;
         break;
      case 'R':         /** recursive process sub-directories ***/
         iFlagRecursive = TRUE;
         break;
      case 'S':         /*** file size in bytes ***/
         iFlagAltOutput = TRUE;
         iFlagSize = TRUE;
         break;
      case 'T':         /*** type of file ***/
         iFlagAltOutput = TRUE;
         iFlagType = TRUE;
         break;
      default:         /*** invalid option ***/
         (void)printf(
            "\nerror: Invalid option: '-%c', use '-h' for help.",c);
         program_usage();
         iFlagAbort = TRUE;
         break;
   }
   return;
}


/**********************************************************************/
void 
command_date_option(char *pOpt)
/**********************************************************************/
{
   /*
    * process date argument sub-options
    */

   char *pOptList[] = {"all","dtm","dta","dts","mdy","dmy","ymd",NULL};
   char *pOptValue;

   while ( *pOpt != '\0' )
   {
      switch ( getsubopt(&pOpt,pOptList,&pOptValue) )
      {
         case OPT_DATE_ALL:
            iFlagAltOutput = TRUE; 
            iFlagAccessed = TRUE;
            iFlagModified = TRUE;
            iFlagStatus = TRUE; 
            break;
         case OPT_DATE_DTM:
            iFlagAltOutput = TRUE;
            iFlagModified = TRUE;
            break;
         case OPT_DATE_DTA:
            iFlagAltOutput = TRUE;
            iFlagAccessed = TRUE;
            break;
         case OPT_DATE_DTS:
            iFlagAltOutput = TRUE;
            iFlagStatus = TRUE;
            break;
         case OPT_DATE_MDY:
            eDateFormat = MDY;
            break;
         case OPT_DATE_DMY:
            eDateFormat = DMY;
            break;
         case OPT_DATE_YMD:
            eDateFormat = YMD;
            break;
         default: /*** invalid option ***/
            (void)fprintf(stderr,
               "\nerror: invalid date option: '%s', use '-h' for help.",
               pOptValue);
            program_usage();
            iFlagAbort = TRUE;
            break;
      }
   }
   return;
}


/**********************************************************************/
void 
command_no_option(char *pOpt)
/**********************************************************************/
{
   /*
    * process "no" argument sub-options
    */

   char *pOptList[] = {"dot","hdr","name","pwd","qty",NULL};
   char *pOptValue;

   while ( *pOpt != '\0' )
   {
      switch ( getsubopt(&pOpt,pOptList,&pOptValue) )
      {
         case OPT_NO_DOT:  iFlagDot = FALSE;    break;
         case OPT_NO_HDR:  iFlagHeader = FALSE; break;
         case OPT_NO_NAME: iFlagName = FALSE;   break;
         case OPT_NO_PWD:  iFlagPath = FALSE;   break;
         case OPT_NO_QTY:  iFlagQty = FALSE;    break;
         default:   /*** invalid option ***/
            (void)fprintf(stderr,
               "\nerror: invalid no option: '%s', use '-h' for help.",
               pOptValue);
            program_usage();
            iFlagAbort = TRUE;
            break;
      }
   }
   return;
}


/**********************************************************************/
void 
command_sort_option(char *pOpt)
/**********************************************************************/
{
   /*
    * process sort argument sub-options
    */

   int iOpt;
   char *pOptList[] = {"none","rev","name","dtm","dta","dts",
                       "user","group","inode","link","size","type",NULL};
   char *pOptValue;

   while ( *pOpt != '\0' )
   {
      iOpt =  getsubopt(&pOpt,pOptList,&pOptValue);
      if ( iOpt == OPT_SORT_NAME ) continue;
      else if ( iOpt == OPT_SORT_NONE ) iFlagSortDefault = FALSE;
      else if ( iOpt == OPT_SORT_REV ) iFlagReverse = TRUE;
      else
      {
         iSortOption = iOpt;
         iFlagAltOutput = TRUE;
         switch ( iSortOption )
         {
            case OPT_SORT_DTM:   iFlagModified = TRUE;     break;
            case OPT_SORT_DTA:   iFlagAccessed = TRUE;     break;
            case OPT_SORT_DTS:   iFlagStatus = TRUE;       break;
            case OPT_SORT_USER:  iFlagPro = TRUE;          break;
            case OPT_SORT_GROUP: iFlagPro = TRUE;          break;
            case OPT_SORT_INODE: iFlagInode = TRUE;        break;
            case OPT_SORT_LINK:  iFlagLinkCnt = TRUE;      break;
            case OPT_SORT_SIZE:  iFlagSize = TRUE;         break;
            case OPT_SORT_TYPE:  iFlagType = TRUE;         break;
            default:  /*** invalid option ***/
               (void)fprintf(stderr,
                  "\nerror: invalid sort option: '%s', use '-h' for help.",
                  pOptValue);
               program_usage();
               iFlagAbort = TRUE;
               break;
         }
      }
   }
   return;
}


/**********************************************************************/
void 
command_unit_option(char *pOpt)
/**********************************************************************/
{
   /*
    * process -u "unit" argument sub-options
    */

   char *pOptList[] = {"b","k","m","g",NULL};
   char *pOptValue;

   while ( *pOpt != '\0' )
   {
      switch ( getsubopt(&pOpt,pOptList,&pOptValue) )
      {
         case OPT_UNIT_BYTE:  iFlagSizeUnit = OPT_UNIT_BYTE;  break;
         case OPT_UNIT_KILO:  iFlagSizeUnit = OPT_UNIT_KILO;  break;
         case OPT_UNIT_MEGA:  iFlagSizeUnit = OPT_UNIT_MEGA;  break;
         case OPT_UNIT_GIGA:  iFlagSizeUnit = OPT_UNIT_GIGA;  break;
         default:   /*** invalid option ***/
            (void)fprintf(stderr,
               "\nerror: invalid unit option: '%s', use '-h' for help.",
               pOptValue);
            program_usage();
            iFlagAbort = TRUE;
            break;
      }
   }
   return;
}


/**********************************************************************/
void 
command_type_option(char *pOpt)
/**********************************************************************/
{
   /*
    * process "type" argument sub-options
    */

   char *pOptList[] = {"file","dir",NULL};
   char *pOptValue;

   while ( *pOpt != '\0' )
   {
      switch ( getsubopt(&pOpt,pOptList,&pOptValue) )
      {
         case OPT_TYPE_FILE: iFlagDirs = FALSE;  break;
         case OPT_TYPE_DIR:  iFlagFiles = FALSE; break;
         default:   /*** invalid option ***/
            (void)fprintf(stderr,
               "\nerror: invalid file option: '%s', use '-h' for help.",
               pOptValue);
            program_usage();
            iFlagAbort = TRUE;
            break;
      }
   }
   if ( !iFlagDirs && !iFlagFiles ) iFlagDirs = iFlagFiles = TRUE;
   return;
}

/**********************************************************************/
void 
add_path(char *szNewDir)
/**********************************************************************/
{
   /*
    * add a fully qualified directory name to list of ones to process
    */

   SrchPath *PathNew, *PathTmp;

#ifdef _DEBUG
   printf("DBG: add_path() NewDir='%s'\n", szNewDir);
#endif

   PathNew = (SrchPath *)malloc((unsigned int)(sizeof(SrchPath)));
   if ( !PathNew )
   {
      perror("add_path(): malloc() failed.");
      exit(errno);
   }

   (void)strcpy(PathNew->szName,szNewDir);
   PathNew->next = NULL;

   if ( !PathFirst )  /* the first one */
   {
      PathFirst = PathCurr = PathInsert = PathNew;
   }
   else
   {

#ifdef _DEBUG
      printf("DBG: add_path() PathCurr='%s', PathInsert='%s'\n",
         PathCurr->szName, PathInsert->szName);
#endif

      PathTmp = PathInsert->next;
      PathInsert->next = PathNew;
      PathInsert = PathNew;
      PathInsert->next = PathTmp;
   }
   iNumPaths++;

#ifdef _DEBUG
   printf("DBG: add_path() exit\n");
#endif

   return;
}


/**********************************************************************/
void 
get_path()
/**********************************************************************/
{
   /*
    * used for recursive processing, add all sub-dirs to the "list"
    */

   int t;
   char szFullPathName[MAX_NAME_LEN];

   for ( t=0 ; t<iNumDirs ; ++t )
   {
#if _DEBUG
   printf("DBG: get_path() szDirName=%s, Dir[%d].szName = %s, iLink=%d\n",
      szDirName,t,Dir[t].szName,Dir[t].iLink);
#endif
      if ( !Dir[t].iLink )
      {
         (void)sprintf(szFullPathName,"%s/%s",szDirName,Dir[t].szName);
         add_path(szFullPathName);
      }
   }
   return;
}


/**********************************************************************/
void 
store_entry(int iType, char *pszName, struct stat *pMyStat, int iLink, 
            int iValid, char *pszLinkName, int iCount)
/**********************************************************************/
{
   switch ( iType )
   {
      /*
       * a directory was specified on the command line
       */

      case CMD_DIR:

         if ( iCount == iAllocCmd )
         {
            iAllocCmd *= 2;
            Cmd = (EntryData *)realloc( (char *)Cmd,
                  (unsigned int)(sizeof(EntryData)*iAllocCmd));
#ifdef _DEBUG
   printf("DBG: store_entry() Cmd realloc(%d) iCOunt=%d\n",
      (unsigned int)(sizeof(EntryData)*iAllocCmd),iCount);
#endif
            if ( !Cmd )
            {
               (void)sprintf(szError,"%s: realloc()",szProgName);
               perror(szError);
               exit(errno);
            }
         }

         (void)strcpy(Cmd[iCount].szName,pszName);
         (void)memcpy( &(Cmd[iCount].Stat), pMyStat, sizeof(struct stat));
         Cmd[iCount].iLink = iLink;
         Cmd[iCount].iValid = iValid;
         (void)strcpy(Cmd[iCount].szLink,pszLinkName);
         break;

      /*
       * the entry is a file
       */

      case ENTRY_FILE:

         if ( iCount == iAllocFile )
         {
            iAllocFile *= 2;
            File = (EntryData *)realloc( (char *)File,
                   (unsigned int)(sizeof(EntryData)*iAllocFile));
#ifdef _DEBUG
   printf("DBG: store_entry() File realloc(%d) iCount=%d\n",
      (unsigned int)(sizeof(EntryData)*iAllocFile),iCount);
#endif            
            if ( !File )
            {
               (void)sprintf(szError,"%s: realloc()",szProgName);
               perror(szError);
               exit(errno);
            }
         }
         (void)strcpy(File[iCount].szName,pszName);
         (void)memcpy( &(File[iCount].Stat), pMyStat, sizeof(struct stat));
         File[iCount].iLink = iLink;
         File[iCount].iValid = iValid;
         (void)strcpy(File[iCount].szLink,pszLinkName);
         break;

      /*
       * the entry is a directory
       */

      case ENTRY_DIR:

         if ( iCount == iAllocDir )
         {
            iAllocDir *= 2;
            Dir = (EntryData *)realloc( (char *)Dir,
                  (unsigned int)(sizeof(EntryData)*iAllocDir));
#ifdef _DEBUG
   printf("DBG: store_entry() Dir realloc(%d) iCount=%d\n",
      (unsigned int)(sizeof(EntryData)*iAllocDir),iCount);
#endif
            if ( !Dir )
            {
               (void)sprintf(szError,"%s: realloc()",szProgName);
               perror(szError);
               exit(errno);
            }
         }
         (void)strcpy(Dir[iCount].szName,pszName);
         (void)memcpy( &(Dir[iCount].Stat), pMyStat, sizeof(struct stat));
         Dir[iCount].iLink = iLink;
         Dir[iCount].iValid = iValid;
         (void)strcpy(Dir[iCount].szLink,pszLinkName);
         break;
   }
   return;
}


/**********************************************************************/
void 
process_entries()
/**********************************************************************/
{
   /*  
    * process command line data, non-options
    */

   int iTemp;
   int          iReturn = FALSE;
   register int i;                             /* local counter */
   DIR          *pDir;                         /* ptr to dir struct DIR */

   szDirName[0] = '\0';

#ifdef _DEBUG
   printf("DBG: process_entries() enter\n");
#endif

   /*
    * process the files
    */

   if ( iNumFiles && iFlagFiles )
   {
      if ( iFlagSortDefault ) sort_entries();
      iTemp = iFlagPath;
      iFlagPath = FALSE;
      iFilesOnly = TRUE;
      display_header();
      display_file();
      iFlagPath = iTemp;
      iFilesOnly = FALSE;
   }

   /*
    * process the directories like files
    */

   if ( iFlagDirAsFile ) display_dirfile();

   /*
    * process the directories like sub-dirs
    */

   else
   {
      while ( PathCurr )
      {

#ifdef _DEBUG
   printf("DBG: process_entries() PathCurr->szName='%s'\n",PathCurr->szName);
#endif

         (void)strcpy(szDirName,PathCurr->szName);
         pDir = opendir(szDirName);
         if ( pDir )
         {
            read_directory(pDir);
            (void)closedir(pDir);
            if ( iFlagSortDefault ) sort_entries();
            if ( iFlagPath )  display_header();
            if ( iFlagFiles ) display_file();
            if ( iFlagDirs )  display_directory();
            if ( iFlagRecursive ) get_path();
         }
         else
         {
            (void)sprintf(szError,"%s: %s",szProgName, szDirName);
            perror(szError);
         }
         PathCurr = PathInsert = PathCurr->next;
      }
   }

#ifdef _DEBUG
   printf("DBG: process_entries() exit\n");
#endif

   return;
}


/**********************************************************************/
void 
read_directory(DIR *pDir)
/**********************************************************************/
{

   /*
    * get the name of all the entries in a directory
    */

   int  iTemp,
        iDone = FALSE;
   char szEntry[MAX_NAME_LEN],      /* just the file name */
        szFullName[MAX_PATH_LEN];   /* fully qualified path/file name */
   struct dirent *pFile;            /* ptr to file struct dirent */

   iTemp = 0;
   iNumFiles = 0;
   iNumDirs = 0;
   iLongest = 0;
   iNumEntries = 0;

#ifdef _DEBUG
   printf("DBG: read_directory() enter szDirName='%s'\n",szDirName);
#endif

   do
   {
      pFile = readdir(pDir);
      if ( pFile )
      {

#ifdef _DEBUG
   printf("DBG: read_directory() pFile->d_name='%s'\n",pFile->d_name);
#endif

         (void)strcpy(szEntry,pFile->d_name);
         if ( strcmp(szEntry,".") && strcmp(szEntry,"..") || iFlagHidden )
         {
            (void)strcpy(szFullName,szDirName);
            if ( strcmp(szDirName,"/") )
               (void)strcat(szFullName,"/");
            (void)strcat(szFullName,szEntry);
            if ( szEntry[0] != '.' || iFlagDot )
            {
               iNumEntries++;
               iDone = read_entry(szFullName, szEntry, FALSE);
            }
         }
      }
      else iDone = TRUE;
   } while ( !iDone );

#ifdef _DEBUG
   printf("DBG: read_directory() exit\n");
#endif

   return;
}

/**********************************************************************/
int 
read_entry(char *szFullName, char *szEntryName, int iCommandLine)
/**********************************************************************/
{

   /*
    * stat the entry and properly store it away for later processing
    */

   int iReturn = FALSE;
   int iLink = FALSE;
   int iValid = TRUE;
   int iTemp = FALSE;
   static char szLinkName[MAX_PATH_LEN];  /* link path/name */

   szLinkName[0] = '\0';
 
#ifdef _DEBUG
   printf("DBG: read_entry() enter");
   printf(" %d:FullName='%s',",strlen(szFullName),szFullName);
   printf(" %d:EntryName='%s', ",strlen(szEntryName),szEntryName);
   printf(" CommandLine='%d'\n",iCommandLine);
#endif

   if ( lstat(szFullName, pStatLink) < 0 )
   {
      /*
       * error; can't get the status of the file
       */

      iFlagAbort = TRUE;
      iReturn = TRUE;
      (void)sprintf(szError,"%s: %s",szProgName,szFullName);
      perror(szError);
   }
   else
   {
      if ( S_ISLNK(pStatLink->st_mode) ) 
      {
         iLink = readlink(szFullName,szLinkName,MAX_PATH_LEN);
         szLinkName[iLink] = '\0';
      }

      if ( stat(szFullName, pStat) < 0 )
      {
         /*
          * a symbolic link that is broken; save the sym link as a file
          */

         iValid = FALSE;
         if ( !szEntryName[0] != '.' || iFlagDot )
            store_entry(ENTRY_FILE,szEntryName,
               pStatLink,iLink,
               iValid,szLinkName,
               iNumFiles++);
      }
      else
      {
         /*
          * valid entry; is it a directory
          */
         if ( S_ISDIR(pStat->st_mode) )
         {
            if ( iLink && iFlagLink )
            {
               /*
                * link points to a directory and the user wants link data
                */
               if ( szEntryName[0] != '.' || iFlagDot )
                  store_entry(ENTRY_DIR,szEntryName,
                     pStatLink,iLink,
                     iValid,szLinkName,
                     iNumDirs++);
            }
            else
            {
               /*
                * a real directory entry
                */

               if ( iCommandLine )
               {
                  if ( szEntryName[0] != '.' || iFlagDot )
                     add_path(szFullName);
               }
               else
               {
                  if ( szEntryName[0] != '.' || iFlagDot )
                     store_entry(ENTRY_DIR,szEntryName,
                        pStat,iLink,
                        iValid,szLinkName,
                        iNumDirs++);
               }
            }
         }    
         else
         {
            if ( iLink && iFlagLink )
            {
               /*
                * link points to a file and user wants link data
                */
               if ( szEntryName[0] != '.' || iFlagDot )
                  store_entry(ENTRY_FILE,szEntryName,
                     pStatLink,iLink,
                     iValid,szLinkName,
                     iNumFiles++);
            }
            else
            {
               /*
                * a real file entry
                */

               if ( szEntryName[0] != '.' || iFlagDot )
                  store_entry(ENTRY_FILE,szEntryName,
                     pStat,iLink,
                     iValid,szLinkName,
                     iNumFiles++);
            }
         }
      }
   }

   iTemp = strlen(szEntryName);
   if ( iTemp > iLongest ) iLongest = iTemp;

#ifdef _DEBUG
   printf("DBG: read_entry() exit\n");
#endif

   return(iReturn);
}

/**********************************************************************/
void 
display_header()
/**********************************************************************/
{
   /*
    * show header: pathname and number of entries
    */

   char *cwd, *getcwd();

#ifdef _DEBUG
   printf("DBG: display_header() enter szDirName=%s\n",szDirName);
#endif

   if ( !iFlagBrief ) (void)printf("\n");

   if ( !strcmp(szDirName,".") )
   {
      /*
       * we're listing the current working directory
       */
      cwd = getcwd((char *)NULL, 128);
      if ( cwd == NULL ) 
        (void)printf("[Can't get path: %s]   ",strerror(errno));
      else             
        (void)printf("%s   ",cwd);
   }
   else if ( strlen(szDirName) && strcmp(szDirName,".") ) 
   {
      /*
       * an explicitly specified directory 
       */
      (void)printf("%s   ",szDirName);
   }

   if (iNumEntries == 1) 
      (void)printf("%d Entry%c\n\n",iNumEntries,cQtyChar);
   else                  
      (void)printf("%d Entries%c\n\n",iNumEntries,cQtyChar);

#ifdef _DEBUG
   printf("DBG: display_header() exit\n");
#endif

   return;
}

/**********************************************************************/
void 
display_file()
/**********************************************************************/
{
   /*
    * show file qty, file name and any requested attirbutes
    */

   register int t,
                i;
   int          iColCnt,
                iColWidth,
                iNblanks;
   char         szPathName[MAX_PATH_LEN];  /* fully qualified path/file */

#ifdef _DEBUG
printf("DBG: display_file() enter, iScrWidth=%d, iLongest=%d\n",
       iScrWidth,iLongest);
#endif

   iNumColumns = iScrWidth / ( iLongest+NUM_BLANK );
   if ( iNumColumns < 1 ) iNumColumns = 1;
   if ( iFlagSingle ) iNumColumns = 1;
   iColWidth = iScrWidth / iNumColumns;

   if ( iFlagBrief && iFlagCount ) iFlagQty = TRUE;
   if ( !iFlagFiles && !iFlagDirs ) iFlagFiles = TRUE;

   /*
    * display the files in the directory
    */

   if ( iNumFiles && iFlagFiles )
   {
      iColCnt = 0;
      iNblanks = 0;
      if ( iFlagQty )
      {
         if (iNumFiles == 1) 
            (void)printf("%d File%c\n",iNumFiles,cQtyChar);
         else
            (void)printf("%d Files%c\n",iNumFiles,cQtyChar);

         if ( !iFlagBrief ) (void)printf("\n");
      }
      if ( !iFlagCount )
      {   
         if ( iFlagAltOutput )
         {

            /*
             * Alternate output, show name and attribute(s)
             */

            if ( iFlagHeader ) output_alt_header();
            for ( t=0 ; t<iNumFiles ; ++t )
            {
               if ( iFilesOnly )
                  (void)strcpy(szPathName,File[t].szName);
               else 
               {
                  (void)strcpy(szPathName,szDirName);
                  if ( strcmp(szDirName,"/") )
                     (void)strcat(szPathName,"/");
                  (void)strcat(szPathName,File[t].szName);
               }
               display_alternate(&File[t],szPathName);
            }
            if ( !iFlagBrief ) (void)printf("\n");
         }
         else  
         {

            /*    
             * standard output, use multiple columns
             */

            for ( t=0 ; t<iNumFiles ; ++t )
            {
               for ( i=0; i<iNblanks ; ++i ) (void)putchar(' ');
               (void)printf("%s",File[t].szName);
               if (++iColCnt >= iNumColumns)
               {
                  (void)putchar('\n');
                  iColCnt = 0;
                  iNblanks = 0;
               }
               else 
                  iNblanks = iColWidth - strlen(File[t].szName);
            }
            if (iColCnt != 0)  (void)putchar('\n');
            if ( !iFlagBrief ) (void)printf("\n");
         }
      }
   }

#ifdef _DEBUG
printf("DBG: display_file() exit\n");
#endif

   return;
}


/**********************************************************************/
void 
display_directory()
/**********************************************************************/
{
   /*
    * show sub-dir qty, name and any requested attirbutes
    */

   register int t,
                i;
   int          iColCnt,
                iColWidth,
                iNblanks;
   char         szPathName[MAX_PATH_LEN];  /* fully qualified path/file */

#ifdef _DEBUG
printf("DBG: display_directory() enter\n");
#endif

   iNumColumns = iScrWidth / ( iLongest+NUM_BLANK );
   if ( iNumColumns < 1 ) iNumColumns = 1;
   if ( iFlagSingle ) iNumColumns = 1;
   iColWidth = iScrWidth / iNumColumns;

   if ( iFlagBrief && iFlagCount ) iFlagQty = TRUE;
   if ( !iFlagFiles && !iFlagDirs ) iFlagDirs = TRUE;

   /*
    * display the sub-directories in the directory
    */

   if ( iNumDirs && iFlagDirs ) 
   {
      iColCnt = 0;
      iNblanks = 0;
      if ( iFlagQty )          
      {
         if (iNumDirs == 1) 
            (void)printf("%d Directory%c\n",iNumDirs,cQtyChar);
         else
            (void)printf("%d Directories%c\n",iNumDirs,cQtyChar);
         if ( !iFlagBrief ) (void)printf("\n");
      }
      if ( !iFlagCount ) 
      {   
         if ( iFlagAltOutput ) 
         {
            /*
             * use alternate output, show name and attribute(s)
             */
            if ( iFlagHeader ) output_alt_header();
            for ( t=0 ; t<iNumDirs ; ++t )
            {
               (void)strcpy(szPathName,szDirName);
               if ( strcmp(szDirName,"/") ) 
                  (void)strcat(szPathName,(char *)"/");
               (void)strcat(szPathName,Dir[t].szName);
               display_alternate(&Dir[t],szPathName);
            }
            if ( !iFlagBrief ) (void)printf("\n");
         }
         else  
         {

            /*
             * use standard output, use multiple columns 
             */

            for ( t=0 ; t<iNumDirs ; ++t )
            {
               for ( i=0 ; i<iNblanks ; ++i ) (void)putchar(' ');
               (void)printf("%s",Dir[t].szName);
               if (++iColCnt >= iNumColumns)
               {
                  (void)putchar('\n');
                  iColCnt = 0;
                  iNblanks = 0;
               }
               else
                  iNblanks = iColWidth - strlen(Dir[t].szName);
            }
            if (iColCnt != 0)  (void)putchar('\n');
            if ( !iFlagBrief ) (void)printf("\n");
         }
      }
   }

#ifdef _DEBUG
printf("DBG: display_directory() exit\n");
#endif

   return;
}


/**********************************************************************/
void 
display_dirfile()
/**********************************************************************/
{
   /*
    * show sub-dir name and attributes, not its content
    */

   register int t,
                i;
   int          iColCnt,
                iColWidth,
                iNblanks;
   char         szPathName[MAX_PATH_LEN];  /* fully qualified path/file */

   iNumColumns = iScrWidth / ( iLongest+NUM_BLANK );
   if ( iFlagSingle ) iNumColumns = 1;
   iColWidth = iScrWidth / iNumColumns;

   if ( iFlagBrief && iFlagCount ) iFlagQty = TRUE;
   if ( !iFlagFiles && !iFlagDirs ) iFlagDirs = TRUE;

   /*
    * show any command line dirs like a file, don't show their content
    */

   if ( iFlagDirAsFile && iCmdDirs && iFlagDirs )
   {
      iColCnt = 0;
      iNblanks = 0;
      if ( iFlagQty )          
      {
         if (iCmdDirs == 1) 
            (void)printf("%d Directory%c\n",iCmdDirs,cQtyChar);
         else
            (void)printf("%d Directories%c\n",iCmdDirs,cQtyChar);
         if ( !iFlagBrief ) (void)printf("\n");
      }
      if ( !iFlagCount ) 
      {   
         if ( iFlagAltOutput ) 
         {

            /*
             * use alternate output, show name and attribute(s)
             */

            if ( iFlagHeader ) output_alt_header();
            for ( t=0 ; t<iCmdDirs ; ++t )
            {
               (void)strcpy(szDirName,Cmd[t].szName);
               (void)strcpy(szPathName,szDirName);
               if ( strcmp(szDirName,"/") ) 
                  (void)strcat(szPathName,(char *)"/");
               (void)strcat(szPathName,Cmd[t].szName);
               display_alternate(&Cmd[t],szPathName);
            }
            if ( !iFlagBrief ) (void)printf("\n");
         }
         else  
         {
            /*
             * use standard output, use multiple columns 
             */
            for ( t=0 ; t<iCmdDirs ; ++t )
            {
               for ( i=0 ; i<iNblanks ; ++i ) (void)putchar(' ');
               (void)printf("%s",Cmd[t].szName);
               if (++iColCnt >= iNumColumns)
               {
                  (void)putchar('\n');
                  iColCnt = 0;
                  iNblanks = 0;
               }
               else
                  iNblanks = iColWidth - strlen(Cmd[t].szName);
            }
            if (iColCnt != 0)
               (void)putchar('\n');
            if ( !iFlagBrief ) 
               (void)printf("\n");
         }
      }
   }

   return;
}


/**********************************************************************/
void 
display_alternate(EntryData *pEntry, char *szPathName)
/**********************************************************************/
{
   int  iSpace = 3;

   (void)printf("%s",pEntry->szName);

   if ( iLongest >= strlen("Name") )
      align_column(iLongest,strlen(pEntry->szName),iSpace);
   else
      align_column(strlen("Name"),strlen(pEntry->szName),iSpace);        

   if ( pEntry->iValid || iFlagLink )
      display_expanded(pEntry, szPathName);
   else
      (void)printf("(Missing Symbolic Link Target)   ");

   if ( strlen(pEntry->szLink) ) 
   {
      if ( iFlagLink ) (void)printf("(Link Status)");
      else             (void)printf("-> %s",pEntry->szLink);
   }

   (void)printf("\n");

   return;
}


/**********************************************************************/
void 
display_expanded(EntryData *pEntry, char *szPathName) 
/**********************************************************************/
{
   int  i, iProCnt, iBitE, iBitU, iBitG, iBitO, iDiv=1;
   char szProBuf[32];
   struct tm *pTm;                   /* ptr to time/date struct */
   struct passwd *pPasswd;           /* ptr to passwd struct */
   struct group  *pGroup;            /* ptr to group struct */

   if ( iFlagInode )   (void)printf("%7ld   ",pEntry->Stat.st_ino);
   if ( iFlagLinkCnt ) (void)printf("%6d    ",pEntry->Stat.st_nlink);
   if ( iFlagBlocks )  (void)printf("%6d  ",pEntry->Stat.st_blocks);
   if ( iFlagSize )
   {
      if ( S_ISCHR(pEntry->Stat.st_mode) || S_ISBLK(pEntry->Stat.st_mode) )
      {
         (void)printf("    %3u,  %3u  ",
            major(pEntry->Stat.st_rdev),minor(pEntry->Stat.st_rdev));
      }
      else
      {
	 /* check to see if we wrapped a (long long), value is -1 */
	 if ( (long long)pEntry->Stat.st_size == (long long)-1 )
	 {
	    (void)printf("%13lld  ",(long long)-1);
	 }
	 else
	 {
	    switch ( iFlagSizeUnit )
	    {
	       case OPT_UNIT_GIGA: iDiv = iDiv * 1024;
	       case OPT_UNIT_MEGA: iDiv = iDiv * 1024;
               case OPT_UNIT_KILO: iDiv = iDiv * 1024;
	       case OPT_UNIT_BYTE:
	       default:
	          (void)printf("%13lld  ",(pEntry->Stat.st_size/iDiv));
	    }
	 }
      }
   }
   if ( iFlagPro )
   {
      iBitE = iBitU = iBitG = iBitO = 0;
      iProCnt = 0;
      szProBuf[iProCnt] = '\0';

      /*** USER ***/
     
      if ( pEntry->Stat.st_mode & S_IRUSR )
      {
         szProBuf[iProCnt++] = 'r';
         iBitU += 4;
      }
      else
         szProBuf[iProCnt++] = '-';

      if ( pEntry->Stat.st_mode & S_IWUSR )
      {
         szProBuf[iProCnt++] = 'w';
         iBitU += 2;
      }
      else
         szProBuf[iProCnt++] = '-';

      if ( pEntry->Stat.st_mode & S_IXUSR ) 
      {
         iBitU += 1;
         if ( pEntry->Stat.st_mode & S_ISUID )
         {
            szProBuf[iProCnt++] = 'S';
            iBitE += 4;
         }
         else
         {
            szProBuf[iProCnt++] = 'x';
         }
      }
      else
         szProBuf[iProCnt++] = '-';

      for ( i=0 ; i<3 ; i++ ) szProBuf[iProCnt++] = ' ';

      /*** GROUP ***/

      if ( pEntry->Stat.st_mode & S_IRGRP ) 
      {
         szProBuf[iProCnt++] = 'r';
         iBitG += 4;
      }
      else
         szProBuf[iProCnt++] = '-';

      if ( pEntry->Stat.st_mode & S_IWGRP ) 
      {
         szProBuf[iProCnt++] = 'w';
         iBitG += 2;
      }
      else
         szProBuf[iProCnt++] = '-';

      if ( pEntry->Stat.st_mode & S_IXGRP )
      {
         if ( pEntry->Stat.st_mode & S_ISGID ) 
         {
            szProBuf[iProCnt++] = 'S';
            iBitE += 2;
         }
         else
            szProBuf[iProCnt++] = 'x';
         iBitG += 1;
      }
      else
      {
         if ( pEntry->Stat.st_mode & S_ISGID ) 
         {
            szProBuf[iProCnt++] = 'L';
            iBitE += 2;
         }
         else
            szProBuf[iProCnt++] = '-';
      }

      for ( i=0 ; i<3 ; i++ ) szProBuf[iProCnt++] = ' ';

      /*** OTHER ***/

      if ( pEntry->Stat.st_mode & S_IROTH ) 
      {
         szProBuf[iProCnt++] = 'r';
         iBitO += 4;
      }
      else
         szProBuf[iProCnt++] = '-';

      if ( pEntry->Stat.st_mode & S_IWOTH ) 
      {
         szProBuf[iProCnt++] = 'w';
         iBitO += 2;
      }
      else
         szProBuf[iProCnt++] = '-';

      if ( pEntry->Stat.st_mode & S_IXOTH ) 
      {
         if ( pEntry->Stat.st_mode & S_ISVTX ) 
         {
            szProBuf[iProCnt++] = 't';
            iBitE += 1;
         }
         else
            szProBuf[iProCnt++] = 'x';

         iBitO += 1;
      }
      else
      {
         if ( pEntry->Stat.st_mode & S_ISVTX ) 
         {
            szProBuf[iProCnt++] = 'T';
            iBitE += 1;
         }
         else
            szProBuf[iProCnt++] = '-';
      }

      szProBuf[iProCnt++] = ' ';

#ifdef _SOLARIS_ACL
      if ( acl(szPathName,GETACLCNT,0,NULL) > MIN_ACL_ENTRIES )
         szProBuf[iProCnt++] = '+';
      else
         szProBuf[iProCnt++] = ' ';
#else
      szProBuf[iProCnt++] = ' ';
#endif

      szProBuf[iProCnt++] = ' ';

      szProBuf[iProCnt] = '\0';
      (void)printf("%s",szProBuf);

      /*
       * show the files Mode value
       */

      if ( iFlagMode ) 
      {
         if ( iBitE )
            (void)printf("[%d%d%d%d] ",iBitE,iBitU,iBitG,iBitO);
         else
            (void)printf("[ %d%d%d] ",iBitU,iBitG,iBitO);
      }

      /*
       * try to get the user and group name unless told not to
       */

      pPasswd = NULL;
      pGroup = NULL;
      if ( iFlagName )
      {
         pPasswd = (struct passwd *)getpwuid((int)pEntry->Stat.st_uid);
         pGroup = (struct group *)getgrgid((int)pEntry->Stat.st_gid);
      }
      if ( pPasswd != NULL ) (void)printf("%8s.",pPasswd->pw_name);
      else                   (void)printf("%8d.",pEntry->Stat.st_uid);
      if ( pGroup != NULL )  (void)printf("%-8s",pGroup->gr_name);
      else                   (void)printf("%-8d",pEntry->Stat.st_gid);
      (void)printf("  ");
   }

   if ( iFlagAccessed )
   {
      pTm = localtime(&pEntry->Stat.st_atime);
      output_timedate(pTm);
   }
   if ( iFlagModified )
   {
      pTm = localtime(&pEntry->Stat.st_mtime);
      output_timedate(pTm);
   }
   if ( iFlagStatus )
   {
      pTm = localtime(&pEntry->Stat.st_ctime);
      output_timedate(pTm);
   }

   if ( iFlagType )
   {
      switch ( pEntry->Stat.st_mode&S_IFMT )
      {
         case S_IFIFO:  (void)printf("FIFO Interface    "); break;
         case S_IFCHR:  (void)printf("Character Device  "); break;
         case S_IFDIR:  (void)printf("Directory         "); break;
         case S_IFBLK:  (void)printf("Block Device      "); break;
         case S_IFREG:  (void)printf("Regular File      "); break;
         case S_IFLNK:  (void)printf("Symbolic Link     "); break;
         case S_IFSOCK: (void)printf("Socket Interface  "); break;
         case S_IFDOOR: (void)printf("Door Descriptor   "); break;
         default:       (void)printf("Uknown Type       "); break;
      }
   }

#ifdef _SOLARIS_ACL
   if ( iFlagAcls ) output_acl_info(szPathName);
#endif

   return;
}


#ifdef _SOLARIS_ACL
/**********************************************************************/
void 
output_acl_info(char *szPathName)
/**********************************************************************/
{
   int            iAclQty,       /* quantity of ACL's on a file            */
                  iSaveQty,      /* save the qty, need it twice            */
                  iShowAcl;      /* flag to determine which acl to display */

   char           szAclName[12], /* ACL name: user_name, group_name        */
                  szAclType[12], /* ACL type: User, Group                  */
                  szMask[4],     /* ACL mask                               */
                  szAclPerm[4],  /* ACL permissions                        */
                  szAclEffe[4];  /* effective ACL permissions              */

   aclent_t      *pAclEnt,       /* pointer to ACL entry type(s)           */
                 *pAclTmp;       /* pointer to a Temp ACL entry            */
   struct passwd *pPasswd;       /* struct for passwd entry                */
   struct group  *pGroup;        /* struct for group entry                 */
   o_mode_t       mask;          /* permission mask                        */

   if ((iAclQty = acl(szPathName,GETACLCNT,0,NULL)) > MIN_ACL_ENTRIES)
   {
      pAclEnt = (aclent_t *) malloc(sizeof(aclent_t) * iAclQty);
      if ( pAclEnt )
      {
         if ( acl(szPathName, GETACL, iAclQty, pAclEnt) > 0 )
         {
            iSaveQty = iAclQty;
            for ( pAclTmp = pAclEnt ; iAclQty-- ; pAclTmp++ )
               if (pAclTmp->a_type == CLASS_OBJ) 
                  mask = pAclTmp->a_perm;
            build_perm_string(mask,szMask);
            (void)printf("[%s] ",szMask);
            iAclQty = iSaveQty;
            for ( pAclTmp = pAclEnt ; iAclQty-- ; pAclTmp++ )
            {
               pPasswd = NULL;
               pGroup = NULL;
               iShowAcl = FALSE;
               szAclType[0] = '\0';
               szAclName[0] = '\0';
               switch (pAclTmp->a_type)
               {
                  case USER:
                     (void)strcpy(szAclType,"u");
                     if ( iFlagName ) 
                        pPasswd = getpwuid(pAclTmp->a_id);
                     if ( pPasswd )
                        (void)strcpy(szAclName,pPasswd->pw_name);
                     else
                        (void)sprintf(szAclName,"%d",pAclTmp->a_id);
                     iShowAcl = TRUE;
                     break;
                  case GROUP:
                     (void)strcpy(szAclType,"g");
                     if ( iFlagName )
                        pGroup = getgrgid(pAclTmp->a_id);
                     if ( pGroup )
                        (void)strcpy(szAclName,pGroup->gr_name);
                     else
                        (void)sprintf(szAclName,"%d",pAclTmp->a_id);
                     iShowAcl = TRUE;
                     break;
               }
               if ( iShowAcl ) 
               {
                  build_perm_string(pAclTmp->a_perm, szAclPerm);
                  build_perm_string(pAclTmp->a_perm & mask, szAclEffe);
                  (void)printf("%s:%s:%s(%s) ",
                     szAclType,szAclName,szAclPerm,szAclEffe);
               }
            }
         }
         free(pAclEnt);
      }
   } 
   return;
}

/**********************************************************************/
void 
build_perm_string(int iPerm, char *szBuf)
/**********************************************************************/
{
   if ( iPerm & 4 ) szBuf[0] = 'r';
   else             szBuf[0] = '-';
   if ( iPerm & 2 ) szBuf[1] = 'w';
   else             szBuf[1] = '-';
   if ( iPerm & 1 ) szBuf[2] = 'x';
   else             szBuf[2] = '-';
   szBuf[3] = '\0';
   return;
}
#endif /* _SOLARIS_ACL */


/**********************************************************************/
void 
output_alt_header()
/**********************************************************************/
{
   register int i;
   int iSpace = 3;

   (void)printf("Name");
   align_column(iLongest,strlen("name"),iSpace);
   if ( iFlagInode )    (void)printf(" Inode #  ");
   if ( iFlagLinkCnt )  (void)printf("Link Cnt  ");
   if ( iFlagBlocks )   (void)printf("Blocks  ");
   if ( iFlagSize )
   {
      switch ( iFlagSizeUnit )
      {
	 case OPT_UNIT_GIGA: (void)printf("Size (Gbytes)  "); break;
	 case OPT_UNIT_MEGA: (void)printf("Size (Mbytes)  "); break;
         case OPT_UNIT_KILO: (void)printf("Size (Kbytes)  "); break;
	 case OPT_UNIT_BYTE:
         default:            (void)printf(" Size (bytes)  "); break;
      }                   
   }
   if ( iFlagPro )     
   {
      (void)printf("User  Group Other ");
      if ( iFlagMode ) (void)printf("[Mode] ");
      (void)printf("    User.Group     ");
   }
   if ( iFlagAccessed ) (void)printf("Accessed             ");
   if ( iFlagModified ) (void)printf("Modified             ");
   if ( iFlagStatus )   (void)printf("Status Changed       ");
   if ( iFlagType )     (void)printf("Type              ");
   if ( iFlagAcls )     
      (void)printf("Access Control Lists [mask] (effective) ");

   (void)printf("\n");
   for ( i=0 ; i<(iScrWidth-1) ; i++ ) (void)putchar('-');
   (void)printf("\n");

   return;
}


/**********************************************************************/
void 
output_timedate(struct tm *pTm)
/**********************************************************************/
{
   int iHour, iMin, iSec, iMonth, iDay, iYear;

   iHour =  pTm->tm_hour; 
   iMin =   pTm->tm_min; 
   iSec =   pTm->tm_sec;
   iMonth = pTm->tm_mon+1; 
   iDay =   pTm->tm_mday; 
   iYear =  pTm->tm_year;

   switch ( eDateFormat )
   {
      case MDY:
         if ( iMonth < 10 ) (void)printf("0%1d/",iMonth);
         else               (void)printf("%2d/",iMonth);
         if ( iDay < 10 )   (void)printf("0%1d/",iDay);
         else               (void)printf("%2d/",iDay);
         (void)printf("%4d",(1900+iYear));
         break;
      case DMY:
         if ( iDay < 10 )   (void)printf("0%1d/",iDay);
         else               (void)printf("%2d/",iDay);
         if ( iMonth < 10 ) (void)printf("0%1d/",iMonth);
         else               (void)printf("%2d/",iMonth);
         (void)printf("%4d",(1900+iYear));
         break;
      case YMD:
         (void)printf("%4d/",(1900+iYear));
         if ( iMonth < 10 ) (void)printf("0%1d/",iMonth);
         else               (void)printf("%2d/",iMonth);
         if ( iDay < 10 )   (void)printf("0%1d",iDay);
         else               (void)printf("%2d",iDay);
         break;
   }

   (void)printf("-");

   if ( iHour < 10 ) (void)printf("0%1d:",iHour);
   else              (void)printf("%2d:",iHour);
   if ( iMin < 10 )  (void)printf("0%1d:",iMin);
   else              (void)printf("%2d:",iMin);
   if ( iSec < 10 )  (void)printf("0%1d",iSec);
   else              (void)printf("%2d",iSec);

   (void)printf("  ");

   return;
}

/**********************************************************************/
void 
sort_entries()
/**********************************************************************/
{
   /*
    * sort the files
    */ 

   if ( iNumFiles )
   {
      iFlagSortName = TRUE;
      qsort((char *)File,iNumFiles,sizeof(EntryData),compare);
      if ( iSortOption > OPT_SORT_NAME )
	{
           iFlagSortName = FALSE;
           qsort((char *)File,iNumFiles,sizeof(EntryData),compare);
        }
   }

   /*
    * sort the sub-directories
    */

   if ( iNumDirs && !iFilesOnly )
   {
      iFlagSortName = TRUE;
      qsort((char *)Dir,iNumDirs,sizeof(EntryData),compare);
      if ( iSortOption > OPT_SORT_NAME )
      {
         iFlagSortName = FALSE;
         qsort((char *)Dir,iNumDirs,sizeof(EntryData),compare);
      }
   }

   /*
    * sort the command-line Dirs it being treated as Files
    */

   if ( iCmdDirs && iFlagDirAsFile )
   {
      iFlagSortName = TRUE;
      qsort((char *)Cmd,iCmdDirs,sizeof(EntryData),compare);
      if ( iSortOption > OPT_SORT_NAME )
      {
         iFlagSortName = FALSE;
         qsort((char *)Cmd,iCmdDirs,sizeof(EntryData),compare);
      }
   }
   return;
}


/**********************************************************************/
static int 
compare(EntryData *e1, EntryData *e2)
/**********************************************************************/
{
   /*
    * routine used by qsort(), compares attributes specified by options
    */

   int iReturn = 0;
   int i1, i2;

   if ( iFlagSortName )
   {
      if ( iFlagReverse ) iReturn = strcmp(e2->szName,e1->szName);
      else                iReturn = strcmp(e1->szName,e2->szName);
   }
   else
   {
      switch ( iSortOption )
      {
         case OPT_SORT_DTM:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_mtime - e1->Stat.st_mtime);
            else                
               iReturn = (e1->Stat.st_mtime - e2->Stat.st_mtime);
            break;
         case OPT_SORT_DTA:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_atime - e1->Stat.st_atime);
            else                
               iReturn = (e1->Stat.st_atime - e2->Stat.st_atime);
            break;
         case OPT_SORT_DTS:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_ctime - e1->Stat.st_ctime);
            else                
               iReturn = (e1->Stat.st_ctime - e2->Stat.st_ctime);
            break;
         case OPT_SORT_USER:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_uid - e1->Stat.st_uid);
            else                
               iReturn = (e1->Stat.st_uid - e2->Stat.st_uid);
            break;
         case OPT_SORT_GROUP:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_gid - e1->Stat.st_gid);
            else                
               iReturn = (e1->Stat.st_gid - e2->Stat.st_gid);
            break;
         case OPT_SORT_INODE:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_ino - e1->Stat.st_ino);
            else                
               iReturn = (e1->Stat.st_ino - e2->Stat.st_ino);
            break;
         case OPT_SORT_LINK:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_nlink - e1->Stat.st_nlink);
            else                
               iReturn = (e1->Stat.st_nlink - e2->Stat.st_nlink);
            break;
         case OPT_SORT_SIZE:
            if ( iFlagReverse ) 
               iReturn = (e2->Stat.st_size - e1->Stat.st_size);
            else                
               iReturn = (e1->Stat.st_size - e2->Stat.st_size);
            break;
         case OPT_SORT_TYPE:
            i1 = ((e1->Stat.st_mode)&S_IFMT);
            i2 = ((e2->Stat.st_mode)&S_IFMT);
            if ( iFlagReverse ) 
               iReturn = ( i2 - i1 );
            else                
               iReturn = ( i1 - i2 );
            break;
      }
   }
   return iReturn;
}


/**********************************************************************/
void 
align_column(int iLong, int iCurrent, int iGap)
/**********************************************************************/
{
   register int i, t;
   t = iLong - iCurrent;
   for ( i=0 ; i<t ; i++ )    (void)putchar(' ');
   for ( i=0 ; i<iGap ; i++ ) (void)putchar(' ');
   return;
}


/**********************************************************************/
void 
program_usage()
/**********************************************************************/
{
   (void)printf("\nusage: %s  [%s]  [files/dirs]\n\n",
      szProgName,GETOPT_STR);
   return;
}


/**********************************************************************/
void 
program_help()
/**********************************************************************/
{
   /*
    * "abcd:hikln:ps:t:vABCDLRST"
    */
   (void)printf("\n");
#ifdef _SOLARIS_ACL
   (void)printf(" -a        access control lists\n");
#endif
   (void)printf(" -b        brief, unformatted output\n");
   (void)printf(" -c        count entries only\n");
   (void)printf(" -d <args> date/time options\n");
   (void)printf("           [all,dtm,dta,dts] [mdy|dmy|ymd]\n");
   (void)printf(" -h        help\n");
   (void)printf(" -i        show inode numbers\n");
   (void)printf(" -l        show symbolics status\n");
   (void)printf(" -m        show permission mode [4777]\n");
   (void)printf(" -n <args> no formating options\n");
   (void)printf("           [dot,hdr,name,pwd,qty]\n");
   (void)printf(" -p        permissions:  rwx rwx rwx  name.group\n");
   (void)printf(" -s <args> sorting options (default = name)\n");
   (void)printf(
   "           [none|name|dtm|dta|dts|user|group|inode|link|size|type,rev]\n");
   (void)printf(" -t <args> select type (default = '-t file,dir')\n");
   (void)printf("           [file,dir]\n");
   (void)printf(" -u <args> select unit for file size (default = '-u b')\n");
   (void)printf("           [b|k|m|g]\n");
   (void)printf(" -v        program version\n");
   (void)printf(" -A        show all entries (including '.' and '..')\n");
   (void)printf(" -B        show file blocks allocated\n");
   (void)printf(" -C        output using a single column\n");
   (void)printf(" -D        show sub-dir data not its entries\n");
   (void)printf(" -L        show hard link count\n");
   (void)printf(" -R        recursive list into sub-directories\n");
   (void)printf(" -S        show file size, (default = bytes)\n");
   (void)printf(" -T        show files type\n");
   program_usage();
   return;
}


/**********************************************************************/
void 
program_version()
/**********************************************************************/
{
   (void)printf(
   "[ print directory (pd)          Scott L. Fehrman          version (%s) ]\n"
   ,VERSION);
   return;
}
