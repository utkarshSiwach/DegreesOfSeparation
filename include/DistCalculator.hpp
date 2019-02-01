#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class DistCalculator{
public:
  std::unordered_map<unsigned, std::unordered_set<unsigned>> actors;
  std::unordered_map<unsigned, std::unordered_set<unsigned>> movies;
  using Node = uint64_t;
  DistCalculator(std::string edgeListFile);
  int64_t dist(unsigned, unsigned);
    bool findNeighbors(unsigned,unsigned, std::unordered_set<unsigned>&, std::vector<unsigned>&);
};
