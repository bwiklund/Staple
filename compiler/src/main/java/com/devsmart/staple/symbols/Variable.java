package com.devsmart.staple.symbols;


import com.devsmart.staple.Symbol;
import com.devsmart.staple.type.Type;

public class Variable implements Symbol {

    public final Type type;
    public final String name;

    public Variable(Type type, String name){
        this.type = type;
        this.name = name;
    }

    @Override
    public String toString() {
        return String.format("%s %s", type.getTypeName(), name);
    }

}