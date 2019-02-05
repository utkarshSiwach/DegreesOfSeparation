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

DistCalculator::DistCalculator(std::vector<std::vector<unsigned>> & actors, std::vector<std::vector<unsigned>> & movies) {
    this->actors = actors;
    this->movies = movies;
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
    
    vector<uint64_t> isDiscovered;
    vector<uint64_t> isDiscovered2;
    isDiscovered.resize(1<<15);
    isDiscovered2.resize(1<<15);
    std::vector<unsigned> queue;
    std::vector<unsigned> queue2;
    queue.resize(NUM_ACTORS);
    queue2.resize(NUM_ACTORS);
    unsigned currPos = 0;
    unsigned qEnd=0;
    queue[qEnd]=a;
    qEnd++;
    queue[qEnd]=NUM_ACTORS;
    isDiscovered[(a)>>6] |= 1<<(a)&63;
    unsigned numOfZeroes = 1;
    
    unsigned currPos2 = 0;
    unsigned qEnd2=0;
    queue2[qEnd2]=b;
    qEnd2++;
    queue2[qEnd2]=NUM_ACTORS;
    isDiscovered2[(b)>>6] |= 1<<(b)&63;
    unsigned numOfZeroes2 = 1;
    
    bool found=false;
    bool isFirst=true;
    unsigned actor,movie,i,j,act_mov_size,mainActor,mov_act_size;
    uint64_t rem,quo;
    
    do{
        mainActor = queue[currPos];
        act_mov_size = actors[mainActor].size(); // number of movies of mainActor
        for(i=0;i<act_mov_size;i++) {   // for all movies of mainActor
            movie=actors[mainActor][i];
            mov_act_size=movies[movie].size();
            for(j=0;j<mov_act_size;j++){
                actor=movies[movie][j];
                if(actor == b) {
                    found = true;
                    goto aa;
                }
                quo=(actor)>>6;
                rem= actor&63;
                if( (isDiscovered[quo] & (1<<rem)) == 0){
                    isDiscovered[quo] |= (1<<rem);
                    qEnd++;
                    queue[qEnd]=actor;
                }
            }
        }
    aa:
        if(found) {
            return numOfZeroes;
        }
        
        if(queue[currPos+1]==NUM_ACTORS && qEnd>currPos+1) {
            qEnd++;
            queue[qEnd]=NUM_ACTORS;
            numOfZeroes++;
            
            for(int i=qEnd-1;(queue[i]!=NUM_ACTORS) && i>=0;i--){
                quo=(queue[i])>>6;
                rem= queue[i]&63;
                if( (isDiscovered2[quo] & (1<<rem)) != 0){
                    return numOfZeroes+numOfZeroes2-2;
                }
            }
            if(isFirst) { isFirst=false;goto switchTo2;} else goto switchTo2a;
        }
    switchTo1:
        if(queue[currPos+1]==NUM_ACTORS) currPos+=2;
        else currPos++;
    }while(currPos<qEnd);
    
    return -1;
    
switchTo2:

    do{
        mainActor = queue2[currPos2];
        act_mov_size = actors[mainActor].size(); // number of movies of mainActor
        for(i=0;i<act_mov_size;i++) {   // for all movies of mainActor
            movie=actors[mainActor][i];
            mov_act_size=movies[movie].size();
            for(j=0;j<mov_act_size;j++){
                actor=movies[movie][j];
                if(actor == a) {
                    found = true;
                    goto cc;
                }
                quo=(actor)>>6;
                rem= actor&63;
                if( (isDiscovered2[quo] & (1<<rem)) == 0){
                    isDiscovered2[quo] |= (1<<rem);
                    qEnd2++;
                    queue2[qEnd2]=actor;
                }
            }
        }
    cc:
        if(found) {
            return numOfZeroes2;
        }
        
        if(queue2[currPos2+1]==NUM_ACTORS && qEnd2>currPos2+1) {
            qEnd2++;
            queue2[qEnd2]=NUM_ACTORS;
            numOfZeroes2++;
            
                for(int i=qEnd2-1;(queue2[i]!=NUM_ACTORS) && i>=0;i--){
                    quo=(queue2[i])>>6;
                    rem= queue2[i]&63;
                    if( (isDiscovered[quo] & (1<<rem)) != 0){
                        return numOfZeroes+numOfZeroes2-2;
                    }
                }
            
                goto switchTo1;
            
        }
    switchTo2a:
        if(queue2[currPos2+1]==NUM_ACTORS) currPos2+=2;
        else currPos2++;
    }while(currPos2<qEnd2);
    
    return -1;
    
}

