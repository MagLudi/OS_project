/*
 * help.h
 *
 */

#ifndef INCLUDES_HELP_H_
#define INCLUDES_HELP_H_

#define HELP_HELP "\r\n help:\t\tdisplay a list of all available commands, what they do and how to use them. Instructions are \r\n\t\tgrouped be category. \r\n\n\t\tTo show the commands for a single category, type `help <group>` where <group> is 'demo', 'file', \r\n\t\t'mem', 'env', or 'admin'.\r\n\n\t\tTo show all commands listed alphabetically type `help all`\r\n"

#define HELP_DATE "\r\n date:\teither gives current date and time if no arguments are passed in or set date time if argument\r\n\t\t is passed in. To set date, pass current time in either milliseconds or ISO 8601 date/time.\r\n\t\t To execute, type `date`, or `date <time in milli>`, or `date <YYYY-MM-DDTHH:MM>`.\r\n"
#define HELP_ECHO "\r\n echo:\techos back the arguments that were passed in if any were. To execute, type `echo` followed by\r\n\t\tany number of arguments.\r\n"
#define HELP_SET "\r\n set:\tsets shell variable. If no arguments specified, set prints the list of all current shell \r\n\t\tvariables. To execute type `set <variable>=<value>`\r\n"
#define HELP_UNSET "\r\n unset:\tunsets shell variable. If a nonexistent variable is given, it prints an error message. To\r\n\t\texecute type `unset <variable>`\r\n"

#define HELP_MALLOC "\r\n malloc:\tallocate a certain number of bytes (specified by user). To execute type `malloc <# of bytes>`\r\n"
#define HELP_FREE "\r\n free:\t\tdeallocate a region of memory. Address of target region is given by user. To execute type\r\n\t\t`free <address>`\r\n"
#define HELP_MEMMAP "\r\n memorymap:\toutput the map of both allocated and free memory regions. To execute type 'memorymap'\r\n"
#define HELP_MEMSET "\r\n memset:\ttakes in a memory region, value to which each byte in the specified region will be set,\r\n\t\tand the length (in bytes) of the memory region. To execute, type \r\n\t\t`memset <address> <value> <# of bytes>`\r\n"
#define HELP_MEMCHK "\r\n memchk:\ttakes in a memory region, value to which each byte in the specified region should contain,\r\n\t\tand the length (in bytes) of the memory region. To execute, type \r\n\t\t`memchk <address> <value> <# of bytes>`\r\n"
#define HELP_DEPOSIT "\r\n deposit:\t deposit a series of bytes to be stored in successive locations in memory starting at a\r\n\t\tparticular location. To execute, type `deposit <first location in memory> <list of the values of\r\n\t\tbytes to be stored in successive locations in memory>`\r\n"
#define HELP_EXAM "\r\n examine:\t display 16 bytes starting at the requested address. To execute, type `examine`\r\n"


#define HELP_FOPEN "\r\n fopen:\t\topen a stream. To execute, type `fopen /<dir path>/<file name> <access mode>`. returns stream`.\r\n"
#define HELP_FPUT "\r\n fputs:\t\tput a string in a file. Must have open stream in correct mode. To execute type\r\n\t\t`fputs <string> <stream id>'\r\n"
#define HELP_FGET "\r\n fgets:\t\tget a string starting from the current character being pointed to in a file. Must have open\r\n\t\tstream in correct mode. To execute type `fgets <stream id>`\r\n"
#define HELP_FCLOSE "\r\n fclose:\tclose a stream. To execute type `fclose <stream id>`\r\n"
#define HELP_CREATE "\r\n create:\tcreate a file or directory. To execute type `create /<dir path>/<file name> <permissions>`\r\n\t\tor 'create /<dir path>/<dir name>/ <permissions>'\r\n"
#define HELP_DELETE "\r\n delete:\tdelete a file or directory. To execute type `delete /<dir path>/<file name>` or\r\n\t\t'delete /<dir path>/<dir name>/'\r\n"
#define HELP_REWIND "\r\n rewind:\tset the location pointer of a file to its beginning. Must have open stream in correct\r\n\t\tmode. To execute type `rewind <stream id>`\r\n"
#define HELP_FPUTC "\r\n fputc:\t\tput a character in a file. Must have open stream in correct mode. To execute type \r\n\t\t'fputc <char> <stream id>'\r\n"
#define HELP_FGETC "\r\n fgetc:\t\tget the current character being pointed to in a file. Must have open stream in correct\r\n\t\tmode. To execute, type `fgetc <stream id>`\r\n"
#define HELP_PRINT "\r\n ls:\t\tprint the files in a directory. To execute, type `ls /<dir path>/`\r\n"
#define HELP_FPUR "\r\n purge:\t\tpurge the contents of a file. To execute, type `purge <file name>`\r\n"

#define HELP_SER2LCD "\r\n ser2lcd:\t continuously copy characters from serial input to LCD. Ends on a ^D (control-D) input \r\n\t\tcharacter. To execute type `ser2lcd'\r\n"
#define HELP_TOUCH "\r\n touch2led:\t continuously copy from each touch sensor to the corresponding LED. End when all four \r\n\t\ttouch sensors are \"depressed\". To execute type `touch2led`\r\n"
#define HELP_POT "\r\n pot2ser:\t continuously output the value of the potentiomemter to the serial device as a decimal number.\r\n\t\t End when SW1 is depressed. To execute type 'pot2ser'\r\n"
#define HELP_THERM "\r\n therm2ser:\t continuously output the value of the thermistor to the serial device as a decimal number.\r\n\t\tEnd when SW1 is depressed. To execute type 'therm2ser'\r\n"
#define HELP_PB2LED "\r\n pb2led:\t continuously copy from SW1 to orange LED and SW2 to yellow LED. End when both SW1 and SW2\r\n\t\tare depressed. To execute, type `pb2led`\r\n"
#define HELP_FLASH "\r\n flashled:\t flash the orange LED on and off approximately every half a second (the LED will light\r\n\t\tapproximately once a second). End when SW1 is depressed. To execute, type `flashled`\r\n"
#define HELP_MULTI "\r\n multitask:\t creates and runs 3 new processes that do three different tasks. To execute, type `multitask`\r\n"
#define HELP_FLASHGB_START "\r\n spawnFlashGB:\t spawns a process to alternate the flashing of the green and blue LEDs. Can\r\n\t\tonly be run by admin. To execute, type `spawnFlashGB`\r\n"
#define HELP_FLASHGB_STOP "\r\n killFlashGB:\t kills the process that alternates the flashing of the green and blue LEDs. Can\r\n\t\tonly be run by admin. To execute, type `killFlashGB`\r\n"

#define HELP_SPAWN "\r\n spawn:\t\tspawns a new process that either runs flashled, touch2led, or pb2led. Can also be used to\r\n\t\tspawn a process that runs the flashing blue and green LEDs (requires killFlashGB to terminate).\r\n\t\tCan only be run by admin. To execute, type `spawn <command>` or `spawn flashGB`\r\n"
#define HELP_USR_ADD "\r\n adduser:\t add a new user or get list of users. To execute type 'adduser <user name> <group> <password>'\r\n\t\tor 'adduser'\r\n"
#define HELP_USR_RM "\r\n remuser:\t remove a user. To execute type 'remuser <user name>'\r\n"
#define HELP_LOG_OUT "\r\n logout:\t logout of, but not exit, the shell. To execute, type `logout`\r\n"
#define HELP_LOG_REW "\r\n logrewind:\t set the location pointer of the security log to its beginning. To execute, type `logrewind`\r\n"
#define HELP_LOG_PUR "\r\n logpurge:\t purge the contents of the security log. To execute, type `logpurge`\r\n"
#define HELP_LOG_RD "\r\n logread:\t read the contents of the security log either one line at a time or in its entirety. To execute,\r\n\t\ttype `logread` or `logread all`\r\n"
#define HELP_EXIT "\r\n exit:\tterminates the shell. To execute, type `exit`.\r\n"


#endif /* INCLUDES_HELP_H_ */
