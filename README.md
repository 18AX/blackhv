# black-hv

BlackHV is a library that aims to make virtualization easy from a programming point of view.

## Dependencies

 * SDL2

```
sudo apt-get install libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev
```

## How to build ?

```sh
make # build the libblackhv in the build directory
```

To build examples

```
cd examples/<example_name>
make
```

If you do not want to compile your own linux image, you can find one here https://gist.github.com/zserge/ae9098a75b2b83a1299d19b79b5fe488.

## Documention

The file `DOCS.md` contains all the documentation related to the library. The file `main.c` is an example of how the library can be used.