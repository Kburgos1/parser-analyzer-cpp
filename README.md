# ParserAnalyzer

This project is the second assignment in the CS280 compiler series.  
It implements a **recursive-descent parser** for a small programming language and builds on top of the **Lexical Analyzer (PA1)**.

The parser reads an input program, checks whether it follows the language grammar, validates variable declarations, and reports detailed syntax errors.

If the program is syntactically correct, it prints:
Successful Parsing


If errors are found, it prints messages in the format:
line #: message
Unsuccessful Parsing
Number of Syntax Errors X

---

## Project Structure

```
ParserAnalyzer/
├── PA2.cpp # Main parser implementation
├── lex.cpp # Tokenizer (from PA1)
├── lex.h # Lexer header
├── prog2.cpp # Instructor-provided driver (contains main())
├── PA2_Test_Cases/ # Instructor test input files
└── README.md # Documentation
```

---

## How to Compile

Run the following inside the `ParserAnalyzer` directory:

```bash
g++ -std=c++17 PA2.cpp lex.cpp prog2.cpp -o prog2
```

This produces an executable named:
prog2

---

## How to Run the Parser

To test an input file:

```bash
./prog2 PA2_Test_Cases/testprog1
./prog2 PA2_Test_Cases/testprog2
./prog2 PA2_Test_Cases/testprog3
...
./prog2 PA_Test_Cases/testprog19
```

For correct programs, output will be:
Successful Parsing

For incorrect programs, you will see an error like:
10: Variable Redefinition
Unsuccessful Parsing
Number of Syntax Errors 1

## Grammar Implemented
The parser fully supports the following grammar

```
Prog        ::= PROGRAM IDENT StmtList END PROGRAM
StmtList    ::= Stmt ; { Stmt ; }
Stmt        ::= DeclStmt | ControlStmt

DeclStmt    ::= (INT | FLOAT) IdentList
IdentList   ::= IDENT { , IDENT }

ControlStmt ::= AssignStmt | IfStmt | WriteStmt
AssignStmt  ::= Var ASSOP Expr
IfStmt      ::= IF ( LogicExpr ) ControlStmt
WriteStmt   ::= WRITE ExprList

ExprList    ::= Expr { , Expr }
Expr        ::= Term { (+ | -) Term }
Term        ::= SFactor { (* | / | %) SFactor }
SFactor     ::= [ + | - ] Factor
Factor      ::= IDENT | ICONST | RCONST | SCONST | ( Expr )
LogicExpr   ::= Expr (== | >) Expr
Var         ::= IDENT
```

## Parser Features

* Recursive-descent implementation of each grammar rule.
* Error checking with line numbers.
* Symbol table for variable types (INT or FLOAT).
* Detection of:

    * Variable redeclaration
    * Use of undeclared variables
    * Missing semicolons
    * Incorrect operators
    * Invalid expressions
    * Incorrect IF statement syntax
    * Missing parentheses
* Produces one clear error per line, rather than cascading duplicate errors.

## Notes
* This parser depends on the lexer from PA1, so lex.cpp, lex.h, and tokens.h must be included.
* The project does not execute programs; it only checks syntax and declarations (execution comes in PA3).
* Error messages don’t need to match the .correct files exactly, only the line number and error count must be correct.