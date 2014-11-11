package com.devsmart.staple.type;


public class PointerType implements Type {

    public final Type baseType;

    public PointerType(Type baseType){
        this.baseType = baseType;
    }

    @Override
    public String getTypeName() {
        return baseType.toString() + "*";
    }

    @Override
    public boolean isAssignableTo(Type dest) {
        boolean retval = false;
        if(dest instanceof PointerType){
            retval = baseType.isAssignableTo(((PointerType) dest).baseType);
        }
        return retval;
    }

    @Override
    public String toString() {
        return getTypeName();
    }
}
