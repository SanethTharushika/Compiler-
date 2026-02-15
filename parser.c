#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_DOUBLE,
    TOKEN_KEYWORD_PRINT,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_DECIMAL,
    TOKEN_OPERATOR_ASSIGN,
    TOKEN_OPERATOR_PLUS,
    TOKEN_OPERATOR_MINUS,
    TOKEN_OPERATOR_MULTIFLY,
    TOKEN_OPERATOR_DIVISION,    
    TOKEN_SYMBOL_SEMICOLON,
    TOKEN_SYMBOL_LPAREN,
    TOKEN_SYMBOL_RPAREN,
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char value[50];
} Token;

typedef struct {
    char name[50];
    int isInt;  
    int intValue;
    double doubleValue;
    int isDeclared;
} Symbol;

Token currentToken;
FILE *inputFile;
int hasError = 0;
int syntaxErrors = 0;
int semanticErrors = 0;
int lineNumber = 1;
int expressionCount = 0;
int suppressTokenPrint = 0;
int tokenPrintDeferred = 0;

static Symbol *symbolTable = NULL;
static int symbolCount = 0;
static int symbolCapacity = 0;

void getNextToken();
void getNextTokenSilently();
void program();
void statement();
void declaration();
void assignment();
void printStmt();
double expression();
double term();
double factor();

void syntaxError(const char *message);
void semanticError(const char *message);
const char *tokenTypeName(TokenType type);
void printToken();
int isVariableDeclared(const char *varName);
int addSymbol(const char *varName);
int findSymbol(const char *varName);
double getSymbolValue(const char *varName);
void setSymbolValue(const char *varName, double value, int isInt);
int addSymbolWithType(const char *varName, int isInt);
void printSymbolTable();
void initSymbolTable();
void freeSymbolTable();

void getNextToken() {
    char ch;
    char buffer[50];
    int i;

    while ((ch = fgetc(inputFile)) != EOF && isspace(ch)) {
        if (ch == '\n') {
            lineNumber++;
        }
    }

    if (ch == EOF) {
        currentToken.type = TOKEN_EOF;
        strcpy(currentToken.value, "EOF");
        if (!suppressTokenPrint) {
            printToken();
        }
        suppressTokenPrint = 0;
        tokenPrintDeferred = 0;
        return;
    }

    if (isalpha(ch)) {
        i = 0;
        buffer[i++] = ch;

        while (isalnum(ch = fgetc(inputFile))) {
            buffer[i++] = ch;
            if (i >= 49) {  
                buffer[49] = '\0';
                syntaxError("Identifier too long (max 49 characters)");
                break;
            }
        }

        buffer[i] = '\0';
        ungetc(ch, inputFile);

        if (strcmp(buffer, "int") == 0) {
            currentToken.type = TOKEN_KEYWORD_INT;
        } else if (strcmp(buffer, "double") == 0) {
            currentToken.type = TOKEN_KEYWORD_DOUBLE;
        } else if (strcmp(buffer, "print") == 0) {
            currentToken.type = TOKEN_KEYWORD_PRINT;
        } else {
            currentToken.type = TOKEN_IDENTIFIER;
        }
        strcpy(currentToken.value, buffer);
    } 
    else if (isdigit(ch)) {
        i = 0;
        buffer[i++] = ch;
        int hasDecimal = 0;

        while (isdigit(ch = fgetc(inputFile)) || (ch == '.' && !hasDecimal)) {
            if (ch == '.') {
                hasDecimal = 1;
            }
            buffer[i++] = ch;
            if (i >= 49) { 
                buffer[49] = '\0';
                syntaxError("Number too long");
                break;
            }
        }

        buffer[i] = '\0';
        ungetc(ch, inputFile);

        currentToken.type = hasDecimal ? TOKEN_DECIMAL : TOKEN_NUMBER;
        strcpy(currentToken.value, buffer);
    }
    
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
    else if (ch == '-') {
        currentToken.type = TOKEN_OPERATOR_MINUS;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else if (ch == '*') {
        currentToken.type = TOKEN_OPERATOR_MULTIFLY;
        currentToken.value[0] = ch;
        currentToken.value[1] = '\0';
    }
    else if (ch == '/') {
        currentToken.type = TOKEN_OPERATOR_DIVISION;
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

    if (suppressTokenPrint) {
        suppressTokenPrint = 0;
        tokenPrintDeferred = 1;
        return;
    }
    tokenPrintDeferred = 0;
    printToken();
}

void getNextTokenSilently() {
    suppressTokenPrint = 1;
    getNextToken();
}

const char *tokenTypeName(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD_INT: return "KEYWORD_INT";
        case TOKEN_KEYWORD_DOUBLE: return "KEYWORD_DOUBLE";
        case TOKEN_KEYWORD_PRINT: return "KEYWORD_PRINT";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_DECIMAL: return "DECIMAL";
        case TOKEN_OPERATOR_ASSIGN: return "OP_ASSIGN";
        case TOKEN_OPERATOR_PLUS: return "OP_PLUS";
        case TOKEN_OPERATOR_MINUS: return "OP_MINUS";
        case TOKEN_OPERATOR_MULTIFLY: return "OP_MULTIPLY";
        case TOKEN_OPERATOR_DIVISION: return "OP_DIVISION";
        case TOKEN_SYMBOL_SEMICOLON: return "SEMICOLON";
        case TOKEN_SYMBOL_LPAREN: return "LPAREN";
        case TOKEN_SYMBOL_RPAREN: return "RPAREN";
        case TOKEN_EOF: return "EOF";
        case TOKEN_UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

void printToken() {
    printf("[TOKEN] Line %d: %s ('%s')\n", lineNumber, tokenTypeName(currentToken.type), currentToken.value);
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
double getSymbolValue(const char *varName) {
    int idx = findSymbol(varName);
    if (idx == -1 || !symbolTable[idx].isDeclared) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' used before declaration", varName);
        semanticError(errorMsg);
        return 0;
    }
    return symbolTable[idx].isInt ? (double)symbolTable[idx].intValue : symbolTable[idx].doubleValue;
}

void setSymbolValue(const char *varName, double value, int isInt) {
    int idx = findSymbol(varName);
    if (idx == -1 || !symbolTable[idx].isDeclared) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' assigned before declaration", varName);
        semanticError(errorMsg);
        return;
    }
    if (isInt) {
        symbolTable[idx].intValue = (int)value;
    } else {
        symbolTable[idx].doubleValue = value;
    }
}

int addSymbol(const char *varName) {
    return addSymbolWithType(varName, 1);
}

int addSymbolWithType(const char *varName, int isInt) {
    if (isVariableDeclared(varName)) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' already declared", varName);
        semanticError(errorMsg);
        return 0;
    }
    
    if (symbolCount >= symbolCapacity) {
        int newCapacity = (symbolCapacity == 0) ? 16 : symbolCapacity * 2;
        Symbol *newTable = (Symbol *)realloc(symbolTable, sizeof(Symbol) * newCapacity);
        if (!newTable) {
            semanticError("Out of memory while growing symbol table");
            return 0;
        }
        symbolTable = newTable;
        symbolCapacity = newCapacity;
    }
    
    strcpy(symbolTable[symbolCount].name, varName);
    symbolTable[symbolCount].isDeclared = 1;
    symbolTable[symbolCount].isInt = isInt;
    symbolTable[symbolCount].intValue = 0;
    symbolTable[symbolCount].doubleValue = 0.0;
    symbolCount++;
    return 1;
}

void printSymbolTable() {
    printf("\n=== Symbol Table ===\n");
    if (symbolCount == 0) {
        printf("(empty)\n");
    } else {
        for (int i = 0; i < symbolCount; i++) {
            if (symbolTable[i].isInt) {
                printf("  Variable: %s (int) = %d\n", symbolTable[i].name, symbolTable[i].intValue);
            } else {
                printf("  Variable: %s (double) = %.2f\n", symbolTable[i].name, symbolTable[i].doubleValue);
            }
        }
    }
    printf("======COMPLETED======\n");
}
void program() {
    printf("=== Starting Parse ===\n\n");
    getNextTokenSilently();
    
    while (currentToken.type != TOKEN_EOF && !hasError) {
        statement();
    }
    printf("\n=== Parse Complete ===\n");
    printf("Syntax Errors: %d\n", syntaxErrors);
    printf("Semantic Errors: %d\n", semanticErrors);
    printf("Expressions: %d\n", expressionCount);
    
    if (!hasError) {
        printf("Status: SUCCESS\n");
        printSymbolTable();
    } else {
        printf("Status: FAILED\n");
    }
}
void statement() {
    expressionCount++;
    printf("\n[EXPR %d]\n", expressionCount);
    if (tokenPrintDeferred) {
        printToken();
        tokenPrintDeferred = 0;
    }
    if (currentToken.type == TOKEN_KEYWORD_INT || currentToken.type == TOKEN_KEYWORD_DOUBLE) {
        declaration();
    } else if (currentToken.type == TOKEN_KEYWORD_PRINT) {
        printStmt();
    } else if (currentToken.type == TOKEN_IDENTIFIER) {
        assignment();
    } else if (currentToken.type == TOKEN_UNKNOWN) {
        syntaxError("Unexpected character in statement");
        getNextToken();  
    } else {
        syntaxError("Expected statement (declaration, assignment, or print)");
        getNextToken();  
    }
}

void declaration() {
    printf("Parsing declaration...\n");
    char varName[50];
    int isInt = 0;
    
    if (currentToken.type == TOKEN_KEYWORD_INT) {
        isInt = 1;
    } else if (currentToken.type == TOKEN_KEYWORD_DOUBLE) {
        isInt = 0;
    } else {
        syntaxError("Expected 'int' or 'double' keyword");
        return;
    }
    getNextToken();
    
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("Expected identifier after type keyword");
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
    
    addSymbolWithType(varName, isInt);
    
    if (currentToken.type != TOKEN_OPERATOR_ASSIGN) {
        syntaxError("Expected '=' operator after identifier");
        return;
    }
    getNextToken();
    
    double value = expression();
    
    if (currentToken.type != TOKEN_SYMBOL_SEMICOLON) {
        syntaxError("Expected ';' at end of declaration");
        return;
    }
    setSymbolValue(varName, value, isInt);
    getNextTokenSilently();
}

void assignment() {
    printf("Parsing assignment...\n");
    char varName[50];
    
    if (currentToken.type != TOKEN_IDENTIFIER) {
        syntaxError("Expected identifier");
        return;
    }
    
    strcpy(varName, currentToken.value);
     
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
    
    double value = expression();
    
    if (currentToken.type != TOKEN_SYMBOL_SEMICOLON) {
        syntaxError("Expected ';' at end of assignment");
        return;
    }
    int idx = findSymbol(varName);
    if (idx != -1) {
        setSymbolValue(varName, value, symbolTable[idx].isInt);
    }
    getNextTokenSilently();
}

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
    
    if (!isVariableDeclared(varName)) {
        char errorMsg[100];
        sprintf(errorMsg, "Variable '%s' used in print() before declaration", varName);
        semanticError(errorMsg);
    }
    
    double value = getSymbolValue(varName);
    int idx = findSymbol(varName);
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
    if (idx != -1 && symbolTable[idx].isInt) {
        printf("Result: %d\n", (int)value);
    } else {
        printf("Result: %.2f\n", value);
    }
    getNextTokenSilently();
}

double expression() {
    double left = term();
    
    while (currentToken.type == TOKEN_OPERATOR_PLUS ||
           currentToken.type == TOKEN_OPERATOR_MINUS) {
        TokenType op = currentToken.type;
        getNextToken();
        double right = term();
        if (op == TOKEN_OPERATOR_PLUS) {
            left = left + right;
        } else if (op == TOKEN_OPERATOR_MINUS) {
            left = left - right;
        }
    }
    return left;
}

double term() {
    double left = factor();

    while (currentToken.type == TOKEN_OPERATOR_MULTIFLY ||
           currentToken.type == TOKEN_OPERATOR_DIVISION) {
        TokenType op = currentToken.type;
        getNextToken();
        double right = factor();
        if (op == TOKEN_OPERATOR_MULTIFLY) {
            left = left * right;
        } else {
            if (right == 0.0) {
                semanticError("Division by zero");
                return 0.0;
            }
            left = left / right;
        }
    }
    return left;
}

double factor() {
    if (currentToken.type == TOKEN_IDENTIFIER) {
        double value = getSymbolValue(currentToken.value);
        getNextToken();
        return value;
    } else if (currentToken.type == TOKEN_NUMBER) {
        double value = (double)atoi(currentToken.value);
        getNextToken();
        return value;
    } else if (currentToken.type == TOKEN_DECIMAL) {
        double value = atof(currentToken.value);
        getNextToken();
        return value;
    } else {
        syntaxError("Expected identifier or number in expression");
        getNextToken();
        return 0.0;
    }
}
int result() {
    return 0;  
}
void initSymbolTable() {
    symbolTable = NULL;
    symbolCount = 0;
    symbolCapacity = 0;
}
void freeSymbolTable() {
    free(symbolTable);
    symbolTable = NULL;
    symbolCount = 0;
    symbolCapacity = 0;
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

    initSymbolTable();
    program();
    
    fclose(inputFile);
    freeSymbolTable();

    return hasError ? 1 : 0;
}