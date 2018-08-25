1. use Javadoc style comments (for doxygen)
    /**
     * example
     */

2. comment functions:
    Document how to use the function in the header file, or more accurately close to the declaration.

    Document how the function works (if it's not obvious from the code) in the source file, or more accurately, close to the definition.

3. for functions, return type is on 1st line, then func name and arguments. arguments should be lined up, 1 per line.
example:

    static void
    activate (GtkApplication* app,
              gpointer        user_data)

4.
