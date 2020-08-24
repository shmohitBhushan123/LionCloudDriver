////////////////////////////////////////////////////////////////////////////////
//
//  File          : lcloud_client.c
//  Description   : This is the client side of the Lion Clound network
//                  communication protocol.
//
//  Author        : Patrick McDaniel
//  Last Modified : Sat 28 Mar 2020 09:43:05 AM EDT
//

// Include Files
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

// Project Include Files
#include <lcloud_network.h>
#include<lcloud_support.h>
#include <cmpsc311_log.h>
#include <cmpsc311_util.h>
#include <lcloud_filesys.h>
#include <lcloud_cache.h>


//Global variables





//
// Functions
void extract_lcloud_registers(LCloudRegisterFrame resp, uint64_t *B0, uint64_t *B1, uint64_t *C0, uint64_t *C1, uint64_t *C2, uint64_t *D0, uint64_t *D1 );

uint64_t B0;
uint64_t B1;
uint64_t C0;
uint64_t C0;
uint64_t C1;
uint64_t C2;
uint64_t D0;
uint64_t D1;
int socket_handle = -1;
int socketfd;
int Rreg;
////////////////////////////////////////////////////////////////////////////////
//
// Function     : client_lcloud_bus_request
// Description  : This the client regstateeration that sends a request to the 
//                lion client server.   It will:
//
//                1) if INIT make a connection to the server
//                2) send any request to the server, returning results
//                3) if CLOSE, will close the connection
//
// Inputs       : reg - the request reqisters for the command
//                buf - the block to be read/written from (READ/WRITE)
// Outputs      : the response structure encoded as needed

LCloudRegisterFrame client_lcloud_bus_request( LCloudRegisterFrame reg, void *buf ) {
    struct sockaddr_in caddr;
    char *ip = LCLOUD_DEFAULT_IP;
    //int port = LCLOUD_DEFAULT_PORT;
    //caddr.sin_addr.s_addr = inet_addr(addr);
    caddr.sin_family = AF_INET;
    caddr.sin_port =htons(LCLOUD_DEFAULT_PORT);
    uint64_t value;
    
    //test for open connection
    if (socket_handle == -1){
        socket_handle = 0;
        //no open connection
        //    (a) Setup the address
        if (inet_aton(ip, &caddr.sin_addr) == 0 ) {         
            return(-1);
        }
        //    (b) Create the socket
        socketfd = socket(PF_INET, SOCK_STREAM, 0);        
        if (socketfd == -1) {
            printf("Error on socket creation [%s]\n", strerror(errno));
            return(-1);
        }
        //    (c) Create the connection
        if (connect(socketfd,(const struct sockaddr *)&caddr, sizeof(caddr)) == -1) {
            printf( "Error on socket connect \n");
            return(-1);
        }
                
    }
    //Rreg = reg;
    //C0 = (reg >>48)&0xff;
    //C2 = (reg >>32)&0xff;
    extract_lcloud_registers(reg,&B0,&B1,&C0,&C1,&C2,&D0,&D1);
    
    //value = htonll64(reg);


    if(C0==LC_BLOCK_XFER){   
        // CASE 1: read operation (look at the c0 and c2 fields)
        if(C2 == LC_XFER_READ){
            value = htonll64(reg);
            //ssize_t writereg = write(socketfd, &reg, sizeof(reg));
            if (write(socketfd, &value, sizeof(value))!= sizeof(value)){
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }


            //ssize_t readreg = read(socketfd, &reg, sizeof(reg));
            if (read(socketfd, &value, sizeof(value))!= sizeof(value)){
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }
            //ssize_t readreg1 = read(socketfd, buf, 256);
            if (read(socketfd, buf, 256)!= 256){
                logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
                return(-1);
            }
            //value = ntohll64(reg);



        }
        // CASE 2: write operation (look at the c0 and c2 fields)
        if(C2 == LC_XFER_WRITE){
            value = htonll64(reg);
            //ssize_t writereg = write(socketfd, &reg, sizeof(reg));
            if (write(socketfd, &value, sizeof(value))!= sizeof(value)){
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }


            //ssize_t writereg1 = write(socketfd, buf, 256);
            if (write(socketfd, buf, 256)!= 256){
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }
            //ssize_t readreg = read(socketfd, buf, 256);
            if (read(socketfd, &value, sizeof(value))!= sizeof(value)){
                logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
                return(-1);
            }
            //value = ntohll64(reg);

        }
        
    }
    // CASE 3: power off operation
    if(C0 == LC_POWER_OFF){
        value = htonll64(reg);
        //ssize_t writereg = write(socketfd, &reg, sizeof(reg));
        if (write(socketfd, &value, sizeof(value))!= sizeof(value)){
            logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
            return(-1);if (read(socketfd, &value, sizeof(value))!= sizeof(value)){
                logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
                return(-1);
            }
        }
        //ssize_t readreg = read(socketfd, &reg, sizeof(reg));
        if (read(socketfd, &value, sizeof(value))!= sizeof(value)){
            logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
            return(-1);
        }
 
        
        close(socketfd);
        socket_handle = -1;
        value = ntohll64(reg);    
    }
        //Case 4: when powering on device
        if (C0 == LC_POWER_ON && C2 == 0){
            value = htonll64(reg);
            if( write( socketfd, &value, sizeof(value)) != sizeof(reg) ) {
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }   
            printf("Sent a value of [%ld]\n", ntohll64(value) );

            if( read( socketfd, &value, sizeof(value)) != sizeof(reg) ) { 
                logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
                return(-1);
            }
            
            value = ntohll64(value);
        }
        //Case 5: when calling devinit
        if (C0 == LC_DEVINIT){
            value = htonll64(reg);
            if( write( socketfd, &value, sizeof(value)) != sizeof(value) ) {
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }   

            if( read( socketfd, &value, sizeof(value)) != sizeof(value) ) { 
                logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
                return(-1);
            
            }   
            value = ntohll64(value);
        }
 
        //Case 6: when probing the device       
        if (C0 == LC_DEVPROBE && C2 == 0){
            value = htonll64(reg);

            if( write( socketfd, &value, sizeof(value)) != sizeof(value) ) {
                logMessage(registerLogLevel, "ERROR: INVALID WRITE SIZE");
                return(-1);
            }   

            if( read( socketfd, &value, sizeof(value)) != sizeof(value) ) { 
                logMessage(registerLogLevel, "ERROR: INVALID READ SIZE");
                return(-1);
            }   
            value = ntohll64(value);

        }


        return (value);
}


