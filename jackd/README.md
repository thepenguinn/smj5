# Earphone Jack Daemon

Listens for Earphone events and plays or pauses MPD accordingly.
Btw, only works on Samsung J5.

# Building

You need `libmpdclient`, and a C compiler.

Run below, if you have `make`.

```sh
make jackd
```

Or

```sh
gcc -o jackd -lmpdclient jackd.c
```
