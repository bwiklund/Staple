package com.devsmart.staple;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.ExecutionException;

import org.antlr.v4.runtime.ANTLRFileStream;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.ParserRuleContext;
import org.antlr.v4.runtime.tree.ParseTree;
import org.antlr.v4.runtime.tree.ParseTreeWalker;
import org.stringtemplate.v4.STGroup;
import org.stringtemplate.v4.STGroupFile;

/**
 * Hello world!
 *
 */
public class Compiler  {
	
    public static void main( String[] args ) {
    	
        
    }
    
    public static int compile(File file) throws IOException {
    	
    	CompileContext context = new CompileContext();
    	
    	context.codegentemplate = new STGroupFile(Compiler.class.getResource("llvm.stg"), "UTF-8", '<', '>');
    	context.file = file;
    	
    	CharStream input = new ANTLRFileStream(file.getAbsolutePath());
    	StapleLexer lex = new StapleLexer(input);
    	CommonTokenStream tokens = new CommonTokenStream(lex);
    	StapleParser parser = new StapleParser(tokens);
    	
    	//Parse
    	ParserRuleContext tree = parser.compileUnit();
    	
    	//Sem Pass 1
    	SemPass1 sempass1 = new SemPass1(context);
    	sempass1.visit(tree);
    	
    	if(context.errorStream.hasErrors()){
    		context.errorStream.print(System.out);
    		return 1;
    	}
    	
    	SemPass2 sempass2 = new SemPass2(context);
    	sempass2.visit(tree);
    	
    	if(context.errorStream.hasErrors()){
    		context.errorStream.print(System.out);
    		return 1;
    	}
    	
    	//Code Generate
    	CodeGenerator codeGenerator = new CodeGenerator(context);
    	codeGenerator.visit(tree);
    	codeGenerator.render(context.codegentemplate);
    	
    	//ParserRuleContext tree = parser.compileUnit();
    	tree.inspect(parser);
		
    	
    	System.out.println( tree.toStringTree(parser) );
    	
    	return 0;
    }
    
    
}
