package com.devsmart.staple.symbols;

import com.devsmart.staple.types.PrimitiveType;
import com.devsmart.staple.types.StapleType;

public class MultiVarableSymbol implements StapleSymbol {

	@Override
	public String getName() {
		return "...";
	}

	@Override
	public StapleType getType() {
		return PrimitiveType.ELIPSE;
	}

}
