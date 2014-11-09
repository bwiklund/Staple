package com.devsmart.staple;


import com.devsmart.staple.symbols.Argument;
import com.devsmart.staple.symbols.Field;
import com.devsmart.staple.symbols.LocalVariable;
import com.devsmart.staple.type.*;

import org.antlr.v4.runtime.misc.NotNull;
import org.antlr.v4.runtime.tree.TerminalNode;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class SemPass2 extends StapleBaseVisitor<Void> {

    private final CompilerContext compilerContext;
    private Scope currentScope;
    private ClassType currentClass;

    public SemPass2(CompilerContext ctx) {
        compilerContext = ctx;
        currentScope = compilerContext.rootScope;
    }

    private void pushScope() {
        currentScope = new Scope(currentScope);
    }

    private void popScope() {
        if(currentScope != null){
            currentScope = currentScope.mParent;
        }
    }

    @Override
    public Void visitClassDecl(@NotNull StapleParser.ClassDeclContext ctx) {

        ClassType lastClass = currentClass;
        currentClass = (ClassType) compilerContext.symbols.get(ctx);

        StapleParser.ExtendsDeclContext extendDecl = ctx.extendsDecl();
        if(extendDecl != null) {
            visit(ctx.extendsDecl());
        } else {
            currentClass.parent = com.devsmart.staple.runtime.Runtime.BaseObject;
        }

        Scope parentScope = currentScope;
        currentScope = currentClass.scope;

        //add init function
        {
            FunctionType initFunction = FunctionType.memberFunction("init", PrimitiveType.Void, new Argument[]{new Argument(new PointerType(PrimitiveType.Void), "self")});
            currentClass.functions.add(initFunction);
        }

        for(StapleParser.ClassDeclContext internalClass : ctx.classDecl()){
            visit(internalClass);
        }

        for(StapleParser.ClassMemberDeclContext memberDecl : ctx.classMemberDecl()){
            visit(memberDecl);
        }

        for(StapleParser.ClassFunctionDeclContext functionDecl : ctx.classFunctionDecl()){
            visit(functionDecl);
        }

        currentScope = parentScope;
        currentClass = lastClass;

        return null;
    }

    @Override
    public Void visitExtendsDecl(@NotNull StapleParser.ExtendsDeclContext ctx) {

        final String className = ctx.Identifier().getText();
        Symbol symbol = currentScope.get(className);
        if(symbol == null){
            compilerContext.errorStream.error("could not find class: " + className, ctx);
        } else if(!(symbol instanceof ClassType)){
            compilerContext.errorStream.error("symbol is not of class type: " + className, ctx);
        } else {
            currentClass.parent = (ClassType) symbol;
            compilerContext.symbols.put(ctx, symbol);
        }
        return null;
    }

    @Override
    public Void visitClassMemberDecl(@NotNull StapleParser.ClassMemberDeclContext ctx) {

        final StapleParser.TypeContext typeCtx = ctx.type();
        visit(typeCtx);
        Type type = (Type) compilerContext.symbols.get(typeCtx);
        if(type == null){
            compilerContext.errorStream.error("Could not determine type: " + typeCtx.getText(), typeCtx);
        } else {
            final String name = ctx.Identifier().getText();
            if(currentClass.getField(name) != null){
                compilerContext.errorStream.error("redefinition of field: " + name, ctx);
            } else {
                Field field = new Field(type, name);
                currentClass.fields.add(field);
                compilerContext.symbols.put(ctx, field);

                currentScope.put(name, field);
            }

        }

        return null;
    }

    @Override
    public Void visitClassFunctionDecl(@NotNull StapleParser.ClassFunctionDeclContext ctx) {

        final String name = ctx.Identifier().getText();

        final String retvalTypeStr = ctx.type().getText();
        Type returnType = getType(retvalTypeStr);

        visit(ctx.argList());

        ArrayList<Argument> arguments = (ArrayList<Argument>) compilerContext.symbols.get(ctx.argList());

        Argument[] argArray = arguments.toArray(new Argument[arguments.size()]);
        FunctionType functionType = FunctionType.memberFunction(name, returnType, argArray);

        currentClass.functions.add(functionType);
        compilerContext.symbols.put(ctx, functionType);

        Scope blockScope = new Scope(currentScope);
        StapleParser.BlockContext blockCtx = ctx.block();
        compilerContext.scope.put(blockCtx, blockScope);

        for(Argument arg : arguments){
            blockScope.put(arg.name, arg);
        }

        visit(blockCtx);

        return null;
    }

    @Override
    public Void visitArgList(@NotNull StapleParser.ArgListContext ctx) {
        List<StapleParser.TypeContext> types = ctx.type();
        List<TerminalNode> names = ctx.Identifier();
        ArrayList<Argument> args = new ArrayList<Argument>(names.size());
        for(int i=0;i<names.size();i++){
            StapleParser.TypeContext typeCtx = types.get(i);
            visit(typeCtx);
            Type type = (Type) compilerContext.symbols.get(typeCtx);
            Argument arg = new Argument(type, names.get(i).getText());
            args.add(arg);
        }

        compilerContext.symbols.put(ctx, args);

        return null;
    }

    @Override
    public Void visitBlock(@NotNull StapleParser.BlockContext ctx) {

        //check to see if the scope already exists for this block
        Scope blockScope = compilerContext.scope.get(ctx);
        if(blockScope == null){
            blockScope = new Scope(currentScope);
            compilerContext.scope.put(ctx, blockScope);
        } else {
            currentScope = blockScope;
        }

        visitChildren(ctx);
        popScope();

        return null;
    }

    @Override
    public Void visitLocalVariableDeclaration(@NotNull StapleParser.LocalVariableDeclarationContext ctx) {

        final StapleParser.TypeContext typeCtx = ctx.type();
        visit(typeCtx);
        Type type = (Type) compilerContext.symbols.get(typeCtx);

        final String name = ctx.Identifier().getText();
        Symbol conflictSymbol = currentScope.get(name);
        if(conflictSymbol != null){
            compilerContext.errorStream.error("redefinition of " + name, ctx);
        } else {

            LocalVariable localVariable = new LocalVariable(type, name);
            currentScope.put(name, localVariable);
            compilerContext.symbols.put(ctx, localVariable);
        }

        return null;
    }

    @Override
    public Void visitType(@NotNull StapleParser.TypeContext ctx) {
        String basetypeStr = ctx.primitiveType() != null ? ctx.primitiveType().getText() : ctx.Identifier().getText();
        Type baseType = getType(basetypeStr);

        if(baseType == null){
            compilerContext.errorStream.error("unknown type: " + basetypeStr, ctx);
        }

        Type theType = baseType;
        if(ctx.POINTER() != null) {
            theType = new PointerType(baseType);
        }
        compilerContext.symbols.put(ctx, theType);

        return null;
    }

    public static HashMap<String, PrimitiveType> PrimitiveTypes = new HashMap<String, PrimitiveType>();
    static {
        PrimitiveTypes.put("void", PrimitiveType.Void);
        PrimitiveTypes.put("int", PrimitiveType.Int32);
        PrimitiveTypes.put("uint", PrimitiveType.UInt32);

        PrimitiveTypes.put("int8", PrimitiveType.Int8);
        PrimitiveTypes.put("int16", PrimitiveType.Int16);
        PrimitiveTypes.put("int32", PrimitiveType.Int32);
        PrimitiveTypes.put("int64", PrimitiveType.Int64);

        PrimitiveTypes.put("bool", PrimitiveType.Bool);
    }

    private Type getType(String typeStr) {

        Type retval = PrimitiveTypes.get(typeStr);
        if(retval == null){
            Symbol symbol = currentScope.get(typeStr);
            if(symbol != null && symbol instanceof ClassType){
                retval = (ClassType)symbol;
            }
        }

        return retval;
    }
}
