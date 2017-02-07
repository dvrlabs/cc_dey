Cloud Connector for Digi Embedded Yocto (DEY)
=============================================
Cloud Connector for Digi Embedded Yocto implements a library and an example
application that allows to connect devices to Digi's Remote Manager.

The repository is structured in the following directories:

* app/cfg_files: connector configuration file
* app/src: connector example application source code
* library/src: library source code

This repository implements the Digi Embedded Yocto support as a layer on top
of the Cloud Connector C API and Cloud Connector Ansi C repositories. Those
repositories are installed as git submodules executing the following command:

```
git submodule update --init --recursive
```

Compiling the library and application
-------------------------------------
The Cloud Connector is meant to be used by Digi Embedded Yocto, which includes
recipes to build the package. It's also part of the default images generated
by Digi Embedded Yocto.

It can be also compiled using a Yocto based toolchain. In that case, make
sure to source the corresponding toolchain of the platform you want to build
the connector for, e.g:

```
. <DEY-toolchain-path>/environment-setup-cortexa7hf-vfp-neon-dey-linux-gnueabi
make
```

More information about [Digi Embedded Yocto](https://github.com/digi-embedded/meta-digi).

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
