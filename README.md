# trim2unmap
A workaround to let SSD TRIM work on som SAS RAID cards.

## Build
### Dependencies
- [BUSE](https://github.com/ReimuNotMoe/BUSE)

### Compile
Nearly all my projects use CMake. It's very simple:

    mkdir build
    cd build
    cmake ..
    make -j `nproc`

## How to use

## Caveats
- You need to load the `nbd` kernel module first.
