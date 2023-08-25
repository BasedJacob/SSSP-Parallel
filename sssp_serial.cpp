#include "core/graph.h"
#include "core/utils.h"
#include <iostream>
#include <stdlib.h>
#include <string>
#include <queue>
#include <assert.h>

std::vector<int> distance;  // distance from source to each vertex
std::vector<int> parent;    // stores previous vertex along shortest path
std::vector<bool> relaxed;  // flag for vertices that have been processed

// custom compare function for priority queue
struct minDistance {
    bool operator() (uintV a, uintV b) {
        return distance[a] > distance[b];
    }
};

// single source shortest path using serial implementation
// uses djikstras shortest path algorithm
void sssp_serial(Graph &g, uintV source) {
    timer timer;
    uintV num_vertices = g.n_;
    distance = std::vector<int>(num_vertices,INT_MAX);
    parent = std::vector<int>(num_vertices,-1);
    relaxed = std::vector<bool>(num_vertices,false);
    std::priority_queue<uintV, std::vector<uintV>, minDistance> minQ;

    distance[source] = 0;
    relaxed[source] = true;
    minQ.push(source);

    timer.start();
    while(!minQ.empty()) {
        uintV v = minQ.top();
        minQ.pop();
        uintE out_degree = g.vertices_[v].getOutDegree();
        for(uintE i = 0; i < out_degree; i++) {
            uintV u = g.vertices_[v].getOutNeighbor(i);
            if(!relaxed[u] && (distance[v] + 1) < distance[u]) {
                distance[u] = distance[v] + 1;
                parent[u] = v;
                minQ.push(u);
            }
        }
        relaxed[v] = true;
    }
    double time_taken = timer.stop();

    for (uintV i = 0; i < num_vertices; i++) {
        std::cout << "Shortest path from " << source << "->" << i << ": ";
        if(distance[i]==INT_MAX) {
            std::cout << "UNREACHABLE\n";
        } else {
            std::cout << distance[i] << std::endl;
        }
    }

    printf("\nTotal time taken: %.4lf\n",time_taken);
}

int main(int argc, char *argv[]) {
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

    std::cout << "Number of Threads : 1" << std::endl;

    Graph g;
    std::cout << "Reading graph\n";
    g.readGraphFromBinary<int>(input_file_path);
    std::cout << "Created graph\n";

    if(source >= g.n_) {
        std::cout << "Source index given is not in the graph\nUsing default source: "
            << DEFAULT_SOURCE_INDEX << " instead...\n";
        source = (uintV)atoi(DEFAULT_SOURCE_INDEX);
    }

    sssp_serial(g,source);

    return 0;
}
