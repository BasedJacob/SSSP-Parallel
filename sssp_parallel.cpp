//CMPT 431 FINAL PROJECT
//Jacob He and Timofey Lykov
//SSSP Parallel Version

//To run with Threads=3, SourceVertex=default, Graph=default:   
//           ./sssp_parallel --nThreads 3 

//To run with Threads=3, SourceVertex=2, Graph=default:   
//           ./sssp_parallel --nThreads 3 --sourceVertex 2

//To run with Threads=3, SourceVertex=2, Graph=Roadnet: 
//          ./sssp_parallel --nThreads 3 --sourceVertex 2 --inputFile input_graphs/roadNet-CA

#include "core/graph.h"
#include "core/utils.h"
#include "pthread.h"
#include <iostream>
#include <stdlib.h>
#include <string>
//#include <filesystem>
#include <queue>

//namespace fs = std::__fs::filesystem;
Graph g;
std::vector<int> distance;
std::vector<int> parent;
std::vector<bool> relaxed;
uintV num_vertices;
pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
double timetakenarr[100];

//data structure to pass into thread
class ThreadDetail {
    public:
    int ThreadID;
};

//helper function to compare distances.
struct minDistance {
    bool operator() (uintV a, uintV b) {
        return distance[a]>distance[b];
    }
};

std::priority_queue<uintV, std::vector<uintV>, minDistance> minQ;


//function for thread
void* threadfunc(void* ptr){
    ThreadDetail* threadinfo = (ThreadDetail*)ptr;
    int myID = threadinfo->ThreadID;
    timer t1;
    double time_taken = 0.0;
    t1.start();
    
    //run while the stack has vertexes in it
    while(!minQ.empty()) {
        //Mutex lock to protect critical section 
        //
        pthread_mutex_lock(&mymutex);
        uintV v = minQ.top();
        //check the size of stack within the mutex to ensure it's actually empty
        if(minQ.size() <= 0){
            pthread_mutex_unlock(&mymutex);
            return NULL;
        }
        minQ.pop();
        relaxed[v] = true;
        pthread_mutex_unlock(&mymutex);
        //Get the out degrees of the vertex
        uintE out_degree = g.vertices_[v].getOutDegree();

        for(uintE i = 0; i < out_degree; i++) {
            uintV u = g.vertices_[v].getOutNeighbor(i);
            //Distance comparison
            if(!relaxed[u] && ((distance[v] + 1) < distance[u])) {
                //Update new shortest distance if it's shorter
                distance[u] = distance[v] + 1;
                parent[u] = v;
                //Protect the stack - ensure nothing new gets pushed while other threads take minQ.top and minQ.pop
                pthread_mutex_lock(&mymutex);
                minQ.push(u);
                pthread_mutex_unlock(&mymutex);
            }
        }
        
    }

    time_taken=t1.stop();
    timetakenarr[myID] = time_taken;
}

void sssp_parallel_initialization(Graph &g, uintV source, int n_threads) {
    uintV num_vertices = g.n_;
    distance = std::vector<int>(num_vertices,INT_MAX);
    parent = std::vector<int>(num_vertices,-1);
    relaxed = std::vector<bool>(num_vertices,false);
    //std::priority_queue<uintV, std::vector<uintV>, minDistance> minQ;

    distance[source] = 0;
    relaxed[source] = true;
    minQ.push(source);

    pthread_t *pthread_arr;
    pthread_arr = new pthread_t[n_threads];
    ThreadDetail *DetailsArr;
    DetailsArr = new ThreadDetail[n_threads];

    timer t2;
    double time_taken_total = 0.0;
    t2.start();

    for(int i = 0; i < n_threads; i++){
        DetailsArr[i].ThreadID = i;
        pthread_create(&pthread_arr[i], NULL, threadfunc, (void*)&DetailsArr[i]);
    }

    void* retval = 0;
    for(int i = 0 ; i < n_threads; i++){
        pthread_join(pthread_arr[i], &retval);
    }

    time_taken_total=t2.stop();

    for (uintV i = 0; i < num_vertices; i++) {
        std::cout << "Shortest path from " << source << "->" << i << ": ";
        if(distance[i]==INT_MAX) {
            std::cout << "INF\n";
        } else {
            std::cout << distance[i] << std::endl;
        }
    }

    for(int i = 0; i < n_threads; i++){
        std::cout<<"Thread "<<i << " time taken: "<<timetakenarr[i]<<"\n";
    }

    std::cout<<"Total time taken: "<<time_taken_total<<"\n";
}

int main(int argc, char *argv[]) {
    cxxopts::Options options(
            "sssp_serial",
            "Calculate single source shortest path");
    options.add_options(
            "",
            {
                    {"nThreads", "Number of Threads",
                            cxxopts::value<uint>()->default_value(DEFAULT_NUMBER_OF_THREADS)},
                    {"sourceVertex", "Index of source vertex",
                            cxxopts::value<uintV>()->default_value(DEFAULT_SOURCE_INDEX)},
                    {"inputFile", "Input graph file path",
                            cxxopts::value<std::string>()->default_value(
                                    "input_graphs/roadNet-CA")},
            });

    auto cl_options = options.parse(argc, argv);
    uint n_threads = cl_options["nThreads"].as<uint>();
    uintV source = cl_options["sourceVertex"].as<uintV>();
    std::string input_file_path = cl_options["inputFile"].as<std::string>();

    std::cout << "Number of Threads : " << n_threads << std::endl;


    std::cout << "Reading graph\n";
    g.readGraphFromBinary<int>(input_file_path);
    std::cout << "Created graph\n";

    if(source >= g.n_) {
        std::cout << "Source index given is not in the graph\nUsing default source: "
            << DEFAULT_SOURCE_INDEX << " instead...\n";
        source = (uintV)atoi(DEFAULT_SOURCE_INDEX);
    }

    sssp_parallel_initialization(g,source, n_threads);

    return 0;
}
