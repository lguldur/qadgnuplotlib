<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>qadgnuplotlib</title>
  <style>
    :root {
      --bg: #f7f8fb;
      --panel: #ffffff;
      --text: #1f2937;
      --muted: #5b6472;
      --border: #d8dee9;
      --accent: #2458a6;
      --accent-soft: #e8f0ff;
      --code-bg: #101828;
      --code-text: #e5e7eb;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      line-height: 1.55;
      color: var(--text);
      background: var(--bg);
    }

    header {
      background: linear-gradient(135deg, #172033, #2458a6);
      color: white;
      padding: 4rem 1.5rem;
    }

    header .wrap,
    main {
      max-width: 1100px;
      margin: 0 auto;
    }

    h1 {
      margin: 0 0 0.5rem;
      font-size: clamp(2.2rem, 6vw, 4rem);
      letter-spacing: -0.04em;
    }

    header p {
      max-width: 780px;
      margin: 0.75rem 0 0;
      font-size: 1.15rem;
      color: #dbeafe;
    }

    nav {
      position: sticky;
      top: 0;
      z-index: 10;
      background: rgba(255,255,255,0.95);
      border-bottom: 1px solid var(--border);
      backdrop-filter: blur(8px);
    }

    nav .wrap {
      max-width: 1100px;
      margin: 0 auto;
      display: flex;
      flex-wrap: wrap;
      gap: 0.5rem 1rem;
      padding: 0.75rem 1.5rem;
    }

    nav a {
      color: var(--accent);
      text-decoration: none;
      font-weight: 600;
    }

    main {
      padding: 2rem 1.5rem 4rem;
    }

    section {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 16px;
      padding: 1.5rem;
      margin: 1.5rem 0;
      box-shadow: 0 8px 24px rgba(15, 23, 42, 0.05);
    }

    h2 {
      margin-top: 0;
      color: #172033;
      font-size: 1.75rem;
    }

    h3 {
      margin-top: 1.6rem;
      color: #243b63;
    }

    h4 {
      margin-bottom: 0.25rem;
      color: #374151;
    }

    a { color: var(--accent); }

    code {
      font-family: ui-monospace, SFMono-Regular, Consolas, "Liberation Mono", monospace;
      background: var(--accent-soft);
      color: #1d4ed8;
      border-radius: 5px;
      padding: 0.1rem 0.3rem;
    }

    pre {
      overflow-x: auto;
      background: var(--code-bg);
      color: var(--code-text);
      border-radius: 12px;
      padding: 1rem;
      line-height: 1.45;
    }

    pre code {
      background: transparent;
      color: inherit;
      padding: 0;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      margin: 1rem 0;
    }

    th, td {
      border: 1px solid var(--border);
      padding: 0.65rem;
      text-align: left;
      vertical-align: top;
    }

    th {
      background: #f1f5f9;
    }

    .badge-row {
      display: flex;
      flex-wrap: wrap;
      gap: 0.5rem;
      margin-top: 1.3rem;
    }

    .badge {
      display: inline-flex;
      align-items: center;
      padding: 0.35rem 0.65rem;
      border-radius: 999px;
      background: rgba(255,255,255,0.14);
      color: white;
      border: 1px solid rgba(255,255,255,0.25);
      font-weight: 600;
      font-size: 0.9rem;
    }

    .note {
      background: #fff7ed;
      border: 1px solid #fed7aa;
      border-radius: 12px;
      padding: 1rem;
      color: #7c2d12;
    }

    .ok {
      background: #ecfdf5;
      border: 1px solid #bbf7d0;
      border-radius: 12px;
      padding: 1rem;
      color: #14532d;
    }

    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
      gap: 1rem;
    }

    .card {
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 1rem;
      background: #fbfdff;
    }

    footer {
      max-width: 1100px;
      margin: 0 auto;
      padding: 0 1.5rem 3rem;
      color: var(--muted);
      font-size: 0.95rem;
    }
  </style>
</head>
<body>
<header>
  <div class="wrap">
    <h1>qadgnuplotlib</h1>
    <p>
      A standalone C++20 static library embedding gnuplot for script-driven plot
      generation, memory-backed datasets, and report-oriented image/PDF output.
    </p>
    <div class="badge-row">
      <span class="badge">Standalone embedded gnuplot</span>
      <span class="badge">C++20 + C API</span>
      <span class="badge">Visual Studio 2022+</span>
      <span class="badge">SVG / PNG / JPG / BMP / PDF</span>
      <span class="badge">mem:// registered data</span>
    </div>
  </div>
</header>

<nav>
  <div class="wrap">
    <a href="#goal">Goal</a>
    <a href="#use">How to use</a>
    <a href="#cpp-api">C++ API</a>
    <a href="#c-api">C API</a>
    <a href="#how">How it works</a>
    <a href="#licences">Licences</a>
  </div>
</nav>

<main>
<section id="goal">
  <h2>1. Goal of the library</h2>
  <p>
    <strong>qadgnuplotlib</strong> embeds gnuplot inside a standalone library.
    It is intended for applications that want to execute gnuplot scripts from
    memory and produce plots without requiring an external gnuplot installation
    on the target machine.
  </p>

  <p>
    The main target is automated report generation, especially workflows that
    would otherwise require a MATLAB-like plotting environment. A program can
    register native C or C++ memory buffers, execute ordinary gnuplot scripts,
    and write plot outputs directly as SVG, raster images, PDF, or raw memory
    buffers.
  </p>

  <div class="grid">
    <div class="card">
      <h3>Standalone</h3>
      <p>
        gnuplot and the required rendering dependencies are built into the
        library. User applications do not need to launch an external
        <code>gnuplot.exe</code>.
      </p>
    </div>
    <div class="card">
      <h3>Script-driven</h3>
      <p>
        Plot descriptions remain gnuplot scripts, so existing gnuplot knowledge
        and syntax still apply.
      </p>
    </div>
    <div class="card">
      <h3>Memory data</h3>
      <p>
        Registered arrays can be plotted through <code>mem://</code> sources
        such as <code>plot "mem://x,y"</code>, avoiding temporary data files.
      </p>
    </div>
    <div class="card">
      <h3>Report output</h3>
      <p>
        The custom <code>qad</code> terminal can produce SVG, PNG, JPG, BMP,
        raster PDF, and raw pixel buffers.
      </p>
    </div>
  </div>

  <div class="note">
    <strong>Build status:</strong> the project is currently tested only on
    Windows with Microsoft Visual Studio, with Visual Studio 2022 / MSVC v143
    or newer recommended. Visual Studio 2026 / v145 has shown compiler issues
    with some third-party C sources in Release mode, so v143 is the current
    practical baseline.
  </div>
</section>

<section id="use">
  <h2>2. How to use it</h2>

  <h3>Minimal C++ example</h3>
  <pre><code>#include "qadgnuplotlib.hpp"

#include &lt;vector&gt;

int main()
{
    std::vector&lt;double&gt; x {0.0, 1.0, 2.0, 3.0};
    std::vector&lt;double&gt; y {0.0, 1.0, 4.0, 9.0};

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
}</code></pre>

  <h3>Minimal C example</h3>
  <pre><code>#include "qadgnuplotlib.h"

#include &lt;stdio.h&gt;

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
}</code></pre>

  <h3>Using registered memory in gnuplot scripts</h3>
  <p>
    Registered columns are referenced using comma-separated <code>mem://</code>
    URIs:
  </p>

  <pre><code>plot "mem://y"     with lines
plot "mem://x,y"   with linespoints
splot "mem://x,y,z" with points</code></pre>

  <p>
    The current implementation supports simple numeric column sources through
    the normal gnuplot datafile pipeline. The memory is borrowed by the library:
    the caller must keep the arrays alive while scripts using them are executed.
  </p>
</section>

<section id="cpp-api">
  <h2>2a. C++ API</h2>

  <h3><code>qadgnuplotlib::Context</code></h3>
  <p>
    <code>Context</code> owns a plotting session and its registered data. A
    single context can run several scripts using the same registered data.
  </p>

  <table>
    <thead>
      <tr>
        <th>Method</th>
        <th>Purpose</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td><code>Context()</code></td>
        <td>Create a plotting context.</td>
      </tr>
      <tr>
        <td><code>~Context()</code></td>
        <td>Destroy the context. Registered data views are discarded.</td>
      </tr>
      <tr>
        <td><code>register_data(name, span)</code></td>
        <td>Register or replace a numeric column from a <code>std::span</code>.</td>
      </tr>
      <tr>
        <td><code>register_data(name, pointer, count)</code></td>
        <td>Register or replace a numeric column from a raw pointer and element count.</td>
      </tr>
      <tr>
        <td><code>register_data(name, vector)</code></td>
        <td>Register or replace a numeric column from a <code>std::vector</code>.</td>
      </tr>
      <tr>
        <td><code>register_data(name, span, width, height, channels)</code></td>
        <td>Register or replace image-like data. This is reserved for image/matrix workflows.</td>
      </tr>
      <tr>
        <td><code>unregister_data(name)</code></td>
        <td>Remove a registered column and/or image by name.</td>
      </tr>
      <tr>
        <td><code>clear_data()</code></td>
        <td>Remove all registered data from the context.</td>
      </tr>
      <tr>
        <td><code>has_column(name)</code>, <code>has_image(name)</code></td>
        <td>Check whether data is registered under a given name.</td>
      </tr>
      <tr>
        <td><code>column(name)</code>, <code>image(name)</code></td>
        <td>Return internal metadata for a registered column or image.</td>
      </tr>
      <tr>
        <td><code>run(script)</code></td>
        <td>Execute a script from <code>const char*</code>, <code>std::string</code>, <code>std::string_view</code>, or <code>Script</code>.</td>
      </tr>
    </tbody>
  </table>

  <h3>Supported column types</h3>
  <p>
    The C++ API uses C++20 templates and concepts to accept only supported,
    trivially-copyable numeric types.
  </p>

  <table>
    <thead>
      <tr><th>C++ type</th><th>Internal gnuplot binary type name</th></tr>
    </thead>
    <tbody>
      <tr><td><code>float</code></td><td><code>%float32</code></td></tr>
      <tr><td><code>double</code></td><td><code>%float64</code></td></tr>
      <tr><td><code>int8_t</code>, <code>uint8_t</code></td><td><code>%int8</code>, <code>%uint8</code></td></tr>
      <tr><td><code>int16_t</code>, <code>uint16_t</code></td><td><code>%int16</code>, <code>%uint16</code></td></tr>
      <tr><td><code>int32_t</code>, <code>uint32_t</code></td><td><code>%int32</code>, <code>%uint32</code></td></tr>
      <tr><td><code>int64_t</code>, <code>uint64_t</code></td><td><code>%int64</code>, <code>%uint64</code></td></tr>
    </tbody>
  </table>

  <h3><code>qadgnuplotlib::Script</code></h3>
  <p>
    <code>Script</code> is a reusable script object. It can be constructed from
    a string, saved to a file, loaded from a file, and executed multiple times.
  </p>

  <table>
    <thead>
      <tr><th>Method</th><th>Purpose</th></tr>
    </thead>
    <tbody>
      <tr><td><code>Script()</code></td><td>Create an empty script.</td></tr>
      <tr><td><code>Script(const char*)</code>, <code>Script(std::string)</code>, <code>Script(std::string_view)</code></td><td>Create a script from text.</td></tr>
      <tr><td><code>Script::load(path)</code></td><td>Load a script from disk.</td></tr>
      <tr><td><code>save(path)</code></td><td>Save the script to disk.</td></tr>
      <tr><td><code>text()</code>, <code>c_str()</code>, <code>size()</code>, <code>empty()</code></td><td>Inspect script content.</td></tr>
      <tr><td><code>assign(text)</code>, <code>clear()</code></td><td>Replace or clear script content.</td></tr>
    </tbody>
  </table>

  <h3>Version API</h3>
  <pre><code>std::string qadgnuplotlib::version(bool verbose = false);</code></pre>
  <p>
    The short form returns the qadgnuplotlib version line. The verbose form also
    lists integrated components such as gnuplot, LunaSVG, PlutoVG, miniz, and
    base64.
  </p>
</section>

<section id="c-api">
  <h2>2b. C API</h2>

  <p>
    The C API is a thin opaque-handle facade over the C++ implementation. It is
    designed for C applications and for later use by other languages through a
    stable C ABI style.
  </p>

  <h3>Context functions</h3>
  <table>
    <thead>
      <tr><th>Function</th><th>Purpose</th></tr>
    </thead>
    <tbody>
      <tr><td><code>qadgnuplotlib_create()</code></td><td>Create a context.</td></tr>
      <tr><td><code>qadgnuplotlib_destroy(ctx)</code></td><td>Destroy a context.</td></tr>
      <tr><td><code>qadgnuplotlib_run(ctx, script_text)</code></td><td>Execute a script string.</td></tr>
      <tr><td><code>qadgnuplotlib_run_script(ctx, script)</code></td><td>Execute a reusable script object.</td></tr>
      <tr><td><code>qadgnuplotlib_unregister_data(ctx, name)</code></td><td>Remove registered data by name.</td></tr>
      <tr><td><code>qadgnuplotlib_clear_data(ctx)</code></td><td>Remove all registered data.</td></tr>
      <tr><td><code>qadgnuplotlib_last_error()</code></td><td>Return the latest thread-local C API diagnostic string.</td></tr>
      <tr><td><code>qadgnuplotlib_clear_last_error()</code></td><td>Clear the thread-local diagnostic string.</td></tr>
    </tbody>
  </table>

  <h3>Column registration functions</h3>
  <p>
    The following functions register borrowed column buffers:
  </p>

  <pre><code>qadgnuplotlib_register_data_float32(ctx, name, data, count)
qadgnuplotlib_register_data_float64(ctx, name, data, count)
qadgnuplotlib_register_data_int8(ctx, name, data, count)
qadgnuplotlib_register_data_uint8(ctx, name, data, count)
qadgnuplotlib_register_data_int16(ctx, name, data, count)
qadgnuplotlib_register_data_uint16(ctx, name, data, count)
qadgnuplotlib_register_data_int32(ctx, name, data, count)
qadgnuplotlib_register_data_uint32(ctx, name, data, count)
qadgnuplotlib_register_data_int64(ctx, name, data, count)
qadgnuplotlib_register_data_uint64(ctx, name, data, count)</code></pre>

  <h3>Image registration functions</h3>
  <p>
    The C API also provides image-style registration functions with
    <code>width</code>, <code>height</code>, and <code>channels</code>
    parameters:
  </p>

  <pre><code>qadgnuplotlib_register_image_float32(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_float64(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_int8(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_uint8(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_int16(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_uint16(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_int32(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_uint32(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_int64(ctx, name, data, width, height, channels)
qadgnuplotlib_register_image_uint64(ctx, name, data, width, height, channels)</code></pre>

  <h3>Script functions</h3>
  <table>
    <thead>
      <tr><th>Function</th><th>Purpose</th></tr>
    </thead>
    <tbody>
      <tr><td><code>qadgnuplotlib_script_create(text)</code></td><td>Create a script object from text.</td></tr>
      <tr><td><code>qadgnuplotlib_script_load_file(path)</code></td><td>Load a script object from disk.</td></tr>
      <tr><td><code>qadgnuplotlib_script_destroy(script)</code></td><td>Destroy a script object.</td></tr>
      <tr><td><code>qadgnuplotlib_script_assign(script, text)</code></td><td>Replace script text.</td></tr>
      <tr><td><code>qadgnuplotlib_script_save_file(script, path)</code></td><td>Save script text to disk.</td></tr>
      <tr><td><code>qadgnuplotlib_script_text(script)</code></td><td>Return a const pointer to script text.</td></tr>
      <tr><td><code>qadgnuplotlib_script_size(script)</code></td><td>Return the script size in bytes.</td></tr>
    </tbody>
  </table>

  <h3>Low-level functions</h3>
  <table>
    <thead>
      <tr><th>Function</th><th>Purpose</th></tr>
    </thead>
    <tbody>
      <tr><td><code>qadgnuplotlib_execute_script(script)</code></td><td>Execute directly through the embedded interpreter. This bypasses <code>Context</code>, so it cannot use registered <code>mem://</code> data.</td></tr>
      <tr><td><code>qadgnuplotlib_set_log_file(path)</code></td><td>Redirect gnuplot diagnostics to a log file. Passing <code>NULL</code> or an empty string restores normal stderr behavior.</td></tr>
      <tr><td><code>qadgnuplotlib_version(verbose)</code></td><td>Return the short or verbose version string.</td></tr>
    </tbody>
  </table>
</section>

<section id="how">
  <h2>3. How it works</h2>

  <p>
    qadgnuplotlib is built by embedding selected gnuplot sources and compiling
    them into a static library together with a custom terminal and glue code.
    The intent is to keep upstream sources as untouched as possible and put
    local adaptations in the <code>src</code> directory.
  </p>

  <h3>Rendering pipeline</h3>
  <pre><code>gnuplot script
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
SVG / PNG / JPG / BMP / raster PDF / raw buffers</code></pre>

  <h3>Memory data pipeline</h3>
  <pre><code>application memory
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
any terminal, including qad</code></pre>

  <p>
    The <code>mem://</code> mechanism is implemented in the gnuplot datafile
    layer, not in the terminal layer. This is important because it makes the
    registered data behave like a normal gnuplot data source rather than a
    terminal-specific trick.
  </p>

  <h3>Directory overview</h3>
  <table>
    <thead>
      <tr><th>Path</th><th>Role</th></tr>
    </thead>
    <tbody>
      <tr><td><code>src/</code></td><td>qadgnuplotlib code, local glue files, custom terminal, public headers, tests.</td></tr>
      <tr><td><code>gnuplot-main/</code></td><td>Embedded gnuplot source tree.</td></tr>
      <tr><td><code>lunasvg/</code></td><td>SVG parsing/rendering library.</td></tr>
      <tr><td><code>lunasvg/plutovg/</code></td><td>2D vector/raster support used by LunaSVG.</td></tr>
      <tr><td><code>miniz/</code></td><td>Deflate support used by the raster PDF writer.</td></tr>
      <tr><td><code>base64/</code></td><td>Base64 support used for embedded image paths.</td></tr>
      <tr><td><code>generated/</code></td><td>Generated configuration/version headers.</td></tr>
      <tr><td><code>licences/</code></td><td>Licence texts and third-party licence notices.</td></tr>
    </tbody>
  </table>

  <h3>Glue files</h3>
  <p>
    Some files are local copies/adaptations of gnuplot source files. This keeps
    <code>gnuplot-main</code> easier to update and makes local changes explicit.
  </p>

  <table>
    <thead>
      <tr><th>File</th><th>Purpose</th></tr>
    </thead>
    <tbody>
      <tr><td><code>src/winmain_glue.c</code></td><td>Windows/main bootstrap adaptation for embedded library execution.</td></tr>
      <tr><td><code>src/plot_glue.c</code></td><td>Plot/session glue adapted from gnuplot.</td></tr>
      <tr><td><code>src/datafile_glue.c</code></td><td>Datafile-layer adaptation implementing <code>mem://</code> sources.</td></tr>
      <tr><td><code>src/show_glue.c</code></td><td>Version-display adaptation that appends qadgnuplotlib version information after genuine gnuplot output.</td></tr>
      <tr><td><code>src/term.c</code>, <code>src/term.h</code></td><td>Local terminal table integration, including the <code>qad</code> terminal.</td></tr>
      <tr><td><code>src/qad.trm</code></td><td>Custom terminal that captures SVG and produces final outputs.</td></tr>
    </tbody>
  </table>

  <h3>Version reporting</h3>
  <p>
    <code>show version</code> and <code>show version long</code> keep the
    genuine gnuplot output and then append qadgnuplotlib version information.
    The public API also exposes <code>qadgnuplotlib::version(bool)</code> and
    <code>qadgnuplotlib_version(int)</code>.
  </p>
</section>

<section id="licences">
  <h2>4. Licences</h2>

  <p>
    qadgnuplotlib is a mixed-source project. The original qadgnuplotlib wrapper
    and library code is distributed under the MIT Licence:
  </p>

  <pre><code>licences/QADGNUPLOTLIB-MIT.txt</code></pre>

  <p>
    The embedded gnuplot source tree and files derived from gnuplot are not
    MIT-licensed. They remain under the gnuplot licence:
  </p>

  <pre><code>licences/GNUPLOT-Copyright.txt</code></pre>

  <p>
    Third-party components remain under their own licences. See the
    <code>licences/</code> directory for the licence texts and summaries.
  </p>

  <table>
    <thead>
      <tr><th>Component</th><th>Licence location / note</th></tr>
    </thead>
    <tbody>
      <tr><td>qadgnuplotlib original code</td><td><code>licences/QADGNUPLOTLIB-MIT.txt</code></td></tr>
      <tr><td>gnuplot and gnuplot-derived glue</td><td><code>licences/GNUPLOT-Copyright.txt</code></td></tr>
      <tr><td>LunaSVG</td><td>See <code>licences/</code> and original source notices.</td></tr>
      <tr><td>PlutoVG and bundled stb-derived files</td><td>See <code>licences/</code> and original source notices.</td></tr>
      <tr><td>miniz</td><td>See <code>licences/</code> and original source notices.</td></tr>
      <tr><td>base64</td><td>Embedded source, see <code>licences/</code> and original source notices.</td></tr>
    </tbody>
  </table>

  <div class="note">
    If you redistribute binaries built from modified gnuplot sources, check the
    gnuplot licence obligations, including identifying modified versions and
    providing corresponding source modifications where applicable.
  </div>
</section>
</main>

<footer>
  <p>
    qadgnuplotlib — Copyright © 2026 David Duchet.
    This page is intentionally plain HTML so it can be used directly as a
    GitHub Pages <code>index.html</code>.
  </p>
</footer>
</body>
</html>
