**CMPT 431 E100**

Jacob He

**Project - Single Source Shortest Path**


For our implementation we used Dijkstra's shortest path algorithm to find the shortest path to every other vertex from a given source vertex. 
All edge weights are assumed to be 1, and the graph is directed. Serial implementation is done in `sssp_serial.cpp`, parallel in `sssp_parallel.cpp`,
and distributed in `sssp_distributed.cpp`. Each can be seperatelly compiled with make serial, make parallel, and make distributed. The input parameters are the path to an input graph file and the source vertex from which shortest paths will be found. These are set in the form 
--inputFile=(path to file) --sourceVertex=(unsigned integer source). By default when no parameters are given, inputFile is set to the included sample 
graph roadNet-CA and source vertex to 0. sssp_parallel has the additional input parameter nThreads to set the number of threads to execute in parallel.

The graph code used is from assignment 3 and can generate a new graph from a txt file. The txt file must have a list of edges, each on separate lines. Each line should be in the form (source) (destination). Alternatively 2 sample graphs are provided: `roadNet-CA` and `web-Google`, found in 
the `input_graphs` folder. These can be inputted as a parameter using `--inputFile=input_graphs/roadNet-CA`.

**Each version is run with the following commands:**

Serial: `./sssp_serial --inputFile --sourceVertex`

Parallel: `./sssp_parallel --inputFile --sourceVertex --nThreads`

Distributed: `mpirun -n (number of processes) ./sssp_distributed --inputFile --sourceVertex`


**Output:**

Once the algorithm terminates, all shortest paths from the source are printed. Afterwards the total time and time of each thread/process is printed.
