# Virtual Storage Device


DESCRIPTON:
LionCloud is a simulation of a Cloud Based virtual storage device. 


To my best abilty;  I translated file system commands into device operatons, created an LRU cache, and ran the driver through network programming 


The three main files that I worked under were;  "lcloud_filesys.c", "lcloud_client.c", and "lcloud_cache.c" 

ABOUT lcloud_filesys.c: 
    
    	lcloud_filesys.c is where most of my work was done. Its purpose is to simulate the same argumaents as the normal UNIX I/O operations: open, read, write, seek, close, and shutdown through my own code 
    
ABOUT  lcloud_client.c:
        
        lcloud_client.c was the usage network programming, to create the network/connection between the classes and the server
    

ABOUT lcloud_cache.c:

    	lcloud_cache.c implements an LRU cache, to prevent any storage issues, along with a hit ratio calculation
    
    
    
    
HOW to run the project (MAC OS instructions):
        
        1. you must navigate to the assign4 folder through two different terminal tabs
        
        2. run this command on either tab to install some libraries: "sudo apt-get install libcurl4-gnutls-dev libgcrypt-dev"
       
        3. to test the program, you have to run the server first in one of the tabs. So in a tab, run-
        "./lcloud_server -v workload/cmpsc311-assign4a-manifest.txt"
        
        4. and in the other tab, run the client: "./lcloud_client -v workload/cmpsc311-assign4a-workload.txt"
        
        5. both files should be running, corresponding to one another, as bits are being passed into the device(s)
        
        IMPORTANT: there are multiple workload files, to make sure the test runs, make sure both commands in steps 3 and 4 are being done under the same files, two different files will resukt in an error in the code
        
        To see the workload files, they are in the assign4 folder under "workload" folder
        
        
rest of the classes made by Professor Patrick McDaniel

