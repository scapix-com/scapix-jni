package com.scapix.test;

public class TestClass
{
	public int intField = 1;
	public String stringField = "field";
	public static int staticIntField = 2;

	public TestClass() {}
	public TestClass(int value) { intField = value; }

	public int add(int a, int b) { return a + b; }
	public static String concat(String a, String b) { return a + b; }
	public static int[] range(int n) { int[] a = new int[n]; for (int i = 0; i < n; ++i) a[i] = i; return a; }
	public static void throwIllegalState(String message) { throw new IllegalStateException(message); }

	// implemented in C++

	public static native int nativeAdd(int a, int b);
	public native String nativeGreet(String name);
	public static native void nativeThrow();

	public static int callNativeAdd(int a, int b) { return nativeAdd(a, b); }
	public String callNativeGreet(String name) { return nativeGreet(name); }
	public static void callNativeThrow() { nativeThrow(); }

	public static String catchNativeThrow()
	{
		try
		{
			nativeThrow();
			return null;
		}
		catch (RuntimeException e)
		{
			return e.getClass().getName() + ": " + e.getMessage();
		}
	}
}
