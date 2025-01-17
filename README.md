TinyErode
=========

~~*Note: I have recently discovered that the implementation of this algorithm is not numerically stable, and will in some cases destroy the height field it is operating on. Be advised that this is currently being worked on. Hopefully it will be resolved soon. See the `develop` branch for the latest work.*~~

*The notice regarding the numeric stability bug that had been up for a while has been addressed.*

This is a single-header, multithreaded C++ library for simulating the effect of hydraulic erosion on
height maps. The algorithm is based on the [the one in this paper](https://hal.inria.fr/inria-00402079/).

The library takes an arbitrary terrain model and simulated the flow of water across it. As water flows,
soil is picked up from the terrain and deposited in other areas depending on the flow of water.

For example, given the following model of a hill:

<img src="https://user-images.githubusercontent.com/69982525/121899116-f4faa000-ccd8-11eb-9dd4-577d1eaedf51.png"/>

When the library simulates erosion, the soil transporation causes the model to change into this:

<img src="https://user-images.githubusercontent.com/69982525/121899109-f3c97300-ccd8-11eb-82dc-6be9d1dc8aba.png"/>

It can also smooth out noisy terrains, such as this one:

<img src="https://user-images.githubusercontent.com/69982525/121828887-d1523e00-cc75-11eb-9b8d-96095d4bc5c8.png"/>

The result:

<img src="https://user-images.githubusercontent.com/69982525/121828890-d1ead480-cc75-11eb-9232-4e01669d48ee.png"/>

### Getting Started

See [the example](https://github.com/tay10r/TinyErode/tree/main/docs) to learn how to integrate the library in an application.
