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
}
