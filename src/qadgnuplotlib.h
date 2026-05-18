/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        Declares the public C API for qadgnuplotlib.
*/

#ifndef QADGNUPLOTLIB_H
#define QADGNUPLOTLIB_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef QADGNUPLOTLIB_API
#define QADGNUPLOTLIB_API
#endif

/*
 * Low-level bootstrap API.
 *
 * qadgnuplotlib_execute_script() executes a script using the embedded gnuplot
 * interpreter directly.  It does not provide a C++ Context and therefore
 * cannot use registered mem:// data sources.
 */
QADGNUPLOTLIB_API const char *qadgnuplotlib_version(int verbose);
QADGNUPLOTLIB_API int qadgnuplotlib_execute_script(const char *script);
QADGNUPLOTLIB_API int qadgnuplotlib_set_log_file(const char *path);

/*
 * Last-error helper for the C API below.
 *
 * Functions returning int use 0 for success and non-zero for failure.
 * Functions returning pointers return NULL on failure.
 * In both cases qadgnuplotlib_last_error() returns a thread-local diagnostic
 * string until the next C API call on the same thread.
 */
QADGNUPLOTLIB_API const char *qadgnuplotlib_last_error(void);
QADGNUPLOTLIB_API void qadgnuplotlib_clear_last_error(void);

/* Opaque handles for the C facade over the C++ API. */
typedef struct qadgnuplotlib_context qadgnuplotlib_context;
typedef struct qadgnuplotlib_script qadgnuplotlib_script;

/* Context lifetime and execution. */
QADGNUPLOTLIB_API qadgnuplotlib_context *qadgnuplotlib_create(void);
QADGNUPLOTLIB_API void qadgnuplotlib_destroy(qadgnuplotlib_context *ctx);

QADGNUPLOTLIB_API int qadgnuplotlib_run(qadgnuplotlib_context *ctx, const char *script_text);
QADGNUPLOTLIB_API int qadgnuplotlib_run_script(qadgnuplotlib_context *ctx, const qadgnuplotlib_script *script);

/* Registered column data for mem:// sources.  Data is borrowed, not copied. */
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_float32(qadgnuplotlib_context *ctx, const char *name, const float *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_float64(qadgnuplotlib_context *ctx, const char *name, const double *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_int8(qadgnuplotlib_context *ctx, const char *name, const int8_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_uint8(qadgnuplotlib_context *ctx, const char *name, const uint8_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_int16(qadgnuplotlib_context *ctx, const char *name, const int16_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_uint16(qadgnuplotlib_context *ctx, const char *name, const uint16_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_int32(qadgnuplotlib_context *ctx, const char *name, const int32_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_uint32(qadgnuplotlib_context *ctx, const char *name, const uint32_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_int64(qadgnuplotlib_context *ctx, const char *name, const int64_t *data, size_t count);
QADGNUPLOTLIB_API int qadgnuplotlib_register_data_uint64(qadgnuplotlib_context *ctx, const char *name, const uint64_t *data, size_t count);

/* Registered image data.  Data is borrowed, not copied. */
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_float32(qadgnuplotlib_context *ctx, const char *name, const float *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_float64(qadgnuplotlib_context *ctx, const char *name, const double *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_int8(qadgnuplotlib_context *ctx, const char *name, const int8_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_uint8(qadgnuplotlib_context *ctx, const char *name, const uint8_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_int16(qadgnuplotlib_context *ctx, const char *name, const int16_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_uint16(qadgnuplotlib_context *ctx, const char *name, const uint16_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_int32(qadgnuplotlib_context *ctx, const char *name, const int32_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_uint32(qadgnuplotlib_context *ctx, const char *name, const uint32_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_int64(qadgnuplotlib_context *ctx, const char *name, const int64_t *data, size_t width, size_t height, size_t channels);
QADGNUPLOTLIB_API int qadgnuplotlib_register_image_uint64(qadgnuplotlib_context *ctx, const char *name, const uint64_t *data, size_t width, size_t height, size_t channels);

QADGNUPLOTLIB_API int qadgnuplotlib_unregister_data(qadgnuplotlib_context *ctx, const char *name);
QADGNUPLOTLIB_API int qadgnuplotlib_clear_data(qadgnuplotlib_context *ctx);

/* Script lifetime and persistence. */
QADGNUPLOTLIB_API qadgnuplotlib_script *qadgnuplotlib_script_create(const char *script_text);
QADGNUPLOTLIB_API qadgnuplotlib_script *qadgnuplotlib_script_load_file(const char *path);
QADGNUPLOTLIB_API void qadgnuplotlib_script_destroy(qadgnuplotlib_script *script);

QADGNUPLOTLIB_API int qadgnuplotlib_script_assign(qadgnuplotlib_script *script, const char *script_text);
QADGNUPLOTLIB_API int qadgnuplotlib_script_save_file(const qadgnuplotlib_script *script, const char *path);
QADGNUPLOTLIB_API const char *qadgnuplotlib_script_text(const qadgnuplotlib_script *script);
QADGNUPLOTLIB_API size_t qadgnuplotlib_script_size(const qadgnuplotlib_script *script);

#ifdef __cplusplus
}
#endif

#endif /* QADGNUPLOTLIB_H */
