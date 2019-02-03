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

Load the `nbd` kernel module first.

```
modprobe nbd
```

Syntax:

```
trim2unmap <physical device> <virtual device>
```

e.g.

```
trim2unmap /dev/sdd /dev/nbd10
```

Then you can mount your virtual device like mounting the physical device as usual

```
mount /dev/nbd10 /mnt
```

## Screenshots
![](https://raw.githubusercontent.com/ReimuNotMoe/ReimuNotMoe.github.io/master/images/trim2unmap/Screenshot_20190203_203012.png)

![](https://raw.githubusercontent.com/ReimuNotMoe/ReimuNotMoe.github.io/master/images/trim2unmap/Screenshot_20190203_203317.png)

![](https://raw.githubusercontent.com/ReimuNotMoe/ReimuNotMoe.github.io/master/images/trim2unmap/Screenshot_20190203_203351.png)
