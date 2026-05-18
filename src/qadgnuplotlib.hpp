/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        Declares the public C++20 API for qadgnuplotlib.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "qadgnuplotlib.h"

namespace qadgnuplotlib {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

enum class DataType {
    Float32,
    Float64,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64
};

constexpr const char* gnuplot_binary_format(DataType type) noexcept {
    switch (type) {
    case DataType::Float32: return "%float32";
    case DataType::Float64: return "%float64";
    case DataType::Int8:    return "%int8";
    case DataType::UInt8:   return "%uint8";
    case DataType::Int16:   return "%int16";
    case DataType::UInt16:  return "%uint16";
    case DataType::Int32:   return "%int32";
    case DataType::UInt32:  return "%uint32";
    case DataType::Int64:   return "%int64";
    case DataType::UInt64:  return "%uint64";
    }
    return "%float64";
}

template<class T>
struct GnuplotType;

template<> struct GnuplotType<float>    { static constexpr DataType type = DataType::Float32; };
template<> struct GnuplotType<double>   { static constexpr DataType type = DataType::Float64; };
template<> struct GnuplotType<int8_t>   { static constexpr DataType type = DataType::Int8;    };
template<> struct GnuplotType<uint8_t>  { static constexpr DataType type = DataType::UInt8;   };
template<> struct GnuplotType<int16_t>  { static constexpr DataType type = DataType::Int16;   };
template<> struct GnuplotType<uint16_t> { static constexpr DataType type = DataType::UInt16;  };
template<> struct GnuplotType<int32_t>  { static constexpr DataType type = DataType::Int32;   };
template<> struct GnuplotType<uint32_t> { static constexpr DataType type = DataType::UInt32;  };
template<> struct GnuplotType<int64_t>  { static constexpr DataType type = DataType::Int64;   };
template<> struct GnuplotType<uint64_t> { static constexpr DataType type = DataType::UInt64;  };

template<class T>
concept SupportedColumnType = requires {
    GnuplotType<std::remove_cv_t<T>>::type;
} && std::is_trivially_copyable_v<std::remove_cv_t<T>>;

struct ColumnView {
    const void* data = nullptr;
    std::size_t count = 0;
    std::size_t stride_bytes = 0;
    DataType type = DataType::Float64;

    [[nodiscard]] constexpr bool empty() const noexcept { return data == nullptr || count == 0; }
    [[nodiscard]] constexpr const char* format() const noexcept { return gnuplot_binary_format(type); }
};

struct ImageView {
    const void* data = nullptr;
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t channels = 1;
    std::size_t stride_bytes = 0;
    DataType type = DataType::Float64;

    [[nodiscard]] constexpr bool empty() const noexcept { return data == nullptr || width == 0 || height == 0; }
    [[nodiscard]] constexpr const char* format() const noexcept { return gnuplot_binary_format(type); }
};

class Script {
public:
    Script() = default;
    Script(const char* text);
    Script(std::string text);
    Script(std::string_view text);

    [[nodiscard]] static Script load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path) const;

    [[nodiscard]] const std::string& text() const noexcept;
    [[nodiscard]] const char* c_str() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    void assign(std::string text);
    void clear() noexcept;

private:
    std::string m_text;
};

class Context {
public:
    Context();
    ~Context();

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) noexcept = delete;
    Context& operator=(Context&&) noexcept = delete;

    template<SupportedColumnType T>
    void register_data(std::string name, std::span<const T> values) {
        if (name.empty()) {
            throw Error("register_data: empty name");
        }
        if (values.empty()) {
            throw Error("register_data: empty data span");
        }

        ColumnView view;
        view.data = values.data();
        view.count = values.size();
        view.stride_bytes = sizeof(T);
        view.type = GnuplotType<std::remove_cv_t<T>>::type;

        std::scoped_lock lock(m_mutex);
        m_columns.insert_or_assign(std::move(name), view);
    }

    template<SupportedColumnType T>
    void register_data(std::string name, const T* data, std::size_t count) {
        if (!data && count != 0) {
            throw Error("register_data: null data pointer");
        }
        register_data(std::move(name), std::span<const T>(data, count));
    }

    template<SupportedColumnType T, class Allocator>
    void register_data(std::string name, const std::vector<T, Allocator>& values) {
        register_data(std::move(name), std::span<const T>(values.data(), values.size()));
    }

    template<SupportedColumnType T>
    void register_data(std::string name,
                        std::span<const T> values,
                        std::size_t width,
                        std::size_t height,
                        std::size_t channels = 1) {
        if (name.empty()) {
            throw Error("register_data: empty name");
        }
        if (width == 0 || height == 0 || channels == 0) {
            throw Error("register_data: invalid image dimensions");
        }
        if (values.size() < width * height * channels) {
            throw Error("register_data: data span is smaller than width * height * channels");
        }

        ImageView view;
        view.data = values.data();
        view.width = width;
        view.height = height;
        view.channels = channels;
        view.stride_bytes = sizeof(T);
        view.type = GnuplotType<std::remove_cv_t<T>>::type;

        std::scoped_lock lock(m_mutex);
        m_images.insert_or_assign(std::move(name), view);
    }

    template<SupportedColumnType T>
    void register_data(std::string name,
                        const T* data,
                        std::size_t width,
                        std::size_t height,
                        std::size_t channels = 1) {
        if (!data) {
            throw Error("register_data: null data pointer");
        }
        register_data(std::move(name),
                      std::span<const T>(data, width * height * channels),
                       width,
                       height,
                       channels);
    }

    void unregister_data(std::string_view name);
    void clear_data();

    [[nodiscard]] bool has_column(std::string_view name) const;
    [[nodiscard]] bool has_image(std::string_view name) const;
    [[nodiscard]] ColumnView column(std::string_view name) const;
    [[nodiscard]] ImageView image(std::string_view name) const;

    void run(const Script& script);
    void run(const std::string& script);
    void run(std::string_view script);
    void run(const char* script);

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ColumnView> m_columns;
    std::unordered_map<std::string, ImageView> m_images;
};

[[nodiscard]] std::string version(bool verbose = false);

[[nodiscard]] Context* current_context() noexcept;

} // namespace qadgnuplotlib
