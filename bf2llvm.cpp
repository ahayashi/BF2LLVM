#include <stack>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;
 
#define MEM_SIZE 65535
 
LLVMContext& context = getGlobalContext();
Module *TheModule;
Function *TheFunc;
BasicBlock *TheEntry;
IRBuilder<> B(context);
 
/* Constant Values */
ConstantInt* Zero = B.getInt64(0);
ConstantInt* One = B.getInt64(1);
 
class LoopInfo {
private:
    unsigned int id;
    BasicBlock *entry;
    BasicBlock *body;
    BasicBlock *exit;
public:
    LoopInfo(BasicBlock* _entry, BasicBlock* _body, BasicBlock* _exit, unsigned int _id): 
	entry(_entry), body(_body), exit(_exit), id(_id){  }
    BasicBlock* getEntry() { return entry; }
    BasicBlock* getBody() { return body; }
    BasicBlock* getExit() { return exit; }
};
 
void initModule()
{
    TheModule = new Module("top", context);
 
    FunctionType *funcType = FunctionType::get(B.getVoidTy(), false);
    TheFunc = Function::Create(funcType, Function::ExternalLinkage, "main", TheModule);
    TheEntry = BasicBlock::Create(context, "entry", TheFunc);
    B.SetInsertPoint(TheEntry);
}
 
void parse(std::string& codes, FILE *input) 
{
    int ch;
    while ((ch=getc(input)) != EOF) {
        switch (ch) {
	case '+':
	case '-':
	case '>':
	case '<':
	case ',':
	case '.':
	case '[':
	case ']':
	    codes.push_back(ch);
	    break;
        }
    }
}
 
void generate(std::string& codes) 
{
    /* Types */
    Type* I8Ty = B.getInt8Ty();
    Type* I8PtrTy = B.getInt8PtrTy();
    Type* I32Ty = B.getInt32Ty();
    Type* I64Ty = B.getInt64Ty();
    Type* I64ArrayTy = ArrayType::get(I64Ty, MEM_SIZE);
    /* For printf  */
    Value *format = B.CreateGlobalStringPtr("%c");
    Constant *putsFunc = TheModule->getOrInsertFunction("printf", I32Ty, I8PtrTy, I64Ty, NULL);
    /* Allocate Memory */
    Value* mem = B.CreateAlloca(I64ArrayTy);
    /* Allocate & Init Program Counter */
    Value* pc = B.CreateAlloca(I64Ty, 0, "pc");    
    B.CreateStore(Zero, pc);
    /* Temporary Values */
    Value *tmp1, *tmp2, *tmp3, *tmp4, *tmp5;
    /* For Loop */
    int loopCount = 0;
    std::stack<LoopInfo*> stack;

    /* Interpretation */
    for (size_t i = 0; i < codes.size(); i++) {
        switch(codes[i]) {
	case '>':
	{
	    tmp1 = B.CreateLoad(pc);
	    tmp2 = B.CreateAdd(tmp1, One);
	    B.CreateStore(tmp2, pc);
	    break;
	}
	case '<':
	{
	    tmp1 = B.CreateLoad(pc);
	    tmp2 = B.CreateSub(tmp1, One);
	    B.CreateStore(tmp2, pc);
	    break;
	}
	case '+':
	{
	    std::vector<Value *> args;
	    tmp1 = B.CreateLoad(pc);
	    args.push_back(Zero);
	    args.push_back(tmp1);
	    ArrayRef<Value*> argsRef(args);
	    tmp2 = B.CreateGEP(mem, argsRef);
	    tmp3 = B.CreateLoad(tmp2);
	    tmp4 = B.CreateAdd(tmp3, One);
	    B.CreateStore(tmp4, tmp2);
	    break;
	}
	case '-':
	{
	    std::vector<Value *> args;
	    tmp1 = B.CreateLoad(pc);
	    args.push_back(Zero);
	    args.push_back(tmp1);
	    ArrayRef<Value*> argsRef(args);
	    tmp2 = B.CreateGEP(mem, argsRef);
	    tmp3 = B.CreateLoad(tmp2);
	    tmp4 = B.CreateSub(tmp3, One);
	    B.CreateStore(tmp4, tmp2);
	    break;
	}
	case ',':
//    *mem = getchar();
	    break;
	case '.':
	{
	    std::vector<Value *> args;
	    tmp1 = B.CreateLoad(pc);
	    args.push_back(Zero);
	    args.push_back(tmp1);
	    ArrayRef<Value*> argsRef(args);
	    tmp2 = B.CreateGEP(mem, argsRef);
	    tmp3 = B.CreateLoad(tmp2);
	    B.CreateCall2(putsFunc, format, tmp3);
	    break;
	}
	case '[': // First Half
	{
	    Twine loopNo(loopCount);
	    BasicBlock *loopentry = BasicBlock::Create(context, "loopentry" + loopNo, TheFunc);
	    BasicBlock *loopbody = BasicBlock::Create(context, "loopbody" + loopNo, TheFunc);
	    BasicBlock *loopexit = BasicBlock::Create(context, "loopexit" + loopNo, TheFunc);
	    LoopInfo *li = new LoopInfo(loopentry, loopbody, loopexit, loopCount);
	    stack.push(li);
	    B.CreateBr(loopentry);
	    B.SetInsertPoint(loopentry);
	    /* Get PC*/
	    std::vector<Value *> args;
	    tmp1 = B.CreateLoad(pc);
	    args.push_back(Zero);
	    args.push_back(tmp1);
	    ArrayRef<Value*> argsRef(args);
	    tmp2 = B.CreateGEP(mem, argsRef);
	    /* Get Data */
	    tmp3 = B.CreateLoad(tmp2);
	    tmp4 = B.CreateICmpEQ(tmp3, Zero);
	    B.CreateCondBr(tmp4, loopexit, loopbody);
	    B.SetInsertPoint(loopbody);
	    loopCount++;
	    break;
	}
	case ']':
	{
	    LoopInfo *li = stack.top();
	    stack.pop();
	    B.CreateBr(li->getEntry());
	    B.SetInsertPoint(li->getExit());
 
	    break;
	}
        }
    }
    BasicBlock *exitBB = BasicBlock::Create(context, "exit", TheFunc);
    B.CreateBr(exitBB);
    B.SetInsertPoint(exitBB);
    B.CreateRetVoid();
}
 
int main() 
{
    std::string codes;
    initModule();
    parse(codes, stdin);
    generate(codes);
    TheModule->dump();
    return 0;
}
