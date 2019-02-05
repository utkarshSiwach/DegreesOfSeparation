#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "DistCalculator.hpp"

//---------------------------------------------------------------------------
void thread_task( DistCalculator &dc){
    dc.dist(1,6);
}

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
    unsigned c=0;
    int res[50];
    vector<thread> allThreads;
    //dc.dist(1, 6);
    
    while (cin >> a && cin >> b) {
    
        allThreads.push_back(
                             thread([&res,a,b,&dc,c](){
            res[c]=dc.dist(a, b);
        })
                             );
        c++;
    }
    /*
    DistCalculator dc1(dc.actors,dc.movies);
    DistCalculator dc2(dc.actors,dc.movies);
    allThreads.push_back(thread(thread_task,ref(dc1)));
    allThreads.push_back(thread(thread_task,ref(dc2)));
    */
     for(auto &t: allThreads) t.join();
    for(unsigned i=0;i<c;i++) cout<<res[i]<<'\n';
    // flush output buffer
    cout.flush();
}
//---------------------------------------------------------------------------
