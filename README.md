cc_dey
=======

This repository contains all the components for the Cloud Connector for Digi
Embedded Yocto library.

The library is structured in the following directories:
* cfg_files: configuration file
* src: source code

Compiling the application
-------------------------
The respository includes a `Makefile` that compiles and generates the shared
library. Before building, make sure to import the corresponding toolchain of
the platform you want to compile the library for.
