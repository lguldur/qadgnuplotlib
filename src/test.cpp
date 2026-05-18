/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        C++ API test program for the qadgnuplotlib static library.
*/

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "qadgnuplotlib.hpp"

typedef struct {
	const char* name;
	const char* ext;
	const char* terminal;
} TestFormat;

typedef struct {
	const char* name;
	const char* body;
	bool terminal_included;
} TestScript;

static int run_one(qadgnuplotlib::Context& gp,
    size_t script_index,
    const TestScript* script,
    const TestFormat* fmt)
{
    char output_path[256];

    std::snprintf(output_path, sizeof(output_path),
        "test_output/qadgnuplotlib_test_%02zu.%s",
        script_index,
        fmt->ext);

    std::string full_script;
    full_script += "reset\n";
    full_script += fmt->terminal;
    full_script += "\n";
    full_script += "set output '";
    full_script += output_path;
    full_script += "'\n";
    full_script += script->body;
    full_script += "\nset output\n";

    std::printf("Running script %02zu %-18s -> %-24s [%s]:\n%s\n\n",
        script_index,
        script->name,
        output_path,
        fmt->name,
        full_script.c_str());

    try {
        gp.run(full_script);
        return 0;
    } catch (const qadgnuplotlib::Error& e) {
        std::fprintf(stderr, "FAILED: script %02zu format %s: %s\n",
            script_index, fmt->name, e.what());
        return 1;
    }
}

int main()
{
    std::filesystem::create_directories("test_output");

    qadgnuplotlib_set_log_file("test_output/qadgnuplotlib_test.log");

    qadgnuplotlib::Context gp;

	gp.run("show version");
	gp.run("show version long");
    gp.run("set terminal");

    std::printf("qadgnuplotlib version: %s\n", qadgnuplotlib_version(0));

    // Exercise the new reusable data registry API.
    // The data is not consumed by the legacy scripts below yet; it is here
    // to verify register/reregister/unregister behavior before mem:// is wired.
    std::vector<double> x {0.0, 1.0, 2.0, 3.0};
    std::vector<double> y {0.0, 1.0, 4.0, 9.0};
    gp.register_data("x", x);
    gp.register_data("y", y);
    y = {0.0, 1.0, 8.0, 27.0};
    gp.register_data("y", y);
    gp.unregister_data("x");
    gp.register_data("x", x);

	const TestFormat formats[] = {
		{
			"svg",
			"svg",
			"set terminal qad size 800,600 format svg"
		},
		{
			"bmp",
			"bmp",
			"set terminal qad size 800,600 format bmp"
		},
		{
			"png",
			"png",
			"set terminal qad size 4000,3000 format png"
		},
		{
			"jpg",
			"jpg",
			"set terminal qad size 800,600 format jpg"
		},
		{
			"pdf",
			"pdf",
			"set terminal qad format pdf size 16cm,9.6cm dpi 300"
		}
	};

	const TestScript scripts[] = {
		{
			"2D sin",
			"set title '2D sin(x)'\n"
			"set xlabel 'x'\n"
			"set ylabel 'sin(x)'\n"
			"set grid\n"
			"plot sin(x) title 'sin(x)' with lines",
			false
		},



        {
            "mem columns",
            "set title 'mem:// registered columns test'\n"
            "set xlabel 'x'\n"
            "set ylabel 'y'\n"
            "set grid\n"
            "plot \"mem://x,y\" with linespoints title 'registered x,y'",
            false
        },

		{
			"2D multiplot",
			"set multiplot layout 2,1 title 'Multiplot test'\n"
			"set grid\n"
			"plot sin(x) title 'sin(x)' with lines\n"
			"plot cos(x) title 'cos(x)' with lines\n"
			"unset multiplot",
			false
		},

		{
			"3D hidden lines",
			"set title '3D hidden3d surface'\n"
			"set xlabel 'X'\n"
			"set ylabel 'Y'\n"
			"set zlabel 'Z'\n"
			"set hidden3d\n"
			"set isosamples 50,50\n"
			"splot sin(x)*cos(y) title 'sin(x)*cos(y)' with lines",
			false
		},

		{
			"3D pm3d surface",
			"set title '3D pm3d surface'\n"
			"set xlabel 'X'\n"
			"set ylabel 'Y'\n"
			"set pm3d at s\n"
			"set palette rgb 33,13,10\n"
			"set isosamples 80,80\n"
			"f(x,y) = (x == 0 && y == 0) ? 1 : sin(sqrt(x*x+y*y))/sqrt(x*x+y*y)\n"
			"splot f(x,y) title 'sinc radius' with pm3d",
			false
		},

		{
			"3D parametric torus",
			"set title 'Parametric torus'\n"
			"set parametric\n"
			"set urange [-pi:pi]\n"
			"set vrange [-pi:pi]\n"
			"set hidden3d\n"
			"set isosamples 50,50\n"
			"splot cos(u)*(3+cos(v)), sin(u)*(3+cos(v)), sin(v) title 'torus' with lines",
			false
		},

		{
			"PM3D map",
			"set title 'PM3D map / image-like test'\n"
			"set view map\n"
			"set pm3d\n"
			"set palette rgbformulae 22,13,-31\n"
			"set samples 150,150\n"
			"set isosamples 150,150\n"
			"splot x*y*sin(x)*cos(y) notitle",
			false
		},

		{
			"Torture Test",
R"zzz(
reset

set multiplot layout 2,2 title "QAD Torture Test"

########################################
# 1. Dense PM3D map (image-like stress)
########################################
set title "PM3D map (high density)"
set view map
set pm3d
set palette rgbformulae 22,13,-31
set samples 250,250
set isosamples 250,250
unset key
splot x*y*sin(x)*cos(y)

########################################
# 2. Transparent PM3D surface
########################################
set title "PM3D + transparency"
unset view
set view 60,30
set pm3d depthorder
set palette rgb 33,13,10
set style fill transparent solid 0.5
set samples 120
set isosamples 120
splot sin(x)*cos(y)

########################################
# 3. Parametric surface (geometry stress)
########################################
set title "Parametric torus"
set parametric
set urange [-pi:pi]
set vrange [-pi:pi]
set hidden3d
set isosamples 80,80
splot cos(u)*(3+cos(v)), sin(u)*(3+cos(v)), sin(v)

unset parametric
unset hidden3d

########################################
# 4. Mixed 2D stress (labels, styles)
########################################
set title "2D mixed styles"
set grid
set key top left

set label 1 "Center" at 0,0
set label 2 "Peak" at pi/2,1
set label 3 "Min" at 3*pi/2,-1

set style line 1 lw 2 lc rgb "red"
set style line 2 lw 2 dt 2 lc rgb "blue"

plot sin(x) with lines ls 1 title "sin(x)", \
     cos(x) with lines ls 2 title "cos(x)", \
     sin(x)*cos(3*x) with points pt 7 ps 0.5 title "points"

########################################

unset multiplot
)zzz",
			false
		},

		{
			"matrix",
R"zzz(
reset

set title "QAD image/matrix test"
set terminal qad size 1000,700 format png
set output "test_output/qadgnuplotlib_image_matrix.png"

set view map
set size ratio -1
unset key
set colorbox
set palette rgbformulae 22,13,-31

set xrange [-0.5:99.5]
set yrange [-0.5:99.5]

# Generate a real regular x/y/z grid.
# Do not use plot '++' here: in 2D plot mode it does not generate the
# full 2D image grid expected by 'with image'.
f(x,y) = sin(x/7.0)*cos(y/9.0) + 0.35*sin((x+y)/5.0)
set print $QAD_IMG_MATRIX
do for [yy=0:99] {
    do for [xx=0:99] {
        print sprintf("%g %g %g", xx, yy, f(xx, yy))
    }
    print ""
}
set print

plot $QAD_IMG_MATRIX using 1:2:3 with image

set output
)zzz",
true
				},

				{
			"checkerboard",
R"zzz(
reset

set title "QAD image checkerboard test"
set terminal qad size 1000,700 format png
set output "test_output/qadgnuplotlib_image_checker.png"

set view map
set size ratio -1
unset key
set colorbox
set palette defined (0 "blue", 1 "white", 2 "red")

set xrange [-0.5:63.5]
set yrange [-0.5:63.5]

# Generate a real regular x/y/z grid.
# This specifically exercises svg.trm's <image ... base64 PNG> path.
set print $QAD_IMG_CHECKER
do for [yy=0:63] {
    do for [xx=0:63] {
        print sprintf("%g %g %g", xx, yy, (int(xx/4)+int(yy/4))%3)
    }
    print ""
}
set print

plot $QAD_IMG_CHECKER using 1:2:3 with image

set output
)zzz",
true
				},
	};

	const size_t format_count = sizeof(formats) / sizeof(formats[0]);
	const size_t script_count = sizeof(scripts) / sizeof(scripts[0]);

	int failures = 0;

	for (size_t i = 0; i < script_count; ++i) {
		for (size_t j = 0; j < format_count; ++j) {
			if (scripts[i].terminal_included)
			{
				if (j)
					continue;

				std::printf("Running script %02zu %-18s:\n%s\n\n",
					i,
					scripts[i].name,
					scripts[i].body);

				int rc = 0;
                try {
                    qadgnuplotlib::Script saved_script(scripts[i].body);
                    saved_script.save("test_output/qadgnuplotlib_last_script.gp");
                    gp.run(qadgnuplotlib::Script::load("test_output/qadgnuplotlib_last_script.gp"));
                } catch (const qadgnuplotlib::Error& e) {
                    rc = 1;
                    std::fprintf(stderr, "FAILED: script %02zu: %s\n", i, e.what());
                }
                failures += rc;
			}
			else
			{
				if (run_one(gp, i, &scripts[i], &formats[j]) != 0)
					++failures;
			}
		}
	}

	std::printf("\nDone. Failures: %d\n", failures);
	std::printf("Press enter key to continue.\n");
	std::getchar();

	return failures ? 1 : 0;
}
