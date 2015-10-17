#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dbg.h"
#include <time.h>
#include <math.h>

#define BSIZE 100
#define MAX_WORD 100

// Node structure for binary search tree
typedef struct node {
	char *str;
	int count;
	int size;
	int *lines;
	struct node *left;
	struct node *right;
} node;

// length of noisewords list.
int numnoisewords = 65;

// List of words that are uninteresting and would be slow to search and display line
// numbers for.
char *noisewords[65] = {
	"the", "I", "a", "and", "it", "of", "that", "to", "this", "from", "be",
	"in", "have", "for", "not", "on", "no", "with", "yes", "he", "she", "as",
	"you", "do", "at", "but", "his", "her", "hers", "by", "they", "them", "we", "get",
	"an", "or", "will", "so", "my", "one", "all", "would", "their", "go",
	"up", "down", "out", "me", "when", "be", "who", "left", "both", "let", "can",
	 "can't", "give", "there", "they're", "may", "might", "are", "am", "man", "woman"
};

node *insert(node *n, char *str);
node *nodealloc();

char buffer[BSIZE];
int bptr;

int curline;
FILE *file;
node *tree;

// The getch, ungetch, ad getword methods are modified versions of those printed
// in The C Programming language, 2nd Edition.
int getch(FILE *fs) {
	return ((bptr != 0) ? buffer[--bptr] : fgetc(fs));
}

// Modified from code in The C Programming language, 2nd Edition
void ungetch(int c) {
	if (bptr >= BSIZE) {
		printf("Buffer overflow.");
	} else {
		buffer[bptr++] = c;
		if (c == '\n') {
			curline--;
		}
	}
}

// Modified from code in The C Programming language, 2nd Edition
int getword(char *str, int max, FILE *fs) {
	int c;
	char *word = str;

	while (isspace(c = getch(fs))) {
		if (c == '\n')
			curline++;
	}

	if (c != EOF) {
		*word++ = c;
	}

	if (!isalpha(c)) {
		*word++ = '\0';
		return c;
	}

	for (; max-- > 0; word++) {
		if (!isalnum(*word = getch(fs))) {
			ungetch(*word);
			break;
		}
	}
	*word = '\0';
	return word[0];
}

// Returns 1 if the word is in the noiseword list, 0 if not and -1 on error.
int isnoiseword(char *word) {
	check(word != NULL, "Invalid word");
	for (int i = 0; i < numnoisewords; i++) {
		if (!strcasecmp(word, noisewords[i])) {
			return 1;
		}
	}
	return 0;
error:
	return -1;
}

// Reallocates memory for the node struct when it needs to be resized
int resize(node *n) {
	check(n != NULL, "Invalid resize.");
	check_mem(n->lines);
	int *temp = (int *) realloc(n->lines, n->size * sizeof(int));
	check_mem(temp);
	n->lines = temp;
	return 0;

error:
	return -1;
}

// Frees all memory allocated to node.
int nodedelete(node *n) {
	check(n != NULL, "Invalid delete.");
	if (n->lines) {
		free(n->lines);
	}
	free(n);
	return 0;

error:
	return -1;
}

// Allocates memory on heap for node
node *nodealloc(void) {
	node *n = malloc(sizeof(struct node));
	check_mem(n);
	n->lines = (int *) malloc(10 * sizeof(int));
	check_mem(n->lines);
	return n;

error:
	nodedelete(n);
	return NULL;
}

// Inserts the provided word into the tree and increments the count. Also, stores
// that word as occuring on the current line.
node *insert(node *n, char *str) {
	int cmp;
	if (!n) {
		n = nodealloc();
		n->str = strdup(str);
		n->count = 1;
		n->size = 10;
		n->left = NULL;
		n->right = NULL;
		(n->lines)[0] = curline;
	} else if (!(cmp = strcasecmp(str, n->str))) {
		n->count++;
		if (n->count >= n->size) {
			n->size *= 2;
			resize(n);
			if (!n->lines) {
				return NULL;
			}
		}
		(n->lines)[n->count - 1] = curline;
	} else if (cmp < 0) {
		n->left = insert(n->left, str);
	} else {
		n->right = insert(n->right, str);
	}
	return n;
}

// Returns the node with the given word or NULL if not found.
node *find(node *n, char *str) {
	int cmp;
	if (!n) {
		return NULL;
	} else if (!(cmp = strcasecmp(str, n->str))) {
		return n;
	} else if (cmp < 0) {
		return find(n->left, str);
	} else {
		return find(n->right, str);
	}
	return NULL;
}

int main(int argc, char *argv[]) {
	// Check correct number of arguments
	if (argc != 2) {
		printf("Usage: ./textFileSearch <filename>\n");
		return -1;
	}

	// Prints help message
	char *argument = argv[1];
	if (!strcmp(argument, "-h")) {
		printf("Usage: ./textFileSearch <filename>\nWhile searching simply Enter "
		"the word to be searched or a '-' followed by either a 'q' or a 'c' to quit or "
		"continue searching.\n");
		return -1;
	}

	file = fopen(argv[1], "r");
	check_mem(file);

	tree = NULL;
	char curword[MAX_WORD];
	curline = 1;
	int rc = 0;

	// Clocks to measure performance
	clock_t start;
	clock_t end;
	start = clock();

  	// Reads from provided file and inserts every word into binary tree.
	while ((rc = getword(curword, MAX_WORD, file)) != EOF) {
		if (isalpha(*curword)) {
			tree = insert(tree, curword);
		}
	}
	end = clock();
	double elapsed = (double) (end - start) / CLOCKS_PER_SEC;
	printf("Indexing all of \'%s\' took: %f seconds.\n", argv[1], elapsed);

	char input[MAX_WORD];
	rc = 0;
	printf("Enter a word to search. Type '-' for options.\n>");

	// Loop to prompt for word to be searched and print data if found
	while((rc = getword(input, MAX_WORD, stdin)) != EOF) {
		if (!strcmp(input, "-")) {
			printf(">Type q to quit or c to continue searching.\n>");
			if ((rc = getword(input, MAX_WORD, stdin)) != EOF) {
				if (!strcmp(input, "q")) {
					printf(">Thanks for searching.");
					break;
				} else if (!strcmp(input, "c")) {
					printf("Enter a word to search. Type '-' for options.\n>");
				}
				continue;
			}
			break;
		}

		// Do not search for word if is in noiseword list
		if (isnoiseword(input)) {
			printf("The word \'%s\' is removed as it occurs too often.\n", input);
			printf(">");
			continue;
		}

		// Display node information
		node *n = find(tree, input);
		if (n) {
			printf("The word \'%s\' appears %d times.\n", input, n->count);
			printf("On lines:\n");
			for (int i = 0; i < n->count; i++) {
				printf("%d\n", n->lines[i]);
			}
		} else {
			printf("The word \'%s\' does not appear.\n", input);
		}
		printf(">");
	}
	printf("\n");

	// Free memory and return.
	nodedelete(tree);
	fclose(file);
	return 0;

error:
	return -1;
}
