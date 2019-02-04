#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class DistCalculator{
public:
    const unsigned NUM_ACTORS = 2000000;
    const unsigned NUM_ACTOR_MOV = 256;
    const unsigned NUM_MOVIES = 1200000;
    const unsigned NUM_MOVIES_ACT = 256;
  std::vector<std::vector<unsigned>> actors;
  std::vector<std::vector<unsigned>> movies;
  DistCalculator(std::string edgeListFile);
  DistCalculator(std::vector<std::vector<unsigned>> & actors, std::vector<std::vector<unsigned>> &movies);
  int64_t dist(unsigned, unsigned);
    
};

