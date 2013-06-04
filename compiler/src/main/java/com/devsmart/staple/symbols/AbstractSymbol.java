package com.devsmart.staple.symbols;

public abstract class AbstractSymbol implements StapleSymbol {

	protected String mName;
	
	public AbstractSymbol(String name) {
		mName = name;
	}
	
	@Override
	public String getName() {
		return mName;
	}

}
