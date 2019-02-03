#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class DistCalculator{
public:
    static const int NUM_ACTORS = 2000000;
    static const int NUM_ACTOR_MOV = 256;
    static const int NUM_MOVIES = 1200000;
    static const int NUM_MOVIES_ACT = 256;
  std::vector<std::vector<unsigned>> actors;
  std::vector<std::vector<unsigned>> movies;
  using Node = uint64_t;
  DistCalculator(std::string edgeListFile);
  int64_t dist(unsigned, unsigned);
};

