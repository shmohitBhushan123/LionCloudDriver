////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_cache.c
//  Description    : This is the cache implementation for the LionCloud
//                   assignment for CMPSC311.
//
//   Author        : Mohit Bhushan
//   Last Modified : Thu 19 Mar 2020 09:27:55 AM EDT
//

// Includes 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmpsc311_log.h>
#include <lcloud_cache.h>

//implement cache structures
typedef struct cache {
    int deviceID;
    int sector;
    int block;    
    char data[256];
    int timeCached;
}cache;
cache cache_line[64];


//Global variables
//check to see if init was called 
int firstPut = 0;
float hits = 0;
float misses = 0;
int timer = 0;
//check to see if cache is filled up
int cacheFull = 0;
//current smallest time
int lowestTime  = 0;
//check if match with block was found
int blockCheck=0;


//
// Functions

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_getcache
// Description  : Search the cache for a block 
//
// Inputs       : did - device number of block to find
//                sec - sector number of block to find
//                blk - block number of block to find
// Outputs      : cache block if found (pointer), NULL if not or failure

char * lcloud_getcache( LcDeviceId did, uint16_t sec, uint16_t blk ) {

    for(int i =0; i<LC_CACHE_MAXBLOCKS; i++){
        if(did == cache_line[i].deviceID && sec == cache_line[i].sector && blk == cache_line[i].sector){
            hits+=1;
            return cache_line[i].data;
        }
    }
    misses+=1;

    /* Return not found */
    return( NULL );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_putcache
// Description  : Put a value in the cache 
//
// Inputs       : did - device number of block to insert
//                sec - sector number of block to insert
//                blk - block number of block to insert
// Outputs      : 0 if succesfully inserted, -1 if failure

int lcloud_putcache( LcDeviceId did, uint16_t sec, uint16_t blk, char *block ) {
    //if nothing is ever put into cache. First time put is called
    //logMessage(LcDriverLLevel, "TESTTTTTTTTTTTTTTTT: %s", block);
    /*
    if (firstPut == 0){
        firstPut=1;
        cache_line[0].deviceID = did;
        cache_line[0].sector = sec;
        cache_line[0].sector = blk;
        memcpy(cache_line[0].data,block, 256);
        cache_line[0].timeCached=timer;
        timer++;
        return (0);

    }
    */
    //if cache is not full 
    for(int a=0;a< LC_CACHE_MAXBLOCKS; a++){
        if(cacheFull <LC_CACHE_MAXBLOCKS){
            //if did sec and blk match replace
            if(did == cache_line[a].deviceID && sec == cache_line[a].sector && blk == cache_line[a].sector){
                memcpy(cache_line[a].data,block, 256);
                cache_line[a].timeCached=timer;
                timer++;
                return( 0 );
            }
            //else throw the block in the cache *****fix 
            else if (cache_line[a].deviceID== 0){
                cache_line[a].deviceID= did;
                cache_line[a].sector = sec;
                cache_line[a].sector = blk;
                memcpy(cache_line[a].data,block, 256);
                cache_line[a].timeCached=timer;
                timer++;
            }
            cacheFull+=1;
            
        }
        else{
            break;
        }

    }
    for(int i = 0; i<LC_CACHE_MAXBLOCKS; i++){
        //if cache is full  use this for loop
        if(cacheFull == 64){
            if(did == cache_line[i].deviceID && sec == cache_line[i].sector && blk == cache_line[i].sector){
                memcpy(cache_line[i].data,block, 256);
                cache_line[i].timeCached=timer;
                timer++;
                blockCheck = 1;
                return( 0 );
            }

        }
        else{
            break;
        }
    }
    //if no block was found to be replaced, get rid of the earliest time
   
    for(int i =0; i<LC_CACHE_MAXBLOCKS;i++){
        if(cache_line[i].timeCached==lowestTime){
            memcpy(cache_line[i].data,block, 256);
            cache_line[i].timeCached=timer;
            timer++;
            lowestTime++;
            blockCheck = 1;
            return( 0 );
        }
    }

   



    /* Return successfully */
    return( 0 );
}







////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_initcache
// Description  : Initialze the cache by setting up metadata a cache elements.
//
// Inputs       : maxblocks - the max number number of blocks 
// Outputs      : 0 if successful, -1 if failure

int lcloud_initcache( int maxblocks ) {

    for(int i =0; i<LC_CACHE_MAXBLOCKS;i++){
        cache_line[i].deviceID= 0;
        cache_line[i].sector = NULL;
        cache_line[i].sector = NULL;
        //cache_line[i].data = NULL;
    }
    

    /* Return successfully */
    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcloud_closecache
// Description  : Clean up the cache when program is closing
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int lcloud_closecache( void ) {
    /* Return successfully */
    float totalAccess; 
    float hitRatio;
    //follow formula
    totalAccess= hits + misses;
    hitRatio = 100*(float)(hits/totalAccess);

    // log message
    //logMessage(LcDriverLLevel, "TOTAL ACCESSES: %f\nTOTAL HITS: %f\n TOTAL MISSES: %f\n HIT RATIO %f",
    //totalAccess,hits,misses, hitRatio);
    return( 0 );
}
