#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include <iostream>

#include <fstream>
#include <sstream>

class MyVisitor : public clang::RecursiveASTVisitor<MyVisitor>
{
  public:
    bool VisitClassDeclaration(clang::ClassTemplateDecl *c)
    {
        if (c->hasBody())
        {
            llvm::outs() << "Class: " << c->getNameAsString() << "\n";
        }
        return true;
    }
};

class MyConsumer : public clang::ASTConsumer
{
  public:
    void HandleTranslationUnit(clang::ASTContext &context) override
    {
        MyVisitor visitor;
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

class MyAction : public clang::ASTFrontendAction
{
  public:
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &, llvm::StringRef) override
    {
        return std::make_unique<MyConsumer>();
    }
};

std::string readFileToString(std::string const &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char const **argv)
{
    if (argc > 1)
    {
        clang::tooling::runToolOnCode(std::make_unique<MyAction>(), readFileToString(argv[1]).c_str());
    }
}
