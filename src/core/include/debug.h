
#ifdef __cplusplus
extern "C" {
#endif

// debug functions
/** @brief Marco for adding output in debug build.
 * @details
 * This macro is a wrapper for `printf`, and will be compiled out if the code is not
 * compiled with a debug build.
 * @param X The string to pass to `printf`
 * @param ... The remaining values to pass to `printf`.
 */
#if defined(WITH_DEBUG)
#define MPIPCL_DEBUG(X, ...) printf(X, ##__VA_ARGS__);
#else
#define MPIPCL_DEBUG(X, ...)
#endif

#ifdef __cplusplus
}
#endif
