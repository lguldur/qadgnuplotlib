# qadgnuplotlib

A standalone C++20 static library embedding gnuplot for script-driven plot generation, memory-backed datasets, and report-oriented image/PDF output.

Standalone embedded gnuplot C++20 + C API Visual Studio 2022+ SVG / PNG / JPG / BMP / PDF mem:// registered data

[Goal](#1-goal-of-the-library) [How to use](#2-how-to-use-it) [C++ API](#2a-C-API) [C API](#2b-C-API) [How it works](#3-How-it-works) [Licences](#4-Licences)

1\. Goal of the library
-----------------------

**qadgnuplotlib** embeds gnuplot inside a standalone library. It is intended for applications that want to execute gnuplot scripts from memory and produce plots without requiring an external gnuplot installation on the target machine.

The main target is automated report generation, especially workflows that would otherwise require a MATLAB-like plotting environment. A program can register native C or C++ memory buffers, execute ordinary gnuplot scripts, and write plot outputs directly as SVG, raster images, PDF, or raw memory buffers.

### Standalone

gnuplot and the required rendering dependencies are built into the library. User applications do not need to launch an external `gnuplot.exe`.

### Script-driven

Plot descriptions remain gnuplot scripts, so existing gnuplot knowledge and syntax still apply.

### Memory data

Registered arrays can be plotted through `mem://` sources such as `plot "mem://x,y"`, avoiding temporary data files.

### Report output

The custom `qad` terminal can produce SVG, PNG, JPG, BMP, raster PDF, and raw pixel buffers.

**Build status:** the project is currently tested only on Windows with Microsoft Visual Studio, with Visual Studio 2022 / MSVC v143 or newer recommended. Visual Studio 2026 / v145 has shown compiler issues with some third-party C sources in Release mode, so v143 is the current practical baseline.

2\. How to use it
-----------------

### Minimal C++ example

    #include "qadgnuplotlib.hpp"
    
    #include <vector>
    
    int main()
    {
        std::vector<double> x {0.0, 1.0, 2.0, 3.0};
        std::vector<double> y {0.0, 1.0, 4.0, 9.0};
    
        qadgnuplotlib::Context gp;
    
        gp.register_data("x", x);
        gp.register_data("y", y);
    
        gp.run(R"GPLOT(
    set terminal qad size 800,600 format png
    set output "example.png"
    set title "qadgnuplotlib C++ example"
    set grid
    plot "mem://x,y" with linespoints title "registered x,y"
    set output
    )GPLOT");
    }

### Minimal C example

    #include "qadgnuplotlib.h"
    
    #include <stdio.h>
    
    int main(void)
    {
        double x[] = {0, 1, 2, 3};
        double y[] = {0, 1, 4, 9};
    
        qadgnuplotlib_context* gp = qadgnuplotlib_create();
    
        qadgnuplotlib_register_data_float64(gp, "x", x, 4);
        qadgnuplotlib_register_data_float64(gp, "y", y, 4);
    
        const char* script =
            "set terminal qad size 800,600 format png\n"
            "set output 'example_c.png'\n"
            "set title 'qadgnuplotlib C example'\n"
            "set grid\n"
            "plot 'mem://x,y' with linespoints title 'registered x,y'\n"
            "set output\n";
    
        if (qadgnuplotlib_run(gp, script) != 0) {
            printf("qadgnuplotlib error: %s\n", qadgnuplotlib_last_error());
        }
    
        qadgnuplotlib_destroy(gp);
        return 0;
    }

### Using registered memory in gnuplot scripts

Registered columns are referenced using comma-separated `mem://` URIs:

    plot "mem://y"     with lines
    plot "mem://x,y"   with linespoints
    splot "mem://x,y,z" with points

The current implementation supports simple numeric column sources through the normal gnuplot datafile pipeline. The memory is borrowed by the library: the caller must keep the arrays alive while scripts using them are executed.

2a. C++ API
-----------

### `qadgnuplotlib::Context`

`Context` owns a plotting session and its registered data. A single context can run several scripts using the same registered data.

### Methods

`Context()` Create a plotting context.

`~Context()` Destroy the context. Registered data views are discarded.

`register_data(name, span)` Register or replace a numeric column from a `std::span`.

`register_data(name, pointer, count)` Register or replace a numeric column from a raw pointer and element count.

`register_data(name, vector)` Register or replace a numeric column from a `std::vector`.

`register_data(name, span, width, height, channels)` Register or replace image-like data. This is reserved for image/matrix workflows.

`unregister_data(name)` Remove a registered column and/or image by name.

`clear_data()` Remove all registered data from the context.

`has_column(name)`, `has_image(name)` Check whether data is registered under a given name.

`column(name)`, `image(name)` Return internal metadata for a registered column or image.

`run(script)` Execute a script from `const char*`, `std::string`, `std::string_view`, or `Script`.

### Supported column types

The C++ API uses C++20 templates and concepts to accept only supported, trivially-copyable numeric types.

C++ type => Internal gnuplot binary type name:

`float` => `%float32`

`double` => `%float64`

`int8_t`, `uint8_t` => `%int8`, `%uint8`

`int16_t`, `uint16_t` => `%int16`, `%uint16`

`int32_t`, `uint32_t` => `%int32`, `%uint32`

`int64_t`, `uint64_t` => `%int64`, `%uint64`

### `qadgnuplotlib::Script`

`Script` is a reusable script object. It can be constructed from a string, saved to a file, loaded from a file, and executed multiple times.

### Methods

`Script()` Create an empty script.

`Script(const char*)`, `Script(std::string)`, `Script(std::string_view)` Create a script from text.

`Script::load(path)` Load a script from disk.

`save(path)` Save the script to disk.

`text()`, `c_str()`, `size()`, `empty()` Inspect script content.

`assign(text)`, `clear()` Replace or clear script content.

### Version API

    std::string qadgnuplotlib::version(bool verbose = false); The short form returns the qadgnuplotlib version line. The verbose form also lists integrated components such as gnuplot, LunaSVG, PlutoVG, miniz, and base64.

2b. C API
---------

The C API is a thin opaque-handle facade over the C++ implementation. It is designed for C applications and for later use by other languages through a stable C ABI style.

### Context functions

`qadgnuplotlib_create()` Create a context.

`qadgnuplotlib_destroy(ctx)` Destroy a context.

`qadgnuplotlib_run(ctx, script_text)` Execute a script string.

`qadgnuplotlib_run_script(ctx, script)` Execute a reusable script object.

`qadgnuplotlib_unregister_data(ctx, name)` Remove registered data by name.

`qadgnuplotlib_clear_data(ctx)` Remove all registered data.

`qadgnuplotlib_last_error()` Return the latest thread-local C API diagnostic string.

`qadgnuplotlib_clear_last_error()` Clear the thread-local diagnostic string.

### Column registration functions

The following functions register borrowed column buffers:

    qadgnuplotlib_register_data_float32(ctx, name, data, count)
    qadgnuplotlib_register_data_float64(ctx, name, data, count)
    qadgnuplotlib_register_data_int8(ctx, name, data, count)
    qadgnuplotlib_register_data_uint8(ctx, name, data, count)
    qadgnuplotlib_register_data_int16(ctx, name, data, count)
    qadgnuplotlib_register_data_uint16(ctx, name, data, count)
    qadgnuplotlib_register_data_int32(ctx, name, data, count)
    qadgnuplotlib_register_data_uint32(ctx, name, data, count)
    qadgnuplotlib_register_data_int64(ctx, name, data, count)
    qadgnuplotlib_register_data_uint64(ctx, name, data, count)

### Image registration functions

The C API also provides image-style registration functions with `width`, `height`, and `channels` parameters:

    qadgnuplotlib_register_image_float32(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_float64(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_int8(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_uint8(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_int16(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_uint16(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_int32(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_uint32(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_int64(ctx, name, data, width, height, channels)
    qadgnuplotlib_register_image_uint64(ctx, name, data, width, height, channels)

### Script functions

`qadgnuplotlib_script_create(text)` Create a script object from text.

`qadgnuplotlib_script_load_file(path)` Load a script object from disk.

`qadgnuplotlib_script_destroy(script)` Destroy a script object.

`qadgnuplotlib_script_assign(script, text)` Replace script text.

`qadgnuplotlib_script_save_file(script, path)` Save script text to disk.

`qadgnuplotlib_script_text(script)` Return a const pointer to script text.

`qadgnuplotlib_script_size(script)` Return the script size in bytes.

### Low-level functions

`qadgnuplotlib_execute_script(script)` Execute directly through the embedded interpreter. This bypasses `Context`, so it cannot use registered `mem://` data.

`qadgnuplotlib_set_log_file(path)` Redirect gnuplot diagnostics to a log file. Passing `NULL` or an empty string restores normal stderr behavior.

`qadgnuplotlib_version(verbose)` Return the short or verbose version string.

3\. How it works
----------------

qadgnuplotlib is built by embedding selected gnuplot sources and compiling them into a static library together with a custom terminal and glue code. The intent is to keep upstream sources as untouched as possible and put local adaptations in the `src` directory.

### Rendering pipeline

    gnuplot script
        ↓
    embedded gnuplot core
        ↓
    qad terminal
        ↓
    internally reused svg.trm output
        ↓
    SVG captured in memory
        ↓
    LunaSVG + PlutoVG rasterization
        ↓
    SVG / PNG / JPG / BMP / raster PDF / raw buffers

### Memory data pipeline

    application memory
        ↓
    Context::register_data("x", ...)
    Context::register_data("y", ...)
        ↓
    gnuplot script: plot "mem://x,y"
        ↓
    datafile_glue.c
        ↓
    registered-memory row provider
        ↓
    normal gnuplot plotting pipeline
        ↓
    any terminal, including qad

The `mem://` mechanism is implemented in the gnuplot datafile layer, not in the terminal layer. This is important because it makes the registered data behave like a normal gnuplot data source rather than a terminal-specific trick.

### Directory overview

`src/` qadgnuplotlib code, local glue files, custom terminal, public headers, tests.

`gnuplot-main/` Embedded gnuplot source tree.

`lunasvg/` SVG parsing/rendering library.

`lunasvg/plutovg/` 2D vector/raster support used by LunaSVG.

`miniz/` Deflate support used by the raster PDF writer.

`base64/` Base64 support used for embedded image paths.

`generated/` Generated configuration/version headers.

`licences/` Licence texts and third-party licence notices.

### Glue files

Some files are local copies/adaptations of gnuplot source files. This keeps `gnuplot-main` easier to update and makes local changes explicit.

`src/winmain_glue.c` Windows/main bootstrap adaptation for embedded library execution.

`src/plot_glue.c` Plot/session glue adapted from gnuplot.

`src/datafile_glue.c` Datafile-layer adaptation implementing `mem://` sources.

`src/show_glue.c` Version-display adaptation that appends qadgnuplotlib version information after genuine gnuplot output.

`src/term.c`, `src/term.h` Local terminal table integration, including the `qad` terminal.

`src/qad.trm` Custom terminal that captures SVG and produces final outputs.

### Version reporting

`show version` and `show version long` keep the genuine gnuplot output and then append qadgnuplotlib version information. The public API also exposes `qadgnuplotlib::version(bool)` and `qadgnuplotlib_version(int)`.

4\. Licences
------------

qadgnuplotlib is a mixed-source project. The original qadgnuplotlib wrapper and library code is distributed under the MIT Licence:

    licences/QADGNUPLOTLIB-MIT.txt

The embedded gnuplot source tree and files derived from gnuplot are not MIT-licensed. They remain under the gnuplot licence:

    licences/GNUPLOT-Copyright.txt

Third-party components remain under their own licences. See the `licences/` directory for the licence texts and summaries.

qadgnuplotlib original code: `licences/QADGNUPLOTLIB-MIT.txt`

gnuplot and gnuplot-derived glue: `licences/GNUPLOT-Copyright.txt`

LunaSVG: See `licences/` and original source notices.

PlutoVG and bundled stb-derived files: See `licences/` and original source notices.

miniz: See `licences/` and original source notices.

base64: Embedded source, see `licences/` and original source notices.

If you redistribute binaries built from modified gnuplot sources, check the gnuplot licence obligations, including identifying modified versions and providing corresponding source modifications where applicable.

qadgnuplotlib — Copyright © 2026 David Duchet.
