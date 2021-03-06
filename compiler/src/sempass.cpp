#include <cstdarg>

#include "node.h"
#include "types/stapletype.h"

#include "sempass.h"
#include "sempass/Pass1ClassVisitor.h"

namespace staple {

using namespace std;
using namespace llvm;

#define CheckType(type, location, name, positive) \
if(type == NULL) { \
    sempass->logError(location, "undefined type: '%s'", name.c_str()); \
} else { \
    positive \
}

class Scope {
public:
    Scope* parent;
    map<string, StapleType*> table;

    StapleType* get(const string& name) {
        StapleType* retval = NULL;

        map<string, StapleType*>::iterator it;
        if((it = table.find(name)) != table.end()) {
            retval = it->second;
        } else if(parent != NULL) {
            retval = parent->get(name);
        }

        return retval;
    }
};


class TypeVisitor : public ASTVisitor {
public:
    StapleFunction* mCurrentFunctionType;
    StapleClass *currentClass;
    Scope* scope;
    SemPass* sempass;

    TypeVisitor(SemPass* sempass)
    : scope(NULL)
    , sempass(sempass) {}

    using ASTVisitor::visit;

    void push() {
        Scope* newScope = new Scope();
        newScope->parent = scope;
        scope = newScope;
    }

    void pop() {
        Scope* oldScope = scope;
        scope = scope->parent;
        delete oldScope;
    }

    void define(const string& name, StapleType* type) {
        scope->table[name] = type;
    }

    StapleType* getType(ASTNode* node) {
        node->accept(this);
        return sempass->ctx.typeTable[node];
    }



    virtual void visit(NCompileUnit* compileUnit) {

        currentClass = NULL;
        push();

        //first pass class declaration
        for(NClassDeclaration* classDeclaration : compileUnit->classes) {
            Pass1ClassVisitor visitor(&sempass->ctx);
            classDeclaration->accept(&visitor);
        }

        //first pass extern functions
        for(NFunctionPrototype* functionPrototype : compileUnit->externFunctions) {

            std::vector<StapleType*> argsType;
            for(NArgument* arg : functionPrototype->arguments){
                StapleType* type = getType(&arg->type);

                CheckType(type, arg->location, arg->name,
                          argsType.push_back(type);
                                  sempass->ctx.typeTable[arg] = type;
                )
            }

            StapleType* returnType = getType(&functionPrototype->returnType);
            CheckType(returnType, functionPrototype->location, functionPrototype->returnType.name,
                      sempass->ctx.typeTable[functionPrototype] = new StapleFunction(returnType, argsType, functionPrototype->isVarg);
                              define(functionPrototype->name, sempass->ctx.typeTable[functionPrototype]);
            )

        }

        //first pass global functions
        for(NFunction* function : compileUnit->functions){
            std::vector<StapleType*> argsType;
            for(NArgument* arg : function->arguments){
                StapleType* type = getType(&arg->type);

                CheckType(type, arg->location, arg->name,
                          argsType.push_back(type);
                                  sempass->ctx.typeTable[arg] = type;
                )
            }

            StapleType* returnType = getType(&function->returnType);
            CheckType(returnType, function->location, function->returnType.name,
                      sempass->ctx.typeTable[function] = new StapleFunction(returnType, argsType, function->isVarg);
                              define(function->name, sempass->ctx.typeTable[function]);
            )
        }

        //class fields and methods
        for(NClassDeclaration* classDeclaration : compileUnit->classes){

            StapleClass* parentClass = sempass->ctx.lookupClassName(classDeclaration->mExtends);

            currentClass = sempass->ctx.lookupClassName(classDeclaration->name);
            currentClass->setParent(parentClass);
            sempass->ctx.typeTable[classDeclaration] = currentClass;

            for(NField* field : classDeclaration->fields) {
                field->accept(this);
            }

            for(NMethodFunction* method : classDeclaration->functions) {

                std::vector<StapleType*> args;
                for(NArgument* arg : method->arguments){
                    StapleType* type = getType(&arg->type);
                    CheckType(type, arg->location, arg->type.name, args.push_back(type); )
                }

                StapleType* returnType = getType(&method->returnType);
                CheckType(returnType, method->returnType.location, method->returnType.name, )

                sempass->ctx.typeTable[method] = currentClass->addMethod(method->name, returnType, args, method->isVarg);
            }
        }

        //second pass methods
        for(NClassDeclaration* classDeclaration : compileUnit->classes) {
            for(NMethodFunction* method : classDeclaration->functions) {
                method->accept(this);
            }
        }

        //second pass global functions
        for(NFunction* function : compileUnit->functions){
            function->accept(this);
        }

        currentClass = NULL;

        pop();
    }

    virtual void visit(NType* type) {

        const string name = type->name;

        StapleType* retval = NULL;
        if(name.compare("void") == 0) {
            retval = StapleType::getVoidType();
        } else if(name.compare("uint") == 0 || name.compare("int") == 0 || name.compare("int32") == 0){
            retval = StapleType::getInt32Type();
        } else if(name.compare("uint8") == 0 || name.compare("int8") == 0) {
            retval = StapleType::getInt8Type();
        } else if(name.compare("uint16") == 0 || name.compare("int16") == 0) {
            retval = StapleType::getInt16Type();
        } else if(name.compare("float") == 0 || name.compare("float32") == 0) {
            retval = StapleType::getFloat32Type();
        } else if(name.compare("bool") == 0) {
            retval = StapleType::getBoolType();
        } else {

            retval = sempass->ctx.lookupClassName(name);
            if(retval == nullptr || !isa<StapleClass>(retval)) {
                sempass->logError(type->location, "unknown type: '%s'", type->name.c_str());
                sempass->ctx.typeTable[type] = NULL;
                return;
            }
        }

        if(type->isArray) {
            retval = new StapleArray(retval, type->size);
        } else {
            for(int i=0;i<type->numPointers;i++) {
                retval = new StaplePointer(retval);
            }
        }
        sempass->ctx.typeTable[type] = retval;
    }

    virtual void visit(NField* field) {
        StapleType* fieldType = getType(&field->type);
        CheckType(fieldType, field->location, field->type.name,
                sempass->ctx.typeTable[field] = currentClass->addField(field->name, fieldType);
        )
    }

    virtual void visit(NExpressionStatement* expressionStatement) {
        sempass->ctx.typeTable[expressionStatement] = getType(expressionStatement->expression);
    }

    virtual void visit(NMethodFunction* methodFunction) {
        push();

        StapleType* thisType = new StaplePointer(currentClass);
        define("this", thisType);

        for(StapleField* field : currentClass->getFields()){
            define(field->getName(), field);
        }

        for(NArgument* arg : methodFunction->arguments){
            StapleType* type = getType(&arg->type);

            CheckType(type, arg->location, arg->type.name,
                    define(arg->name, type);
                    sempass->ctx.typeTable[arg] = type;
            )
        }

        StapleType* returnType = getType(&methodFunction->returnType);
        CheckType(returnType, methodFunction->returnType.location, methodFunction->returnType.name, )

        mCurrentFunctionType = cast<StapleFunction>(sempass->ctx.typeTable[methodFunction]);

        for(NStatement* statement : methodFunction->block.statements){
            statement->accept(this);
        }
        pop();
    }

    virtual void visit(NReturn* returnexp) {
        StapleType* returnType = getType(returnexp->ret);
        if(!returnType->isAssignable(mCurrentFunctionType->getReturnType())) {
            sempass->logError(returnexp->location, "return type mismatch");
        }

        sempass->ctx.typeTable[returnexp] = StapleType::getVoidType();
    }

    virtual void visit(NFunction* function) {
        push();
        for(NArgument* arg : function->arguments){
            StapleType* type = getType(&arg->type);

            CheckType(type, arg->location, arg->type.name,
                    define(arg->name, type);
                    sempass->ctx.typeTable[arg] = type;
            )
        }

        StapleType* returnType = getType(&function->returnType);
        CheckType(returnType, function->returnType.location, function->returnType.name, )

        mCurrentFunctionType = cast<StapleFunction>(sempass->ctx.typeTable[function]);

        for(NStatement* statement : function->block.statements){
            statement->accept(this);
        }
        pop();
    }

    virtual void visit(NVariableDeclaration* variableDeclaration) {
        StapleType* type = getType(variableDeclaration->type);
        CheckType(type, variableDeclaration->location, variableDeclaration->type->name,
                define(variableDeclaration->name, type);
                sempass->ctx.typeTable[variableDeclaration] = type;
        )

        if(variableDeclaration->assignmentExpr != NULL) {
            variableDeclaration->assignmentExpr->accept(this);
            StapleType* rhs = sempass->ctx.typeTable[variableDeclaration->assignmentExpr];

            if(!rhs->isAssignable(type)) {
                sempass->logError(variableDeclaration->location, "cannot convert rhs to lhs");
            }
        }
    }

    virtual void visit(NIntLiteral* intLiteral) {
        sempass->ctx.typeTable[intLiteral] = StapleType::getInt32Type();
    }

    virtual void visit(NStringLiteral* literal) {
        sempass->ctx.typeTable[literal] = StapleType::getInt8PtrType();
    }

    virtual void visit(NAssignment* assignment) {
        assignment->lhs->accept(this);
        assignment->rhs->accept(this);

        StapleType* lhsType = sempass->ctx.typeTable[assignment->lhs];
        StapleType* rhsType = sempass->ctx.typeTable[assignment->rhs];

        if(!rhsType->isAssignable(lhsType)){
            sempass->logError(assignment->location, "cannot convert rhs to lhs");
        }
    }

    virtual void visit(NSizeOf* nsizeOf) {
        StapleType* type = getType(nsizeOf->type);

        if(type != NULL) {
            sempass->ctx.typeTable[nsizeOf] = StapleType::getInt32Type();
        }
    }

    virtual void visit(NNew* newNode) {
        StapleType* type = sempass->ctx.lookupClassName(newNode->id);

        if(StapleClass* classType = dyn_cast<StapleClass>(type)) {
            sempass->ctx.typeTable[newNode] = new StaplePointer(classType);
        } else {
            sempass->logError(newNode->location, "undefined class: '%s'", newNode->id.c_str());
        }
    }

    virtual void visit(NIdentifier* identifier) {
        StapleType* type = scope->get(identifier->name);
        sempass->ctx.typeTable[identifier] = type;
    }

    virtual void visit(NArrayElementPtr* arrayElementPtr) {

        StapleType* baseType = getType(arrayElementPtr->base);

        if(StapleArray* arrayType = dyn_cast<StapleArray>(baseType)) {
            StapleType* exprType = getType(arrayElementPtr->expr);

            if(isa<StapleInt>(exprType)) {
                sempass->ctx.typeTable[arrayElementPtr] = new StaplePointer(baseType);
            } else {
                sempass->logError(arrayElementPtr->expr->location, "array index is not an integer");
            }
        }

    }

    virtual void visit(NMemberAccess* memberAccess) {

        StapleType* baseType = getType(memberAccess->base);

        StaplePointer* ptr = nullptr;
        StapleClass* classPtr = nullptr;

        if((ptr = dyn_cast<StaplePointer>(baseType)) && (classPtr = dyn_cast<StapleClass>(ptr->getElementType()))) {
            memberAccess->base = new NLoad(memberAccess->base);
            memberAccess->base->accept(this);
        } else if(!(classPtr = dyn_cast<StapleClass>(baseType))) {
            sempass->logError(memberAccess->base->location, "not a class type");
            return;
        }

        uint index = 0;
        StapleField* field = classPtr->getField(memberAccess->field, index);
        if(field != nullptr){
            sempass->ctx.typeTable[memberAccess] = field;
            memberAccess->fieldIndex = index;
        } else {
            sempass->logError(memberAccess->location, "class '%s' does not have field named: '%s'",
                              classPtr->getClassName().c_str(),
                              memberAccess->field.c_str());
        }

    }

    virtual void visit(NIfStatement* ifStatement) {
        StapleType* conditionType = getType(ifStatement->condition);

        if(!isa<StapleInt>(conditionType)){
            sempass->logError(ifStatement->condition->location, "cannot evaluate condition");
        }

        ifStatement->thenBlock->accept(this);
        if(ifStatement->elseBlock != NULL) {
            ifStatement->elseBlock->accept(this);
        }
    }

    virtual void visit(NBinaryOperator* binaryOperator) {
        binaryOperator->lhs->accept(this);
        binaryOperator->rhs->accept(this);

        StapleType* returnType = StapleType::getVoidType();

        switch(binaryOperator->op) {
            case TCEQ:
            case TCNE:
            case TCGT:
            case TCLT:
            case TCGE:
            case TCLE:
                returnType = StapleType::getBoolType();
                break;

            case TPLUS:
            case TMINUS:
            case TMUL:
            case TDIV:
                returnType = sempass->ctx.typeTable[binaryOperator->lhs];
                break;
        }

        sempass->ctx.typeTable[binaryOperator] = returnType;
    }

    virtual void visit(NMethodCall* methodCall) {

        StapleType* baseType = getType(methodCall->base);

        StaplePointer* ptr = nullptr;
        StapleClass* classPtr = nullptr;

        if((ptr = dyn_cast<StaplePointer>(baseType)) && (classPtr = dyn_cast<StapleClass>(ptr->getElementType()))) {
            //methodCall->base = new NLoad(methodCall->base);
            //methodCall->base->accept(this);
        } else if(!(classPtr = dyn_cast<StapleClass>(baseType))) {
            sempass->logError(methodCall->base->location, "not a class type");
            return;
        }

        int index = 0;
        StapleMethodFunction* method = classPtr->getMethod(methodCall->name, index);
        if(method != nullptr) {
            methodCall->methodIndex = index;

            int i = 0;
            for(auto arg : methodCall->arguments) {
                StapleType* argType = getType(arg);

                if(i < method->getArguments().size()) {
                    StapleType* definedArgType = method->getArguments()[i];
                    if(!argType->isAssignable(definedArgType)) {
                        sempass->logError(arg->location, "argument mismatch");
                    }
                }
                i++;
            }

            sempass->ctx.typeTable[methodCall] = method->getReturnType();
        } else {
            sempass->logError(methodCall->base->location, "class '%s' does not have method: '%s'",
                              classPtr->getClassName().c_str(), methodCall->name.c_str());
        }
    }

    virtual void visit(NFunctionCall* functionCall) {
        StapleType* type = scope->get(functionCall->name);

        if(StapleFunction* function = dyn_cast<StapleFunction>(type)) {
            int i = 0;
            for(auto arg : functionCall->arguments) {
                StapleType* argType = getType(arg);

                if(i < function->getArguments().size()) {
                    StapleType* definedArgType = function->getArguments()[i];
                    if(!argType->isAssignable(definedArgType)) {
                        sempass->logError(arg->location, "argument mismatch");
                    }
                }
                i++;
            }

            sempass->ctx.typeTable[functionCall] = function->getReturnType();
        } else {
            sempass->logError(functionCall->location, "undefined function: '%s'", functionCall->name.c_str());
        }
    }

    virtual void visit(NLoad* load) {
        StapleType* type = getType(load->expr);
        if(StaplePointer* ptrType = dyn_cast<StaplePointer>(type)) {
            sempass->ctx.typeTable[load] = ptrType->getElementType();
        }
        sempass->ctx.typeTable[load] = type;
    }

    virtual void visit(NBlock* block) {
        push();
        for(NStatement* statement : block->statements){
            statement->accept(this);
        }
        pop();
    }
};

SemPass::SemPass(CompilerContext& ctx)
: ctx(ctx)
, numErrors(0) {

}

bool SemPass::hasErrors() {
    return numErrors > 0;
}

void SemPass::doSemPass(NCompileUnit& root)
{
    TypeVisitor typeVisitor(this);
    root.accept(&typeVisitor);
}

void SemPass::logError(YYLTYPE location, const char *format, ...)
{
    numErrors++;
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "%s:%d:%d: ", location.filename, location.first_line, location.first_column);
    fprintf(stderr, "error: ");
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

void SemPass::logWarning(YYLTYPE location, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);

    fprintf(stderr, "%s:%d:%d: ", location.filename, location.first_line, location.first_column);
    fprintf(stderr, "warning: ");
    vfprintf(stderr, format, argptr);
    va_end(argptr);
}

}


