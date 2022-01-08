# Polyline2D
*Polyline2D* is a header-only C++17 library that generates a triangle mesh from a list of points.
It can be used to draw thick lines with rendering APIs like *OpenGL*, which do not support line drawing out of the box.
It supports common joint and end cap styles.

## Example usage
```c++
#include <Polyline2D/Polyline2D.h>
using namespace crushedpixel;

std::vector<Vec2> points{
		{ -0.25f, -0.5f  },
		{ -0.25f,  0.5f  },
		{  0.25f,  0.25f },
		{  0.0f,   0.0f  },
		{  0.25f, -0.25f },
		{ -0.4f,  -0.25f }
};

auto thickness = 0.1f;
auto vertices = Polyline2D::create(points, thickness,
		Polyline2D::JointStyle::ROUND,
		Polyline2D::EndCapStyle::SQUARE);

// render vertices, for example using OpenGL...
```
This code results in the following mesh:
![Result](https://i.imgur.com/D0lvyYT.png)
For demonstration purposes, the generated mesh is once rendered in wireframe mode (light green), and once in fill mode (transparent green).

The red points show the input points.

There is *no overdraw* within segments, only lines that overlap are filled twice.

For an example application using this software, visit [Polyline2DExample](https://github.com/CrushedPixel/Polyline2DExample).

## Installation
### Manual
To use *Polyline2D*, simply clone this repository and include the file `Polyline2D.h`  from the `include` directory.

### Using CMake
To install the header files into your global header directory, you can use CMake:
```
mkdir build
cd build
cmake ..
make install
```

You can then include the header file using `#include <Polyline2D/Polyline2D.h>`.
