#!/gnuplot
################################################################################
# Results plotting configuration for spatial_index_benchmark
# https://github.com/mloskot/spatial_index_benchmark
################################################################################
# Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
################################################################################
#
#set terminal wxt
set key on left horizontal
set terminal png size 1200,1000 font ",13"
outfmt = ".png"
#set terminal svg size 1200,1000 dynamic font ",13"
#outfmt = ".svg"

algos = "linear quadratic rstar"
variants = "quadratic quadratic_custom quadratic_sphere quadratic_sphere_custom"

array libs[2] = ["bgi", "thst"]
array arr[2] = [algos, variants]
array ext[2] = ["_ct", ""]

set xlabel "max capacity (min capacity = max * 0.5, quadtree capacity = max * 64 )"
#
# Plot loading times
#
set ylabel "load 1M objects in seconds"

set title "Iterative loading using RTree balancing algorithms"
set output "benchmark_load_bgi_vs_thst".outfmt
plot for[i=1:2] for [ m in arr[i] ] \
  "<(head -20 ".libs[i]."_".m.ext[i].".dat)" using 1:3 with linespoints title libs[i]."-".m noenhanced

set title "BGI: Iterative loading using R-tree balancing algorithms vs bulk loading (blk)"
set output "bgi_benchmark_rtree_load_itr_vs_blk".outfmt
plot for [l in "bgi"] for [ m in algos." rstar\_blk" ] \
   "<(head -20 ".l."_".m."_ct.dat)" using 1:3 with linespoints title l."-".m noenhanced

set title "THST: Iterative loading between the tree variations"
set output "thst_benchmark_load_itr".outfmt
plot for [l in "thst"] for [ m in variants." quadtree" ] \
  "<(head -20 ".l."_".m.".dat)" using 1:3 with linespoints title l."-".m noenhanced

set title "Bulk loading (blk) times not affected by R-tree balancing algorithms"
set output "bgi_benchmark_rtree_load_blk_vs_balancing".outfmt
plot for [l in "bgi"] for [m in algos] \
  "<(head -20 ".l."_".m."_blk_ct.dat)" using 1:3 with linespoints title l." ".m."_blk" noenhanced

#
# Plot querying times
#
set ylabel "query 100K of 1M objects in seconds"

set title "Query times for each tree balancing algorithms"
set output "benchmark_query_bgi_vs_thst".outfmt
plot for[i=1:2] for [ m in arr[i] ] \
  "<(head -20 ".libs[i]."_".m.ext[i].".dat)" using 1:4 with linespoints title libs[i]."-".m noenhanced

array qarr[2] = ["rstar rstar_blk", "quadratic_custom quadratic_sphere_custom"]
set title "Best query times for the tree balancing algorithms"
set output "benchmark_query_bgi_vs_thst_best".outfmt
plot for[i=1:2] for [ m in qarr[i] ] \
  "<(head -20 ".libs[i]."_".m.ext[i].".dat)" using 1:4 with linespoints title libs[i]."-".m noenhanced

set title "BGI: Query times using R-tree balancing algorithms vs bulk loading (blk)"
set output "bgi_benchmark_rtree_query_itr_vs_blk".outfmt
plot for [l in "bgi"] for [ m in algos." rstar_blk" ] \
  "<(head -20 ".l."_".m."_ct.dat)" using 1:4 with linespoints title l."-".m noenhanced

set title "THST: Query times between the tree variations"
set output "thst_benchmark_query_itr".outfmt
plot for [l in "thst"] for [ m in variants." quadtree" ] \
  "<(head -20 ".l."_".m.".dat)" using 1:4 with linespoints title l."-".m noenhanced

set title "THST: Query times between the custom allocator variant"
set output "thst_benchmark_query_cst".outfmt
plot for [l in "thst"] for [ m in variants ] \
  "<(head -20 ".l."_".m.".dat)" using 1:4 with linespoints title l."-".m noenhanced

set title "BGI: Query times not affected by R-tree bulk loading (blk) vs balancing algorithms"
set output "bgi_benchmark_rtree_query_blk_vs_balancing".outfmt
plot for [l in "bgi"] for [m in algos] \
  "<(head -20 ".l."_".m."_blk_ct.dat)" using 1:4 with linespoints title l." ".m."_blk" noenhanced

#
# Plot dynamic use case(querying + insert + clear) times
#

set xlabel "number of iterations(insertions/queries)"
set ylabel "average time for an 16, 8 RTree of 200 runs in msec"

set title "Dynamic use case(clear followed by query and insert) using RTree balancing algorithms"
set output "benchmark_dynamic_bgi_vs_thst".outfmt
plot for[i=1:2] for [ m in arr[i] ] \
  "<(sed -n '21, 107p' ".libs[i]."_".m.ext[i].".dat)" using 3:4 with linespoints title libs[i]."-".m noenhanced

array darr[2] = ["linear quadratic", "quadratic quadratic_custom quadratic_sphere"]
set title "Dynamic use case(clear followed by query and insert) using RTree balancing algorithms"
set output "benchmark_dynamic_vsmall_bgi_vs_thst".outfmt
plot for[i=1:2] for [ m in darr[i] ] \
  "<(sed -n '21, 60p' ".libs[i]."_".m.ext[i].".dat)" using 3:4 with linespoints title libs[i]."-".m noenhanced

set title "Dynamic use case(clear followed by query and insert) using RTree balancing algorithms"
set output "benchmark_dynamic_small_bgi_vs_thst".outfmt
plot for[i=1:2] for [ m in darr[i] ] \
  "<(sed -n '21, 80p' ".libs[i]."_".m.ext[i].".dat)" using 3:4 with linespoints title libs[i]."-".m noenhanced

#pause -1
#    EOF
