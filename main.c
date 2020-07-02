#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parser.h"

int main(int argc, char** argv) {
    if (argc != 2) {
	printf("No code file provided, exiting.\n");
	return EXIT_FAILURE;
    }
    CodeBuffer cb;
    if (!buffer_code_file(&cb, argv[1])) {
	printf("Failed to buffer code file %s, exiting.\n", argv[1]);
	return EXIT_FAILURE;
    }
    TokenNode* head = tokenize(&cb);
    for (TokenNode* node = head; node; node = node->next) {
	print_token(node->token);
	if (node->next)
	    printf("\n");
    }
	
    return EXIT_SUCCESS;
}
