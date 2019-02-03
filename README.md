# trim2unmap
A workaround to let SSD TRIM work on some SAS RAID cards.

## Warning
No warranty provided, use this at your own risk

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

```
trim2unmap <physical device> <virtual device>
```

e.g.

```
trim2unmap /dev/sdd /dev/nbd/nbd10
```

Then you can mount your virtual device like mounting the physical device as usual

```
mount /dev/nbd/nbd10 /mnt
```

## Caveats
- You need to load the `nbd` kernel module first.
