////////////////////////////////////////////////////////////////////////////////
//
//  File           : lcloud_filesys.c
//  Description    : This is the implementation of the Lion Cloud device 
//                   filesystem interfaces.
//
//   Author        : *** Mohit Bhushan***
//   Last Modified : ***  ***
//

// Include files
#include <stdlib.h>
#include <string.h>
#include <cmpsc311_log.h>
#include<lcloud_cache.h>
// Project include files
#include <lcloud_filesys.h>
#include <lcloud_controller.h>


//
// File system interface implementation

/*
    Global Variables
*/
//global powerOn
int PowerOn = 0;
//fileCount
int fileCount = 0;
//global deviceID
int deviceID;
//global registers for personal use throughout program
uint64_t B0,B1,C0,C1,C2,D0,D1; 
int overwrite = 0;
//assign compare value to 00001
int compare_value = 1;
//have a counter to calc how many shifts happen after each iteration
int shift = 0;
//to adjust device id
int deviceTrack =0;
//create malloc char
char *dynmem;
char *dynmem2;
//set write to 1 var if write is called
int firstWrite =0;
//filehandle check
int i= 0;
//set read to 1 var if write is called
int firstRead =0;
//device count
int deviceCount = 0;
//empty list counter
int emptyListCounter= 0;
//compare empty List Value
int emptyListCompare =0;

int device = 0;
int sector =0;
int block = 0;

/*
    Structures
*/
//block/sector structure
/*
typedef struct blocks{
    int sectorNum;
    int blockNum;

}blocks;
*/
// create file structure
typedef struct File{
    int writeCount;//how many times you call write for manipulating lists
    //deviceTrack where you left a block partially filled
    int fillerIndex;
    size_t position; //position of file
    char pathName; //name of the file
    size_t length; //length of the file
    int fh; //number correlated to the file
    int open; //value is one if in 
    int deviceList[3000]; //list of devices in which the file was written to
    int sectorList[3000];//list of sectors in which the file was written to
    int blockList[3000];//list of sectors in which the file was written to
    int posList[3000];//list of current position    
    int offset; //tracker for position
} File;
//instances of File structure
File fileInstance[300]; 
//create device structure
typedef struct Device
{
    size_t position; //position of file
    int emptySector[200];//freed sectorList
    int emptyBlock[200];//freed blockList
    int amountEmpty;
    int amountEmptyIndex;
    int deviceID;
    int sectors;//compare value to check if occupied
    int blocks;
    int curBlock;
    int curSector;
    int amountEmptyCounter;
    //blocks blockArr[];//block list consisting of all blocks from each sector. T
}Device;
Device deviceInstance[100];




////////////////////////////////////////////////////////////////////////////////
//
// Function     : create_lcloud_registers
// Description  : create the individual registers and pack them all into 1 64 bit number
//
// Inputs       : B0, B1, C0, C1, C2, D0, D1
// Outputs      : returns 64 bit integer from packed registers
LCloudRegisterFrame create_lcloud_registers(uint64_t B0, uint64_t B1, uint64_t C0, uint64_t C1, uint64_t C2, uint64_t D0, uint64_t D1){
    
    int64_t retval = 0x0, tempB0, tempB1, tempC0, tempC1, tempC2, tempD0, tempD1;
    
    tempB0 = (B0&0xf)<<60; //shift furthest to create 4 bits space makingn it MSB
    tempB1 = (B1&0xf)<<56; //shift left 56 to create 4 bit space making it 2nd MSB
    tempC0 = (C0&0xff)<<48; //shift 48 to make 8 bit space
    tempC1 = (C1&0xff)<<40; //shift 40 to create 8 bit space
    tempC2 = (C2&0xff)<<32; //shift 32 to create 8 bit space
    tempD0 = (D0&0xffff)<<16; //shift 16 to create 16 bit space
    tempD1 = (D1&0xffff); //no shift, LSB space 16 bit

    retval = tempB0|tempB1|tempC0|tempC1|tempC1|tempC2|tempD0|tempD1; //combine statement into one. Pack all into one and return final 64 bit value
    return retval;




}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : extract_lcloud_registers
// Description  : use pass by reference to get individual values of your regisers from the 64 bit value
//
// Inputs       : res(64 bit num), B0, B1, C0, C1, C2, D0, D1
void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *B0, uint64_t *B1, uint64_t *C0, uint64_t *C1, uint64_t *C2, uint64_t *D0, uint64_t *D1 ){
    //pass by reference used for void extract function
    
    //MSB
    *B0 = (resp>>60)&0xf; //shift right to get rid of rest of bits, left with 4 bit number
    *B1 = (resp>>56)&0xf; //shift right to get rid of trailing numbers and mask to appropriate size
    //similiar rules apply to the rest of the registers
    *C0 = (resp >>48)&0xff;
    *C1 = (resp>>40)&0xff;
    *C2 = (resp >>32)&0xff;
    *D0 = (resp >>16)&0xffff;
    *D1 = (resp)&0xffff;
    

}


int LC_DEV_INIT(){
    for(int i=0; i<deviceCount; i++){
        //call create Lcloud registers function to pack into 64 bit # bitNum. LC_POWER_ON as third parameter 
        uint64_t bitNum = create_lcloud_registers(0,0, LC_DEVINIT, deviceInstance[i].deviceID, 0, 0, 0);
        //assign bitNum to new 64 but bitNum2, which will go to client_lcloud_bus_request to get the return 64 bit value from regs
        uint64_t bitNum2 = client_lcloud_bus_request( bitNum, NULL);

         //extract bitnum2 to the independent register addresses
         extract_lcloud_registers(bitNum2,&B0,&B1,&C0,&C1,&C2,&D0,&D1);

         //logMessage(LcDriverLLevel, "This is the value of D1: %i", D1);

         deviceInstance[i].blocks = D1;
         deviceInstance[i].sectors = D0;
         deviceInstance[i].amountEmpty = 0;
         deviceInstance[i].amountEmptyIndex = 0;

    }
    return(0);

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : Power_On
// Description  : Power on the device by calling create, sending to the io bus, and then extract
//
// Inputs       : res(64 bit num), B0, B1, C0, C1, C2, D0, D1
void Power_On(uint64_t *B0,uint64_t *B1, uint64_t *C0,uint64_t *C1,uint64_t *C2,uint64_t *D0,uint64_t *D1){

    //call create Lcloud registers function to pack into 64 bit # bitNum. LC_POWER_ON as third parameter 
    uint64_t bitNum = create_lcloud_registers(0,0, LC_POWER_ON, 0, 0, 0, 0);
    //assign bitNum to new 64 but bitNum2, which will go to client_lcloud_bus_request to get the return 64 bit value from regs
    uint64_t bitNum2 = client_lcloud_bus_request( bitNum, NULL);

    //extract bitnum2 to the independent register addresses
    extract_lcloud_registers(bitNum2,&B0,&B1,&C0,&C1,&C2,&D0,&D1);
   
}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : Device_Probe
// Description  : Probe device by packing, sending to bus, and extracting values, calls get_device_id
//
// Inputs       : res(64 bit num), B0, B1, C0, C1, C2, D0, D1
void Device_Probe(uint64_t *B0,uint64_t *B1, uint64_t *C0,uint64_t *C1,uint64_t *C2,uint64_t *D0,uint64_t *D1){
    

    //pack values with LC_DEVPROBE as third param
    uint64_t bitnum3 = create_lcloud_registers(0,0, LC_DEVPROBE, 0, 0, 0, 0);
    //send packaged value to client_lcloud_bus_request
    uint64_t bitnum4 = client_lcloud_bus_request( bitnum3, NULL);

    //extract values to assign to each independent register address
    extract_lcloud_registers(bitnum4,&B0,&B1,&C0,&C1,&C2,&D0,&D1);

    //assign conparison var to value of D0
    int tempD0 = D0;
    //count devices
    int count = 0;
    //shift count 
    int shift  = 0;
    //index
    int index = 0;
    //compare value
    int compare_value = 1;

    while(compare_value<=D0){
        if((tempD0 & compare_value)!=0){
            deviceInstance[index].deviceID = shift;
            shift++;
            index++;
            deviceCount++;
            compare_value= compare_value<<1;
        }
        else{
            shift++;
            compare_value= compare_value<<1;
        }
    }
    //deviceCount=1;


}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcopen
// Description  : Open the file for for reading and writing
//
// Inputs       : path - the path/filename of the file to be read
// Outputs      : file handle if successful test, -1 if failure

LcFHandle lcopen( const char *path ) {
    

    int x = 1;
    //call Power on function and pass temp 64 bit registers
    if (PowerOn == 0){
        Power_On(&B0,&B1,&C0,&C1,&C2,&D0,&D1);
        //Probe Device
        Device_Probe(&B0,&B1,&C0,&C1,&C2,&D0,&D1);
        LC_DEV_INIT();
        lcloud_initcache( LC_CACHE_MAXBLOCKS );
        PowerOn = 1;


    }
    

    



    
    //if device is powered on (B1==1) proceed to following 
    if(PowerOn ==1)
    {
        
       
        for( i; i<= fileCount; i++){
            if (fileInstance[i].pathName == *path){
                return(-1); //statement fails since file already exists
            } 
        }
        


        fileInstance[fileCount].pathName = *path;
        fileInstance[fileCount].length = 0;
        fileInstance[fileCount].fh = fileCount;
        fileInstance[i].fillerIndex =0;
        fileInstance[i].open = 1;
        //fileInstance[i].position = 0;
        
        //x+=1;
        fileCount+=1;
        return fileInstance[fileCount-1].fh;
        
    }



    return( 0 ); // Likely wrong
} 

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcread
// Description  : Read data from the file 
//
// Inputs       : fh - file handle for the file to read from
//                buf - place to put the data
//                len - the length of the read
// Outputs      : number of bytes read, -1 if failure
int lcread( LcFHandle fh, char *buf, size_t len ) {
    char localbuff[256];
    size_t templen = len;
    int totalcWrite = 0;
    int initialBlock;
    //find file 
    int f = 0;
    for(f; f< 1000; f++){
        if(fileInstance[f].fh==fh){
            break;
        }
    }

 

    int h =0;
    while (totalcWrite <=fileInstance[f].offset){
        totalcWrite+=fileInstance[f].posList[h];
        h++;

    }
    
    //prevent mallocing everytime
    if(firstRead ==0){
        char dynmem = (char)malloc(256*sizeof(char));
        firstRead+=1;
    }
    else{
        memset(dynmem,0,256);
    }

    //index of where to start reading from
    //int initialBlock = (int)(fileInstance[f].offset/256);
   
    
    initialBlock = h-1;
    
    //incrementation of index as reads continue
    int counter = 0;
    //keep track of position of reading
    int amountRead = 0;
    //position in block
    int position = fileInstance[f].posList[initialBlock] - (totalcWrite -fileInstance[f].offset);
    fileInstance[f].offset+=len;
    while(templen>0){
        //if you need to read more than one block(len>256)
        if(templen>=(256-position)){
            //how many blocks you need to fill with 256
            //int value = (int)(len/256);
            //read blocks that go until 256
            
            //if reading the first block, it is highly likely you are going to be reading from the middle block           
                               
            //tracker for index of lists
            int currentInstance = initialBlock+counter;
            //get the localBuff
            
            
            
            
            if(lcloud_getcache(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
            fileInstance[f].blockList[currentInstance])==NULL){
                getBlock(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
                fileInstance[f].blockList[currentInstance], localbuff);
                lcloud_putcache(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].curSector,
                deviceInstance[deviceTrack].curBlock,localbuff);
                
            }
            else{
                memcpy(localbuff,lcloud_getcache(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
                fileInstance[f].blockList[currentInstance]),256);
            }
            
            //getBlock(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
            //fileInstance[f].blockList[currentInstance], localbuff);
    

            
            //logMessage(LcDriverLLevel, "buf >=256:%s", localbuff);

            //for dynmem position
            int c=0;
            int holderPosition = position;
            //allocate contents of localbuf into dynmem
            for(position; position<fileInstance[f].posList[currentInstance]; position++){
                dynmem[c] = localbuff[position];
                c+=1;
            }
            position = 0;
            
            //memcpycontents
            memcpy(&buf[amountRead],dynmem, c);
            c= 0;
            //increment counter
            counter+=1;
            if (counter == 1){
                amountRead = fileInstance[f].posList[currentInstance] - holderPosition;
                templen-= fileInstance[f].posList[currentInstance] - holderPosition;
            }
            else{
                //add to amount read
                amountRead += fileInstance[f].posList[currentInstance];
                //adjust templen
                templen-=fileInstance[f].posList[currentInstance];
            }
            
            

        }
        if(templen<256 && templen!=0){
            memset(localbuff, "\0",templen);
            //tracker for index of lists
            int currentInstance = initialBlock+counter;
            //get the localBuff
            
            
            if(lcloud_getcache(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
            fileInstance[f].blockList[currentInstance])==NULL){
                getBlock(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
                fileInstance[f].blockList[currentInstance], localbuff);
                lcloud_putcache(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].curSector,
                deviceInstance[deviceTrack].curBlock,localbuff);
                
            }
            else{
                memcpy(localbuff,lcloud_getcache(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
                fileInstance[f].blockList[currentInstance]),256);
            }
            
            getBlock(fileInstance[f].deviceList[currentInstance],fileInstance[f].sectorList[currentInstance], 
            fileInstance[f].blockList[currentInstance], localbuff);
            //logMessage(LcDriverLLevel, "buf <256:%s", localbuff);

    
            
            int b;
            int placerVal= 0;
            int holderPosition = position;
            if (fileInstance[f].posList[currentInstance]-holderPosition<=templen){
                placerVal = 1;
                for(b =0; b<fileInstance[f].posList[currentInstance]-holderPosition; b++){
                dynmem[b] = localbuff[position];
                position++;

                }
            }
            else{
                for(b =0; b<templen; b++){
                dynmem[b] = localbuff[position];
                position++;
                }
            }
           
            //memcpy contents into current position of buf
            memcpy(&buf[amountRead],dynmem,templen);
            //set amountRead back to 0
            
            //set templen to 0 since this only occurs once
            if(templen <=(fileInstance[f].posList[currentInstance]-holderPosition)){
                amountRead +=templen;
                templen = 0;
                
            }
            else
            {
                if (placerVal ==1){
                amountRead +=fileInstance[f].posList[currentInstance] -holderPosition;
                templen -=fileInstance[f].posList[currentInstance] - holderPosition;  
                }
                else{
                amountRead +=b -holderPosition;
                templen -=b - holderPosition;  
                }
                
            }
            
            placerVal =0;
            position =0;
            counter+=1;
           
        }



    }
    
    return(len);

}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcwrite
// Description  : write data to the file
//
// Inputs       : fh - file handle for the file to write to
//                buf - pointer to data to write
//                len - the length of the write
// Outputs      : number of bytes written if successful test, -1 if failure

int lcwrite( LcFHandle fh, char *buf, size_t len ) {
      
    //add on excess len that was leftover from previous block if exceeded by 256
    size_t  templen = len;
    //assign mem allocate
    //make char *dynmem global*************************
    /*
        if dynmem// then i memset
        else//malloc
        //put in the while loop
    */
    if (firstWrite==0){
        firstWrite = 1;
        dynmem = (char*)malloc(256*sizeof(char));

    }
    else{
        //dynmem = (char*)realloc(dynmem, 256*sizeof(char));
        memset(dynmem,"",256);
    }
    
    //
    int device, block,sector;
    
    //create local buff value
    char localbuff[256]; 

   
    //find file object through f
    int f;
    for(f = 0; f< 1000; f++){
        if (fileInstance[f].fh == fh){

            break;
        }
    }
    
    fileInstance[f].length+=len;
    //fileInstance[f].writeCount
    compare_value =0;
    while(templen>0){
        
        //if previous file was not finished up in a block
        if(fileInstance[f].posList[fileInstance[f].fillerIndex]!=256 &&fileInstance[f].posList[fileInstance[f].fillerIndex]!=0){
            //get block that was previously written
            getBlock(fileInstance[f].deviceList[fileInstance[f].fillerIndex], fileInstance[f].sectorList[fileInstance[f].fillerIndex],
            fileInstance[f].blockList[fileInstance[f].fillerIndex], localbuff);
            int startpos = fileInstance[f].posList[fileInstance[f].fillerIndex];
            int holderpos = startpos;
            if((templen+startpos)>=256){
                device = fileInstance[f].deviceList[fileInstance[f].fillerIndex];
                sector = fileInstance[f].sectorList[fileInstance[f].fillerIndex];
                block = fileInstance[f].blockList[fileInstance[f].fillerIndex];
                int fillerdata = 256-startpos;
                int c=0;
                for(startpos;startpos<256;startpos++){
                    dynmem[c] =buf[compare_value];
                    c+=1;
                    compare_value+=1;
                }
                memcpy(&localbuff[holderpos],dynmem,fillerdata);
                //update file structure 
                fileInstance[f].posList[fileInstance[f].fillerIndex]= 256;
                //update writeCount
                //put into
                lcloud_putcache(fileInstance[f].deviceList[fileInstance[f].fillerIndex], fileInstance[f].sectorList[fileInstance[f].fillerIndex],
                fileInstance[f].blockList[fileInstance[f].fillerIndex], localbuff);

                putblock(device,sector,block, localbuff);
                
                //update templen
                templen-=fillerdata;
            }
            //if templen +startpos is <256
            else{
                device = fileInstance[f].deviceList[fileInstance[f].fillerIndex];
                sector = fileInstance[f].sectorList[fileInstance[f].fillerIndex];
                block = fileInstance[f].blockList[fileInstance[f].fillerIndex];
                int fillerdata = templen;
                int c = 0;
                for(startpos;startpos<templen+holderpos;startpos++){
                    dynmem[c]=buf[compare_value];
                    c+=1;
                    compare_value+=1;
                }
                memcpy(&localbuff[holderpos],dynmem,templen);
                //update file structure
                fileInstance[f].posList[fileInstance[f].fillerIndex]= templen+holderpos;
                //put into
                lcloud_putcache(fileInstance[f].deviceList[fileInstance[f].fillerIndex], fileInstance[f].sectorList[fileInstance[f].fillerIndex],
                fileInstance[f].blockList[fileInstance[f].fillerIndex], localbuff);
                putblock(device,sector,block,localbuff);
                //update templen
                templen=0;

            }

        }
        
        //if at beginning of block
        if (templen>=256){
            //get value to fill up blocks with 256 bytes
            int value  = (int)(templen/256);
            for(int a = 0; a<value; a++){
                for( int b = 0;b<256;b++){
                    dynmem[b] = buf[compare_value];
                    compare_value+=1;
                }
                memcpy(localbuff,dynmem,256);
                    
                //logMessage(lcDriverLLevel, “buf:[%s]”, localbuff);
                if(deviceInstance[deviceTrack].amountEmpty==1){

                    lcloud_putcache(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].emptySector[deviceInstance[deviceTrack].amountEmptyIndex],
                    deviceInstance[deviceTrack].emptyBlock[deviceInstance[deviceTrack].amountEmptyIndex],localbuff);
                        
                    putblock(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].emptySector[deviceInstance[deviceTrack].amountEmptyIndex],
                    deviceInstance[deviceTrack].emptyBlock[deviceInstance[deviceTrack].amountEmptyIndex],localbuff);
                    
                    fileInstance[f].deviceList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].deviceID;
                    fileInstance[f].sectorList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].emptySector[deviceInstance[deviceTrack].amountEmptyIndex];
                    fileInstance[f].blockList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].emptyBlock[deviceInstance[deviceTrack].amountEmptyIndex];
                    fileInstance[f].posList[fileInstance[f].writeCount]= 256;

                    deviceInstance[deviceTrack].amountEmptyIndex+=1;
                    if(deviceInstance[deviceTrack].amountEmptyIndex>=deviceInstance[deviceTrack].amountEmptyCounter){
                        deviceInstance[deviceTrack].amountEmptyIndex=0;
                    }
                    fileInstance[f].writeCount+=1;
                    emptyListCompare+=1;
                    
                    if(emptyListCompare>=emptyListCounter){
                        //emptyListCompare=0;
                        //start empty index from beginning
                        for(int v=0;v<=deviceCount;v++){
                            deviceInstance[v].amountEmptyIndex=0;
                        }
                    }
                    
                }
                else{
                    //check if device is filled up
                    for(int x = 0; x<deviceCount;x++){
                        if(deviceInstance[deviceTrack].curSector >= deviceInstance[deviceTrack].sectors){
                            deviceTrack+=1;
                            if(deviceTrack>=deviceCount){
                            deviceTrack=0;
                         }

                        }
                        else{
                            break;
                        }
                    }
                    //empty contents of memory allocation
                    //update file structor to keep track of where file was written to 
                    fileInstance[f].deviceList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].deviceID;
                    fileInstance[f].sectorList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].curSector;
                    fileInstance[f].blockList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].curBlock;
                    fileInstance[f].posList[fileInstance[f].writeCount]= 256;
                    //update put block/write count to increase the index with the file
                    fileInstance[f].writeCount+=1;
                    lcloud_putcache(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].curSector,deviceInstance[deviceTrack].curBlock,localbuff);
                    putblock(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].curSector,deviceInstance[deviceTrack].curBlock,localbuff);
                    //increment block within device
                    deviceInstance[deviceTrack].curBlock+=1;
                    //if block num exceeds device capacity
                    if(deviceInstance[deviceTrack].curBlock>=deviceInstance[deviceTrack].blocks){
                        //increment block and sector within device
                        deviceInstance[deviceTrack].curBlock=0;
                        deviceInstance[deviceTrack].curSector+=1;
                    }
                    //move to next device
                    deviceTrack+=1;
                    //if devicetrack goes beyond list
                    if(deviceTrack>=deviceCount){
                        deviceTrack=0;
                    }

                }//end else
                       
            }//end for value
                //adjust templen to remaining block needing to be written into final block
                //compare_value =0;
                //templen-=256;
                templen = templen%256;
        }
        //if less than 256 to write
        if(templen<256&&templen!=0){
            int c = 0;
            //make startposition for tracking
            for(int b = 0; b<templen; b++){
                dynmem[b] = buf[compare_value];
                compare_value+=1;
            }
            //check if device is filled up
            for(int x = 0; x<deviceCount;x++){
                if(deviceInstance[deviceTrack].curSector >= deviceInstance[deviceTrack].sectors){
                    deviceTrack+=1;
                    if(deviceTrack>=deviceCount){
                        deviceTrack=0;
                    }

                }
                else{
                    break;
                }
            }
            memset(localbuff, '\0', 256);
            //put dynmem into localbuff
            memcpy(localbuff,dynmem,templen);

            if(deviceInstance[deviceTrack].amountEmpty==1){
                //deviceInstance[deviceTrack].amountEmpty-=1;
                //store in cache
                lcloud_putcache(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].emptySector[deviceInstance[deviceTrack].amountEmptyIndex],
                deviceInstance[deviceTrack].emptyBlock[deviceInstance[deviceTrack].amountEmptyIndex],localbuff);
                //store in block        
                putblock(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].emptySector[deviceInstance[deviceTrack].amountEmptyIndex],
                deviceInstance[deviceTrack].emptyBlock[deviceInstance[deviceTrack].amountEmptyIndex],localbuff);
                //update memlists        
                fileInstance[f].deviceList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].deviceID;
                fileInstance[f].sectorList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].emptySector[deviceInstance[deviceTrack].amountEmptyIndex];
                fileInstance[f].blockList[fileInstance[f].writeCount]= deviceInstance[deviceTrack].emptyBlock[deviceInstance[deviceTrack].amountEmptyIndex];
                fileInstance[f].posList[fileInstance[f].writeCount]= templen;
                //incrememnt tracker index
                deviceInstance[deviceTrack].amountEmptyIndex+=1;
                if(deviceInstance[deviceTrack].amountEmptyIndex>=deviceInstance[deviceTrack].amountEmptyCounter){
                    deviceInstance[deviceTrack].amountEmptyIndex=0;
                }
                fileInstance[f].writeCount+=1;
                
                emptyListCompare+=1;
                if(emptyListCompare>=emptyListCounter){
                    //emptyListCompare=0;
                    //start empty index from beginning
                    for(int v=0;v<=deviceCount;v++){
                        deviceInstance[v].amountEmptyIndex=0;
                    }
                }
                

            }
                
            else{
                //update file structor to keep track of where file was written to
                fileInstance[f].deviceList[fileInstance[f].writeCount] = deviceInstance[deviceTrack].deviceID;
                fileInstance[f].sectorList[fileInstance[f].writeCount] = deviceInstance[deviceTrack].curSector;
                fileInstance[f].blockList[fileInstance[f].writeCount] = deviceInstance[deviceTrack].curBlock;
                fileInstance[f].posList[fileInstance[f].writeCount]= templen;
                //since the block is half filled, you want to keep track of this by assigning the deviceTrack to a temp holder var
                fileInstance[f].fillerIndex = fileInstance[f].writeCount;
                //update put block/write count to increase the index with the file
                fileInstance[f].writeCount+=1;
                //throw in cache
                lcloud_putcache(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].curSector,deviceInstance[deviceTrack].curBlock,localbuff);
                //send to device
                putblock(deviceInstance[deviceTrack].deviceID, deviceInstance[deviceTrack].curSector,deviceInstance[deviceTrack].curBlock,localbuff);
                //increment block within device
                deviceInstance[deviceTrack].curBlock+=1;
                //if block num exceeds device capacity
                if(deviceInstance[deviceTrack].curBlock>=deviceInstance[deviceTrack].blocks){
                    //increment block and sector within device
                    deviceInstance[deviceTrack].curSector+=1;
                    deviceInstance[deviceTrack].curBlock=0;
                }
                //move to next device
                deviceTrack+=1;
                //if device track goes beyond index of deviceInstance
                if(deviceTrack>=deviceCount){
                   deviceTrack=0;
                }

            } 
            //set templen to 0         
            templen = 0;
            c= 0;
        }            
    }
    fileInstance[f].offset += len;
    return(len);
}

    
    





////////////////////////////////////////////////////////////////////////////////
//
// Function     : putblock
// Description  : send the value to the ioBus/physically put into block
//
// Inputs       : deviceID, sector, block, *buffer
//               
// Outputs      : 0 if successful test, -1 if failure
int putblock(int deviceID, int sector, int block, char *buffer){
    LCloudRegisterFrame readValues, readValToIoBus; 
    readValues = create_lcloud_registers(0, 0, LC_BLOCK_XFER, deviceID, LC_XFER_WRITE, sector, block);

    /*
    if  ((readValues== -1) || ((readValToIoBus= client_lcloud_bus_request(readValues, buffer)) == -1)){
        return (-1)
    } 
    else if( (extract_lcloud_registers(readValToIoBus, &B0, &B1, &C0, &C1, &C2, &D0, &D1)) &&
      (B0 != 1) || (B1 != 1) || (*C0 != LC_BLOCK_XFER) ) {
        logMessage( LOG_ERROR_LEVEL, "LC failure writing blkc[%d/%d/%d].", deviceID, sector, block );
        return( -1 );
        }
      */  
               
    readValToIoBus = client_lcloud_bus_request(readValues, buffer);
    extract_lcloud_registers(readValToIoBus, &B0,&B1,&C0,&C1,&C2,&D0,&D1);

    
    return(0);

   


}
////////////////////////////////////////////////////////////////////////////////
//
// Function     : getBlock
// Description  : get value/certain len from block
//
// Inputs       : deviceID, sector, block, *buffer
//               
// Outputs      : 0 if successful test, -1 if failure
int getBlock(int deviceID, int sector, int block, char *buffer){
    LCloudRegisterFrame readValues, readValToIoBus; 
    readValues = create_lcloud_registers(0, 0, LC_BLOCK_XFER, deviceID, LC_XFER_READ, sector, block);
    /*
    if  ((readValues== -1) || ((readValToIoBus= client_lcloud_bus_request(readValues, buffer)) == -1)){
        return (-1)
    } 
    else if( (extract_lcloud_registers(readValToIoBus, &B0, &B1, &C0, &C1, &C2, &D0, &D1)) &&
      (B0 != 1) || (B1 != 1) || (*C0 != LC_BLOCK_XFER) ) {
        logMessage( LOG_ERROR_LEVEL, "LC failure writing blkc[%d/%d/%d].", deviceID, sector, block );
        return( -1 );
        }
      */  
               
    readValToIoBus = client_lcloud_bus_request(readValues, buffer);
    extract_lcloud_registers(readValToIoBus, &B0,&B1,&C0,&C1,&C2,&D0,&D1);

    
    return(0);

   


}   


    
       
                 



////////////////////////////////////////////////////////////////////////////////
//
// function     : lcseek
// Description  : Seek to a specific place in the file
//
// Inputs       : fh - the file handle of the file to seek in
//                off - offset within the file to seek to
// Outputs      : 0 if successful test, -1 if failure

int lcseek( LcFHandle fh, size_t off ) {
    int i;
    //find file handle
    for(i = 0; i< 1000; i++){
        if (fileInstance[i].fh == fh){

            break;
        }
    }
    fileInstance[i].offset = off;
    // create pointer val for fileInstance
    File *val = &fileInstance[i];
    //if offset is > length of fileInstance return false
    if(off>(val->length)) {
        return(-1);

    }
    //set position of val = to offset
    val->position = off;

    return( val->position );

}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcclose
// Description  : Close the file
//
// Inputs       : fh - the file handle of the file to close
// Outputs      : 0 if successful test, -1 if failure

int lcclose( LcFHandle fh ) {
    int device;
    int i;
    for(i= 0; i<= 1000; i++){
        if (fileInstance[i].fh == fh){
            fileInstance[i].open = 0;
            break;           
        }
    } 
    
    //allow empty space
    for (int a= 0; a<3000; a++){
        if (fileInstance[i].deviceList[a]!=0){
            device = fileInstance[i].deviceList[a];
            //match device number
            int b;
            for(b=0;b<30;b++){
                if(device==deviceInstance[b].deviceID){
                    break;
                }
            }
            //append empty sector and block to device
            
            
            deviceInstance[b].emptySector[deviceInstance[b].amountEmpty] = fileInstance[i].sectorList[a];
            deviceInstance[b].emptyBlock[deviceInstance[b].amountEmpty] = fileInstance[i].blockList[a];
            deviceInstance[b].amountEmpty=1;
            deviceInstance[b].amountEmptyCounter= fileInstance[i].writeCount;
            emptyListCounter+=1;
            
        }
        else{
            break;
        }      
    }
    
     

    return(0);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : lcshutdown
// Description  : Shut down the filesystem
//
// Inputs       : none
// Outputs      : 0 if successful test, -1 if failure

int lcshutdown( void ) {
    free(dynmem);
    //pack registers with lcloud registers
    uint64_t bitNum = create_lcloud_registers(0,0, LC_POWER_OFF, 0, 0, 0, 0);
    //send to io bus
    uint64_t bitNum2 = client_lcloud_bus_request( bitNum, NULL);
    //extract individualn registers
    extract_lcloud_registers(bitNum2,&B0,&B1,&C0,&C1,&C2,&D0,&D1);
    lcloud_closecache();

    return( 0 );

}








