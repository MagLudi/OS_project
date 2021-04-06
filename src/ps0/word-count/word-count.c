/* This program uses file I/O to read a text file whose name is specified on the command line
 * to the program. The program will parse the input file into words and will use a linked
 * list data structure to keep track of the number of occurrences of each unique word that
 * is found in the input file. Words will be delimited by white space which is defined to
 * be any mix of spaces (' '), horizontal tabs ('\t'), newlines ('\n'), or
 * carriage-returns ('\r'), including multiple occurrences of white space. Special characters
 * (i.e., punctuation, hyphens, etc) are treated as part of the words. It is also case sensitive.
 * Upon reaching end-of-file, the program will output to stdout:
 *	(1) the number of lines in the input file (include blank lines in the count of the
 * 	    number of lines)
 *	(2) the total number of words in the input file (*not* the number of unique words)
 *	(3) an alphabetical list of each unique word in the input file along with the number
 *	    of times that word appears in the file.
 *
 * Functions:
 *		main(): main routine. Takes in a file name
 *		insert(): updates the list of unique words. Will either add a new word to the list
 *			  or update the counter of a word currently on the list.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "linkedList.h"

#define MAXSTRING	101

/*
 * This function inserts a word into a sorted linked list of words. If the word is
 * not yet on the list, it is inserted/added in alphabetical order with the count
 * set to 1. Otherwise, the count is simply incremented. This is also case sensitive
 * where uppercase letters take precedence over lowercase. The parameters it takes
 * are a char pointer representing a word and a boolean pointer. The purpose of the
 * boolean is to signal the main() function if it needs to free space in the char
 * pointer that stores the words. It only does so if the word passed in is already in
 * the linked list.
 */
void insert(char *str_p, bool *unique_p) {
	/* search for the word */
	llLnkdListNode_t *n_p;
	bool added = false;
	for (n_p = llList.firstNode_p; (n_p != NULL) && !added; n_p = n_p->next_p) {
		/* comparing each character in both words so that new word gets added
		 * alphabetically as soon as the difference is found
		 */
		unsigned int i;
		bool match = true;
		for (i = 0; match && (str_p[i] != '\0'); i++) {
			if (str_p[i] < n_p->word_p[i]) {
				/* new word is ahead of current word so adding it before the current */
				llInsertLink_p(str_p, 1, n_p->prev_p, &llList);
				match = false;
				added = true;
			} else if (str_p[i] > n_p->word_p[i]) {
				/* new word is farther down the list */
				match = false;
			}
		}

		if (match) {
			if (n_p->word_p[i] != '\0') {
				/* word is actually a new word, so being placed before the current */
				llInsertLink_p(str_p, 1, n_p->prev_p, &llList);
			} else {
				/* word found, update counter */
				n_p->count++;
				*unique_p = false;
			}
			added = true;
		}
	}

	/* add word since it is last alphabetically */
	if (!added) {
		llAppendLink_p(str_p, 1, &llList);
	}

}


/* main program. Takes in the standard arguments. Second argument contains the name of the file  to open.
 * Prints out the number of lines, words, and the list of unique words and their frequencies.
 * Returns 0 on completion.
 */
int main(int argc, char **argv) {
	FILE *fp;
	char *wrd_p = NULL;
	int cnt = 0;
	llLnkdListNode_t *node_p = NULL;

	llCreateLnkdList(&llList);

	/* read all the words in the file */
	fp = fopen(argv[1], "r");

	/* validate the input file */
	if (feof(fp) || ferror(fp)){
		llStatusEXIT(llFAIL,"\nll:: input file is empty or corrupted ");
 	} 

	/* creating the first node */
	wrd_p = (char *) calloc(MAXSTRING, 1);
	if (fscanf(fp, "%100s", wrd_p) != EOF) {
		node_p = llAppendLink_p(wrd_p, 1, &llList);
		cnt++;
	}

	/* traverse file to count the number of words and record all unique words and their frequency*/
	wrd_p = (char *) calloc(MAXSTRING, 1);
	while (fscanf(fp, "%1000s", wrd_p) != EOF) {
		bool unique = true;
		insert(wrd_p, &unique);
		if(!unique){
			free(wrd_p);
		}
		cnt++;
		wrd_p = (char *) calloc(MAXSTRING, 1);
	}


	/* get number of lines */
	rewind(fp);
	int line = 0;
	char *c_p = (char *) calloc(5, 1);
	while (fscanf(fp, "%c", c_p) != EOF) {
		if (*c_p == '\n'){
			line++;
		}
	}

	fclose(fp);

	/* print the results */
	printf("Lines: %d\n", line);
	printf("Words: %d\n", cnt);
	printf("Word count: \n");
	for (node_p = llList.firstNode_p; node_p != NULL; node_p = node_p->next_p) {
		printf("  %s: %d\n", node_p->word_p, node_p->count);
	}
	return 0;
}

