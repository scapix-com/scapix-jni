package com.scapix.test;

// Functional interface converted to/from std::function: tests/source/function.cpp

@FunctionalInterface
public interface IntFunction
{
	int call(int v);

	static IntFunction twice() { return v -> v * 2; }
	static int apply(IntFunction f, int v) { return f.call(v); }
}
