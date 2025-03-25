LDFLAGS=-lcurl
LD=g++
CXXFLAGS=-I ~/rapidjson/include

static-crawler: static-crawler.o
	$(LD) -o $@ $< $(LDFLAGS)

static-crawler.o : static-graph-crawler.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<
