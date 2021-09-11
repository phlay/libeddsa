# libeddsa

This is a small cryptographic library for signatures with [ed25519](http://ed25519.cr.yp.to/ed25519-20110705.pdf) and diffie-hellman key exchange with [x25519](http://cr.yp.to/ecdh/curve25519-20060209.pdf).

My goal is to give a fast, but still readable, C implemantation of these two crypto primitives without any complex framework. (If you need a full and easy to use framework with symmetric cipher and MAC included, please have a look at [libnacl](http://nacl.cr.yp.to) or [libsodium](https://github.com/jedisct1/libsodium) which are both great.)

If you need just ed25519-signatures or x25519-key-exchange with a simple API, however, libeddsa may be for you: It is small (under 90kb) and quite fast.


### Features:
- written in C
- fast and small
- cmake build system
- protection against timing attacks as far as possible in C
- static and dynamic link support
- easy to use (see wiki)
- public domain license
