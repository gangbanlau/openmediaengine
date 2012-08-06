1, first patch pjproject 2.0.1 release;
2, replace config.guess and config.sub
3, 
./aconfigure --host=arm-linux-androideabi --enable-ext-sound --disable-speex-codec --disable-speex-aec --disable-l16-codec --disable-g722-codec --disable-floating-point
make dep; make

you can take a look user.mak to change compile flags.

