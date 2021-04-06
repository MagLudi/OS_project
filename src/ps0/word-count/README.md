# CSCI-E92 Problem Set 0:

The program word-count uses file I/O to read a text file whose name is specified on the command line
to the program. The program will parse the input file into words and will use a linked list data
structure to keep track of the number of occurrences of each unique word that is found in the input
file. Words will be delimited by white space which is defined to be any mix of spaces (' '),
horizontal tabs ('\t'), newlines ('\n'), or carriage-returns ('\r'), including multiple occurrences
of white space. Special characters (i.e., punctuation, hyphens, etc) are treated as part of the words.
It is also case sensitive. Upon reaching end-of-file, the program will output to stdout:
 	(1) the number of lines in the input file (include blank lines in the count of the number of lines)
	(2) the total number of words in the input file (`not` the number of unique words)
	(3) an alphabetical list of each unique word in the input file along with the number of times that
	    word appears in the file.

## Files:

- README.md
- Makefile
- linkedList.h
- linkedList.c
- word-count.c

## Building:

To build using 'make':

    make

## Run

To run the program with an input file from the terminal:

    ./wordCount  <file name>

If the file is not is the same directory as the executable, then make sure to include the file path.

On execution, the following output will be desplaiyed:

   `Lines: ` <number of lines>
   `Words: ` <number of total words>
   `Word count: ` <alphabetical list of each unique word and their frequency>

To save the output, add the following command on execution:

    > <output file name>

## Test Files

The following test files are provided:

- test.txt
- simple.c
- asignment.txt

Output files of the results are also provided:

- test_out.txt
- simple_out.c
- asignment_out.txt

## Notes

The files linkedList.c and linkedList.h are taken from my personal library of utilities.
The data structure for the nodes in the linked list `linkedListNode` has been modified specificaly
for the purposes of this assignment so that it stores a word and a counter seperatly instead of
an object.
