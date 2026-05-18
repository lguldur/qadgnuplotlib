/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        Implements the C++20 Context and Script APIs, the C API bridge, version reporting, and registered-memory data access.
*/

#include "qadgnuplotlib.hpp"

#include "../generated/qadgnuplotlib_version.h"
#include "lunasvg.h"
#include "plutovg.h"
#include "miniz.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string_view>
#include <vector>

extern "C" {
extern const char gnuplot_version[];
extern const char gnuplot_patchlevel[];
}

namespace qadgnuplotlib {
namespace {
    thread_local Context* t_current_context = nullptr;

    std::string make_key(std::string_view name) {
        return std::string(name.data(), name.size());
    }
}

Script::Script(const char* text)
    : m_text(text ? text : "")
{
}

Script::Script(std::string text)
    : m_text(std::move(text))
{
}

Script::Script(std::string_view text)
    : m_text(text.data(), text.size())
{
}

Script Script::load(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw Error("Script::load: cannot open input file");
    }

    std::ostringstream ss;
    ss << in.rdbuf();

    if (!in.good() && !in.eof()) {
        throw Error("Script::load: error while reading input file");
    }

    return Script(ss.str());
}

void Script::save(const std::filesystem::path& path) const {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw Error("Script::save: cannot open output file");
    }

    out.write(m_text.data(), static_cast<std::streamsize>(m_text.size()));
    if (!out) {
        throw Error("Script::save: error while writing output file");
    }
}

const std::string& Script::text() const noexcept {
    return m_text;
}

const char* Script::c_str() const noexcept {
    return m_text.c_str();
}

bool Script::empty() const noexcept {
    return m_text.empty();
}

std::size_t Script::size() const noexcept {
    return m_text.size();
}

void Script::assign(std::string text) {
    m_text = std::move(text);
}

void Script::clear() noexcept {
    m_text.clear();
}

Context::Context() = default;

Context::~Context() {
    if (t_current_context == this) {
        t_current_context = nullptr;
    }
}

void Context::unregister_data(std::string_view name) {
    std::scoped_lock lock(m_mutex);
    const std::string key = make_key(name);
    m_columns.erase(key);
    m_images.erase(key);
}

void Context::clear_data() {
    std::scoped_lock lock(m_mutex);
    m_columns.clear();
    m_images.clear();
}

bool Context::has_column(std::string_view name) const {
    std::scoped_lock lock(m_mutex);
    return m_columns.find(make_key(name)) != m_columns.end();
}

bool Context::has_image(std::string_view name) const {
    std::scoped_lock lock(m_mutex);
    return m_images.find(make_key(name)) != m_images.end();
}

ColumnView Context::column(std::string_view name) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_columns.find(make_key(name));
    if (it == m_columns.end()) {
        throw Error("Context::column: column not found");
    }
    return it->second;
}

ImageView Context::image(std::string_view name) const {
    std::scoped_lock lock(m_mutex);
    auto it = m_images.find(make_key(name));
    if (it == m_images.end()) {
        throw Error("Context::image: image not found");
    }
    return it->second;
}

void Context::run(const Script& script) {
    run(std::string_view(script.text()));
}

void Context::run(const std::string& script) {
    run(std::string_view(script));
}

void Context::run(std::string_view script) {
    if (script.empty()) {
        return;
    }

    std::string null_terminated(script.data(), script.size());

    Context* previous = t_current_context;
    t_current_context = this;

    const int rc = qadgnuplotlib_execute_script(null_terminated.c_str());

    t_current_context = previous;

    if (rc != 0) {
        throw Error("Context::run: gnuplot script execution failed");
    }
}

void Context::run(const char* script) {
    run(std::string_view(script ? script : ""));
}

Context* current_context() noexcept {
    return t_current_context;
}

std::string version(bool verbose) {
    std::ostringstream out;

    out << "qadgnuplotlib " << QADGNUPLOTLIB_VERSION_STRING
        << " - " << QADGNUPLOTLIB_COPYRIGHT_STRING;

    if (verbose) {
        out << '\n' << "gnuplot " << gnuplot_version;
        if (gnuplot_patchlevel[0] != '\0' && !(gnuplot_patchlevel[0] == '0' && gnuplot_patchlevel[1] == '\0')) {
            out << '.' << gnuplot_patchlevel;
        }
        out << '\n' << "LunaSVG " << LUNASVG_VERSION_STRING;
        out << '\n' << "PlutoVG " << PLUTOVG_VERSION_STRING;
        out << '\n' << "miniz 3.1.0";
        out << '\n' << "base64 embedded source (no version macro)";
    }

    return out.str();
}

} // namespace qadgnuplotlib

extern "C" const char* qadgnuplotlib_version(int verbose) {
    static thread_local std::string text;

    try {
        text = qadgnuplotlib::version(verbose != 0);
        return text.c_str();
    } catch (...) {
        return "qadgnuplotlib version unavailable";
    }
}


namespace {

std::vector<std::string_view> split_mem_uri_columns(std::string_view uri) {
    constexpr std::string_view prefix = "mem://";
    if (!uri.starts_with(prefix)) {
        throw qadgnuplotlib::Error("not a mem:// URI");
    }

    std::string_view body = uri.substr(prefix.size());
    if (body.empty()) {
        throw qadgnuplotlib::Error("empty mem:// URI");
    }

    std::vector<std::string_view> names;
    while (!body.empty()) {
        const std::size_t pos = body.find(',');
        std::string_view part = body.substr(0, pos);
        if (part.empty()) {
            throw qadgnuplotlib::Error("empty column name in mem:// URI");
        }
        names.push_back(part);
        if (pos == std::string_view::npos) {
            break;
        }
        body.remove_prefix(pos + 1);
    }

    return names;
}

double column_value_as_double(const qadgnuplotlib::ColumnView& col, std::size_t index) {
    const auto* base = static_cast<const unsigned char*>(col.data) + index * col.stride_bytes;

    switch (col.type) {
    case qadgnuplotlib::DataType::Float32: return static_cast<double>(*reinterpret_cast<const float*>(base));
    case qadgnuplotlib::DataType::Float64: return *reinterpret_cast<const double*>(base);
    case qadgnuplotlib::DataType::Int8:    return static_cast<double>(*reinterpret_cast<const int8_t*>(base));
    case qadgnuplotlib::DataType::UInt8:   return static_cast<double>(*reinterpret_cast<const uint8_t*>(base));
    case qadgnuplotlib::DataType::Int16:   return static_cast<double>(*reinterpret_cast<const int16_t*>(base));
    case qadgnuplotlib::DataType::UInt16:  return static_cast<double>(*reinterpret_cast<const uint16_t*>(base));
    case qadgnuplotlib::DataType::Int32:   return static_cast<double>(*reinterpret_cast<const int32_t*>(base));
    case qadgnuplotlib::DataType::UInt32:  return static_cast<double>(*reinterpret_cast<const uint32_t*>(base));
    case qadgnuplotlib::DataType::Int64:   return static_cast<double>(*reinterpret_cast<const int64_t*>(base));
    case qadgnuplotlib::DataType::UInt64:  return static_cast<double>(*reinterpret_cast<const uint64_t*>(base));
    }

    return 0.0;
}

char* duplicate_for_c(const std::string& text) {
    char* out = static_cast<char*>(std::malloc(text.size() + 1));
    if (!out) {
        return nullptr;
    }
    std::memcpy(out, text.data(), text.size());
    out[text.size()] = '\0';
    return out;
}

} // namespace

struct MemDataSource {
    std::vector<qadgnuplotlib::ColumnView> columns;
    std::size_t row_count = 0;
};

extern "C" void* qadgnuplotlib_mem_open(const char* uri, char** errmsg) {
    if (errmsg) {
        *errmsg = nullptr;
    }

    try {
        if (!uri) {
            throw qadgnuplotlib::Error("null mem:// URI");
        }

        qadgnuplotlib::Context* ctx = qadgnuplotlib::current_context();
        if (!ctx) {
            throw qadgnuplotlib::Error("mem:// used without an active qadgnuplotlib::Context");
        }

        const auto names = split_mem_uri_columns(uri);

        auto* source = new MemDataSource();
        source->columns.reserve(names.size());

        for (std::string_view name : names) {
            auto col = ctx->column(name);
            if (col.empty()) {
                delete source;
                throw qadgnuplotlib::Error("empty column in mem:// URI");
            }

            if (source->columns.empty()) {
                source->row_count = col.count;
            } else {
                source->row_count = std::min(source->row_count, col.count);
            }

            source->columns.push_back(col);
        }

        if (source->columns.empty() || source->row_count == 0) {
            delete source;
            throw qadgnuplotlib::Error("mem:// URI resolved to no data");
        }

        return source;
    } catch (const std::exception& e) {
        if (errmsg) {
            *errmsg = duplicate_for_c(std::string("mem:// error: ") + e.what());
        }
        return nullptr;
    } catch (...) {
        if (errmsg) {
            *errmsg = duplicate_for_c("mem:// error: unknown exception");
        }
        return nullptr;
    }
}

extern "C" void qadgnuplotlib_mem_close(void* handle) {
    delete static_cast<MemDataSource*>(handle);
}

extern "C" std::size_t qadgnuplotlib_mem_row_count(const void* handle) {
    const auto* source = static_cast<const MemDataSource*>(handle);
    return source ? source->row_count : 0;
}

extern "C" std::size_t qadgnuplotlib_mem_column_count(const void* handle) {
    const auto* source = static_cast<const MemDataSource*>(handle);
    return source ? source->columns.size() : 0;
}

extern "C" int qadgnuplotlib_mem_read_row(const void* handle,
                                           std::size_t row,
                                           double* out,
                                           std::size_t out_count) {
    const auto* source = static_cast<const MemDataSource*>(handle);
    if (!source || !out || row >= source->row_count) {
        return 0;
    }

    const std::size_t n = std::min(out_count, source->columns.size());
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = column_value_as_double(source->columns[i], row);
    }
    return 1;
}

extern "C" void qadgnuplotlib_mem_free(void* ptr) {
    std::free(ptr);
}

struct qadgnuplotlib_context {
    qadgnuplotlib::Context impl;
};

struct qadgnuplotlib_script {
    qadgnuplotlib::Script impl;
};

namespace {
thread_local std::string g_c_api_last_error;

void clear_c_api_error() noexcept {
    g_c_api_last_error.clear();
}

void set_c_api_error(const char* text) noexcept {
    try {
        g_c_api_last_error = text ? text : "unknown error";
    } catch (...) {
        /* Keep this noexcept. */
    }
}

void set_c_api_error(const std::exception& e) noexcept {
    set_c_api_error(e.what());
}

int require_context(const qadgnuplotlib_context* ctx) {
    if (!ctx) {
        throw qadgnuplotlib::Error("null qadgnuplotlib_context");
    }
    return 0;
}

int require_script(const qadgnuplotlib_script* script) {
    if (!script) {
        throw qadgnuplotlib::Error("null qadgnuplotlib_script");
    }
    return 0;
}

int require_string(const char* text, const char* what) {
    if (!text) {
        throw qadgnuplotlib::Error(what);
    }
    return 0;
}

template<class F>
int c_api_int(F&& f) noexcept {
    try {
        clear_c_api_error();
        f();
        return 0;
    } catch (const std::exception& e) {
        set_c_api_error(e);
        return 1;
    } catch (...) {
        set_c_api_error("unknown C++ exception");
        return 1;
    }
}

template<class T>
int register_column_c(qadgnuplotlib_context* ctx, const char* name, const T* data, std::size_t count) noexcept {
    return c_api_int([&] {
        require_context(ctx);
        require_string(name, "register_data: null name");
        ctx->impl.register_data(name, data, count);
    });
}

template<class T>
int register_image_c(qadgnuplotlib_context* ctx, const char* name, const T* data,
                     std::size_t width, std::size_t height, std::size_t channels) noexcept {
    return c_api_int([&] {
        require_context(ctx);
        require_string(name, "register_image: null name");
        ctx->impl.register_data(name, data, width, height, channels);
    });
}
} // namespace

extern "C" {

const char* qadgnuplotlib_last_error(void) {
    return g_c_api_last_error.empty() ? "" : g_c_api_last_error.c_str();
}

void qadgnuplotlib_clear_last_error(void) {
    clear_c_api_error();
}

qadgnuplotlib_context* qadgnuplotlib_create(void) {
    try {
        clear_c_api_error();
        return new qadgnuplotlib_context();
    } catch (const std::exception& e) {
        set_c_api_error(e);
        return nullptr;
    } catch (...) {
        set_c_api_error("unknown C++ exception");
        return nullptr;
    }
}

void qadgnuplotlib_destroy(qadgnuplotlib_context* ctx) {
    delete ctx;
}

int qadgnuplotlib_run(qadgnuplotlib_context* ctx, const char* script_text) {
    return c_api_int([&] {
        require_context(ctx);
        require_string(script_text, "run: null script text");
        ctx->impl.run(script_text);
    });
}

int qadgnuplotlib_run_script(qadgnuplotlib_context* ctx, const qadgnuplotlib_script* script) {
    return c_api_int([&] {
        require_context(ctx);
        require_script(script);
        ctx->impl.run(script->impl);
    });
}

int qadgnuplotlib_register_data_float32(qadgnuplotlib_context* ctx, const char* name, const float* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_float64(qadgnuplotlib_context* ctx, const char* name, const double* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_int8(qadgnuplotlib_context* ctx, const char* name, const int8_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_uint8(qadgnuplotlib_context* ctx, const char* name, const uint8_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_int16(qadgnuplotlib_context* ctx, const char* name, const int16_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_uint16(qadgnuplotlib_context* ctx, const char* name, const uint16_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_int32(qadgnuplotlib_context* ctx, const char* name, const int32_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_uint32(qadgnuplotlib_context* ctx, const char* name, const uint32_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_int64(qadgnuplotlib_context* ctx, const char* name, const int64_t* data, size_t count) { return register_column_c(ctx, name, data, count); }
int qadgnuplotlib_register_data_uint64(qadgnuplotlib_context* ctx, const char* name, const uint64_t* data, size_t count) { return register_column_c(ctx, name, data, count); }

int qadgnuplotlib_register_image_float32(qadgnuplotlib_context* ctx, const char* name, const float* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_float64(qadgnuplotlib_context* ctx, const char* name, const double* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_int8(qadgnuplotlib_context* ctx, const char* name, const int8_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_uint8(qadgnuplotlib_context* ctx, const char* name, const uint8_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_int16(qadgnuplotlib_context* ctx, const char* name, const int16_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_uint16(qadgnuplotlib_context* ctx, const char* name, const uint16_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_int32(qadgnuplotlib_context* ctx, const char* name, const int32_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_uint32(qadgnuplotlib_context* ctx, const char* name, const uint32_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_int64(qadgnuplotlib_context* ctx, const char* name, const int64_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }
int qadgnuplotlib_register_image_uint64(qadgnuplotlib_context* ctx, const char* name, const uint64_t* data, size_t width, size_t height, size_t channels) { return register_image_c(ctx, name, data, width, height, channels); }

int qadgnuplotlib_unregister_data(qadgnuplotlib_context* ctx, const char* name) {
    return c_api_int([&] {
        require_context(ctx);
        require_string(name, "unregister_data: null name");
        ctx->impl.unregister_data(name);
    });
}

int qadgnuplotlib_clear_data(qadgnuplotlib_context* ctx) {
    return c_api_int([&] {
        require_context(ctx);
        ctx->impl.clear_data();
    });
}

qadgnuplotlib_script* qadgnuplotlib_script_create(const char* script_text) {
    try {
        clear_c_api_error();
        auto* script = new qadgnuplotlib_script();
        script->impl.assign(script_text ? std::string(script_text) : std::string());
        return script;
    } catch (const std::exception& e) {
        set_c_api_error(e);
        return nullptr;
    } catch (...) {
        set_c_api_error("unknown C++ exception");
        return nullptr;
    }
}

qadgnuplotlib_script* qadgnuplotlib_script_load_file(const char* path) {
    try {
        clear_c_api_error();
        require_string(path, "script_load_file: null path");
        auto* script = new qadgnuplotlib_script();
        script->impl = qadgnuplotlib::Script::load(path);
        return script;
    } catch (const std::exception& e) {
        set_c_api_error(e);
        return nullptr;
    } catch (...) {
        set_c_api_error("unknown C++ exception");
        return nullptr;
    }
}

void qadgnuplotlib_script_destroy(qadgnuplotlib_script* script) {
    delete script;
}

int qadgnuplotlib_script_assign(qadgnuplotlib_script* script, const char* script_text) {
    return c_api_int([&] {
        require_script(script);
        script->impl.assign(script_text ? std::string(script_text) : std::string());
    });
}

int qadgnuplotlib_script_save_file(const qadgnuplotlib_script* script, const char* path) {
    return c_api_int([&] {
        require_script(script);
        require_string(path, "script_save_file: null path");
        script->impl.save(path);
    });
}

const char* qadgnuplotlib_script_text(const qadgnuplotlib_script* script) {
    if (!script) {
        set_c_api_error("script_text: null script");
        return nullptr;
    }
    clear_c_api_error();
    return script->impl.c_str();
}

size_t qadgnuplotlib_script_size(const qadgnuplotlib_script* script) {
    if (!script) {
        set_c_api_error("script_size: null script");
        return 0;
    }
    clear_c_api_error();
    return script->impl.size();
}

} /* extern "C" */
