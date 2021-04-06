# CSCI E-92: Principles of Operating Systems Final Project:

This program simulates a simple shell. It will prompt input from the user with a "$ " and will accept a line of
text up to 256 characters in length, not counting any newline or null termination. From the input line, an
integer named "argc" and an array named "argv" will created. The integer integer argc will contain the count
of the number of white space separated white space separated fields found in the input line. If text is enclosed in
double-quotes, then any whitespace within will be part of the argument. The array argv will contain a list of
pointers to copies of each of the fields found in the input line as null-terminated strings. The following features
are encluded in the shell:

Commands:
	date
	exit
	help
	echo
	set
	unset
	malloc
	free
	memorymap
	memset
	memchk
	fopen
	fclose
	create
	delete
	rewind
	fputc
	fgetc
	ls
	ser2lcd
	touch2led
	pot2ser
	therm2ser
	pb2led
	examine
	deposit
	flashled
	multitask
	fputs
	fgets
	adduser
	remuser
	logout
	purge
	logrewind
	logpurge
	logread
	spawnFlashGB
	killFlashGB

Escape characters:
	\0
	\a
	\b
	\e
	\f
	\n
	\r
	\t
	\v

Code for memory allocation and deallocation has been implemented. Memory available is currently set to a maximum
of 128 bytes minus the storage for overhead structures such as linked list of free and allocated sections of memory
and the LCD. Aditional methods for memory management, assigning values in memory, and confirming which values are
stored are also included. Commands have been added to the shell to test and use these new fetures (see above for the
commands).

The algorithm for allocation/deallocation of memory makes use of two separate linked lists: 
  1. list of allocated segments, sorted by address
  2. list of free segments sorted by size

This makes 'first fit' equal to 'best fit', which makes allocatio/deallocation efficient.

This design is based on the reading from  Tanenbaum's  Modern Operating Systems and discussions in class,
with one addition: the free memory linked list is designed to be double linked (not just one directional as
described in the textbook), similar to the linked list for the allocated memory. The free/alloc link
information is stored within the 12K bytes of available memory at the beginning of free/alloc section.
Making free link to be double linked, while will not reduce the size of memory available for the access by
the caller, since it requires no more space than the allocated link (i.e. will not increase the size of overhead),
will speed up the management of free link list, and make the merging of address-adjacent sections of free memory
fast and simple.

The inode table is set up so that file data is stored in a linked list of blocks. The first block is created
when the file is created, and other added dynamically as necessary.

The file system design is based on the one used in UNIX systems. It consists of an system wide inode table and
the stream table dedicated to each process, i.e. each PCB structure will have its own stream array. Inode table
is an array of inodes, that contain the information about the type of the file, data blocks, the size of the file,
owner, permissions and other book keeping info. The stream table contains device type, file path name, access mode,
index into the inode table, and current position within a file data blocks.

The file path name consists of `/` indicating delimiting dir nodes, letters, numbers, `_`, `-`, and `.`.
The first character in a file name must be a letter. If a pathname starts with `/`, the search for this
file starts with root dir, else it starts with whatever current dir is set to. Since no 'cd' command
is implemented current dir is set to root, for now. To create a file in dir /home/anna/:

  - new dir: fopen("/home/anna/<new-dir-name>/", "a")
  - or in shell: $fopen /home/anna/<new-dir-name>/ a

fopen returns an index into a stream table wich is used in fclose, fputc, fgetc, and rewind commands.
Use shell help command for description of file utilities.

Similarly to UNIX, the dir data consists of records for each file contained in that directory. The records
contain a file name and it inode index - the index of the corresponding entry in the inode table. Since this
supports a hierachical directory, the special entries `.` and `..` for self and parent dir info, and are
always present when the `ls` command is used.

The shell has been modified so that now a supervisor call is required when calls are made for memory allocation,
device access, or any of the file IO calls. As a result, there is now an additional step taken when the user
executes any of the following commands:

  - free
  - malloc
  - fopen
  - fclose
  - create
  - delete
  - rewind
  - fputc
  - fgetc
  - fputs
  - fgets

Other commands that require interacting with the hardware device will also use the same supervisor calls that
the above commands use.

New interupts for the FlexTimer, I/O for the UART2, and the PDB (Process Delay, Block) have been added. These
new interupts are to be use instead of the defaults. The FlexTimer is to be used as the system clock that keeps
time to the nearest millisecond. The PDB is to schedule a delay between 1 sec to 50 msec (as specified by the user)
before completeing an action. The systick and pendsv interupts have been added to turn the shell into a multi
process system.

As a result of the addition of the new interupts, priorities had to be set up for each and the supervisor calls.
Current order, from highest to lowest is as follows:

  - PDB
  - FlexTimer
  - I/O
  - Systick & Pendsv
  - SVC

In conjunction with the systick and pendsv interupts, a Round Robin scheduler has been added to manage the new
process setup. The scheduler traverses a circular, double linked list, and if it comes across a process thats
been terminated, it removes it from the linked list and frees up the memory allocated to said process. When a
process in a ready state is found, the scheduler sets it as the current process, sets its state to running, and
then runs it.

With the upgrade to a multi process system comes with functions meant to spawn, block, wake, wait on, and kill
processes. Testing of the multi process capabilities can currently be done with the command `multitask` (which
runs the `ser2lcd` and `flashled` commands as well as a new pushbutton to UART2 interaction), and the commands
`spawnFlashGB` and `killFlashGB` (which respectively spawn a process that alternates the flashing of the blue
and green LEDs and kills said process). The later two commands allow the user to have another process running in
the background while interacting with the shell.

Login capability has been added. Upon shell startup, the USERNAME prompt is presented. Due to the admin being the
only valid user on initialization, only the admin or the exit command can be entered response to the user name prompt.
If the exit command isn't given, then the PASSWORD prompt is issued. The admin (and only admin) can register new
users, by executing adduser command, or remove a user - removeuser command. All users have unique user names, and
no user can be added with the same group as admin. Password information contained in the list of registered users
is in encrypted form only. There is no 'reset password' capability yet, so if the user forgets the password then
they are out of luck.

When user logs in, the log file (see bellow) is opened automatically, and then any other file is opened on request.
When the user logs out, all files opened on users request and the log file are closed for that user; user created
files stay in the system until explicitly deleted by any user with permissions or admin.

When a file is created, permissions are specified in a char string, where first three char are the owners permissions,
second set is for the group, and third set of for the world; first char out of three indicates if read access is
permitted: 'r' - yes, '-' no; second similarly for write access: 'w'/'-', and third for execute: 'x'/'-'. In this
implementation, the 'x' permission lets a user to delete a file: e.g "rwx-wx-w-" owner - the user that created this
file- can read, write and delete the file,  group can write and delete, and world can only write.

Default permission values are set when a file is not explicitly created (via create command), but instead is created
as a result of call to fopen (if it does not already exist): rwxrw-rw-; Besides `admin`, a file can be deleted by a
user if `-x` is specified for the owner which matches the user, or the matching group or the world.

When a file is opened, access mode is specified, and can be one of the following:
"r" Opens a file for reading; if file does not exist it is created*; read access starts from the beginning of the file
"w" Opens a file for writing; if file does not exist it is created*; write access starts from the beginning of the file
"a" Opens a file for appending; if file does not exist it is created*; write access starts from the end of the file	
"r+" Opens a file for reading and writing: if file does not exist it is created*; access starts from the beginning of the file	
"w+" same as "r+'	
"a+" Opens a file for reading and writing: if file does not exist it is created*; access starts from the end of the file
A process may have multiple streams opened that connect to the same file if each stream's access mode is different;
i.e. one-stream-per-mode-per-file.

A log file is created by shell during startup with permissions "wrxwrx-w-" - so that is can be read and purged by the
shell (its creator) and user admin that belongs to the same group as shell (shell in this case is treated as pseudo user);
no other user can be added to this privileged group. The log file is automatically opened for each user at log in, and
closed at logout. It can be purged or deleted by admin user only. While it can be accessed for read and write by admin only,
it can be accessed only for append by any other user.

The following security checks are performed, with the log file updated accordingly:
   1. User logs in to/out of the shell.
 
   2. The file access permissions preformed for the following fio requests:
		- `fopen`
		- `delete`
		- `purge`
		- `rewind`

   3. User privileges are checked for the following requests:
		- `adduser`
		- `remuser`
		- `logread`
		- `logpurge`
		- `logrewind`
		- `spawnFlashGB`
		- `killFlashGB`
		- `spawn`

   4. Mode is check for the following fio requests
		- `rewind` : cannot be rewound if accessed with `a` mode
		- `fputc`
		- `fgetc`
		- `fputs`
		- `fgets`

The Oscillator Clock has been added to the shell. It is used by the security log to record the timestamps of any security
violations that occur. The clock gets set when the date command is used to set the flex timer. However, unlike the flex
timer, the RTC doesn't need to be set each time the shell boots up. It will keep track of the time to the nearest second
regardless of run state, making it the ideal choice for the security log.

## Files:

- README.md
- utl.h
- utl.c
- cmd.h
- cmd.c
- shell.h
- shell.c
- mem.h
- mem.c
- fio.h
- fio.c
- pcb.h
- pcb.c
- uart.h
- uart.c
- delay.h
- delay.c
- led.h
- led.c
- pushbutton.h
- pushbutton.c
- derivative.h
- adc.h
- adc.c
- ctp.h
- ctp.c
- lcdc.h
- lcdc.c
- lcdcConsole.h
- lcdcConsole.c
- mcg.h
- mcg.c
- priv.h
- priv.c
- profont.h
- profont.c
- sdram.h
- sdram.c
- svc.h
- svc.c
- MK70F12_KDS_Addons.h
- PDB.h
- PDB.c
- flexTimer.h
- flexTimer.c
- intSerialIO.h
- intSerialIO.c
- tm.h
- tm.c
- systick.h
- systick.c
- fioutl.h
- fioutl.c
- usr.h
- usr.c
- rtc.h
- rtc.c
- help.h


## Building:

To build use the build command/icon provided by the IDE.

## Run

Open either KDS or CodeWarriors and load the project. Enable semihosting and hook up the tower to the
computer with the jlink and keyspan. Open the terminal emulator with `/dev/ttyUSB0`, and make sure that
the baud is set to 115200. Build the project, and then run it in debug mode.

When the program runs, the shell will prompt the user to log in. On start up, the only valid profile is the
admin, so the user must type the following credentials:

    User name: `admin`
    Password: `anna`

After a succesfull login, the following will be displayed:

    `$ `

This is to prompt the user for input. Depending on what is entered, the shell with either execute the
command given, or it will print an error message. The list of commands given above is what's currently
accepted by the shell.

To exit simply type the following command:

    `exit`

## Changes

### The fio

### Cleanup

File system source code has been split into two .c and two .h modules, with fio.c containing file system
calls, and fioutl.c containing supporting function to be used mainly by the file system itself.

The hardware devices (with the exception of stdin, stdout, and stderr) do not open on
initalization of a new process. They need to be opened by the user to interact with, and as a result
can also be closed. Also all commands that interact with the hardware assume that there is no open
stream for the hardware device and will open/close it when it begins/ends execution. If a hardware
device is already open, then an error message will occure and the command won't execute. It will also
close any devices it successfully opened before the failure occured.

### Date

The date command is now fully operational again because of the FlexTimer. However, it comes with some
additions. It can now accept one argument to set the time. To do so, the time must either be in
milliseconds or a simplified ISO 8601 date/time string in the form of YYYY-MM-DDTHH:MM, with the hour
being writen as a 24 hour clock. No argument being passed in will result in the current date/time being
displayed; if the date was called before being set, then it will show the incorrect time (i.e. January 1st
of 1970 instead of November 18th 2018).

## Bug Fixes and Corrections

### PS 4

The `pb2led` command has been corrected so that the LEDs are on only while the pushbutton is depressed.

The issue with `\r` and `\n` not being read when typing to the LCD has yet to be addressed due to current
time constraints.

### PS 3

It turns out that in the original version of the ls command, any directory it was used on would be
closed after execution. This included the root directory, which (in my design approach) has a dedicated
stream and isn't supposed to be closed. This would result in problems later on where a file/directory
would be opened, and its stream ID would be the one associated with the root directory. This has been fixed.

### Additions

The ls command has been modified that it can take in no arguments. Doing so will cause it to print out
the files and subdirectories in the current directory, which for now is always going to be the root directory.

A sub directory called dev has been created in the root directory on initialization. It will contain all
the directories for the hardware devices, which will in turn list all the available devices.

### Clarifications

The shell imitates a UNIX system in regards to format and style. In UNIX, fclose takes in FILE *, and myfclose
takes in FILE variable, i.e. stream id instead of a pointer to a stream.

The fputc command expects two arguments. The first argument is the character that is to be written into the
file/device. The second is the stream ID. If it is one of the dedicated streams, then the following string 
ID's are accepted for all the file commands:

    STDIN
    STDOUT
    STDERR
    YELLOW
    ORANGE
    GREEN
    BLUE
    SW1
    SW2
    LCD
    POTENTIOMETER
    THERMISTOR
    TS1
    TS2
    TS3
    TS4


## Notes

Note: Ausers will have a stream to logfile thats automatically opened in append mode upon loggin,
while only admin will have an additional stream to logfile in read mode.

Note: The password is not 'echoed' during loggin.

Note: Password information is recorded in encripted form only.

Note: The help command has been modified. When called without arguments, it lists all the help
options. When passed an argument - a valid help option - it lists all the commands and their
descriptions available under that category.

Note: Pictures have been included that demo the complete security system. It shows aspects of it
that were not included in the video demo.

IMPORTANT: The command `spawn` has an error that occurs when trying to spawn a process that runs
any of the LED commands other than flashing the green and blue LEDs. While everything runs smoothly
and the shell can still be interacted with, when those processes terminate naturally, the shell gets
thrown into the default handler when attempting to do a yeild. This error doesn't occur in multitask,
so it is unknown what causes it.

IMPORTANT: The commands, examine and deposit, display and set memory locations respectively.
The deposit command will perform a check on the memory location specified, to insure that it is
malloc-ed by the current process; if not, or too many values are specified to fit in the allocated
space, it will return an error. The mem utility (myMemcpy) was created for that purpose.

IMPORTANT: the memory source code can be easely configured to handle 64, 32, or 16 bit system.
However it can only support one at a time. A check will be done at compile time to see if the machine 
architecture is compatable with the expected address size. If not, a message will be displayed
indicating the need to update the configuration information. To reflect the current architecture
the following changes would need to be made to the source code:

- utl.h: in section under /* machine dependent configuration  :
       set all instances of 64 to 32/16, or back to 64 if needed (e.g. uint64_t -> uint32_t/uint16_t). 
- shell.c: /* configuration dependent:
      change uint64_t to uint32_t/uint16_t, or back to uint64_t if needed.

The provided unit test is currently set to a 64 bit architecture. Only one section makes use of
sprintf statements that use %ll formats, so these all would have to be changed to reflect the right
format. However it has been changed for a 32 bit architecture so that it can run on the tower.

IMPORTANT: when using the memory commands free, memset, and memchk, make sure to use the addresses
returned from malloc. The ones shown in memorymap are offset, so using them will result in error messages.
Also it is currently set up that if using one of those three commands, the address needs to begin
with an 'O' instead of a '0'. There are plans to change this so that both are acceptable.

IMPORTANT: A KDS project has been set up in the repo, so all the source code can be found in the
Shell/Source directory where they will remain.

Additional: some of the documentation from the previous problem sets has been fixed up to better
explain some of the functionality. Also due to now using KDS, some of the formating ended up changing
slightly, which might show up as changes in the pull request.
