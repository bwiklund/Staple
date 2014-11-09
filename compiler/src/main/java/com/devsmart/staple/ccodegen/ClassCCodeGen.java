package com.devsmart.staple.ccodegen;


import com.devsmart.staple.CompilerContext;
import com.devsmart.staple.StapleBaseVisitor;
import com.devsmart.staple.StapleParser;
import com.devsmart.staple.symbols.Field;
import com.devsmart.staple.type.ClassType;
import com.devsmart.staple.type.FunctionType;
import org.antlr.v4.runtime.misc.NotNull;
import org.stringtemplate.v4.ST;

import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.Collection;

public class ClassCCodeGen extends StapleBaseVisitor<Void> {

    private final CompilerContext compilerContext;
    private final OutputStreamWriter output;

    public ClassCCodeGen(CompilerContext ctx, OutputStreamWriter output){
        this.compilerContext = ctx;
        this.output = output;
    }

    @Override
    public Void visitClassDecl(@NotNull StapleParser.ClassDeclContext ctx) {
        ClassType currentClassType = (ClassType) compilerContext.symbols.get(ctx);

        ST classTypeTmp = CCodeGen.codegentemplate.getInstanceOf("classTypeDecl");
        classTypeTmp.add("name", currentClassType.name);
        classTypeTmp.add("parent", (currentClassType.parent != null ? currentClassType.parent.name + "Class parent;"  : ""));
        classTypeTmp.add("functions", functions(currentClassType.functions));


        ST classObj = CCodeGen.codegentemplate.getInstanceOf("classObjDecl");
        classObj.add("name", currentClassType.name);
        classObj.add("parent", (currentClassType.parent != null ? currentClassType.parent.name + " parent;"  : ""));
        classObj.add("fields", fields(currentClassType.fields));

        try {
            String code = classTypeTmp.render();
            output.write(code);

            code = classObj.render();
            output.write(code);
        } catch (IOException e) {
            e.printStackTrace();
        }

        return null;
    }

    static String[] fields(Collection<Field> fields) {
        String[] retval = new String[fields.size()];
        int i = 0;
        for(Field field : fields){
            retval[i++] = String.format("%s %s;",
                    CCodeGen.renderType(field.type),
                    field.name);
        }
        return retval;
    }

    static String[] functions(Collection<FunctionType> functions) {
        String[] retval = new String[functions.size()];
        int i = 0;
        for(FunctionType function : functions){
            retval[i++] = CCodeGen.renderType(function) + ";";
        }
        return retval;
    }
}
