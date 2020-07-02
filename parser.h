#pragma once
#include <stdbool.h>

typedef enum {
    UNKNOWN = 0,
    IDENTIFIER,
    KEYWORD,
    OPERATOR,
    SEPARATOR,
    LITERAL,
    COMMENT,
} TokenKind;

typedef enum {
    KW_IF,
    KW_ELSE,
    KW_FOR,
    KW_WHILE,
    KW__COUNT
} Keyword;
extern const char* keyword_strings[];

typedef enum {
    OP_PLUS,
    OP_MINUS,
    OP_TIMES,
    OP_DIV,
    OP_MOD,
    OP_EQUAL,
    OP_EQUAL_EQUAL,
    OP_BANG_EQUAL,
    OP__COUNT
} Operator;
extern const char* operator_strings[];

enum {
    SEP_SEMICOLON,
    SEP_COLON,
    SEP_APOSTROPHE,
    SEP_QUOTE,
    SEP_LPAREN,
    SEP_RPAREN,
    SEP_LBRACKET,
    SEP_RBRACKET,
    SEP_LBRACE,
    SEP_RBRACE,
    SEP__COUNT
};
extern const char* separator_strings[];

typedef union {
    int data;
    char* string;
} TokenValue;

typedef struct {
    char** lines;
    long line_count;
    long* line_lengths;
} CodeBuffer;

typedef struct {
    // Line and column info
    // *1 points to the first character of the token
    // *2 points to the character just after the last character of the token
    int l1;
    int c1;
    int l2;
    int c2;

    TokenKind kind;
    TokenValue value;
} Token;

typedef struct TokenNode {
    Token token;
    struct TokenNode* next;
} TokenNode;


bool buffer_code_file(CodeBuffer* buffer, const char* fpath);
TokenNode* tokenize(const CodeBuffer* buffer);
