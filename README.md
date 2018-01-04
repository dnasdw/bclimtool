# bclimtool

A tool for decoding/encoding bclim file.

## History

- v1.1.0 @ 2018.01.04 - A new beginning

### v1.0

- v1.0.0 @ 2015.03.11 - First release
- v1.0.1 @ 2017.07.27 - Commandline support unicode

## Platforms

- Windows
- Linux
- macOS

## Building

### Dependencies

- cmake
- zlib
- libpng

### Compiling

- make 64-bit version
~~~
mkdir project
cd project
cmake -DUSE_DEP=OFF ..
make
~~~

- make 32-bit version
~~~
mkdir project
cd project
cmake -DBUILD64=OFF -DUSE_DEP=OFF ..
make
~~~

### Installing

~~~
make install
~~~

## Usage

### Windows

~~~
bclimtool [option...] [option]...
~~~

### Other

~~~
bclimtool.sh [option...] [option]...
~~~

## Options

See `bclimtool --help` messages.
