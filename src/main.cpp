#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "DistCalculator.hpp"

//---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
   using namespace std;
   if (argc != 2) {
      cout << "Usage: " << argv[0] << " <playedin.csv>";
      exit(1);
   }

   string playedinFile(argv[1]);
   //string playedinFile("/Users/utkarshsiwach/Documents/fde/fde18-bonusproject3/test/SimpleGraph.txt");
   //string playedinFile("/Users/utkarshsiwach/Downloads/playedin.csv");
    // Create dist calculator
   DistCalculator dc(playedinFile);

   // read queries from standard in and return distances
   //DistCalculator::Node a, b;
    unsigned a,b;
    //cout<<"\nEnter 2 nums:";
    while (cin >> a && cin >> b) cout << dc.dist(a, b) << "\n";

   // flush output buffer
   cout.flush();
}
//---------------------------------------------------------------------------
