Cloud Connector C API
=====================

Overview
--------

Cloud Connector C API (CCAPI) is a layer built on top of Cloud Connector Finite State Machine (CCFSM) (a.k.a. "Cloud Connector for Embedded" or "cc_ansic"). It is
aimed to facilitate the development of applications that communicate with Device Cloud in platforms that support threading and dynamic memory allocation.

Required Tools
--------------

* **GNU make** 3.8.1 or later (`sudo apt-get install make`)
* **gcc 4.8.2** or later (`sudo apt-get install gcc`)
* **g++ 4.8.2** or later (`sudo apt-get install g++`)
* **JDK 1.8.0** or later (`sudo apt-get install default-jdk` or from [here](http://www.oracle.com/technetwork/java/javase/downloads/index.html))
* **Apache Ant 1.9.3** or later (`sudo apt-get install ant` or from [here](http://ant.apache.org/))
* **CppUTest 3.6** available at [CppUTest home page](https://cpputest.github.io). Download, extract and execute:
  * `$ ./configure`
  * `$ make`
  * `$ make check`
  * `$ sudo make install`
* **gcov 4.8.2** or later (`sudo apt-get install ggcov`)
* **gcovr 3.2** or later (`sudo apt-get install gcovr` or from [here](https://github.com/gcovr/gcovr/releases))

Both **JDK** and **Ant** are needed to build *ConfigGenerator.jar* CASE tool that is used to generate RCI service information. **CppUTest** is the Unit Tests harness
and mocking interface used. Finally, **gcov** and **gcovr** are needed only to build the *coverage* target that output how much of the code is executed by the unit tests

Directory organization
----------------------

- `include` 
    - `ccapi`: the CCAPI function and types definitions (this path must be added to compiler's include path).
    - `ccimp`: the CCIMP functions and types definitions (this path must be added to compiler's include path)
    - `custom`: files that user can modify to adapt either CCIMP or CCAPI to the application requirements (this path must be added to compiler's include path).
- `source`:
    - `cc_ansic`: CCFSM repository added as a submodule.
        - `public/include`: this path must be added to the compiler's include path.
        - `private`: file connector_api.c: needs to be compiled.
    - `cc_ansic_custom_include`: files needed by CCFSM adapted to CCAPI
    - All files in this subdirectory are needed to build CCAPI, and all of them must be compiled.
- `tests`
    - `cc_testcases`: files used by cc_testharness to run integration tests.
    - `ccimp`: a linux implementation of CCIMP for testing CCAPI, not for production use.
    - `samples`: a collection of simple applications to test functionality, not for distribution.
    - `unit_tests`: all the Unit Tests that will be compiled and run by CppUTest.
    

Building and Running UnitTests
------------------------------
The Makefile in the root directory has several useful targets. To only build the binary execute:
- `$ make clean all`

This will build "test" that can be executed as follows:
- `$ ./test`: will execute all the tests in all test groups
- `$ ./test -g <group_name>`: will execute all tests inside "group_name" group
- `$ ./test -g <group_name> -n <test_name>`: will only execute tests <test_name> of "group_name" group
- `$ ./test -r <N>`: will execute all tests N times

To build and run unit tests, execute:
- `make clean run_test`

To get the coverage information, execute:
- `$ make coverage`: This will build, link and run the test binary and run gcov and gcovr to output the coverage information for all files in "source/" subdirectory

License
-------
Copyright 2017, Digi International Inc.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
