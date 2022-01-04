# pd

This program will read the current directory or the directories listed on the command line and display the contents showing the "files" first then the "directories". This program can be used instead of "ls".

## Compile

`gcc -o pd pd.c -O3 -lncurses`

## Install

Copy the binary file `pd` to a folder that is in the command search path:

`cp pd /usr/local/bin`

Copy the **man page** to the manual folder:

`cp pd.1 /usr/local/share/man/man1`

## Run

List the current directory:

```
% pd

/Users/sfehrman/git/pd   7 Entries.

5 Files.

ChangeLog  README     README.md  pd.1       pd.c

2 Directories.

.git       .vscode
```

List the current directory, show the **size** and the **date/time modified**:

```
% pd -S -ddtm

/Users/sfehrman/git/pd   7 Entries.

5 Files.

Name         Size (bytes)  Modified             
---------------------------------------------------------------------------------------------------
ChangeLog           10601  2022/01/03-18:21:59  
README                720  2022/01/03-15:36:00  
README.md             215  2022/01/03-15:36:00  
pd.1                12716  2022/01/03-18:17:29  
pd.c                54615  2022/01/03-19:05:35  

2 Directories.

Name         Size (bytes)  Modified             
---------------------------------------------------------------------------------------------------
.git                  416  2022/01/03-19:20:31  
.vscode                96  2022/01/03-15:38:26  
```

List the `/` directory:

```
% pd /

/   21 Entries.

3 Files.

.DS_Store           .VolumeIcon.icns    .file

18 Directories.

.fseventsd          .vol                Applications        Library             System
Users               Volumes             bin                 cores               dev
etc                 home                opt                 private             sbin
tmp                 usr                 var

```

