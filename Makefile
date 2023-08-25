CXX = g++
MPICXX = mpic++
CXXFLAGS = -std=c++17 -O3 -pthread

COMMON= core/utils.h core/cxxopts.h core/get_time.h core/graph.h core/quick_sort.h
SERIAL= sssp_serial
PARALLEL= sssp_parallel
DISTRIBUTED= sssp_distributed
ALL= $(SERIAL) $(DISTRIBUTED) $(PARALLEL)


all : $(ALL)
serial : $(SERIAL)
parallel : $(PARALLEL)
distributed : $(DISTRIBUTED)

$(SERIAL): % : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(DISTRIBUTED): %: %.cpp
	$(MPICXX) $(CXXFLAGS) -o $@ $<

.PHONY : clean

clean :
	rm -f *.o *.obj $(ALL)
