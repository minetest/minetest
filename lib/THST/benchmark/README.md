
## Installation

### Requirements
* C++11 compiler
* CMake
* Boost headers 
* gnuplot

### Instructions

```bash
cmake .. -G Xcode -DBGI_ENABLE_CT=ON -DCMAKE_BUILD_TYPE=Release
make -j 8
./run_benchmark.sh ./build/Release/ <your_results_folder>

# the benchmark script will also run the gnuplot command:
gnuplot ./plot_results.plt
```

## Benchmarks

Benchmark setup is based on [spatial_index_benchmark](https://github.com/mloskot/spatial_index_benchmark) by Mateusz Loskot and Adam Wulkiewicz.

### Results

HW: Intel(R) Core(TM) i7-4870HQ CPU @ 2.50GHz, 16 GB RAM; OS: macOS Sierra 10.12.16

* Loading times for each of the R-tree construction methods

![load thst_vs_bgi](results/benchmark_load_bgi_vs_thst.png)

Best loading performance is given by the bulk loading algorithm, followed by linear:
![load boost::geometry](results/bgi_benchmark_rtree_load_itr_vs_blk.png)

As expected, the heurestic type doesn't affect bulk loading performance:
![load balance boost::geometry](results/bgi_benchmark_rtree_load_blk_vs_balancing.png)

Creating the quadtree is very slow for capacities between 256 and 1024, while the custom quadratic variant produces the best performance:
![load thst](results/thst_benchmark_load_itr.png)

* Query times for each of the R-tree construction methods

![query thst_vs_bgi](results/benchmark_query_bgi_vs_thst.png)

Best query times, rstar and bulk loading produce best times and followed by the thst's quadratic custom:
![query thst_vs_bgi](results/benchmark_query_bgi_vs_thst_best.png)

Bulk loading produces the best query times from all heurestics:
![query boost::geometry](results/bgi_benchmark_rtree_query_itr_vs_blk.png)

As expected, the heurestic type doesn't affect query performance of the bulk loading algorithm:
![query balance boost::geometry](results/bgi_benchmark_rtree_query_blk_vs_balancing.png)

Query times for the quadtree are really slow:
![query thst](results/thst_benchmark_query_itr.png)

The custom allocator is slightly faster than the normal one:
![query thst](results/thst_benchmark_query_cst.png)

* Dynamic use case, average time for each of the R-tree construction methods

![dynamic thst_vs_bgi](results/benchmark_dynamic_bgi_vs_thst.png)

At around 400 objects the thst quadratic method is faster than the bgi linear version:
![dynamic2 thst_vs_bgi](results/benchmark_dynamic_small_bgi_vs_thst.png)

Under 400 objects bgi linear is slightly faster, however this heavily depends on how many objects pass the query test, in the given test case almost all objects pass the intersection test.
![dynamic2 thst_vs_bgi](results/benchmark_dynamic_vsmall_bgi_vs_thst.png)

### Legend
------

* ```bgi``` - boost::geometry::index, compile time
* ```thst``` - thst
* ```ct``` - compile-time specification of rtree parameters
* ```rt``` (or non suffix) - Boost.Geometry-only, run-time specification of rtree parameters
* ```L``` - linear
* ```Q``` - quadratic
* ```QT``` - quadtree
* ```R``` - rstar
* ```itr (or no suffix)```  - iterative insertion method of building rtree
* ```blk```  - bulk loading method of building R-tree (custom algorithm for ```bgi```)
* ```custom``` - custom allocator variant for thst(cache friendly, linear memory)
* ```sphere``` - sphere volume for computing the boxes's volume, better splitting but costlier
* insert 1000000 - number of objects small random boxes
* query   100000 - number of instersection-based queries with random boxes 10x larger than those inserted
* dynamic 200 - number of runs composed of clear, instersection-based queries and insert with small random boxes
