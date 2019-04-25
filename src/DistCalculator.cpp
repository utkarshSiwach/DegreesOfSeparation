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

// Function that uses vector instructions to search for a character symbol,
// in a file starting from *iter till *fileEnd.
// Parameters:
// char symbol          the character to search for
// const char* iter     the position to start searching the pattern from
// const char* fileEnd  file end position
// Returns:
// const char* iter     the position where symbol was found, file end otherwise
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

// Constructor for DistCalculator class. It uses memory mapping technique to read
// a text file and parse it into two pre-allocated vector-of-vectors acting as hash maps.
// This considerably reduces processing time by avoiding data copy and too many memory allocations.
// Parameters:
// string edgeListFile      the file name containing the acto movie mappings
DistCalculator::DistCalculator(std::string edgeListFile) {
    // implemented graph parsing here in the constructor
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
    unsigned actorNum,movieNum;
    
    for(auto iter = cLine; iter < fileEnd;) {
        auto comma = findSymbol<','>(iter, fileEnd);
        if(comma>=fileEnd) break;
        actorNum=0;
        for(;iter!=comma;iter++)
            actorNum = actorNum*10 + (*iter)-'0';
        
        movieNum = 0;
        iter=comma+1;
        comma = findSymbol<'\n'>(iter,fileEnd);
        for(;iter!=comma;iter++)
            movieNum = movieNum*10 + (*iter)-'0';
        
        iter++;
        
        // start filling out 2 tables, one for actors->movies, another for movies->actors
        actors[actorNum].push_back(movieNum);
        movies[movieNum].push_back(actorNum);
        
    }
}

// Function to calculate the Kevin Bacon distance between two actors.
// The function uses bidirectional breadth first search algorithm with parallelization
// to speed up search when queue size crosses a certain threshold.
// Parameters:
// unsigned a       Actor 1
// unsigned b       Actor 2
// Returns
// int64_t          The Kevin Bacon distance if valid, else -1 if two actors are not related
//                  or not found.
int64_t DistCalculator::dist(unsigned a, unsigned b){
    // implemented distance calculation here
    
    //boundary conditions checked
    if(a==b)
        return 0;
    
    if(actors[b][0]==0)
        return -1;
    if(actors[a][0]==0)
        return -1;
    
    vector<thread> tPool1,tPool2;
    vector<uint64_t> isDiscovered;
    vector<uint64_t> isDiscovered2;
    
    //reserve required space so that memory allocation happens only once
    isDiscovered.resize(1<<15);
    isDiscovered2.resize(1<<15);
    std::vector<unsigned> queue;    // queue for one end of BFS parsing
    std::vector<unsigned> queue2;   // queue for second end of BFS parsing
    queue.resize(NUM_ACTORS);
    queue2.resize(NUM_ACTORS);
    
    unsigned currPos = 0;
    unsigned qEnd=0;
    queue[qEnd]=a;
    qEnd++;
    queue[qEnd]=NUM_ACTORS;
    isDiscovered[(a)>>6] |= 1<<(a)&63;
    unsigned numOfZeroes = 1;       // distance traversed from starting point 1
    
    unsigned currPos2 = 0;
    unsigned qEnd2=0;
    queue2[qEnd2]=b;
    qEnd2++;
    queue2[qEnd2]=NUM_ACTORS;
    isDiscovered2[(b)>>6] |= 1<<(b)&63;
    unsigned numOfZeroes2 = 1;       // distance traversed from starting point 2
    
    bool found=false;
    bool isFirst=true;
    bool tResult=false;
    bool isSafe;
    unsigned actor,movie,i,j,act_mov_size,mainActor,mov_act_size,diff,start,ending,iThread,nActiveThreads=0;
    uint64_t rem,quo,t,one=1;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    do{
        if(currPos!=0 && queue[currPos-1]==NUM_ACTORS && qEnd>2000){
            diff=(qEnd-currPos-2)/4;
            start=currPos;
            nActiveThreads=4;
            tResult=false;
            isSafe=false;
            for(unsigned i=0;i<4;i++){
                start=start+i*diff;
                ending=start+diff;
                
                auto& tmp = actors;
                auto& tmp2=movies;
                
                tPool1.push_back(thread([& queue,start,ending,b,&tmp,&tmp2,& tResult,& mutex,& nActiveThreads](){
                    
                    unsigned mainActor,movie,i,j,k,act_mov_size,mov_act_size;
                    for(k=start;k<ending;k++) {
                        mainActor = queue[k];
                        act_mov_size = tmp[mainActor].size(); // number of movies of mainActor
                        for(i=0;i<act_mov_size;i++) {   // for all movies of mainActor
                            movie=tmp[mainActor][i];
                            mov_act_size=tmp2[movie].size();
                            for(j=0;j<mov_act_size;j++)
                                if(tmp2[movie][j] == b){
                                    tResult |= true;
                                    pthread_mutex_lock(&mutex);
                                    nActiveThreads--;
                                    pthread_mutex_unlock(&mutex);
                                    return true;
                                }
                        }
                    }
                    pthread_mutex_lock(&mutex);
                    nActiveThreads--;
                    pthread_mutex_unlock(&mutex);
                    return false;
                }));
                 
            }
        }
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
                t=one<<rem;
                if( (isDiscovered[quo] & t) == 0){
                    isDiscovered[quo] |= t;
                    qEnd++;
                    queue[qEnd]=actor;
                }
            }
        }
    aa:
        if(found) {
            pthread_mutex_lock(&mutex);
            if(nActiveThreads!=0)
                for(iThread=0;iThread<tPool1.size();iThread++)
                    tPool1[iThread].join();
            pthread_mutex_unlock(&mutex);
            return numOfZeroes;
        }
        
        pthread_mutex_lock(&mutex);
        if(nActiveThreads==0)isSafe=true;
        pthread_mutex_unlock(&mutex);
        if(isSafe) {
            if(tResult) return numOfZeroes+1;
            isSafe=false;
        }
        
        if(queue[currPos+1]==NUM_ACTORS && qEnd>currPos+1) {
            qEnd++;
            queue[qEnd]=NUM_ACTORS;
            numOfZeroes++;
            
            for(int i=qEnd-1;(queue[i]!=NUM_ACTORS) && i>=0;i--){
                quo=(queue[i])>>6;
                rem= queue[i]&63;
                if( (isDiscovered2[quo] & one<<rem) != 0){
                    pthread_mutex_lock(&mutex);
                    if(nActiveThreads!=0)
                        for(iThread=0;iThread<tPool1.size();iThread++)
                            tPool1[iThread].join();
                    pthread_mutex_unlock(&mutex);
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
        if(currPos2!=0 && queue2[currPos2-1]==NUM_ACTORS && qEnd2>2000){
            diff=(qEnd2-currPos2-2)/12;
            start=currPos2;
            tResult=false;
            isSafe=false;
            nActiveThreads=12;
            for(unsigned i=0;i<12;i++){
                start=start+i*diff;
                ending=start+diff;
                
                auto& tmp = actors;
                auto& tmp2=movies;
                
                tPool2.push_back(thread([& queue2,start,ending,a,&tmp,&tmp2,& tResult,& mutex,& nActiveThreads](){
                    
                    unsigned mainActor,movie,i,j,k,act_mov_size,mov_act_size;
                    for(k=start;k<ending;k++) {
                        mainActor = queue2[k];
                        act_mov_size = tmp[mainActor].size(); // number of movies of mainActor
                        for(i=0;i<act_mov_size;i++) {   // for all movies of mainActor
                            movie=tmp[mainActor][i];
                            mov_act_size=tmp2[movie].size();
                            for(j=0;j<mov_act_size;j++)
                                if(tmp2[movie][j] == a){
                                    tResult |= true;
                                    pthread_mutex_lock(&mutex);
                                    nActiveThreads--;
                                    pthread_mutex_unlock(&mutex);
                                    return true;
                                }
                        }
                    }
                    pthread_mutex_lock(&mutex);
                    nActiveThreads--;
                    pthread_mutex_unlock(&mutex);
                    return false;
                }));
                
            }
        }
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
                t=one<<rem;
                if( (isDiscovered2[quo] & t) == 0){
                    isDiscovered2[quo] |= t;
                    qEnd2++;
                    queue2[qEnd2]=actor;
                }
            }
        }
    cc:
        if(found) {
            pthread_mutex_lock(&mutex);
            if(nActiveThreads!=0)
                for(iThread=0;iThread<tPool2.size();iThread++)
                    tPool2[iThread].join();
            pthread_mutex_unlock(&mutex);
            return numOfZeroes2;
        }
        pthread_mutex_lock(&mutex);
        if(nActiveThreads==0)isSafe=true;
        pthread_mutex_unlock(&mutex);
        if(isSafe) {
            if(tResult) return numOfZeroes2+1;
            isSafe=false;
        }
        if(queue2[currPos2+1]==NUM_ACTORS && qEnd2>currPos2+1) {
            qEnd2++;
            queue2[qEnd2]=NUM_ACTORS;
            numOfZeroes2++;
            
            for(int i=qEnd2-1;(queue2[i]!=NUM_ACTORS) && i>=0;i--){
                quo=(queue2[i])>>6;
                rem= queue2[i]&63;
                if( (isDiscovered[quo] & one<<rem) != 0){
                    pthread_mutex_lock(&mutex);
                    if(nActiveThreads!=0)
                        for(iThread=0;iThread<tPool2.size();iThread++)
                            tPool2[iThread].join();
                    pthread_mutex_unlock(&mutex);
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

