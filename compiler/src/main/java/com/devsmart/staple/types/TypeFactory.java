package com.devsmart.staple.types;

import com.devsmart.staple.Scope;

public class TypeFactory {

	public static StapleType getType(String typeStr, Scope mCurrentScope) {
		
		StapleType retval = null;
		if("bool".equals(typeStr)){
			retval = PrimitiveType.BOOL;
		} else if("int".equals(typeStr)){
			retval = PrimitiveType.INT;
		} else if("void".equals(typeStr)){
			retval = PrimitiveType.VOID;
		} else if("...".equals(typeStr)){
			retval = PrimitiveType.ELIPSE;
		} else if("byte".equals(typeStr)){
			retval = PrimitiveType.BYTE;
		}
		
		return retval;
	}

}
