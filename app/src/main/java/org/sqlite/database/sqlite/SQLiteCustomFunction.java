package org.sqlite.database.sqlite;

/**
 * Companion class for Growtopia's bundled libsqliteX.so.
 *
 * <p>Growtopia forks the open-source "requery/sqlite-android" library (its
 * native library is literally named libsqliteX.so, matching that project's
 * own build output name), which ships this class as part of its own Java
 * side. We only run Growtopia's native libraries, not its Java classes, so
 * this class was missing from our classpath entirely -- and libsqliteX.so's
 * JNI_OnLoad eagerly caches this class's field/method IDs (for its
 * user-defined SQL function callback bridge) unconditionally at load time.
 * Missing it took down the whole process: FindClass failed with
 * ClassNotFoundException, and the native code then called GetFieldID while
 * that exception was still pending, which is a JNI usage violation ART's
 * strict checking aborts on immediately.
 *
 * <p>The exact field and method names/signatures below were not guessed --
 * they were extracted directly from the real libsqliteX.so binary (strings
 * table): fields {@code name} (Ljava/lang/String;) and {@code numArgs} (I),
 * and method {@code dispatchCallback} with signature
 * {@code ([Ljava/lang/String;)V}. A native method taking
 * {@code (JLorg/sqlite/database/sqlite/SQLiteCustomFunction;)V} is also
 * present in the binary, matching a native-side "register this function on
 * this connection handle" call that passes an instance of this class in.
 *
 * <p>Nothing in this app ever needs to actually register a custom SQL
 * function, so there is no real implementation to provide here -- this
 * exists purely so libsqliteX.so's own load-time initialization has a valid
 * class to introspect.
 */
public abstract class SQLiteCustomFunction {
    public final String name;
    public final int numArgs;

    public SQLiteCustomFunction(String name, int numArgs) {
        this.name = name;
        this.numArgs = numArgs;
    }

    public abstract void dispatchCallback(String[] args);
}
