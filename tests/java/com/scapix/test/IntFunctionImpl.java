package com.scapix.test;

// Wraps C++ std::function: tests/source/function.cpp

final class IntFunctionImpl extends com.scapix.Function implements IntFunction
{
	private IntFunctionImpl(long p) { super(p); }
	public native int call(int v);
}
