# libsbc

These portable C sources come from [Google libsbc](https://github.com/google/libsbc)
at revision `6e505650145c9973d08a0bdd5e5f5e1914305e40`. They retain their
Apache 2.0 notices. The Bluetooth2 build maps `memset` to a freestanding implementation.
The local copy uses it explicitly in `sbc_reset` and initializes the decoder's
scale variables. It uses multiplication for signed filter scaling so negative
PCM samples do not rely on signed left shifts.
