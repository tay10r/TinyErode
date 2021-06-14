Getting Started
===============

To get started using the library, start by including the header `TinyErode.h`.

```cpp
#include <TinyErode.h>
```

Then declare a @ref Simulation class instance in a function that you need to
simulation erosion in.

```cpp
TinyErode::Simulation simulation;
```

If you know the size of the terrain at this point, then you can pass it as an
argument to the constructor.

```cpp
int w = 640;
int h = 480;
TinyErode::Simulation simulation(w, h);
```

Or call @ref Simulation::Resize later on.

```cpp
int w = 640;
int h = 480;
TinyErode::Simulation simulation;
simulation.Resize(w, h);
```

### Defining the Terrain Model

The terrain can be defined in several ways. For this example, the terrain
and water model are defined using STL vectors.

```cpp
float initWaterLevel = 1.0f;
int w = 640;
int h = 480;
std::vector<float> height(w * h);
std::vector<float> water(w * h, initWaterLevel);
```

For a little more asymmetry, the water levels can be randomly jittered.

There are several functions that need to be defined so that the simulator can
read and write to the terrain model. The first two are used for reading and
writing height values. They look like this:

```cpp
auto getHeight = [](int x, int y) -> float {
  return /* return height value at (x, y) */ 0.0f;
};

auto addHeight = [heightMap](int x, int y, float deltaHeight) {
  /* add 'deltaHeight' to the location (x, y)
};
```

Since the movement of water is what causes terrain erosion, a set of functions
will also need to be defined in order to read and write to the water model.
These functions will look like this:

```cpp
auto getWater = [](int x, int y) -> float {
  return /* return water level at (x, y) */ return 0.0f;
};

auto addWater = [](int x, int y, float deltaWater) -> float {

  /* Note: 'deltaWater' can be negative. */

  float previousWaterLevel = 1.0f;

  /* The function returns the new water level. It shuold not
   * fall below zero. */

  return std::max(0.0f, previousWaterLevel + deltaWater);
};
```

To describe the soil transporation characteristics, three functions must be
defined.

 - **Carry Capacity** - A function to describe how much soil can be carried by
 water in a given cell. Typical values are between `0.01` and `0.1`.

 - **Deposition** - Describes how much soil is deposited when the carry capacity
 of a cell is reached. Typical values are between `0` and `1`.

 - **Erosion** - Describes how much height is subtracted from the terrain when
 water picks up soil. Typical values are between `0` and `1`, but can differ
 based on the scale of the terrain.

The functions look like this:

```
auto carryCapacity = [](int x, int y) -> float {
  return 0.1;
};

auto deposition = [](int x, int y) -> float {
  return 0.1;
};

auto erosion = [](int x, int y) -> float {
  return 0.1;
};
```

The next function allows the simulator to evaporate water during each iteration.
The function returns the evaporation constant at every point in the terrain. To
keep this simple, the example considers the evaporation constant uniform across
the terrain, but more complex models can take heat into account in order to
achieve more realistic results. The function looks like this:

```cpp
auto evaporation = [](int x, int y) -> float {
  return 0.1;
};
```

The value returned by the function indicates what ratio of water should be
evaporated from the specified location.

### Running the Simulation

Once all the correct functions have been defined, the erosion process can be
simulated. It has to be done in a specific order.

Be sure to first set the number of meters per pixel. For example, if the height
value ranges from 0 to 200 meters, then the total width and height of the
terrain might be somewhere around 1000 meters. To account for this, use the
following functions:

```cpp
simulation.SetMetersPerX(1000.0f / w);
simulation.SetMetersPerY(1000.0f / h);
```

The simulation will need several hundred iterations in order to compute the
transporation of sediment. The specific number of iterations depends on how long
it takes for the water to evaporation. For simplicity, we can just go for 1024
iterations, which may be more than what is needed.

```cpp
for (int i = 0; i < iterations; i++) {

  // Determines where the water will flow.
  simulation.ComputeFlowAndTilt(getHeight, getWater);

  // Moves the water around the terrain based on the previous computed values.
  simulation.TransportWater(addWater);

  // Where the magic happens. Soil is picked up from the terrain and height
  // values are subtracted based on how much was picked up. Then the sediment
  // moves along with the water and is later deposited.
  simulation.TransportSediment(carryCapacity, deposition, erosion, addHeight);

  // Due to heat, water is gradually evaported. This will also cause soil
  // deposition since there is less water to carry soil.
  simulation.Evaporate(addWater, evaporation);
}

// Drops all suspended sediment back into the terrain.
simulation.TerminateRainfall(addHeight);
```

And that sums it up! To get a better understanding of how the algorithm works,
try messing around with the number of iterations or modifying parameters. The
transportation of water and sediment can also be visualized in order to
understand how each parameter affects the simulation.
