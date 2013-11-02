#!/bin/bash

> sigmaEffPoints.dat
echo "12.5" > voltage.dat

for (( i = 100; i > 0; i -= 10))
do

    ./ASCII2ROOT $1 dataTree.root $i voltage.dat
    ./CreatePlots

    ./ASCII2ROOT $1 dataTree.root $i voltage.dat
    ./CreatePlots

done

root -l -q plotTimingResolution.C
