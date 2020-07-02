#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024
#define ERROR(msg, ...) fprintf(stderr, "Error : " msg "\n", ## __VA_ARGS__);
#define WARNING(msg, ...) fprintf(stderr, "Warning : " msg "\n", ## __VA_ARGS__);

const char* keyword_strings[] = {
    "if",
    "else",
    "for",
    "while",
};

const char* operator_strings[] = {
    "+",
    "-",
    "*",
    "/",
    "%",
    "=",
    "==",
    "!=",
};

const char* separator_strings[] = {
    ";",
    ",",
    "'",
    "\"",
    "(",
    ")",
    "[",
    "]",
    "{",
    "}",
};

void parser_error(const char* message, int line, int col) {
    printf("Parser error on line %d, column %d : %s\n", line + 1, col + 1, message);
}

void free_token_list(TokenNode* head) {
    TokenNode* node = head;
    while (node) {
	TokenNode* next = node->next;
	free(node);
	node = next;
    }
}

bool buffer_code_file(CodeBuffer* buffer, const char* fpath) {
    FILE* input_file = fopen(fpath, "r");
    if (!input_file) {
	return false;
    }
    buffer->line_count = 0;
    int c;
    int prevc = EOF;
    while ((c = fgetc(input_file)) != EOF) {
	if (c == '\n')
	    buffer->line_count++;
	prevc = c;
    }
    if (prevc == EOF) {
	WARNING("Empty file '%s'", fpath);
	buffer->lines = NULL;
	buffer->line_lengths = NULL;
	buffer->line_count = 0;
	return true;
    } else if (prevc != '\n') {
	WARNING("Missing trailing newline in file %s", fpath);
	buffer->line_count++;
    }
    rewind(input_file);

    buffer->lines = malloc(sizeof(char*) * buffer->line_count);
    buffer->line_lengths = malloc(sizeof(long) * buffer->line_count);
    char line_buffer[MAX_LINE_LENGTH];
    int i = 0;
    while (fgets(line_buffer, MAX_LINE_LENGTH, input_file)) {
	buffer->lines[i] = malloc(strlen(line_buffer) + 1);
	strcpy(buffer->lines[i], line_buffer);
	// Remove trailing newline : 
	buffer->lines[i][strcspn(buffer->lines[i], "\n")] = '\0';
	buffer->line_lengths[i] = strlen(buffer->lines[i]);
	i++;
    }
    if (i != buffer->line_count) {
	ERROR("Failed to parse file '%s'", fpath);
	return false;
    }

    return true;
}

bool startswith(const char* s, const char* prefix) {
    int i = 0;
    while (s[i] && prefix[i]) {
	if (s[i] != prefix[i])
	    return false;
	i++;
    }
    return !prefix[i];
}

typedef struct {
    int l;
    int c;
} Cursor;

bool b_done(const CodeBuffer* buffer, const Cursor* cursor) {
    return cursor->l >= buffer->line_count;
}

char b_peekc(const CodeBuffer* buffer, const Cursor* cursor) {
    if (b_done(buffer, cursor))
	return '\0';
    return buffer->lines[cursor->l][cursor->c];
}

char b_getc(const CodeBuffer* buffer, Cursor* cursor) {
    if (b_done(buffer, cursor))
	return '\0';
    int c = buffer->lines[cursor->l][cursor->c];
    cursor->c++;
    while (cursor->c == buffer->line_lengths[cursor->l]) {
	cursor->l++;
	cursor->c = 0;
    }

    return c;
}

char b_lgetc(const CodeBuffer* buffer, Cursor* cursor) {
    if (b_done(buffer, cursor) || cursor->c == buffer->line_lengths[cursor->l])
	return '\0';
    int c = buffer->lines[cursor->l][cursor->c];
    cursor->c++;
    return c;
}

char b_lpeekc(const CodeBuffer* buffer, Cursor* cursor) {
    if (b_done(buffer, cursor) || cursor->c == buffer->line_lengths[cursor->l])
	return '\0';
    return buffer->lines[cursor->l][cursor->c];
}

void push_token(TokenNode** head, TokenNode** last, Token token) {
    TokenNode* node = malloc(sizeof(TokenNode));
    node->next = NULL;
    node->token = token;
    if (*last) {
	(*last)->next = node;
	*last = (*last)->next;
    } else {
	*last = *head = node;
    }
}

void b_skip_space(const CodeBuffer* cb, Cursor* cursor) {
    char c;
    while ((c = b_peekc(cb, cursor)) && isspace(c))
	b_getc(cb, cursor);
}

bool b_match(const CodeBuffer* cb, Cursor* cursor, const char* pattern) {
    int i = 0;
    Cursor prev_cursor = *cursor;
    char c;
    while ((c = b_getc(cb, cursor)) && *pattern && (c == *pattern))
	pattern++;

    if (*pattern) {
	*cursor = prev_cursor;
	return false;
    }
    return true;
}

bool b_match_any(const CodeBuffer* cb, Cursor* cursor, const char** patterns, int pattern_count, int* match) {
    for (int i = 0; i < pattern_count; i++) {
	if (b_match(cb, cursor, patterns[i])) {
	    *match = i;
	    return true;
	}
    }
    return false;
}

bool is_letter(char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool is_digit(char c) {
    return '0' <= c && c <= '9';
}

bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n';
}

TokenNode* tokenize(const CodeBuffer* cb) {
    TokenNode* result = NULL;
    TokenNode* last = NULL;

    Cursor cursor = {};
    // Skip empty lines at the beginning 
    while (cursor.l < cb->line_count && cursor.c == cb->line_lengths[cursor.l]) {
	cursor.l++;
	cursor.c = 0;
    }
    b_skip_space(cb, &cursor);

    while (!b_done(cb, &cursor)) {
	printf("%d:%d  %c\n", cursor.l, cursor.c, b_peekc(cb, &cursor));
	printf("Before skip space : %d:%d\n", cursor.l, cursor.c);
	b_skip_space(cb, &cursor);
	printf("After skip space : %d:%d\n", cursor.l, cursor.c);
	Token token;
	token.kind = UNKNOWN;
	token.l1 = cursor.l;
	token.c1 = cursor.c;
	int i;
	char* p;
	
	if (b_match(cb, &cursor, "//")) {
	    token.kind = COMMENT;
	    token.l2 = cursor.l;
	    token.c2 = cb->line_lengths[cursor.l];
	    cursor.l++;
	    cursor.c = 0;
	    int comment_length = cb->line_lengths[cursor.l] - cursor.c + 1;
	    token.value.string = malloc(comment_length);
	    strncpy(token.value.string, &cb->lines[cursor.l][cursor.c], comment_length);
	} else if (b_match_any(cb, &cursor, keyword_strings, KW__COUNT, &i)) {
	    token.kind = KEYWORD;
	    token.value.data = i;
	    token.l2 = cursor.l;
	    token.c2 = cursor.c;
	} else if (b_match_any(cb, &cursor, operator_strings, OP__COUNT, &i)) {
	    token.kind = OPERATOR;
	    token.value.data = i;
	    token.l2 = cursor.l;
	    token.c2 = cursor.c;
	} else if (b_match_any(cb, &cursor, separator_strings, SEP__COUNT, &i)) {
	    token.kind = SEPARATOR;
	    token.value.data = i;
	    token.l2 = cursor.l;
	    token.c2 = cursor.c;
	} else if (is_digit(cb->lines[cursor.l][cursor.c])
		   || cb->lines[cursor.l][cursor.c] == '.') {
	    token.kind = LITERAL;
	    int startcol = cursor.c;
	    while (cursor.c < cb->line_lengths[cursor.l]
		   && (is_digit(cb->lines[cursor.l][cursor.c])
		       || cb->lines[cursor.l][cursor.c] == '.')) {
		cursor.c++;
	    }
	    token.value.string = malloc(cursor.c - startcol + 1);
	    strncpy(token.value.string, &cb->lines[cursor.l][startcol], cursor.c - startcol);
	    token.value.string[cursor.c - startcol] = '\0';
	} else if (is_letter(cb->lines[cursor.l][cursor.c])
		   || cb->lines[cursor.l][cursor.c] == '_') {
	    token.kind = IDENTIFIER;
	    int startcol = cursor.c;
	    while (cursor.c < cb->line_lengths[cursor.l]
		   && (is_letter(cb->lines[cursor.l][cursor.c])
		       || cb->lines[cursor.l][cursor.c] == '_')) {
		cursor.c++;
	    }
	    token.value.string = malloc(cursor.c - startcol + 1);
	    strncpy(token.value.string, &cb->lines[cursor.l][startcol], cursor.c - startcol);
	    token.value.string[cursor.c - startcol] = '\0';
	}

	if (!token.kind) {
	    ERROR("Unknown token at %d:%d\n", token.l1 + 1, token.c1 + 1);
	    break;
	} else {
	    push_token(&result, &last, token);

	    // Make sure we end up in a valid cursor state
	    while (cursor.c == cb->line_lengths[cursor.l]) {
		cursor.l++;
		cursor.c = 0;
	    }
	}
	b_skip_space(cb, &cursor);
    }

    return result;
}
