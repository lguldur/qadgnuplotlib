/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        Provides internal SVG rasterization, image encoding, raw buffer, and raster PDF helper routines.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "lunasvg.h"
#include "miniz.h"

typedef struct {
    unsigned char* data;
    size_t size;
    size_t cap;
} ByteBuffer;

static int ByteBuffer_reserve(ByteBuffer* b, size_t extra)
{
    size_t need = b->size + extra;

    if (need <= b->cap)
        return 0;

    size_t new_cap = b->cap ? b->cap * 2 : 4096;
    while (new_cap < need)
        new_cap *= 2;

    unsigned char* p = (unsigned char*)realloc(b->data, new_cap);
    if (!p)
        return 1;

    b->data = p;
    b->cap = new_cap;
    return 0;
}

static int ByteBuffer_write(ByteBuffer* b, const void* data, size_t len)
{
    if (ByteBuffer_reserve(b, len))
        return 1;

    memcpy(b->data + b->size, data, len);
    b->size += len;
    return 0;
}

static int ByteBuffer_printf(ByteBuffer* b, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (n < 0) {
        va_end(ap);
        return 1;
    }

    if (ByteBuffer_reserve(b, (size_t)n + 1)) {
        va_end(ap);
        return 1;
    }

    vsnprintf((char*)b->data + b->size, (size_t)n + 1, fmt, ap);
    b->size += (size_t)n;

    va_end(ap);
    return 0;
}

/*
 * Creates a one-page raster PDF containing one Flate-compressed RGB image.
 *
 * Inputs:
 *   image_width_px       image width in pixels
 *   image_height_px      image height in pixels
 *   page_width_pt        PDF page width in points, 1 pt = 1/72 inch
 *   page_height_pt       PDF page height in points
 *   flate_rgb            miniz-compressed RGB byte stream
 *   flate_rgb_size       compressed stream size
 *
 * Outputs:
 *   *out_pdf             malloc/realloc-owned PDF buffer
 *   *out_pdf_size        PDF size in bytes
 *
 * Caller must free(*out_pdf).
 */
extern "C" int qadgnuplotlib_make_raster_pdf_from_flate_rgb(
    unsigned image_width_px,
    unsigned image_height_px,
    double page_width_pt,
    double page_height_pt,
    const unsigned char* flate_rgb,
    size_t flate_rgb_size,
    unsigned char** out_pdf,
    size_t* out_pdf_size)
{
    if (!flate_rgb || !out_pdf || !out_pdf_size)
        return 1;

    ByteBuffer b = { 0 };

    size_t obj1, obj2, obj3, obj4, obj5, xref;

    char content[256];
    int content_len = 0;

    if (ByteBuffer_printf(&b, "%%PDF-1.4\n%%\xE2\xE3\xCF\xD3\n"))
        goto fail;

    obj1 = b.size;
    if (ByteBuffer_printf(&b,
        "1 0 obj\n"
        "<< /Type /Catalog /Pages 2 0 R >>\n"
        "endobj\n"))
        goto fail;

    obj2 = b.size;
    if (ByteBuffer_printf(&b,
        "2 0 obj\n"
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>\n"
        "endobj\n"))
        goto fail;

    obj3 = b.size;
    if (ByteBuffer_printf(&b,
        "3 0 obj\n"
        "<< /Type /Page /Parent 2 0 R\n"
        "   /MediaBox [0 0 %.6f %.6f]\n"
        "   /Resources << /XObject << /Im0 4 0 R >> >>\n"
        "   /Contents 5 0 R\n"
        ">>\n"
        "endobj\n",
        page_width_pt, page_height_pt))
        goto fail;

    obj4 = b.size;
    if (ByteBuffer_printf(&b,
        "4 0 obj\n"
        "<< /Type /XObject\n"
        "   /Subtype /Image\n"
        "   /Width %u\n"
        "   /Height %u\n"
        "   /ColorSpace /DeviceRGB\n"
        "   /BitsPerComponent 8\n"
        "   /Filter /FlateDecode\n"
        "   /Length %zu\n"
        ">>\n"
        "stream\n",
        image_width_px, image_height_px, flate_rgb_size))
        goto fail;

    if (ByteBuffer_write(&b, flate_rgb, flate_rgb_size))
        goto fail;

    if (ByteBuffer_printf(&b, "\nendstream\nendobj\n"))
        goto fail;

    content_len = snprintf(
        content, sizeof(content),
        "q\n"
        "%.6f 0 0 %.6f 0 0 cm\n"
        "/Im0 Do\n"
        "Q\n",
        page_width_pt, page_height_pt
    );

    if (content_len < 0 || (size_t)content_len >= sizeof(content))
        goto fail;

    obj5 = b.size;
    if (ByteBuffer_printf(&b,
        "5 0 obj\n"
        "<< /Length %d >>\n"
        "stream\n",
        content_len))
        goto fail;

    if (ByteBuffer_write(&b, content, (size_t)content_len))
        goto fail;

    if (ByteBuffer_printf(&b, "endstream\nendobj\n"))
        goto fail;

    xref = b.size;
    if (ByteBuffer_printf(&b,
        "xref\n"
        "0 6\n"
        "0000000000 65535 f \n"
        "%010zu 00000 n \n"
        "%010zu 00000 n \n"
        "%010zu 00000 n \n"
        "%010zu 00000 n \n"
        "%010zu 00000 n \n"
        "trailer\n"
        "<< /Size 6 /Root 1 0 R >>\n"
        "startxref\n"
        "%zu\n"
        "%%%%EOF\n",
        obj1, obj2, obj3, obj4, obj5, xref))
        goto fail;

    *out_pdf = b.data;
    *out_pdf_size = b.size;
    return 0;

fail:
    free(b.data);
    *out_pdf = NULL;
    *out_pdf_size = 0;
    return 1;
}


/*
 * Creates a one-page raster PDF from raw RGB bytes.
 *
 * raw_rgb must be:
 *   width * height * 3 bytes
 *
 * Caller must free(*out_pdf).
 */
extern "C" int qadgnuplotlib_make_raster_pdf_from_raw_rgb(
    unsigned image_width_px,
    unsigned image_height_px,
    double page_width_pt,
    double page_height_pt,
    const unsigned char* raw_rgb,
    size_t raw_rgb_size,
    unsigned char** out_pdf,
    size_t* out_pdf_size)
{
    if (!raw_rgb || !out_pdf || !out_pdf_size)
        return 1;

    size_t expected_size = (size_t)image_width_px * image_height_px * 3;
    if (raw_rgb_size != expected_size)
        return 1;

    mz_ulong compressed_bound = compressBound((mz_ulong)raw_rgb_size);

    unsigned char* compressed =
        (unsigned char*)malloc((size_t)compressed_bound);

    if (!compressed)
        return 1;

    mz_ulong compressed_size = compressed_bound;

    int zret = compress2(
        compressed,
        &compressed_size,
        raw_rgb,
        (mz_ulong)raw_rgb_size,
        MZ_BEST_COMPRESSION
    );

    if (zret != Z_OK) {
        free(compressed);
        return 1;
    }

    int err = qadgnuplotlib_make_raster_pdf_from_flate_rgb(
        image_width_px,
        image_height_px,
        page_width_pt,
        page_height_pt,
        compressed,
        (size_t)compressed_size,
        out_pdf,
        out_pdf_size
    );

    free(compressed);
    return err;
}


extern "C" {

    /*
     * Render SVG text to raw RGBA bytes using LunaSVG.
     *
     * Output:
     *   out_rgba = malloc-owned buffer, 4 bytes per pixel: R,G,B,A
     *
     * Caller must free(*out_rgba).
     */
    int qadgnuplotlib_svg_to_raw_rgba_lunasvg(
        const char* svg_text,
        unsigned width_px,
        unsigned height_px,
        unsigned char** out_rgba,
        size_t* out_rgba_size)
    {
        if (!svg_text || !out_rgba || !out_rgba_size)
            return 1;

        *out_rgba = NULL;
        *out_rgba_size = 0;

        if (width_px == 0 || height_px == 0)
            return 1;

        std::unique_ptr<lunasvg::Document> document =
            lunasvg::Document::loadFromData(svg_text);

        if (!document)
            return 1;

        lunasvg::Bitmap bitmap =
            document->renderToBitmap((int)width_px, (int)height_px);

        if (bitmap.isNull())
            return 1;

        /*
         * LunaSVG/PlutoVG renders to ARGB32 premultiplied internally.
         * Convert in place to plain RGBA before handing pixels to image encoders/PDF.
         * Also respect bitmap.stride(): PlutoVG rows may be padded, so a
         * single width*height*4 memcpy can shift rows or copy padding bytes.
         */
        bitmap.convertToRGBA();

        const int bw = bitmap.width();
        const int bh = bitmap.height();
        const int stride = bitmap.stride();

        if (bw <= 0 || bh <= 0 || stride < bw * 4)
            return 1;

        size_t size = (size_t)bw * (size_t)bh * 4u;

        unsigned char* rgba = (unsigned char*)std::malloc(size);
        if (!rgba)
            return 1;

        const unsigned char* src = (const unsigned char*)bitmap.data();
        for (int y = 0; y < bh; ++y) {
            std::memcpy(
                rgba + (size_t)y * (size_t)bw * 4u,
                src + (size_t)y * (size_t)stride,
                (size_t)bw * 4u
            );
        }

        *out_rgba = rgba;
        *out_rgba_size = size;

        return 0;
    }

}


extern "C" {

    /*
     * Render SVG text to raw RGB bytes using LunaSVG.
     *
     * Output:
     *   out_rgb = malloc-owned buffer, 3 bytes per pixel: R,G,B
     *
     * Caller must free(*out_rgb).
     */
    int qadgnuplotlib_svg_to_raw_rgb_lunasvg(
        const char* svg_text,
        unsigned width_px,
        unsigned height_px,
        unsigned char background_r,
        unsigned char background_g,
        unsigned char background_b,
        unsigned char** out_rgb,
        size_t* out_rgb_size)
    {
        if (!svg_text || !out_rgb || !out_rgb_size)
            return 1;

        *out_rgb = NULL;
        *out_rgb_size = 0;

        unsigned char* rgba = NULL;
        size_t rgba_size = 0;

        if (qadgnuplotlib_svg_to_raw_rgba_lunasvg(
            svg_text,
            width_px,
            height_px,
            &rgba,
            &rgba_size))
            return 1;

        size_t pixel_count = (size_t)width_px * height_px;
        size_t rgb_size = pixel_count * 3;

        unsigned char* rgb = (unsigned char*)std::malloc(rgb_size);
        if (!rgb) {
            std::free(rgba);
            return 1;
        }

        for (size_t i = 0; i < pixel_count; i++) {
            unsigned char r = rgba[4 * i + 0];
            unsigned char g = rgba[4 * i + 1];
            unsigned char b = rgba[4 * i + 2];
            unsigned char a = rgba[4 * i + 3];

            /*
             * Alpha composite over background:
             * out = src * a + bg * (1 - a)
             */
            rgb[3 * i + 0] = (unsigned char)((r * a + background_r * (255 - a) + 127) / 255);
            rgb[3 * i + 1] = (unsigned char)((g * a + background_g * (255 - a) + 127) / 255);
            rgb[3 * i + 2] = (unsigned char)((b * a + background_b * (255 - a) + 127) / 255);
        }

        std::free(rgba);

        *out_rgb = rgb;
        *out_rgb_size = rgb_size;

        return 0;
    }

}


#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "plutovg.h"

typedef struct {
    unsigned char* data;
    size_t size;
    size_t cap;
} MemoryWriter;

/* ---------- small memory buffer ---------- */

static int MemoryWriter_reserve(MemoryWriter* b, size_t extra)
{
    size_t need = b->size + extra;

    if (need <= b->cap)
        return 0;

    size_t new_cap = b->cap ? b->cap * 2 : 4096;
    while (new_cap < need)
        new_cap *= 2;

    unsigned char* p = (unsigned char*)realloc(b->data, new_cap);
    if (!p)
        return 1;

    b->data = p;
    b->cap = new_cap;
    return 0;
}

static void memory_writer_callback(void* context, void* data, int size)
{
    MemoryWriter* b = (MemoryWriter*)context;

    if (!b || !data || size <= 0)
        return;

    if (MemoryWriter_reserve(b, (size_t)size))
        return;

    memcpy(b->data + b->size, data, (size_t)size);
    b->size += (size_t)size;
}

static int write_file_bytes(const char* filename, const unsigned char* data, size_t size)
{
    if (!filename || (!data && size != 0))
        return 1;

    FILE* fp = fopen(filename, "wb");
    if (!fp)
        return 1;

    int err = 0;
    if (size != 0 && fwrite(data, 1, size, fp) != size)
        err = 1;

    if (fclose(fp) != 0)
        err = 1;

    return err;
}

/*
 * PlutoVG surfaces use native-endian premultiplied ARGB32.
 * Public PNG/JPG surface writers temporarily convert that surface to plain RGBA
 * internally, then call PlutoVG's bundled encoder.
 *
 * This adapter lets old raw RGB/RGBA helpers use the public PlutoVG API instead
 * of including PlutoVG's private plutovg-stb-image-write.h directly.
 */
static unsigned char* raw_to_argb32_surface_data(
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw)
{
    if (!raw || width == 0 || height == 0 || components < 1 || components > 4)
        return NULL;

    size_t pixel_count = (size_t)width * (size_t)height;
    if (pixel_count > SIZE_MAX / 4u)
        return NULL;

    size_t size = pixel_count * 4u;
    unsigned char* rgba = (unsigned char*)malloc(size);
    if (!rgba)
        return NULL;

    for (size_t i = 0; i < pixel_count; ++i) {
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 255;

        const unsigned char* p = raw + i * components;

        switch (components) {
        case 1:
            r = g = b = p[0];
            break;
        case 2:
            r = g = b = p[0];
            a = p[1];
            break;
        case 3:
            r = p[0];
            g = p[1];
            b = p[2];
            break;
        case 4:
            r = p[0];
            g = p[1];
            b = p[2];
            a = p[3];
            break;
        default:
            free(rgba);
            return NULL;
        }

        rgba[4 * i + 0] = r;
        rgba[4 * i + 1] = g;
        rgba[4 * i + 2] = b;
        rgba[4 * i + 3] = a;
    }

    plutovg_convert_rgba_to_argb(
        rgba,
        rgba,
        (int)width,
        (int)height,
        (int)(width * 4u)
    );

    return rgba;
}

static plutovg_surface_t* surface_from_raw(
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw,
    unsigned char** owned_argb)
{
    if (!owned_argb)
        return NULL;

    *owned_argb = raw_to_argb32_surface_data(width, height, components, raw);
    if (!*owned_argb)
        return NULL;

    plutovg_surface_t* surface = plutovg_surface_create_for_data(
        *owned_argb,
        (int)width,
        (int)height,
        (int)(width * 4u)
    );

    if (!surface) {
        free(*owned_argb);
        *owned_argb = NULL;
        return NULL;
    }

    return surface;
}

extern "C" int qadgnuplotlib_write_bmp_memory(
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw,
    unsigned char** out_data,
    size_t* out_size);

/* ============================================================
 * SVG DIRECT MEMORY OUTPUT
 *
 * These use LunaSVG/PlutoVG public output APIs directly.
 * Caller owns *out_data and must free(*out_data).
 * ============================================================ */

extern "C" int qadgnuplotlib_svg_to_png_memory_lunasvg(
    const char* svg_text,
    unsigned width_px,
    unsigned height_px,
    unsigned char background_r,
    unsigned char background_g,
    unsigned char background_b,
    unsigned char** out_data,
    size_t* out_size)
{
    if (!svg_text || !out_data || !out_size || width_px == 0 || height_px == 0)
        return 1;

    *out_data = NULL;
    *out_size = 0;

    std::unique_ptr<lunasvg::Document> document =
        lunasvg::Document::loadFromData(svg_text);

    if (!document)
        return 1;

    uint32_t background =
        ((uint32_t)background_r << 24) |
        ((uint32_t)background_g << 16) |
        ((uint32_t)background_b << 8) |
        0xFFu;

    lunasvg::Bitmap bitmap =
        document->renderToBitmap((int)width_px, (int)height_px, background);

    if (bitmap.isNull())
        return 1;

    MemoryWriter b = { 0 };

    if (!bitmap.writeToPng(memory_writer_callback, &b)) {
        free(b.data);
        return 1;
    }

    *out_data = b.data;
    *out_size = b.size;
    return 0;
}

extern "C" int qadgnuplotlib_svg_to_jpg_memory_lunasvg(
    const char* svg_text,
    unsigned width_px,
    unsigned height_px,
    unsigned char background_r,
    unsigned char background_g,
    unsigned char background_b,
    int quality,
    unsigned char** out_data,
    size_t* out_size)
{
    if (!svg_text || !out_data || !out_size || width_px == 0 || height_px == 0)
        return 1;

    if (quality < 1)
        quality = 1;
    if (quality > 100)
        quality = 100;

    *out_data = NULL;
    *out_size = 0;

    std::unique_ptr<lunasvg::Document> document =
        lunasvg::Document::loadFromData(svg_text);

    if (!document)
        return 1;

    uint32_t background =
        ((uint32_t)background_r << 24) |
        ((uint32_t)background_g << 16) |
        ((uint32_t)background_b << 8) |
        0xFFu;

    lunasvg::Bitmap bitmap =
        document->renderToBitmap((int)width_px, (int)height_px, background);

    if (bitmap.isNull())
        return 1;

    MemoryWriter b = { 0 };

    if (!plutovg_surface_write_to_jpg_stream(bitmap.surface(), memory_writer_callback, &b, quality)) {
        free(b.data);
        return 1;
    }

    *out_data = b.data;
    *out_size = b.size;
    return 0;
}

/* ============================================================
 * FILE OUTPUT
 * ============================================================ */

extern "C" int qadgnuplotlib_write_png_file(
    const char* filename,
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw)
{
    if (!filename || !raw || width == 0 || height == 0)
        return 1;

    if (components < 1 || components > 4)
        return 1;

    unsigned char* owned_argb = NULL;
    plutovg_surface_t* surface = surface_from_raw(width, height, components, raw, &owned_argb);
    if (!surface)
        return 1;

    int err = plutovg_surface_write_to_png(surface, filename) ? 0 : 1;

    plutovg_surface_destroy(surface);
    free(owned_argb);

    return err;
}

extern "C" int qadgnuplotlib_write_jpg_file(
    const char* filename,
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw,
    int quality)
{
    if (!filename || !raw || width == 0 || height == 0)
        return 1;

    if (components < 1 || components > 4)
        return 1;

    if (quality < 1)
        quality = 1;
    if (quality > 100)
        quality = 100;

    unsigned char* owned_argb = NULL;
    plutovg_surface_t* surface = surface_from_raw(width, height, components, raw, &owned_argb);
    if (!surface)
        return 1;

    int err = plutovg_surface_write_to_jpg(surface, filename, quality) ? 0 : 1;

    plutovg_surface_destroy(surface);
    free(owned_argb);

    return err;
}

extern "C" int qadgnuplotlib_write_bmp_file(
    const char* filename,
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw)
{
    unsigned char* data = NULL;
    size_t size = 0;

    if (qadgnuplotlib_write_bmp_memory(width, height, components, raw, &data, &size))
        return 1;

    int err = write_file_bytes(filename, data, size);
    free(data);

    return err;
}

/* ============================================================
 * MEMORY OUTPUT
 *
 * Caller owns *out_data and must free(*out_data).
 * ============================================================ */

extern "C" int qadgnuplotlib_write_png_memory(
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw,
    unsigned char** out_data,
    size_t* out_size)
{
    if (!raw || !out_data || !out_size || width == 0 || height == 0)
        return 1;

    if (components < 1 || components > 4)
        return 1;

    *out_data = NULL;
    *out_size = 0;

    unsigned char* owned_argb = NULL;
    plutovg_surface_t* surface = surface_from_raw(width, height, components, raw, &owned_argb);
    if (!surface)
        return 1;

    MemoryWriter b = { 0 };

    int ok = plutovg_surface_write_to_png_stream(
        surface,
        memory_writer_callback,
        &b
    );

    plutovg_surface_destroy(surface);
    free(owned_argb);

    if (!ok) {
        free(b.data);
        return 1;
    }

    *out_data = b.data;
    *out_size = b.size;
    return 0;
}

extern "C" int qadgnuplotlib_write_jpg_memory(
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw,
    int quality,
    unsigned char** out_data,
    size_t* out_size)
{
    if (!raw || !out_data || !out_size || width == 0 || height == 0)
        return 1;

    if (components < 1 || components > 4)
        return 1;

    if (quality < 1)
        quality = 1;
    if (quality > 100)
        quality = 100;

    *out_data = NULL;
    *out_size = 0;

    unsigned char* owned_argb = NULL;
    plutovg_surface_t* surface = surface_from_raw(width, height, components, raw, &owned_argb);
    if (!surface)
        return 1;

    MemoryWriter b = { 0 };

    int ok = plutovg_surface_write_to_jpg_stream(
        surface,
        memory_writer_callback,
        &b,
        quality
    );

    plutovg_surface_destroy(surface);
    free(owned_argb);

    if (!ok) {
        free(b.data);
        return 1;
    }

    *out_data = b.data;
    *out_size = b.size;
    return 0;
}

/*
 * PlutoVG intentionally exposes PNG/JPG writers but not a BMP writer.
 * Keep BMP tiny and local so we still avoid depending on PlutoVG's private
 * bundled stb headers.
 */
extern "C" int qadgnuplotlib_write_bmp_memory(
    unsigned width,
    unsigned height,
    unsigned components,
    const unsigned char* raw,
    unsigned char** out_data,
    size_t* out_size)
{
    if (!raw || !out_data || !out_size || width == 0 || height == 0)
        return 1;

    if (components < 1 || components > 4)
        return 1;

    *out_data = NULL;
    *out_size = 0;

    size_t row_size = ((size_t)width * 3u + 3u) & ~3u;
    size_t pixel_size = row_size * (size_t)height;
    size_t file_size = 14u + 40u + pixel_size;

    if (file_size > UINT32_MAX)
        return 1;

    unsigned char* bmp = (unsigned char*)calloc(1, file_size);
    if (!bmp)
        return 1;

    uint32_t bf_size = (uint32_t)file_size;
    uint32_t bf_off_bits = 14u + 40u;
    uint32_t bi_size = 40u;
    int32_t bi_width = (int32_t)width;
    int32_t bi_height = (int32_t)height;
    uint16_t bi_planes = 1u;
    uint16_t bi_bit_count = 24u;
    uint32_t bi_size_image = (uint32_t)pixel_size;

#define PUT16(p, v) do { \
        (p)[0] = (unsigned char)((v) & 0xFFu); \
        (p)[1] = (unsigned char)(((v) >> 8) & 0xFFu); \
    } while (0)

#define PUT32(p, v) do { \
        (p)[0] = (unsigned char)((uint32_t)(v) & 0xFFu); \
        (p)[1] = (unsigned char)(((uint32_t)(v) >> 8) & 0xFFu); \
        (p)[2] = (unsigned char)(((uint32_t)(v) >> 16) & 0xFFu); \
        (p)[3] = (unsigned char)(((uint32_t)(v) >> 24) & 0xFFu); \
    } while (0)

    bmp[0] = 'B';
    bmp[1] = 'M';
    PUT32(bmp + 2, bf_size);
    PUT32(bmp + 10, bf_off_bits);

    PUT32(bmp + 14, bi_size);
    PUT32(bmp + 18, bi_width);
    PUT32(bmp + 22, bi_height);
    PUT16(bmp + 26, bi_planes);
    PUT16(bmp + 28, bi_bit_count);
    PUT32(bmp + 34, bi_size_image);

#undef PUT16
#undef PUT32

    unsigned char* pixels = bmp + bf_off_bits;

    for (unsigned y = 0; y < height; ++y) {
        unsigned src_y = height - 1u - y;
        unsigned char* dst = pixels + (size_t)y * row_size;

        for (unsigned x = 0; x < width; ++x) {
            const unsigned char* p = raw + ((size_t)src_y * width + x) * components;

            unsigned char r = 0;
            unsigned char g = 0;
            unsigned char b = 0;

            switch (components) {
            case 1:
                r = g = b = p[0];
                break;
            case 2:
                r = g = b = p[0];
                break;
            case 3:
            case 4:
                r = p[0];
                g = p[1];
                b = p[2];
                break;
            default:
                free(bmp);
                return 1;
            }

            dst[3u * x + 0] = b;
            dst[3u * x + 1] = g;
            dst[3u * x + 2] = r;
        }
    }

    *out_data = bmp;
    *out_size = file_size;
    return 0;
}
