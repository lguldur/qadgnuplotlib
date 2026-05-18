/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        C API test program for the qadgnuplotlib static library.
*/

#include "qadgnuplotlib.h"

#include <stdio.h>

int main(void)
{
    qadgnuplotlib_set_log_file("test_output/qadgnuplotlib_test.log");

    double x[] = {0, 1, 2, 3};
    double y[] = {0, 1, 8, 27};

    qadgnuplotlib_context* ctx = qadgnuplotlib_create();

    if(!ctx) {
        printf("failed to create context\n");
        return 1;
    }

    qadgnuplotlib_register_data_float64(ctx, "x", x, 4);
    qadgnuplotlib_register_data_float64(ctx, "y", y, 4);

    const char* script =
        "set terminal qad size 800,600 format png\n"
        "set output 'test_output/c_api_test.png'\n"
        "set title 'C API mem:// test'\n"
        "plot 'mem://x,y' with linespoints title 'x,y from C API'\n";

    if(qadgnuplotlib_run(ctx, script)) {
        printf("run failed: %s\n", qadgnuplotlib_last_error());
        qadgnuplotlib_destroy(ctx);
        return 1;
    }

    qadgnuplotlib_destroy(ctx);

    printf("done\n\nPress Enter to exit\n");

    getchar();
    return 0;
}
