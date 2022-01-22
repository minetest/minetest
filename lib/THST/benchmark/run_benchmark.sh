#!/bin/sh
################################################################################
# Script running spatial_index_benchmark programs
# https://github.com/mloskot/spatial_index_benchmark
################################################################################
# Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
################################################################################

if [[ ! -d $1 ]]; then
    echo "Cannot find '$1' build directory"
    exit 1
fi

RDIR="$PWD"
if [[ -d $2 ]]; then
    RDIR="$2"
fi 

BDIR="$1"
LOGEXT="dat"
#
# Benchmark iterative loading (takes long time, skipped on Travis CI)
#
if [[ "$TRAVIS" != "true" ]] ; then
for variant in linear quadratic rstar quadtree
do
    for benchmark in `find $BDIR -type f -name "*${variant}*" | sort`
    do
        name=`basename ${benchmark}`
        echo "$name"
        ${benchmark} > ${RDIR}/${name}.${LOGEXT}
    done;
done;
fi

#
# Benchmark bulk loading
#
for variant in linear quadratic rstar
do
    for benchmark in `find $BDIR -type f -name "*${variant}_blk" | sort`
    do
        name=`basename ${benchmark}`
        echo "$name"
        ${benchmark} > ${RDIR}/${name}.${LOGEXT}
    done;
done;

cd ./results
gnuplot ./plot_results.plt
