#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Token types
typedef enum {
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_PRINT,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_PLUS,
    TOKEN_SYMBOL_SEMICOLON,
    TOKEN_SYMBOL_LPAREN,
    TOKEN_SYMBOL_RPAREN,
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char value[50];
} Token;

// Symbol table entry
typedef struct {
    char name[50];
    int value;
    int isDeclared;
} Symbol;

// Global variables for parsing
Token currentToken;
FILE *inputFile;
int hasError = 0;
int syntaxErrors = 0;
int semanticErrors = 0;
int lineNumber = 1;

// Symbol table
Symbol symbolTable[100];
int symbolCount = 0;

// Function prototypes
void getNextToken();
void program();
void statement();
void declaration();
void assignment();
void printStmt();
int expression();
int term();
void syntaxError(const char *message);
void semanticError(const char *message);
int isVariableDeclared(const char *varName);
int addSymbol(const char *varName);
int findSymbol(const char *varName);
int getSymbolValue(const char *varName);
void setSymbolValue(const char *varName, int value);
void printSymbolTable();

// Get next token from input
void getNextToken() {
    char ch;
    char buffer[50];
    int i;

    // Skip whitespace and track line numbers
    while ((ch = fgetc(inputFile)) != EOF && isspace(ch)) {
        if (ch == '\n') {
            lineNumber++;
        }
    }

    if (ch == EOF) {
        currentToken.type = TOKEN_EOF;
        strcpy(currentToken.value, "EOF");
        return;
    }

    // Identifier or Keyword
    if (isalpha(ch)) {
        i = 0;
        buffer[i++] = ch;

        while (isalnum(ch = fgetc(inputFile))) {
            buffer[i++] = ch;
            if (i >= 49) {  // Buffer overflow protection
                buffer[49] = '\0';
                syntaxError("Identifier too long (max 49 characters)");
                break;
            }
        }

        buffer[i] = '\0';
        ungetc(ch, inputFile);

        if (strcmp(buffer, "int") == 0) {
            currentToken.type = TOKEN_KEYWORD_INT;
        } else if (strcmp(buffer, "print") == 0) {
            currentToken.type = TOKEN_KEYWORD_PRINT;
        } else {
            currentToken.type = TOKEN_IDENTIFIER;
        }
        strcpy(currentToken.value, buffer);
    }
    // Number
    else if (isdigit(ch)) {
        i = 0;
        buffer[i++] = ch;

        while (isdigit(ch = fgetc(inputFile))) {
            buffer[i++] = ch;
            if (i >= 49) {  // Buffer overflow protection
                buffer[49] = '\0';
                syntaxError("Number too long");
                break;
            }
        }

        buffer[i] = '\0';
        ungetc(ch, inputFile);

        currentToken.type = TOKEN_NUMBER;
        strcpy(currentToken.value, buffer);
    }
    // Operators and Symbols
    else if (ch == '=') {
        currentToken.type = TOKEN_OPERATOR_ASSIGN;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else if (ch == '+') {
        currentToken.type = TOKEN_OPERATOR_PLUS;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else if (ch == ';') {
        currentToken.type = TOKEN_SYMBOL_SEMICOLON;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else if (ch == '(') {
        currentToken.type = TOKEN_SYMBOL_LPAREN;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else if (ch == ')') {
        currentToken.type = TOKEN_SYMBOL_RPAREN;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else {
        currentToken.type = TOKEN_UNKNOWN;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
        char errorMsg[100];
        sprintf(errorMsg, "Unknown character '%c' (ASCII: %d)", ch, (int)ch);
        syntaxError(errorMsg);
    }
}

void syntaxError(const char *message) {
    printf("\n[SYNTAX ERROR] Line %d: %s\n", lineNumber, message);
    printf("  Current token: '%s'\n", currentToken.value);
    syntaxErrors++;
    hasError = 1;
}

void semanticError(const char *message) {
    printf("\n[SEMANTIC ERROR] Line %d: %s\n", lineNumber, message);
    printf("  Current token: '%s'\n", currentToken.value);
    semanticErrors++;
    hasError = 1;
}

// Check if variable is declared
int isVariableDeclared(const char *varName) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, varName) == 0) {
            return 1;
        }
    }
    return 0;
}

int findSymbol(const char *varName) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, varName) == 0) {
            return i;
        }
    }
    return -1;
}

int getSymbolValue(const char *varName) {
    int idx = findSymbol(varName);
    if (idx == -1 || !symbolTable[idx].isDeclared) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' used before declaration", varName);
        semanticError(errorMsg);
        return 0;
    }
    return symbolTable[idx].value;
}

void setSymbolValue(const char *varName, int value) {
    int idx = findSymbol(varName);
    if (idx == -1 || !symbolTable[idx].isDeclared) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' assigned before declaration", varName);
        semanticError(errorMsg);
        return;
    }
    symbolTable[idx].value = value;
}

// Add symbol to symbol table
int addSymbol(const char *varName) {
    // Check for duplicate declaration
    if (isVariableDeclared(varName)) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' already declared", varName);
        semanticError(errorMsg);
        return 0;
    }
    
    if (symbolCount >= 100) {
        semanticError("Symbol table full (max 100 variables)");
        return 0;
    }
    
    strcpy(symbolTable[symbolCount].name, varName);
    symbolTable[symbolCount].isDeclared = 1;
    symbolTable[symbolCount].value = 0;
    symbolCount++;
    return 1;
}

void printSymbolTable() {
    printf("\n=== Symbol Table ===\n");
    if (symbolCount == 0) {
        printf("(empty)\n");
    } else {
        for (int i = 0; i < symbolCount; i++) {
            printf("  Variable: %s = %d\n", symbolTable[i].name, symbolTable[i].value);
        }
    }
    printf("====================\n");
}

// CFG: program -> statement*
void program() {
    printf("=== Starting Parse ===\n\n");
    getNextToken();
    
    while (currentToken.type != TOKEN_EOF && !hasError) {
        statement();
    }
    
    printf("\n=== Parse Complete ===\n");
    printf("Syntax Errors: %d\n", syntaxErrors);
    printf("Semantic Errors: %d\n", semanticErrors);
    
    if (!hasError) {
        printf("Status: SUCCESS\n");
        printSymbolTable();
    } else {
        printf("Status: FAILED\n");
    }
}

// CFG: statement -> declaration | assignment | printStmt
void statement() {
    if (currentToken.type == TOKEN_KEYWORD_INT) {
        declaration();
    } else if (currentToken.type == TOKEN_KEYWORD_PRINT) {
        printStmt();
    } else if (currentToken.type == TOKEN_IDENTIFIER) {
        assignment();
    } else if (currentToken.type == TOKEN_UNKNOWN) {
        syntaxError("Unexpected character in statement");
        getNextToken();  // Try to recover
    } else {
        syntaxError("Expected statement (declaration, assignment, or print)");
        getNextToken();  // Try to recover
    }
}

// CFG: declaration -> 'int' IDENTIFIER '=' expression ';'
void declaration() {
    printf("Parsing declaration...\n");
    char varName[50];
    
    if (currentToken.type != TOKEN_KEYWORD_INT) {
        syntaxError("Expected 'int' keyword");
        return;
    }
    getNextToken();
    
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("Expected identifier after 'int'");
        while (currentToken.type != TOKEN_SYMBOL_SEMICOLON && 
               currentToken.type != TOKEN_EOF) {
            getNextToken();
        }
        if (currentToken.type == TOKEN_SYMBOL_SEMICOLON) {
            getNextToken();
        }
        return;
    }
    
    strcpy(varName, currentToken.value);
    getNextToken();
    
    // Add to symbol table (semantic check)
    addSymbol(varName);
    
    if (currentToken.type != TOKEN_OPERATOR_ASSIGN) {
        syntaxError("Expected '=' operator after identifier");
        return;
    }
    getNextToken();
    
    int value = expression();
    
    if (currentToken.type != TOKEN_SYMBOL_SEMICOLON) {
        syntaxError("Expected ';' at end of declaration");
        return;
    }
    setSymbolValue(varName, value);
    getNextToken();
}

// CFG: assignment -> IDENTIFIER '=' expression ';'
void assignment() {
    printf("Parsing assignment...\n");
    char varName[50];
    
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("Expected identifier");
        return;
    }
    
    strcpy(varName, currentToken.value);
    
    // Check if variable is declared (semantic check)
    if (!isVariableDeclared(varName)) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' used before declaration", varName);
        semanticError(errorMsg);
    }
    
    getNextToken();
    
    if (currentToken.type != TOKEN_OPERATOR_ASSIGN) {
        syntaxError("Expected '=' operator");
        return;
    }
    getNextToken();
    
    int value = expression();
    
    if (currentToken.type != TOKEN_SYMBOL_SEMICOLON) {
        syntaxError("Expected ';' at end of assignment");
        return;
    }
    setSymbolValue(varName, value);
    getNextToken();
}

// CFG: printStmt -> 'print' '(' IDENTIFIER ')' ';'
void printStmt() {
    printf("Parsing print statement...\n");
    char varName[50];
    
    if (currentToken.type != TOKEN_KEYWORD_PRINT) {
        syntaxError("Expected 'print' keyword");
        return;
    }
    getNextToken();
    
    if (currentToken.type != TOKEN_SYMBOL_LPAREN) {
        syntaxError("Expected '(' after print");
        return;
    }
    getNextToken();
    
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("Expected identifier inside print()");
        while (currentToken.type != TOKEN_SYMBOL_RPAREN && 
               currentToken.type != TOKEN_EOF) {
            getNextToken();
        }
        if (currentToken.type == TOKEN_SYMBOL_RPAREN) {
            getNextToken();
        }
        return;
    }
    
    strcpy(varName, currentToken.value);
    
    // Check if variable is declared (semantic check)
    if (!isVariableDeclared(varName)) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' used in print() before declaration", varName);
        semanticError(errorMsg);
    }
    
    int value = getSymbolValue(varName);
    getNextToken();
    
    if (currentToken.type != TOKEN_SYMBOL_RPAREN) {
        syntaxError("Expected ')' after identifier");
        return;
    }
    getNextToken();
    
    if (currentToken.type != TOKEN_SYMBOL_SEMICOLON) {
        syntaxError("Expected ';' at end of print statement");
        return;
    }
    printf("Result: %d\n", value);
    getNextToken();
}

// CFG: expression -> term ('+' term)*
int expression() {
    int left = term();
    
    while (currentToken.type == TOKEN_OPERATOR_PLUS) {
        getNextToken();
        int right = term();
        left = left + right;
    }
    return left;
}

// CFG: term -> IDENTIFIER | NUMBER
int term() {
    if (currentToken.type == TOKEN_IDENTIFIER) {
        int value = getSymbolValue(currentToken.value);
        getNextToken();
        return value;
    } else if (currentToken.type == TOKEN_NUMBER) {
        int value = atoi(currentToken.value);
        getNextToken();
        return value;
    } else {
        syntaxError("Expected identifier or number in expression");
        getNextToken();
        return 0;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./parser inputfile\n");
        return 1;
    }

    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        printf("Error opening file: %s\n", argv[1]);
        return 1;
    }

    program();
    
    fclose(inputFile);

    return hasError ? 1 : 0;
}