#ifndef COMMON_MACROS_H
#define COMMON_MACROS_H

#define MACRO_STR(X) STRINGIFY(X)
#define STR(X) #X

// ----------------------------------------------------------------------------------------------------------
// Format Logging--------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

#define LOG_FMT(fmt, ...) printf("%s:%d: %s() :: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...) fprintf(stderr, "%s:%d: %s() :: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef DEBUG
#define LOG_DEBUG_FMT(msg, ...) LOG_FMT(msg, ##__VA_ARGS__)
#define LOG_DEBUG_NEWLINE() printf("\n")
#else
#define LOG_DEBUG_FMT(msg, ...)
#define LOG_DEBUG_NEWLINE()
#endif
#define LOG_COND_FMT(cond, fmt, ...) do {\
    if ((cond)) LOG_FMT(fmt, ##__VA_ARGS__);\
} while(0)

#define DEBUG_LN LOG_DEBUG_FMT


// ----------------------------------------------------------------------------------------------------------
// ERROR_RETURN and its options------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------

/// this lets ERROR_RETURN log the error before returning.
#ifndef ERROR_RETURN_LOG_DISABLE
    #define ERROR_RETURN_LOG(error_code) LOG_ERROR_FMT("error: %d (%s)", error_code, #error_code)
    #define ERROR_RETURN_LOG_FMT(error_code, fmt, ...) \
        LOG_ERROR_FMT("error: %d (%s): " fmt, error_code, #error_code, ##__VA_ARGS__)
#else
    #define ERROR_RETURN_LOG(error_code)
    #define ERROR_RETURN_LOG_FMT(error_code, fmt, ...)
#endif

/// defines additional logic prior to return. This can log and assert on error_code. It can be useful for
/// propogating errors or internal errors that should be asserted during development and should never occur.
/// the logging and assertion can be individually turned off so that it functions like a normal return.
#define ERROR_RETURN(error_code) do {\
    ERROR_RETURN_LOG(error_code);\
    return error_code;\
}while(0)


/// similar to ERROR_RETURN, but this also takes a custom error message.
#define ERROR_RETURN_FMT(error_code, fmt, ...) do {\
    ERROR_RETURN_LOG_FMT(error_code, fmt, ##__VA_ARGS__);\
    return error_code;\
}while(0)

#define ERROR_RAISE_ASSERT_SIGNAL() free((void*)1)

/// this is used to issue a fault signal and can be used to generate a backtrace, and also
/// enables examination with gdb. Replace an ERROR_RETURN with this for inspection.
#define ERROR_ASSERT(error_code) do {\
    ERROR_RETURN_LOG(error_code);\
    LOG_ERROR_FMT("assertion: raising segfault");\
    ERROR_RAISE_ASSERT_SIGNAL();\
    __builtin_unreachable();\
}while(0)

/// similar to ERROR_ASSERT, but this also takes a custom error message.
#define ERROR_ASSERT_FMT(error_code, fmt, ...) do {\
    ERROR_RETURN_LOG_FMT(error_code, fmt, ##__VA_ARGS__);\
    LOG_ERROR_FMT("assertion: raising segfault");\
    ERROR_RAISE_ASSERT_SIGNAL();\
    __builtin_unreachable();\
}while(0)

/// asserts going by a similar name to ERROR_RETURN for ease of changing ERROR_RETURN calls to ASSERT instead
#define ASSERT_ERROR_RETURN ERROR_ASSERT
#define ASSERT_ERROR_RETURN_FMT ERROR_ASSERT_FMT

/// asserts are used only in DEBUG releases but are removed otherwise so only use them for internal errors, expectations from caller, 
/// and illegal states
#ifdef DEBUG
#define ASSERT(bool_expr) do {\
    if (!__builtin_expect(bool_expr, true)) {\
        LOG_ERROR_FMT("assertion %s failed: raising segfault", #bool_expr);\
        ERROR_RAISE_ASSERT_SIGNAL();\
        __builtin_unreachable();\
    }\
}while(0)

#define ASSERT_FMT(bool_expr, fmt, ...) do {\
    if (!__builtin_expect(bool_expr, true)) {\
        LOG_ERROR_FMT("assertion%s failed: %s: raising segfault", #bool_expr, fmt, ##__VA_ARGS__);\
        ERROR_RAISE_ASSERT_SIGNAL();\
        __builtin_unreachable();\
    }\
}while(0)
#else
#define ASSERT(bool_expr)
#define ASSERT_FMT(bool_expr, fmt, ...)
#endif

/// macros for functions that shouldn't be called yet
#define ERROR_NOT_IMPLEMENTED() do{\
    ERROR_ASSERT_FMT(-1, "not implemented");\
    return;\
}while(0)

#define ERROR_NOT_IMPLEMENTED_RETURN(return_expr) do {\
    ERROR_ASSERT_FMT(-1, "not implemented");\
    return (return_expr);\
}while(0)

// ----------------------------------------------------------------------------------------------------------
// EXPECT Macros---------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
/* - EXPECT macros are wrappers around error handling function calls. EXPECT assumes a call convention and
 * simplifies the error handling based on an output parameter and a return error. It turns the error handling
 * logic into an expression so that it can be used simultaneously with other expressions or passed to 
 * function calls. This feature is available for GCC compilers but not for all compilers such as msvc.
 * - ASSERT_EXPECT.* variants are similar but they don't return, they assert a fault.
 * - EXPECT_PASS and EXPECT_TRUE only perform error handling on error return, and no redirection of output params.
 */

/// warning: this is valid with gcc compilers but may not be supported by some compilers
///
/// this is used by functions that return the same error code type. If a function call is expected to be valid,
/// it error returns on failure and just routes the actual result as an expression.
/// it follows this convention: int func(T *res, ...) -> EXPECT(T, func, ...) which is an expression of type T.
/// 
/// note that only output parameters that can be passed as expressions are evaluated through this.
#define EXPECT(func, T, ...) ({\
    T res;\
    int expect_error = (int)func(&res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_RETURN(expect_error);}\
    res;\
})

#define ASSERT_EXPECT(func, T, ...) ({\
    T res;\
    int expect_error = (int)func(&res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_ASSERT(expect_error);}\
    res;\
})

/// this is like EXPECT, but it uses a different convention for method-like functions taking a self pointer.
/// it follows this convention: int func(self, T *res, ...) -> EXPECT_S(T, func, self, ...) which is an expression of type T.
#define EXPECT_S(func, self, T, ...) ({\
    T res;\
    int expect_error = (int)func(self, &res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_RETURN(expect_error);}\
    res;\
})

#define ASSERT_EXPECT_S(func, self, T, ...) ({\
    T res;\
    int expect_error = (int)func(self, &res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_ASSERT(expect_error);}\
    res;\
})

/// similar to EXPECT, but if an error occurs it ERROR_RETURNs a specified error.
#define EXPECT_OR_REDIR(err, func, T, ...) ({\
    T res;\
    int expect_error = (int)func(&res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_RETURN(err);}\
    res;\
})

#define ASSERT_EXPECT_OR_REDIR(err, func, T, ...) ({\
    T res;\
    int expect_error = (int)func(&res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_ASSERT(err);}\
    res;\
})

#define EXPECT_S_OR_REDIR(err, func, self, T, ...) ({\
    T res;\
    int expect_error = (int)func(self, &res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_RETURN(err);}\
    res;\
})

#define ASSERT_EXPECT_S_OR_REDIR(err, func, self, T, ...) ({\
    T res;\
    int expect_error = (int)func(self, &res, ##__VA_ARGS__);\
    if (expect_error != 0) {ERROR_ASSERT(err);}\
    res;\
})

/// similar to EXPECT, but evaluates to a default expression in case of failure
/// unlike other variants, this cannot be asserted since execution always carries on.
#define EXPECT_OR_ELSE(err_default, func, T, ...) ({\
    T res;\
    int expect_error = (int)func(&res, ##__VA_ARGS__);\
    expect_error == 0 ? res : err_default;\
})

/// takes an expression that evaluates to an error code such as the return of a function call and ERROR_RETURN its error code if it's not 0
#define EXPECT_PASS(expr) do{\
    int expect_error = (int)(expr);\
    if (expect_error != 0) ERROR_RETURN(expect_error);\
}while(0)

#define ASSERT_EXPECT_PASS(expr) do{\
    int expect_error = (int)(expr);\
    if (expect_error != 0) ERROR_ASSERT(expect_error);\
}while(0)

/// like EXPECT_PASS, but in case of error, it ERROR_RETURNs a specified error
#define EXPECT_PASS_OR_REDIR(expr, output_error_code) do{\
    if ((expr) != 0) ERROR_RETURN(output_error_code);\
}while(0)

#define ASSERT_EXPECT_PASS_OR_REDIR(expr, output_error_code) do{\
    if ((expr) != 0) ERROR_ASSERT(output_error_code);\
}while(0)

/// takes a boolean expression such as the return of a function call and ERROR_RETURNs it if it's false
#define EXPECT_TRUE(expr) do{\
    if (!(expr)) ERROR_RETURN(false);\
}while(0)

#define ASSERT_EXPECT_TRUE(expr) do{\
    if (!(expr)) ERROR_ASSERT(false);\
}while(0)

/// like EXPECT_TRUE, but in case of error, it ERROR_RETURNs a specified error
#define EXPECT_TRUE_OR_REDIR(expr, output_error_code) do{\
    if (!(expr)) ERROR_RETURN(output_error_code);\
}while(0)

#define ASSERT_EXPECT_TRUE_OR_REDIR(expr, output_error_code) do{\
    if (!(expr)) ERROR_ASSERT(output_error_code);\
}while(0)

#endif // COMMON_MACROS_H
