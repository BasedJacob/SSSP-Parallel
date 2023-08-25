#include "core/graph.h"
#include "core/utils.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <queue>
#include <mpi.h>
#include <unistd.h>

int world_rank;     // process id
int world_size;     // number of processes
int num_vertices;   // total vertices in graph
int root;           // process to which source vertex belongs

std::vector<int> distance;  // distance from source to each vertex
std::vector<bool> relaxed;  // flag for vertices that have been processed

// stores a vertex and its distance from source
class node {
public:
    int ver;
    int dist;

    node(int v, int d) {
        ver = v;
        dist = d;
    }
};

// custom compare function for priority queue minQ
struct minDistance {
    bool operator() (node* a, node* b) {
        return a->dist > b->dist;
    }
};

// custom compare function for priority queue when using 1 process
struct minDistanceSerial {
    bool operator() (uintV a, uintV b) {
        return distance[a] > distance[b];
    }
};

// queue of vertices sorted by distance from source
// used for djikstra's algorithm
std::priority_queue<node*, std::vector<node*>, minDistance> minQ;

// function called by process that contains the last popped vertex from minQ
void root_func(Graph &g, uint v) {
    if(v==-1) return;
    int dist = distance[v/world_size];
    if(dist!=INT_MAX) dist+=1;
    uintE out_degree = g.vertices_[v].getOutDegree();
    for(uintE i = 0; i < out_degree; i++) {
        uint u = g.vertices_[v].getOutNeighbor(i);
        int pid = u%world_size;
        int push;
        if(pid==world_rank) {
            if (!relaxed[u/world_size] && dist < distance[u/world_size]) {
                distance[u/world_size] = dist;
                if(world_rank==root) {
                    minQ.push(new node((int)u,dist));
                } else {
                    push = u;
                    MPI_Send(&push,1,MPI_INT,root,0,MPI_COMM_WORLD);
                    MPI_Send(&dist,1,MPI_INT,root,0,MPI_COMM_WORLD);
                }
            } else if(world_rank!=root) {
                push = -1;
                MPI_Send(&push,1,MPI_INT,root,0,MPI_COMM_WORLD);
            }
        } else {
            MPI_Send(&dist,1,MPI_INT,pid,0,MPI_COMM_WORLD);
            if(world_rank==root) {
                MPI_Recv(&push,1,MPI_INT,pid,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                if(push!=-1) {
                    int d;
                    MPI_Recv(&d, 1, MPI_INT, pid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    minQ.push(new node(push,d));
                }
            }
        }
    }
    relaxed[v/world_size] = true;
}

// function called by all other processes that don't contain last popped vertex from minQ
void distr_func(Graph &g, uint v) {
    if(v==-1) return;
    uintE out_degree = g.vertices_[v].getOutDegree();
    for(uintE i = 0; i < out_degree; i++) {
        uint u = g.vertices_[v].getOutNeighbor(i);
        int pid = u % world_size;
        int push;
        if (pid == world_rank) {
            int dist;
            MPI_Recv(&dist, 1, MPI_INT, v % world_size, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (!relaxed[u / world_size] && dist < distance[u / world_size]) {
                distance[u / world_size] = dist;
                if (world_rank == root) {
                    minQ.push(new node((int)u,dist));
                } else {
                    push = u;
                    MPI_Send(&push, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
                    MPI_Send(&dist, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
                }
            } else if (world_rank != root) {
                push = -1;
                MPI_Send(&push, 1, MPI_INT, root, 0, MPI_COMM_WORLD);
            }
        } else if (world_rank == root) {
            MPI_Recv(&push, 1, MPI_INT, pid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (push!=-1) {
                int d;
                MPI_Recv(&d, 1, MPI_INT, pid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                minQ.push(new node(push,d));
            }
        }
    }
}

// single source shortest path using MPI
// uses variation of djikstras shortest path algorithm
void sssp_distributed(Graph &g, uintV source) {
    timer total_timer;
    total_timer.start();

    uintV num_per_process = num_vertices/world_size;
    uintV excess = num_vertices%world_size;
    if(world_rank < excess) {
        num_per_process++;
    }

    // processes store a portion of all distances
    // communicate when a distance stored in another process is needed
    distance = std::vector<int>(num_per_process,INT_MAX);
    relaxed = std::vector<bool>(num_per_process,false);

    root = source%world_size;   // root process contains source in its subset of vertices
    if(root == world_rank) {
        distance[source/world_size] = 0;
        relaxed[source/world_size] = true;
        if(world_size>1) minQ.push(new node(source,0));
        //printf("Root process: %d\n",root);
        //printf("Pushed source: %u into minQueue\n\n",source);
    }

    timer timer;
    timer.start();
    if(world_size>1) {
        int v = source;
        while (true) {
            if (world_rank == root) {
                if (minQ.empty()) {
                    v = -1;
                } else {
                    node *top = minQ.top();
                    v = top->ver;
                    minQ.pop();
                }
            }
            MPI_Bcast(&v, 1, MPI_UNSIGNED, root, MPI_COMM_WORLD);
            if (v == -1) break;

            if (v % world_size == world_rank) {
                root_func(g, v);
            } else {
                distr_func(g, v);
            }
        }
    } else if(world_size==1) {
        std::priority_queue<uintV, std::vector<uintV>, minDistanceSerial> minQ_Serial;
        minQ_Serial.push(source);

        while(!minQ_Serial.empty()) {
            uintV v = minQ_Serial.top();
            minQ_Serial.pop();
            uintE out_degree = g.vertices_[v].getOutDegree();
            for(uintE i = 0; i < out_degree; i++) {
                uintV u = g.vertices_[v].getOutNeighbor(i);
                if(!relaxed[u] && (distance[v] + 1) < distance[u]) {
                    distance[u] = distance[v] + 1;
                    minQ_Serial.push(u);
                }
            }
            relaxed[v] = true;
        }
    }
    double time_taken = timer.stop();

    if(world_rank==0) timer.start();
    for (uintV i = 0; i < num_vertices; i++) {
        if(i%world_size == world_rank) {
            if (distance[i/world_size] == INT_MAX) {
                printf("Shortest path from %u->%u: UNREACHABLE\n",source,i);
            } else {
                printf("Shortest path from %u->%u: %d\n",source,i,distance[i/world_size]);
            }

        }
    }
    double print_time;
    if(world_rank==0) print_time = timer.stop();

    if(world_size==1) {
        printf("\n%d, time taken:%.4lf\n", world_rank, time_taken);
        double total_time_taken = total_timer.stop();
        total_time_taken = total_time_taken - print_time;
        printf("Total time taken: %lf\n",total_time_taken);
        return;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    sleep(1);
    int tmp = 0;
    if(world_rank==0) {
        printf("\n%d, time taken:%.4lf\n", world_rank, time_taken);
    } else {
        MPI_Recv(&tmp, 1, MPI_INT, world_rank-1, world_rank-1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%d, time taken:%.4lf\n", world_rank, time_taken);
    }
    MPI_Send(&tmp, 1, MPI_INT, (world_rank+1)%world_size, world_rank, MPI_COMM_WORLD);

    if(world_rank==0) {
        MPI_Recv(&tmp, 1, MPI_INT, world_size-1, world_size-1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        double total_time_taken = total_timer.stop();
        total_time_taken = total_time_taken - print_time - 1;
        printf("Total time taken: %.4lf\n",total_time_taken);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    cxxopts::Options options(
            "sssp_serial",
            "Calculate single source shortest path");
    options.add_options(
            "",
            {
                    {"sourceVertex", "Index of source vertex",
                            cxxopts::value<uintV>()->default_value(DEFAULT_SOURCE_INDEX)},
                    {"inputFile", "Input graph file path",
                            cxxopts::value<std::string>()->default_value(
                                    "input_graphs/roadNet-CA")},
            });

    auto cl_options = options.parse(argc, argv);
    uintV source = cl_options["sourceVertex"].as<uintV>();
    std::string input_file_path = cl_options["inputFile"].as<std::string>();

    if(world_rank==0) std::cout << "Number of Processes : " << world_size << std::endl;

    Graph g;
    if(world_rank==0) std::cout << "Reading graph\n";
    g.readGraphFromBinary<int>(input_file_path);
    if(world_rank==0) std::cout << "Created graph\n";

    if(source >= g.n_) {
        if(world_rank==0) std::cout << "Source index given is not in the graph\nUsing default source: "
            << DEFAULT_SOURCE_INDEX << " instead...\n";
        source = (uintV)atoi(DEFAULT_SOURCE_INDEX);
    }

    num_vertices = g.n_;
    sssp_distributed(g,source);

    MPI_Finalize();
    return 0;
}
