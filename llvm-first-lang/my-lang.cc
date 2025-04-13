#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <map>

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
    virtual Value *codegen() = 0;
};


class IfExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Cond, Then, Else;

public:
    IfExprAST(std::unique_ptr<ExprAST> Cod, std::unique_ptr<ExprAST> Then,
                std::unique_ptr<ExprAST> Else)
    : Cond(std::move(COnd)), Then(std::move(Then)), Else(std::move(Else)) {}

    Value *codegen() override;
};

static std::unique_ptr<ExprAST> ParseIfExpr() {
    getNextToken(); // 'if' move next token
    
    auto Cond = ParseExpression(); // 조건 부분 파싱
    if (!Cond) return nullptr;

    if (CurTok != tok_then) return LogError("expected then");
    getNextToken();

    auto Then = ParseExpression();
    if (!Then) return nullptr;

    if (CurTok != tok_else) return LogError("expected then");
    getNextToken();

    auto Else = ParseExpression();
    if (!Else) return nullptr;

    return std::make_unique<IfExprAST>(
        std::move(Cond), std::move(Then), std::move(Else)
    );
}

Value *IfExprAST::codegen(){
    Value *CondV = Cond -> codegen();
    if (!CondV) return nullptr;

    CondV = Builder -> CreateFCmpONE(
        CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");
    
    Function *TheFunction = Builder -> GetInsertBlock() -> getParent();

    BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
    BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
    BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");
    
    Builder -> CreateCondBr(CondV, ThenBB, ElseBB);

    Builder -> SetInsertPoint(ThenBB);

    Value *ThenV = Then -> codegen();
    if (!ThenV)
        return nullptr;
    
    Builder -> CreateBr(MergeBB);
    ThenBB = Builder->GetInsertBlock();

    TheFunction -> Insert(TheFunction -> end(), ElseBB);
    Builder -> SetInsertPoint(ElseBB);

    Value *ElseV = Else -> codegen();


}




class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double V) : Val(V) {}
    virtual Value *codegen();
};

Value *NumberExprAST::codegen() {
    return ConstantFP::get(TheContext, APFloat(Val));
}


/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(const std::string &Name) : Name(Name) {}
};

Value *VariableExprAST::codegen() {
    Value *V = NamedValues[Name];
    if (!V) {
        LogError("Unknown variable name");
    }
    return V;
}

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

Value *BinaryExprAST::codegen() {
    Value *L = LHS -> codegen();
    Value *R = RHS -> codegen();
    if (!L || !R){
        return nullptr;
    }

    switch(Op){
        case '+':
            return Builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return Builder.CreateFAdd(L, R, "subtmp");
        case '*':
            return Builder.CreateFAdd(L, R, "multmp");
        case '<':
            L = Builder.CreateUIToFP(L, Type::getDoubleTy)
    }
}


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

static std::unique_ptr<ExprAST> ParsePrimary();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int, std::unique_ptr<ExprAST>);

static std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS)
        return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr(){
    std::string IdName = IdentifierStr;
    getNextToken(); // eat identifier

    if (CurTok != '(')
        return std::make_unique<VariableExprAST>(IdName);

    // Function call
    getNextToken(); // eat '('
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (CurTok != ')') {
        while (true) {
            auto Arg = ParseExpression();
            if (!Arg) return nullptr;
            Args.push_back(std::move(Arg));

            if (CurTok == ')') break;
            if (CurTok != ',')
                return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }
    getNextToken(); // eat ')'
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

static std::unique_ptr<ExprAST> ParseParenExpr();

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

static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier){
        return LogErrorP("Expected function name in prototype");
    }

    std::string FnName = IdentifierStr;
    getNextToken(); //eat identifier

    if (CurTok != '(')
        return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> ArgNames;
    while (getNextToken() == tok_identifier) {
        ArgNames.push_back(IdentifierStr);
    if (CurTok != ')'){
        return LogErrorP("Expected ')' in prototype");
    }

    getNextToken();  // eat ')'.

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));

    }
}


static std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat def
    auto Proto = ParsePrototype();
    if (!Proto) {
        return nullptr;
    }

    auto E = ParseExpression();
    if (E) {
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    } else {
        return nullptr;
    }
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken(); // eat extern
    return ParsePrototype();
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    auto E = ParseExpression();
    if (E) {
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}


static void HandleDefinition() {
    if (ParseDefinition()) {
      fprintf(stderr, "Parsed a function definition.\n");
    } else {
      // Skip token for error recovery.
      getNextToken();
    }
}
  
  static void HandleExtern() {
    if (ParseExtern()) {
      fprintf(stderr, "Parsed an extern\n");
    } else {
      // Skip token for error recovery.
      getNextToken();
    }
}
  
  static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr()) {
      fprintf(stderr, "Parsed a top-level expr\n");
    } else {
      // Skip token for error recovery.
      getNextToken();
    }
}
  
  /// top ::= definition | external | expression | ';'
  static void MainLoop() {
    while (true) {
      fprintf(stderr, "ready> ");
      switch (CurTok) {
      case tok_eof:
        return;
      case ';': // ignore top-level semicolons.
        getNextToken();
        break;
      case tok_def:
        HandleDefinition();
        break;
      case tok_extern:
        HandleExtern();
        break;
      default:
        HandleTopLevelExpression();
        break;
      }
    }
}


static std::unique_ptr<LLVMContext> TheContext;
static std::unique_ptr<IRBuilder<>> Builder;
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;

int main() {
    fprintf(stderr, "ready> ");
    getNextToken();

    MainLoop();
    return 0;
}



void InitializeModuleAndManager(void) {
    TheContext = std::make_unique<LLVMCOntext>();
    TheModule = std::make_unique<Module>("KaleidoscopeJIT", *TheContext);
    TheModule -> setDataLayout(TheJIT -> getDataLayout());

    Builder = std::make_unique<IRBuilder<>>(*TheContext);

    // Create new pass and analysis manager.
    TheFPM = std::make_unique<FunctionPassManager>();
    TheLAM = std::make_unique<LoopAnalysisManager>();
    TheFAM = std::make_unique<FunctionAnalysisManager>();
    TheCGAM = std::make_unique<CGSCCAnalysisManager>();
    TheMAM = std::make_unique<ModuleAnalysisManager>();
    ThePIC = std::make_unique<PassInstrumentationCallbacks>();
    TheSI = std::make_unique<StandardInstrumentations>(*TheContext,
                                                    /*DebugLogging*/ true);
    TheSI->registerCallbacks(*ThePIC, TheMAM.get());
}


