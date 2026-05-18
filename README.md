# qadgnuplotlib

Standalone embedded gnuplot library for C and C++20.

`qadgnuplotlib` embeds gnuplot directly into an application without requiring an external gnuplot installation.  
It provides modern C++20 and plain C APIs to execute gnuplot scripts entirely from memory and generate graphics in multiple formats.

The library is currently developed and tested only with:

- Microsoft Visual Studio 2022 or newer
- Windows x64

---

# Features

- Standalone embedded gnuplot library
- No external gnuplot executable required
- Execute scripts directly from memory
- C++20 API
- Plain C API
- SVG generation
- PNG generation
- JPG generation
- BMP generation
- Raster PDF generation
- Raw RGBA/RGB buffer generation
- In-memory datasets using `mem://`
- No temporary files required for plotting
- Supports:
  - multiplot
  - pm3d
  - image/matrix plots
  - transparency
  - parametric surfaces
  - labels/styles

---

# Example

```cpp
#include "qadgnuplotlib.hpp"

#include <vector>

int main()
{
    std::vector<double> x {0,1,2,3,4};
    std::vector<double> y {0,1,4,9,16};

    qadgnuplotlib::Context gp;

    gp.register_data("x", x);
    gp.register_data("y", y);

    gp.run(R"GPLOT(
        set terminal qad size 800,600 format png
        set output "example.png"

        plot "mem://x,y" with linespoints
    )GPLOT");

    return 0;
}
