package com.scapix.test;

// Native methods implemented in C++: tests/source/native_method.cpp

public class NativeTest
{
	public long ptr; // C++ counter object, see convert_this()
	public int value;

	public NativeTest(int value) { this.value = value; }

	// static, ref<class_> first parameter, JNI signature deduced

	public static native int add(int a, int b);
	public static native String nullString();

	// static, primitive types

	public static native boolean notBoolean(boolean v);
	public static native byte incByte(byte v);
	public static native char incChar(char v);
	public static native short incShort(short v);
	public static native int incInt(int v);
	public static native long incLong(long v);
	public static native float halfFloat(float v);
	public static native double halfDouble(double v);
	public static native double sumAll(boolean z, byte b, char c, short s, int i, long j, float f, double d);
	public static native void setStatic(int v);
	public static native int getStatic();

	// static, no ref<class_> parameter, JNI signature specified

	public static native int mul(int a, int b);

	// static, C++ types

	public static native String concat(String a, String b);
	public static native String concatRef(String a, String b);
	public static native int[] range(int n);
	public static native int sum(int[] a);
	public static native String[] repeat(String s, int n);
	public static native boolean isEven(int v);
	public static native int nextColor(int color);

	// static, overloaded C++ functions

	public static native int twiceInt(int v);
	public static native long twiceLong(long v);
	public static native int negInt(int v);
	public static native long negLong(long v);
	public static native String labelInt(int v);
	public static native String labelDouble(double v);
	public static native String describeInt(int v);
	public static native String describeDouble(double v);

	// static member function

	public static native int staticMember(int v);

	// instance, ref<NativeTest> first parameter

	public native int plusValue(int a);
	public native String greet(String name);
	public native String greetCpp(String name);

	// instance, C++ member function

	public native int counterAdd(int a);
	public native int counterGet();
	public native void counterSet(int v);
	public native int counterGetNoexcept();
	public native String counterName();
	public native int counterMulInt(int v);
	public native long counterMulLong(long v);
	public native String counterFormatInt(int v);
	public native String counterFormatDouble(double v);

	// exceptions

	public static native void throwCpp(String message);
	public static native void throwJava(String message);
	public native void counterThrow();
	public static void throwIllegalState(String message) { throw new IllegalStateException(message); }

	// called from Java

	public static int callAdd(int a, int b) { return add(a, b); }
	public int callCounterAdd(int a) { return counterAdd(a); }
	public String callGreet(String name) { return greet(name); }
	public static void callThrowCpp(String message) { throwCpp(message); }

	public static String catchThrowCpp(String message)
	{
		try
		{
			throwCpp(message);
			return null;
		}
		catch (RuntimeException e)
		{
			return e.getClass().getName() + ": " + e.getMessage();
		}
	}

	public static String catchThrowJava(String message)
	{
		try
		{
			throwJava(message);
			return null;
		}
		catch (RuntimeException e)
		{
			return e.getClass().getName() + ": " + e.getMessage();
		}
	}
}
