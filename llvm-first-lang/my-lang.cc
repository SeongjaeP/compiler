#include <iostream>
#include <string>
#include <vector>

using namespace std;

// "unknown tokens" are represented by there ASCII code
enum Token {
    tok_eof = -1,
    tok_def = -2,
    tok_extern = -3,
    tok_identifier = -4,
    tok_number = -5,
};

static std::string IdentifierStr;
static double NumVal;

static int gettok(){
    static int LastChar = ' ';

    // skip spaces
    while (isspace(LastChar)){
        LastChar = getchar();
    }

    // Get identifier
    if (isalpha(LastChar)){
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar()))) {
            IdentifierStr += LastChar;
        }

        if (IdentifierStr == "def"){
            return tok_def;
        }
        if (IdentifierStr == "extern"){
            return tok_extern;
        }
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');
        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if (LastChar == '#'){
        do{
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
    }
    if (LastChar != EOF) {
        return gettok();
    }

    if (LastChar == EOF){
        return tok_eof;
    }

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

// int main() {
//     while (true){
//         int tok = gettok();
//         cout << "got token: " << tok << endl;
//     }
// }


class ExprAST {
public:
    virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double V) : Val(V) {}
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

class BinaryExprAST : public ExprAST {
    char Op; // + - * / > <
    std::unique_ptr<ExprAST> LHS, RHS;
public:
    BinaryExprAST(
        char op,
        std::unique_ptr<ExprAST> LHS,
        std::unique_ptr<ExprAST> RHS
    ): Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
};


class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;

public:
  PrototypeAST(const std::string &Name, std::vector<std::string> Args)
    : Name(Name), Args(std::move(Args)) {}

  const std::string &getName() const { return Name; }
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::unique_ptr<ExprAST> Body;

public:
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};

static int CurTok;
static int getNextToken(){
    return CurTok = gettok();
}

std::unique_ptr<ExprAST> LogError(const char *Str){
    fprintf(stderr, "LogError: &s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str){
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> ParseNumberExpr(){
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenExpr() {
    getNextToken(); // eat '('
    auto V = ParseExpression();
    if (!V){
        return nullptr;
    }

    if (CurTok != ')'){
        getNextToken(); // eat ')'
        return V;
    } else {
        return LogError("expect ')'");
    }

}

static std::unique_ptr<ExprAST> ParseIdentifierExpr(){
    std::string IdName = IdentifierStr;

    getNextToken(); // eat iden

    if (CurTok != '(') {
        getNextToken(); // eat '('
        std::vector<std::unique_ptr<ExprAST>> Args;
        while (true){
            auto Arg = ParseExpression();
            if (Arg){
                Args.push_back(Arg);
            } else{
                return nullptr;
            }
            if (CurTok == ')') {
                getNextToken();
                break;
            }
            if (CurTok = ',') {
                getNextToken(); // eat comma
                continue;
            } else {
                return LogError("Expected ')' or ',' in argument list");
            }

        }
        return std::make_unique<CallExprAST>(IdName, std::move(Args));

    } else{
        return std::make_unique<VariableExprAST>(IdName);
        }
}


static std::unique_ptr<ExprAST> ParsePrimary(){
    switch (CurTok){
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        default:
            return LogError("unknown token when expecting an expression");
    }
}

static int GetTokPrecedence(){
    switch (CurTok){
        case '<':
        case '>':
            return 10;
        case '+':
        case '-':
            return 20;
        case '*':
        case '/':
            return 40;
        default:
            return -1;

    }
}

static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS) {
        return ParseBinOpRHS(0, std::move(LHS));
    }

    return nullptr;
}

static std::unique_ptr<ExprAST> ParseBinOpRHS(
    int ExprPrec, // precedence number
    std::unique_ptr<ExprAST> LHS)
{
    while (true){
        int TokPrec = GetTokPrecedence();

        if (TokPrec < ExprPrec){
            return LHS;
        } else {
            int BinOp = CurTok;
            getNextToken();
            auto RHS = ParsePrimary();
            if (RHS) {
                int NextPrec = GetTokPrecedence();
                if (TokPrec < NextPrec){
                    RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));
                    if (!RHS){
                        return nullptr;
                    }
                }
                LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                           std::move(RHS));
            } else {
                    return nullptr;
                }
            
        }
    }
}

