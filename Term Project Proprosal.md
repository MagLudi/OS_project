#Project proposal:
There are several improvements that can be made to the OS so that it is more secure. One important issue is the lack of security to prevent data from being read or manipulated by a third party. Another is the ability to log any security related incidents. In this project, both of these problems will be addressed.

One important issue is the lack of permissions. At the moment, anyone can read and/or write to a file/directory. It currently doesn't matter since there is only one process (and therefore one user) active at a time. However, this will change soon with the sixth problem set. As a result, it is imperative that permissions are set and enforced so that the wrong people aren't accessing protected information.

Another problem is with the clock. Currently, it must be initialized on startup every single time. Fortunately there is an oscillator device that comes with the Kinetis tower that can fix that issue. Using this oscillator to set the clock will remove the need to re-initialize on every startup. This will make time-stamping a possibility since the clock will remain accurate and consistent.

With the two above issues resolved, it is possible to set up an event logging system designed to keep track of what security issues regarding read/write access occur. This information can then be used to help improve the security of the OS.

##Permissions:
This feature will result in some code clean up since certain helper functions will no longer be needed. Implementation for this part will be broken down into two steps:

###Step 1:
Two boolean flags will be added (one for reading and one for writing) to indicate if you can read/write to file. The will be stored in the Inode data struct. These will be checked with the fgetc and fputc methods before execution and will return the appropriate error message. As a result, the program will expect to see an `r`, `w` or both on creation of the file with the `fopen` or `create` commands.

###Step 2:
A new user data structure will be added to hold the follow information:

- User ID
- List of groups the user belongs to
- Boolean flag if current user
- Password (optional)

This data structure will initially be stored in a linked list. At this point, it will primarily be used as an aid for the next step. If step 4 is reached, more will be done with this data structure.

###Step 3:
A new permission data structure will be created with the following:

- User ID: creator, group (will likely have custom feature with a string called `groupName`), everyone
- Two boolean flags (one for reading and one for writing)

In order to allow for multiple user types with multiple permissions, a list of this new data structure will be stored in the Inode data struct. The code will be modified to first check the ID before checking the permissions.

On creation of a new file, if the user is not specified, then the default will be that everyone has the permissions given. If the creator is specified with their own permissions, then they will have the option of giving other groups access to the file as they see fit.

###Step 4 (Optional):
An additional boolean flag will be added for execution (`x`). Additional checks will be made to ensure proper handling of this permission. The list of users will also be stored in a more secure method. Add login capabilities.

##Oscillator clock:
Instead of fully replacing the current date command and it's helper functions, I would have a second date command for the oscillator clock. The functions that would be written for it would be in a separate C file (`oscClock` for now). 

##Event logging:
The focus of event logging is to record to the millisecond when certain users attempt to read/write to a device/file/directory that they don't have access to. The initial set up will store the information in a linked list that's saved to memory. In the next step, it will be modified so that the event logs will be saved on a file with restricted read/write access.

###Step 1:
A new logging data structure will be created that will contain the following:

- Time-stamp: Month Day, Year Hour:Minute:Sec.Milli
- Event type: failed read access, failed write access
- File/Dev/Dir name
- User ID (if step 2 of permissions has been successfully completed)

When a user attempts to access a file/dev/dir that it doesn't have the correct permission for, a call will be made to a logging function, where a new instance of the data structure will be created. This will then be added to the linked list so that it can be accessed at a later point in time should someone want to do a security check of the system.

To demo this particular feature, a new command called `printEvents` will be added to display what security issues occurred when. It will traverse the linked list/file and print the data in the following format:

`<timestamp>	Security event:<event>	File:<file/dir/dev name>	UID:<uid>`

###Step 2:

Instead of storing the security events in a linked list, it will be stored in a file with permissions set that only a sys admin can access the file. With this change a `clear` command will be added. This will clear out all information in the log file as long as the person giving the command has sys admin privileges. The command `printEvents` will be modified to handle the new storage medium.


