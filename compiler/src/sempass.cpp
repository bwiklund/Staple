#include <cstdarg>

#include "node.h"
#include "codegen.h"
#include "type.h"
#include "types/stapletype.h"

#include "sempass.h"
#include "sempass/Pass1ClassVisitor.h"

namespace staple {

using namespace std;


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
    StapleClass *currentClass;
    Scope* scope;
    SemPass* sempass;

    TypeVisitor(SemPass* sempass)
    : scope(NULL)
    , sempass(sempass) {}

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

#define CheckType(type, location, name, positive) \
if(type == NULL) { \
    sempass->logError(location, "undefined type: '%s'", name.c_str()); \
} else { \
    positive \
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
        for(vector<NFunctionPrototype*>::iterator it = compileUnit->externFunctions.begin();it != compileUnit->externFunctions.end();it++) {
            NFunctionPrototype* functionPrototype = *it;

            std::vector<StapleType*> argsType;
            for(vector<NArgument*>::iterator it = functionPrototype->arguments.begin();it != functionPrototype->arguments.end();it++){
                NArgument* arg = *it;
                StapleType* type = getType(&arg->type);

                CheckType(type, arg->location, arg->name,
                        argsType.push_back(type);
                        sempass->ctx.typeTable[arg] = type;
                )
            }

            StapleType* returnType = getType(&functionPrototype->returnType);
            CheckType(returnType, functionPrototype->location, functionPrototype->returnType.name,
                    StapleFunction* functionType = new StapleFunction(returnType, argsType, functionPrototype->isVarg);
                    define(functionPrototype->name, functionType);
            )

        }

        //first pass global functions
        for(vector<NFunction *>::iterator it = compileUnit->functions.begin();it != compileUnit->functions.end();it++){
            NFunction* function = *it;

            std::vector<StapleType*> argsType;
            for(vector<NArgument*>::iterator it = function->arguments.begin();it != function->arguments.end();it++){
                NArgument* arg = *it;
                StapleType* type = getType(&arg->type);

                CheckType(type, arg->location, arg->name,
                        argsType.push_back(type);
                        sempass->ctx.typeTable[arg] = type;
                )
            }

            StapleType* returnType = getType(&function->returnType);
            CheckType(returnType, function->location, function->returnType.name,
                    StapleFunction* functionType = new StapleFunction(returnType, argsType, function->isVarg);
                    define(function->name, functionType);
            )
        }

        //second pass class fields and methods
        for(vector<NClassDeclaration*>::iterator it = compileUnit->classes.begin();it != compileUnit->classes.end();it++){
            NClassDeclaration* classDeclaration = *it;
            currentClass = cast<StapleClass>(scope->get(classDeclaration->name));

            for(auto field=classDeclaration->fields.begin();field != classDeclaration->fields.end();field++) {
                (*field)->accept(this);
            }

            for(NMethodFunction* method : classDeclaration->functions) {

                std::vector<StapleType*> args;
                for(NArgument* arg : method->arguments){
                    StapleType* type = getType(&arg->type);
                    CheckType(type, arg->location, arg->type.name, args.push_back(type); )
                }

                StapleType* returnType = getType(&method->returnType);
                CheckType(returnType, method->returnType.location, method->returnType.name, )


                StapleMethodFunction* functionType = currentClass->addMethod(method->name, returnType, args, method->isVarg);
                method->classType = currentClass;
            }

            for(auto method=classDeclaration->functions.begin();method != classDeclaration->functions.end();method++) {
                (*method)->accept(this);
            }

        }

        //second pass global functions
        for(vector<NFunction *>::iterator it = compileUnit->functions.begin();it != compileUnit->functions.end();it++){
            (*it)->accept(this);
        }

        currentClass = NULL;

        pop();
    }

    virtual void visit(NType* type) {

        const string name = type->name;

        StapleType* retval = NULL;
        if(name.compare("void") == 0) {
            retval = StapleType::getVoidType();
        } else if(name.compare("int") == 0 || name.compare("int32") == 0){
            retval = StapleType::getInt32Type();
        } else if(name.compare("int8") == 0) {
            retval = StapleType::getInt8Type();
        } else if(name.compare("int16") == 0) {
            retval = StapleType::getInt16Type();
        } else if(name.compare("float") == 0 || name.compare("float32") == 0) {
            retval = StapleType::getFloat32Type();
        } else if(name.compare("bool") == 0) {
            retval = StapleType::getBoolType();
        } else {

            retval = scope->get(name);
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
        expressionStatement->expression->accept(this);
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

        for(vector<NStatement*>::iterator it = methodFunction->block.statements.begin();it != methodFunction->block.statements.end();it++){
            NStatement* statement = *it;
            statement->accept(this);
        }
        pop();
    }

    virtual void visit(NFunction* function) {
        push();
        for(vector<NArgument*>::iterator it = function->arguments.begin();it != function->arguments.end();it++){
            NArgument* arg = *it;
            StapleType* type = getType(&arg->type);

            CheckType(type, arg->location, arg->type.name,
                    define(arg->name, type);
                    sempass->ctx.typeTable[arg] = type;
            )
        }

        StapleType* returnType = getType(&function->returnType);
        CheckType(returnType, function->returnType.location, function->returnType.name, )

        for(vector<NStatement*>::iterator it = function->block.statements.begin();it != function->block.statements.end();it++){
            NStatement* statement = *it;
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
        StapleType* type = scope->get(newNode->id);

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

        if((ptr = dyn_cast<StaplePointer>(baseType))
                && (classPtr = dyn_cast<StapleClass>(ptr->getElementType()))) {

            sempass->ctx.typeTable[memberAccess] = new StaplePointer(classPtr);

        }

        if(baseType == NULL || (!baseType->isClassTy() && !(baseType->isPointerTy() && ((SPointerType*)baseType)->elementType->isClassTy()))) {
            sempass->logError(memberAccess->base->location, "not a class type");
        } else {
            SClassType* classType = NULL;
            if(baseType->isClassTy()){
                classType = (SClassType*) baseType;
            } else {
                classType = (SClassType *) ((SPointerType*)baseType)->elementType;
            }

            int index = classType->getFieldIndex(memberAccess->field);
            if(index < 0) {
                sempass->logError(memberAccess->location, "class '%s' does not have field: '%s'", classType->name.c_str(), memberAccess->field.c_str());
            } else {
                memberAccess->fieldIndex = index + 1;
                sempass->ctx.typeTable[memberAccess] = classType->fields[index].second;
            }
        }
    }

    virtual void visit(NIfStatement* ifStatement) {
        ifStatement->condition->accept(this);
        StapleType* conditionType = sempass->ctx.typeTable[ifStatement->condition];
        if(!conditionType->type->isIntegerTy(1)) {
            sempass->logError(ifStatement->condition->location, "if condition is not a boolean");
        }

        ifStatement->thenBlock->accept(this);
        if(ifStatement->elseBlock != NULL) {
            ifStatement->elseBlock->accept(this);
        }
    }

    virtual void visit(NBinaryOperator* binaryOperator) {
        binaryOperator->lhs->accept(this);
        binaryOperator->rhs->accept(this);

        StapleType* returnType;



        switch(binaryOperator->op) {
            case TCEQ:
            case TCNE:
            case TCGT:
            case TCLT:
            case TCGE:
            case TCLE:
                returnType = SType::get(llvm::Type::getInt1Ty(getGlobalContext()));
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
        methodCall->base->accept(this);
        StapleType* baseType = sempass->ctx.typeTable[methodCall->base];

        if(baseType == NULL || (!baseType->isClassTy() && !(baseType->isPointerTy() && ((SPointerType*)baseType)->elementType->isClassTy()))) {
            sempass->logError(methodCall->base->location, "not a class type");
        } else {

            SClassType* classType = NULL;
            if(baseType->isClassTy()){
                classType = (SClassType*) baseType;
            } else {
                classType = (SClassType *) ((SPointerType*)baseType)->elementType;
            }

            int index = classType->getMethodIndex(methodCall->name);
            if(index < 0) {
                sempass->logError(methodCall->location, "class '%s' does not have method: '%s'", classType->name.c_str(), methodCall->name.c_str());
            } else {
                methodCall->methodIndex = index;
                methodCall->classType = classType;
                StapleFunction* method = classType->getMethod(methodCall->name);

                int i = 0;
                for(auto it=methodCall->arguments.begin();it!=methodCall->arguments.end();it++) {
                    (*it)->accept(this);
                    StapleType* argType = sempass->ctx.typeTable[*it];

                    if(i < method->arguments.size()) {
                        StapleType *definedType = method->arguments[i];
                        if (!argType->isAssignable(definedType)) {
                            sempass->logError((*it)->location, "argument mismatch");
                        }
                    }

                    i++;
                }

                sempass->ctx.typeTable[methodCall] = method->returnType;
            }


        }
    }

    virtual void visit(NFunctionCall* methodCall) {
        StapleType* type = scope->get(methodCall->name);
        if(type != NULL && type->isFunctionTy()){

            StapleFunction* function = (StapleFunction*) type;

            int i = 0;
            for(auto it=methodCall->arguments.begin();it!=methodCall->arguments.end();it++) {
                (*it)->accept(this);
                StapleType* argType = sempass->ctx.typeTable[*it];

                if(i < function->arguments.size()) {
                    StapleType *definedType = function->arguments[i];
                    if (!argType->isAssignable(definedType)) {
                        sempass->logError((*it)->location, "argument mismatch");
                    }
                }

                i++;
            }


            sempass->ctx.typeTable[methodCall] = function->returnType;
        } else {
            sempass->logError(methodCall->location, "undefined function: '%s'", methodCall->name.c_str());
        }


    }

    virtual void visit(NLoad* load) {
        load->expr->accept(this);
        sempass->ctx.typeTable[load] = sempass->ctx.typeTable[load->expr];
    }

    virtual void visit(NBlock* block) {
        push();
        for(vector<NStatement*>::iterator it = block->statements.begin();it != block->statements.end();it++){
            NStatement* statement = *it;
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


