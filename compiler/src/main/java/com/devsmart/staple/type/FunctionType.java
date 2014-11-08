package com.devsmart.staple.type;

import com.devsmart.staple.symbols.Argument;

import java.util.Arrays;

public class FunctionType implements Type {

    public final String name;
    public final Type returnType;
    public final Argument[] arguments;

    public FunctionType(String name, Type returnType, Argument[] arguments){
        this.name = name;
        this.returnType = returnType;
        this.arguments = arguments;
    }

    @Override
    public String getTypeName() {
        return name;
    }

    @Override
    public String toString() {

        return String.format("%s %s", returnType, Arrays.toString(arguments));
    }
}
