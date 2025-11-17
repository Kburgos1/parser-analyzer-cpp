#include "parse.h"
using namespace std;

// symbol table and declared-variable map
map<string, bool> defVar;       // name -> declared?
map<string, Token> SymTable;    // name -> INT or FLOAT

int error_count = 0;

// ------------ Token wrapper (Parser namespace) ------------

namespace Parser {
    bool    pushed_back = false;
    LexItem pushed_token;

    // Wrapper around the lexical analyzer's getNextToken()
    LexItem GetNextToken(istream& in, int& line) {
        if (pushed_back) {
            pushed_back = false;
            return pushed_token;
        }
        return getNextToken(in, line);  // from lex.cpp
    }

    void PushBackToken(LexItem& t) {
        if (pushed_back) {
            abort(); // logic bug in parser if this happens
        }
        pushed_back = true;
        pushed_token = t;
    }
}

// ------------ Error handling helpers ------------

int ErrCount() {
    return error_count;
}

void ParseError(int line, string msg) {
    error_count++;
    cout << line << ": " << msg << endl;
}

// ------------ Forward declarations of nonterminals ------------

bool Prog      (istream& in, int& line);
bool StmtList  (istream& in, int& line);
bool Stmt      (istream& in, int& line);
bool DeclStmt  (istream& in, int& line);
bool IdentList (istream& in, int& line, LexItem tok);
bool ControlStmt(istream& in, int& line);
bool WriteStmt (istream& in, int& line);
bool IfStmt    (istream& in, int& line);
bool AssignStmt(istream& in, int& line);
bool ExprList  (istream& in, int& line);
bool Expr      (istream& in, int& line);
bool Term      (istream& in, int& line);
bool SFactor   (istream& in, int& line);
bool Factor    (istream& in, int& line, int sign);
bool LogicExpr (istream& in, int& line);
bool Var       (istream& in, int& line);

// =============================================================
// Prog ::= PROGRAM IDENT StmtList END PROGRAM
// =============================================================

bool Prog(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);

    // Empty file
    if (t.GetToken() == DONE) {
        ParseError(line, "Empty File");
        return false;
    }

    // Must start with PROGRAM
    if (t.GetToken() != PROGRAM) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing PROGRAM.");
        return false;
    }

    // Program name
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != IDENT) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing Program Name.");
        return false;
    }

    // Statement list
    if (!StmtList(in, line)) {
        // StmtList or its children already printed an error
        return false;
    }

    // Must see END
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != END) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing END at end of program.");
        return false;
    }

    // Must see PROGRAM after END
    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != PROGRAM) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing PROGRAM at the End");
        return false;
    }

    return true;
}

// =============================================================
// StmtList ::= Stmt ; { Stmt ; }
// =============================================================

bool StmtList(istream& in, int& line) {
    // First statement
    if (!Stmt(in, line)) {
        return false;
    }

    // After a statement, we must see a semicolon
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != SEMICOL) {
        if (t.GetToken() == END) {
            // Allow missing final semicolon JUST before END? (most specs
            // require it, but this covers some edge cases)
            Parser::PushBackToken(t);
            ParseError(line, "Missing a semicolon.");
            return false;
        }
        Parser::PushBackToken(t);
        ParseError(line, "Missing a semicolon.");
        return false;
    }

    // We have at least one "Stmt ;". Continue parsing more.
    t = Parser::GetNextToken(in, line);
    while (true) {
        if (t.GetToken() == END) {
            // End of statement list; give END back to Prog
            Parser::PushBackToken(t);
            return true;
        }

        // t is the first token of the next statement
        Parser::PushBackToken(t);

        if (!Stmt(in, line)) {
            return false;
        }

        // After each statement, expect another semicolon
        t = Parser::GetNextToken(in, line);
        if (t.GetToken() != SEMICOL) {
            if (t.GetToken() == END) {
                Parser::PushBackToken(t);
                ParseError(line, "Missing a semicolon.");
                return false;
            }
            Parser::PushBackToken(t);
            ParseError(line, "Missing a semicolon.");
            return false;
        }

        // Get token after the semicolon to decide loop or stop
        t = Parser::GetNextToken(in, line);
    }
}

// =============================================================
// Stmt ::= DeclStmt | ControlStmt
// =============================================================

bool Stmt(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);

    Token tt = t.GetToken();
    if (tt == INT || tt == FLOAT) {
        // Declaration statement
        Parser::PushBackToken(t);
        return DeclStmt(in, line);
    }
    else if (tt == IDENT || tt == IF || tt == WRITE) {
        // Assignment, IF, or WRITE
        Parser::PushBackToken(t);
        return ControlStmt(in, line);
    }

    Parser::PushBackToken(t);
    ParseError(line, "Invalid Statement");
    return false;
}

// =============================================================
// DeclStmt ::= (INT | FLOAT) IdentList
// =============================================================

bool DeclStmt(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);
    Token tt = t.GetToken();

    if (tt != INT && tt != FLOAT) {
        Parser::PushBackToken(t);
        ParseError(line, "Incorrect Declaration Type.");
        return false;
    }

    // Pass the type token to IdentList
    return IdentList(in, line, t);
}

// =============================================================
// IdentList ::= IDENT { , IDENT }
// (type comes from parameter tok)
// =============================================================

bool IdentList(istream& in, int& line, LexItem tok) {
    // tok holds the type token (INT or FLOAT)
    Token typeTok = tok.GetToken();

    // First identifier
    LexItem idTok = Parser::GetNextToken(in, line);
    if (idTok.GetToken() != IDENT) {
        ParseError(line, "Invalid Identifier List");
        return false;
    }

    string name = idTok.GetLexeme();
    if (defVar.find(name) != defVar.end()) {
        ParseError(line, "Variable Redefinition");
        return false;
    }

    // Record in symbol tables
    defVar[name] = true;
    SymTable[name] = typeTok;

    // Process tail: { , IDENT }
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() == COMMA) {
        // More identifiers with same type
        return IdentList(in, line, tok);
    }

    Parser::PushBackToken(t);
    return true;
}

// =============================================================
// ControlStmt ::= AssignStmt | IfStmt | WriteStmt
// =============================================================

bool ControlStmt(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);

    if (t.GetToken() == IDENT) {
        Parser::PushBackToken(t);
        return AssignStmt(in, line);
    }
    else if (t.GetToken() == IF) {
        Parser::PushBackToken(t);
        return IfStmt(in, line);
    }
    else if (t.GetToken() == WRITE) {
        Parser::PushBackToken(t);
        return WriteStmt(in, line);
    }

    Parser::PushBackToken(t);
    ParseError(line, "Invalid Control Statement");
    return false;
}

// =============================================================
// WriteStmt ::= WRITE ExprList
// =============================================================

bool WriteStmt(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != WRITE) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing WRITE Keyword");
        return false;
    }

    if (!ExprList(in, line)) {
        ParseError(line, "Missing expression after WRITE");
        return false;
    }

    return true;
}

// =============================================================
// IfStmt ::= IF ( LogicExpr ) ControlStmt
// =============================================================

bool IfStmt(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != IF) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing IF");
        return false;
    }

    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != LPAREN) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing Left Parenthesis of IF");
        return false;
    }

    if (!LogicExpr(in, line)) {
        return false;
    }

    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != RPAREN) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing Right Parenthesis of IF");
        return false;
    }

    if (!ControlStmt(in, line)) {
        ParseError(line, "Missing Statement after IF");
        return false;
    }

    return true;
}

// =============================================================
// AssignStmt ::= Var ASSOP Expr
// =============================================================

bool AssignStmt(istream& in, int& line) {
    if (!Var(in, line)) {
        return false;
    }

    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != ASSOP) {
        Parser::PushBackToken(t);
        ParseError(line, "Missing Assignment Operator");
        return false;
    }

    if (!Expr(in, line)) {
        ParseError(line, "Missing Expression in Assignment Statement");
        return false;
    }

    return true;
}

// =============================================================
// ExprList ::= Expr { , Expr }
// =============================================================

bool ExprList(istream& in, int& line) {
    if (!Expr(in, line)) {
        ParseError(line, "Missing Expression");
        return false;
    }

    LexItem t = Parser::GetNextToken(in, line);
    while (t.GetToken() == COMMA) {
        if (!Expr(in, line)) {
            ParseError(line, "Missing Expression after Comma");
            return false;
        }
        t = Parser::GetNextToken(in, line);
    }

    Parser::PushBackToken(t);
    return true;
}

// =============================================================
// Expr ::= Term { (+ | -) Term }
// =============================================================

bool Expr(istream& in, int& line) {
    if (!Term(in, line)) {
        ParseError(line, "Expression error");
        return false;
    }

    LexItem t = Parser::GetNextToken(in, line);
    while (t.GetToken() == PLUS || t.GetToken() == MINUS) {
        if (!Term(in, line)) {
            ParseError(line, "Missing operand after operator");
            return false;
        }
        t = Parser::GetNextToken(in, line);
    }

    Parser::PushBackToken(t);
    return true;
}

// =============================================================
// Term ::= SFactor { ( * | / | % ) SFactor }
// =============================================================

bool Term(istream& in, int& line) {
    if (!SFactor(in, line)) {
        ParseError(line, "Term Error");
        return false;
    }

    LexItem t = Parser::GetNextToken(in, line);
    while (t.GetToken() == MULT || t.GetToken() == DIV || t.GetToken() == REM) {
        if (!SFactor(in, line)) {
            ParseError(line, "Missing operand after operator");
            return false;
        }
        t = Parser::GetNextToken(in, line);
    }

    Parser::PushBackToken(t);
    return true;
}

// =============================================================
// SFactor ::= [ + | - ] Factor
// =============================================================

bool SFactor(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);
    int sign = 1;

    if (t.GetToken() == PLUS) {
        sign = 1;
    }
    else if (t.GetToken() == MINUS) {
        sign = -1;
    }
    else {
        // no sign; put token back for Factor
        Parser::PushBackToken(t);
    }

    return Factor(in, line, sign);
}

// =============================================================
// Factor ::= IDENT | ICONST | RCONST | SCONST | ( Expr )
// sign is carried but not used for PA2 (will matter in PA3)
// =============================================================

bool Factor(istream& in, int& line, int /*sign*/) {
    LexItem t = Parser::GetNextToken(in, line);
    Token tt = t.GetToken();

    // Identifier
    if (tt == IDENT) {
        string name = t.GetLexeme();
        if (defVar.count(name) == 0) {
            ParseError(line, "Undeclared Variable");
            return false;
        }
        return true;
    }

    // Literal constants
    if (tt == ICONST || tt == RCONST || tt == SCONST) {
        return true;
    }

    // Parenthesized expression
    if (tt != LPAREN) {
        ParseError(line, "No left parenthesis");
        return false;
    }

    if (!Expr(in, line)) {
        ParseError(line, "Factor error");
        return false;
    }

    t = Parser::GetNextToken(in, line);
    if (t.GetToken() != RPAREN) {
        ParseError(line, "No right parenthesis");
        return false;
    }

    return true;
}

// =============================================================
// LogicExpr ::= Expr ( == | > ) Expr
// =============================================================

bool LogicExpr(istream& in, int& line) {
    if (!Expr(in, line)) {
        ParseError(line, "Missing Expression in Logic Expression");
        return false;
    }

    LexItem t = Parser::GetNextToken(in, line);
    if (t.GetToken() != EQUAL && t.GetToken() != GTHAN) {
        Parser::PushBackToken(t);
        ParseError(line, "Relational Operator Error");
        return false;
    }

    if (!Expr(in, line)) {
        ParseError(line, "Missing Expression after Relational Operator");
        return false;
    }

    return true;
}

// =============================================================
// Var ::= IDENT
// (must be declared)
// =============================================================

bool Var(istream& in, int& line) {
    LexItem t = Parser::GetNextToken(in, line);

    if (t.GetToken() != IDENT) {
        Parser::PushBackToken(t);
        ParseError(line, "Incorrect Identifier Statement");
        return false;
    }

    string name = t.GetLexeme();
    if (defVar.count(name) == 0) {
        ParseError(line, "Undeclared Variable");
        return false;
    }

    return true;
}