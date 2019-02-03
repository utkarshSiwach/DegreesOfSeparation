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
    
    actors.resize(NUM_ACTORS);
    movies.resize(NUM_MOVIES);
    
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
        
        // start filling out 2 tables, one for actors->movies, another for movies->actors
        actors[actorNum].push_back(movieNum);
        movies[movieNum].push_back(actorNum);
        
    }
    //cout<<"maxM "<<maxM<<", maxA "<<maxA<<", sizeA "<<actors.size()<<", sizeM "<<movies.size();
}

int64_t DistCalculator::dist(unsigned a, unsigned b)
{
   // TODO: implement distance calculation here
    if(a==b)
        return 0;
    
    if(actors[b][0]==0)
        return -1;
    if(actors[a][0]==0)
        return -1;
    
    uint64_t isDiscovered[1<<15];
    
    unsigned queue[NUM_ACTORS*2];
    unsigned currPos = 0;
    unsigned queueSize=0;
    queue[queueSize]=a;
    queueSize++;
    queue[queueSize]=NUM_ACTORS;
    queueSize++;
    unsigned numOfZeroes = 1;
    bool found=false;
    //vector<unsigned>::iterator actor,aMovie;
    do{
        for(auto aMovie=actors[queue[currPos]].begin();aMovie!=actors[queue[currPos]].end();aMovie++) {
            for(auto actor=movies[*aMovie].begin();actor!=movies[*aMovie].end();actor++){
                if(*actor == b) {
                    found = true;
                    goto aa;
                }
                if((isDiscovered[(*actor)>>6] & 1<<(*actor)&63) == 0){
                    isDiscovered[(*actor)>>6] |= 1<<(*actor)&63;
                    queue[queueSize]= *actor;
                    queueSize++;
                }
            }
        }
    aa:
        if(found) {
            return numOfZeroes;
        }
        if(queue[currPos+1]==NUM_ACTORS && queueSize>currPos+2) {
            queue[queueSize]=NUM_ACTORS;
            queueSize++;
            numOfZeroes++;
        }
        if(queue[currPos+1]==NUM_ACTORS) currPos+=2;
        else currPos++;
    }while(currPos<queueSize);
    
    return -1;
}
