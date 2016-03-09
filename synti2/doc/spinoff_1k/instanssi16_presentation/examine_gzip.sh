#!/bin/sh
# Quick hack.
make -B i16example.run \
    && ~/files/hacking/gzthermal_04c/gzthermal -b -n i16example.run.payload.gz \
    && mv gzthermal-result.png gzthermal-result.binary.png \
    && ~/files/hacking/gzthermal_04c/gzthermal -n i16example.run.payload.gz \
    && mv gzthermal-result.png gzthermal-result.text.png \
    && ~/files/hacking/defdb_04b/defdb -t i16example.run.payload.gz > defdb-result.dat \
    && ~/files/hacking/defdb_04b/defdb i16example.run.payload.gz \
    && ./i16example.bin 
