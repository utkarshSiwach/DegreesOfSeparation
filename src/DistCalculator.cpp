#include "DistCalculator.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <emmintrin.h>

using namespace std;

template <char symbol>
static inline const char* findSymbol(const char* iter,const char* fileEnd){
    auto safeFileEnd = fileEnd - 16;
    auto pattern = _mm_set1_epi8(symbol);
    for (;iter<safeFileEnd;iter+=16) {
        auto block=_mm_loadu_si128(reinterpret_cast<const __m128i*>(iter));
        auto matchVec=_mm_cmpeq_epi8(block,pattern);
        auto matches=_mm_movemask_epi8(matchVec);
        
        if (matches)
            return iter + (__builtin_ctzll(matches));
    }
    while ((iter<fileEnd)&&((*iter)!=symbol)) ++iter;
    return iter;
}

DistCalculator::DistCalculator(std::string edgeListFile) {
    // TODO: implement graph parsing here
    long filePtr;
    
    filePtr = open(edgeListFile.c_str(),O_RDONLY);
    if(filePtr<0){
        cerr<< "Unable to open file";
    }
    
    long fileSize = lseek(filePtr, 0, SEEK_END);
    auto fileBegin = static_cast <const char*>(mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, filePtr, 0));
    auto fileEnd = fileBegin + fileSize;
    
    auto cLine = findSymbol<'\n'>(fileBegin, fileEnd)+1;    // skip the header
    
    unsigned maxM=0,maxA=0,temp;
    actors.rehash(sizeof(unsigned)*2000000);
    movies.rehash(sizeof(unsigned)*1200000);
    for(auto iter = cLine; iter < fileEnd;) {
        auto comma = findSymbol<','>(iter, fileEnd);
        if(comma>=fileEnd) break;
        unsigned actorNum = 0;
        for(;iter!=comma;iter++)
            actorNum = actorNum*10 + (*iter)-'0';
        
        unsigned movieNum = 0;
        iter=comma+1;
        comma = findSymbol<'\n'>(iter,fileEnd);
        for(;iter!=comma;iter++)
            movieNum = movieNum*10 + (*iter)-'0';
        
        iter++;
        
        // start filling out 2 hash tables, one for actors->movies, another for movies->actors
        
        auto actorIter = actors.find(actorNum);
        if(actorIter!=actors.end()) {
            actorIter->second.insert(movieNum);
            temp = actorIter->second.size();
            if(temp>maxM) maxM = temp;
        }
        else {
            unordered_set<unsigned> tempMovies;
            tempMovies.insert(movieNum);
            actors.insert({actorNum,tempMovies});
        }
        
        auto movieIter = movies.find(movieNum);
        if(movieIter!=movies.end()) {
            movieIter->second.insert(actorNum);
            temp = movieIter->second.size();
            if(temp>maxA) maxA = temp;
        }
        else {
            unordered_set<unsigned> tempActors;
            tempActors.insert(actorNum);
            movies.insert({movieNum,tempActors});
        }
    }
    //cout<<"maxM "<<maxM<<", maxA "<<maxA<<", sizeA "<<actors.size()<<", sizeM "<<movies.size();
}

int64_t DistCalculator::dist(unsigned a, unsigned b)
{
   // TODO: implement distance calculation here
    if(a==b)
        return 0;
    
    auto aPos = actors.find(b);
    if(aPos==actors.end())
        return -1;
    aPos = actors.find(a);
    if(aPos==actors.end())
        return -1;
    vector<unsigned> queue;
    unordered_set<unsigned> q2;
    queue.reserve(sizeof(unsigned)*2000000);
    q2.reserve(sizeof(unsigned)*2000000);
    unsigned currPos = 0;
    q2.insert(a);
    queue.push_back(a);
    queue.push_back(0);
    unsigned numOfZeroes = 1;
    bool found=false;
    do{
        found = findNeighbors(queue.at(currPos),b, q2, queue);
        if(found) {
            return numOfZeroes;
        }
        if(queue.at(currPos+1)==0 && queue.size()>currPos+2) {
            queue.push_back(0);
            numOfZeroes++;
        }
        if(queue.at(currPos+1)==0) currPos+=2;
        else currPos++;
    }while(currPos<queue.size());
    
    return -1;
}

bool DistCalculator::findNeighbors(unsigned a, unsigned target, unordered_set<unsigned>& q2,vector<unsigned>& queue) {
    auto aPos = actors.find(a);
    unordered_set<unsigned> allNeighbors;
    for(auto aMovie = aPos->second.begin();aMovie!=aPos->second.end();aMovie++) {
        auto actorSet=movies.find(*aMovie)->second;
        for(auto actor=actorSet.begin();actor!=actorSet.end();actor++){
            if(*actor == target)
                return true;
            if(q2.find(*actor)==q2.end()){
                q2.insert(*actor);
                queue.push_back(*actor);
            }
        }
    }
    return false;
}
