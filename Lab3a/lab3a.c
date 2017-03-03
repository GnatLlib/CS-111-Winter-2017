#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <sys/stat.h>


int SUPERBLOCKOFF = 1024;
int SUPERBLOCKSZ = 1024;

int img_fd, superblock_fd, group_fd, bitmap_fd, inode_fd, directory_fd, indirect_fd;

int num_groups;

//superblock info
uint16_t magic_number;
uint32_t inode_count, block_count, block_size, fragment_size, blocks_per_group, inodes_per_group, fragments_per_group, first_data_block;

//block groups info

struct bgroupinfo_t {
    uint16_t contained_blocks, free_blocks, free_inodes, num_directories;
    uint32_t free_inode_bitmap, free_block_bitmap, inode_table_start;

};

//directory info
struct dirinfo_t {
    int inode;
    int inode_num;
};


int d_index;
struct dirinfo_t * directories;

//valid node info
int * used_inodes;
int i_index;

struct bgroupinfo_t * bgroups;

int main(int argc, char** argv){
    
    //buffers
    uint64_t b64;
    uint32_t b32;
    uint16_t b16;
    uint8_t b8;
    uint8_t b8_2;
    int signedb32;
    struct stat file_stat;
    
    //parse arguments
    if(argc!=2){
        fprintf(stderr, "Error, Inavlid Arguments");
        exit(EXIT_FAILURE);
    }
    else{
        img_fd = open(argv[1], O_RDONLY);
       
    }

    //get file size
    fstat(img_fd, &file_stat);
    int file_size = file_stat.st_size;
    
    
    //*************************************SUPERBLOCK INFO *****************************************
    //generate superblock csv
    superblock_fd = creat("super.csv", S_IRWXU);
    
    //read superblock info from file
    pread(img_fd, &b16, 2, SUPERBLOCKOFF + 56);
    magic_number = b16;
   
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 0);
    inode_count = b32;
    
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 4);
    block_count = b32;
    
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 4);
    block_size = 1024 << b32;
    
    pread(img_fd, &signedb32, 4, SUPERBLOCKOFF + 28);
    if(signedb32 > 0){
        fragment_size = 1024 << signedb32;
    }
    else{
        fragment_size = 1024 >> -signedb32;
    }
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 32);
    blocks_per_group = b32;
   
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 40);
    inodes_per_group = b32;
    
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 36);
    fragments_per_group = b32;
    
    pread(img_fd, &b32, 4, SUPERBLOCKOFF + 20);
    first_data_block = b32;
    
    //check magic number 
    if(magic_number != 0xEf53){
        fprintf(stderr, "Superblock - Invalid magic number: %x\n", magic_number);
        exit(EXIT_FAILURE);
    }
     //check block size
    if( block_size<512 || block_size > 64000 || (block_size&(block_size-1)))
    {
        fprintf(stderr, "Superblock - Invalid block size: %d\n", block_size);
        exit(EXIT_FAILURE);
    }
    //check block groups
    if(block_size * block_count > file_size){
        fprintf(stderr, "Superblock - Invalid block count: %d > %d\n", block_size * block_count, file_size);
        exit(EXIT_FAILURE);
    }
    //check first data block
    if(first_data_block>file_size){
        fprintf(stderr, "Superblock - Invalid first block: %d > %d\n", first_data_block, file_size);
        exit(EXIT_FAILURE);
    }
    //check blocks per group divides in blocks
    if(block_count % blocks_per_group != 0){
        fprintf(stderr, "Superblock - %d blocks, %d blocks/group\n", block_count, blocks_per_group);
        exit(EXIT_FAILURE);
    }
    //check inodes per group divides into inodes
    if(inode_count % inodes_per_group!=0){
        fprintf(stderr, "Superblock - %d inodes, %d inodes/group\n", inode_count, inodes_per_group);
        exit(EXIT_FAILURE);
    }

    //write to file
    dprintf(superblock_fd, "%x," , magic_number);
    dprintf(superblock_fd, "%d," , inode_count);
    dprintf(superblock_fd, "%d," , block_count);
    dprintf(superblock_fd, "%d," , block_size);
    dprintf(superblock_fd, "%d," , fragment_size);
    dprintf(superblock_fd, "%d," , blocks_per_group);
    dprintf(superblock_fd, "%d," , inodes_per_group);
    dprintf(superblock_fd, "%d," , fragments_per_group);
    dprintf(superblock_fd, "%d\n" , first_data_block);
    close(superblock_fd);

    //*********************************BLOCK GROUPS INFO***************************************

    //create csv 
    group_fd = creat("group.csv", S_IRWXU);

    num_groups = block_count / blocks_per_group;
    
    bgroups = malloc(num_groups * sizeof(struct bgroupinfo_t));

    
    for(int i = 0; i< num_groups; i++){
        
        //get and store block group info for each group
        bgroups[i].contained_blocks = blocks_per_group;
        
        pread(img_fd, &b16, 2, SUPERBLOCKOFF + block_size + (i*32) + 12);
        bgroups[i].free_blocks = b16;

        pread(img_fd, &b16, 2, SUPERBLOCKOFF + block_size + (i*32) + 14);
        bgroups[i].free_inodes = b16;

        pread(img_fd, &b16, 2, SUPERBLOCKOFF + block_size + (i*32) + 16);
        bgroups[i].num_directories = b16;

        pread(img_fd, &b32, 4, SUPERBLOCKOFF + block_size + (i*32) + 4);
        bgroups[i].free_inode_bitmap = b32;

        pread(img_fd, &b32, 4, SUPERBLOCKOFF + block_size + (i*32) + 0);
        bgroups[i].free_block_bitmap = b32;

        pread(img_fd, &b32, 4, SUPERBLOCKOFF + block_size + (i*32) + 8);
        bgroups[i].inode_table_start = b32;

        //check for errors
      
        //i don't know it's possible to check number of contained blocks

        //check free inode bitmap is in group
        if(bgroups[i].free_inode_bitmap > i*blocks_per_group + blocks_per_group || bgroups[i].free_inode_bitmap < i*blocks_per_group){
            fprintf(stderr, "Group %d: blocks %d-%d, free Inode map starts at %d\n", i, i*blocks_per_group, i*blocks_per_group + blocks_per_group, bgroups[i].free_inode_bitmap);
            
        }

        //check free block bitmap is in group
         if(bgroups[i].free_block_bitmap > i*blocks_per_group + blocks_per_group || bgroups[i].free_block_bitmap < i*blocks_per_group){
            fprintf(stderr, "Group %d: blocks %d-%d, free Block map starts at %d\n", i, i*blocks_per_group, i*blocks_per_group + blocks_per_group, bgroups[i].free_block_bitmap);
            
        }

        //check inode table start is in group
         if(bgroups[i].inode_table_start > i*blocks_per_group + blocks_per_group || bgroups[i].inode_table_start < i*blocks_per_group){
           fprintf(stderr, "Group %d: blocks %d-%d, Inode table starts at %d\n", i, i*blocks_per_group, i*blocks_per_group + blocks_per_group, bgroups[i].inode_table_start);
           
        }
        
        //write to group csv file
        dprintf(group_fd, "%d,", blocks_per_group);
        dprintf(group_fd, "%d,", bgroups[i].free_blocks);
        dprintf(group_fd, "%d,", bgroups[i].free_inodes);
        dprintf(group_fd, "%d,", bgroups[i].num_directories);
        dprintf(group_fd, "%x,", bgroups[i].free_inode_bitmap);
        dprintf(group_fd, "%x,", bgroups[i].free_block_bitmap);
        dprintf(group_fd, "%x\n", bgroups[i].inode_table_start);

    }
    close(group_fd);

    
    //************************************* FREE BITMAP ENTRY **************************************

    //create csv file
    //I used fopen and fprintf instead of open and dprintf here because the it was way faster
    FILE * bitmap_f;
   
    bitmap_f = fopen("bitmap.csv", "w+");

    //iterate through groups
    for(int i = 0; i<num_groups; i++){

        //make sure free block bitmap is not out of group range
        if (i*blocks_per_group <= bgroups[i].free_block_bitmap && bgroups[i].free_block_bitmap < (i+1)*blocks_per_group){
        //iterate through each byte in inode and block bitmap
        for(int j = 0; j<block_size; j++){
             
                pread(img_fd, &b8, 1, (bgroups[i].free_block_bitmap * block_size) + j);
                unsigned int m = 1;
                //iterate through each bit
                for(int k=0; k<8; k++){
               
                    if((b8&m)==0){
                       
                        fprintf(bitmap_f, "%x,", bgroups[i].free_block_bitmap);
                        fprintf(bitmap_f, "%d\n", k + 1 + j*8 + i*blocks_per_group);

                    }
                   
                    m = m << 1;

                }

        }   
        }
        
        //make sure free inode bitmap is in group range
        if (i*blocks_per_group <= bgroups[i].free_inode_bitmap && bgroups[i].free_inode_bitmap < (i+1)*blocks_per_group){
        for(int j = 0; j<inodes_per_group/8; j++){
               
                pread(img_fd, &b8, 1, (bgroups[i].free_inode_bitmap * block_size) + j);
                unsigned int m = 1;
                //iterate through each bit
                for(int k=0; k<8; k++){
                
                    if((b8&m)==0){
                        fprintf(bitmap_f, "%x,", bgroups[i].free_inode_bitmap);
                        fprintf(bitmap_f, "%d\n", k + 1 + j*8 + i*inodes_per_group);

                    }
                   
                    m = m << 1;

                }

        }  
        }
    }
    fclose(bitmap_f);

    //************************************ INODE INFO *************************************
    
    //create csv
    inode_fd = creat("inode.csv", S_IRWXU);

        //Create space for directories
        directories = malloc(inode_count* sizeof(struct dirinfo_t));
        d_index = 0;

        //Create space for used inodes
        used_inodes = malloc(inode_count* sizeof(int));
        i_index = 0;

        //parse inode bitmaps
        for(int i =0; i < num_groups; i++){


            //make sure inode tables is within the correct group block range
            if (i*blocks_per_group > bgroups[i].inode_table_start && bgroups[i].inode_table_start >= (i+1)*blocks_per_group){
                continue;
            }
            //parse each byte
            for(int j =0; j<inodes_per_group/8; j++){

                pread(img_fd, &b8, 1, bgroups[i].free_inode_bitmap * block_size + j);
                int m = 1;

                //parse each byte
                for(int k = 0; k<8; k++){
                    
                    //read valid inodes off inode_table
                    if(b8 & m){

                        int id = i*inodes_per_group + j*8 + k + 1;
                        dprintf(inode_fd, "%d,", id);

                        //add inode to used inode array
                        used_inodes[i_index] = bgroups[i].inode_table_start * block_size + (j*8 + k)* 128;
                        i_index++;

                        pread(img_fd, &b16, 2, bgroups[i].inode_table_start * block_size + (j*8 + k)* 128);

                        if(b16 & 0x8000){
                            dprintf(inode_fd, "f,");
                        }

                        else if (b16 & 0xA000){
                            dprintf(inode_fd, "s,");
                        }

                        //add directories to directories array
                        else if (b16 & 0x4000){
                            dprintf(inode_fd, "d,");
                            directories[d_index].inode = bgroups[i].inode_table_start * block_size + (j*8 + k) * 128;
                            directories[d_index].inode_num = id;
                            d_index++;
                        }

                        else{
                            dprintf(inode_fd, "?,");
                        }

                       //mode
                        dprintf(inode_fd, "%o,", b16);

                        //owner
                        pread(img_fd, &b16, 2, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 2);
                        dprintf(inode_fd, "%d,", b16);
                        
                        //group
                        pread(img_fd, &b16, 2, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 24);
                        dprintf(inode_fd, "%d,", b16);
                        
                        //link count
                        pread(img_fd, &b16, 2, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 26);
                        dprintf(inode_fd, "%d,", b16);

                        //creation time 
                        pread(img_fd, &b32, 4, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 12);
                        dprintf(inode_fd, "%x,", b32);

                        //modification time
                        pread(img_fd, &b32, 4, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 16);
                        dprintf(inode_fd, "%x,", b32);
                        
                        //access time
                        pread(img_fd, &b32, 4, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 8);
                        dprintf(inode_fd, "%x,", b32);

                        //file size
                        pread(img_fd, &b32, 4, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 4);
                        dprintf(inode_fd, "%d,", b32);
                        
                        //number of blocks
                        pread(img_fd, &b32, 4, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 28);
                        dprintf(inode_fd, "%d,", b32/(block_size/512));
                        
                        //block pointers
                        for(int c = 0; c<15; c++){
                            pread(img_fd, &b32, 4, bgroups[i].inode_table_start * block_size + (j*8 + k)*128 + 40 + 4*c);
                            
                            if (c<14){
                                dprintf(inode_fd, "%x,",b32);
                            }
                            else{
                                dprintf(inode_fd, "%x", b32);
                            }
                        }
                        dprintf(inode_fd, "\n");
                    }
                        

                         m = m << 1;
                    
                }
            }
        }

    close(inode_fd);

    //************************************** DIRECTORY INFO ****************************************

    //create csv
    directory_fd = creat("directory.csv", S_IRWXU);


    //variables to keep track of values
    int entry_num;
    int block_num;
    int current;
    int name_len;
    int entry_len;
    int file_inode;
    char * name;
    int error_detected;
    
    //iterate through directories
    for(int i = 0; i< d_index; i++){

        entry_num = 0;
        error_detected = 0;

        //iterate through direct blocks
        for(int j =0; j<12; j++){
            if(error_detected){
                break;
            }
            pread(img_fd, &b32, 4, directories[i].inode + 40 + 4*j);
            block_num = b32;
            
            if(block_num > 0){
                current = block_size * block_num;
                while(current < (block_size*block_num + block_size)){
                    
                    //
                    pread(img_fd, &b8, 1, current + 6);
                    name_len = b8;

                    pread(img_fd, &b16, 2, current + 4);
                    entry_len = b16;

                    pread(img_fd, &b32, 4, current);
                    file_inode = b32;

                    //sanity checks
                    if(entry_len<8 || entry_len > 1024){
                        fprintf(stderr, "Inode %d, block %x - bad direct: entry len = %d",directories[i].inode_num,block_num,entry_len);
                        error_detected = 1;
                         break;

                    } 
                    if (name_len + 8 > entry_len){
                        fprintf(stderr, "Inode %d, block %x - bad direct: len = %d, namelen = %d",directories[i].inode_num,block_num,entry_len, name_len);
                        error_detected = 1;
                        break;
                    }
                    if (file_inode > inode_count){
                        fprintf(stderr, "Inode %d, block %x - bad direct: Inode = %d",directories[i].inode_num,block_num,file_inode);
                        error_detected = 1;
                        break;
                    }
                    
                    //skip to next entry if current entry is not valid
                    if(file_inode == 0){
                        current += entry_len;
                        entry_num ++;
                        continue;
                    }

                    //read name
                    name = malloc(name_len * sizeof(char));
                    char buffer;
                    for( int c = 0; c < name_len; c++){
                        pread(img_fd, &buffer, 1, current + 8 + c);
                        name[c] = buffer;
                    }
                    
                    //print gathered values
                    dprintf(directory_fd, "%d,", directories[i].inode_num);

                    dprintf(directory_fd, "%d,", entry_num);

                    dprintf(directory_fd, "%d,", entry_len);

                    dprintf(directory_fd, "%d,", name_len);

                    dprintf(directory_fd, "%d,", file_inode);

                    dprintf(directory_fd, "\"");
                    for ( int c = 0; c < name_len; c++){
                        dprintf(directory_fd, "%c", name[c]);
                    }
                    dprintf(directory_fd, "\"\n");

                    free(name);
                    
                    entry_num++;
                    current += entry_len;
                    
                }
            }
        }

        //read from indirect blocks
        pread(img_fd, &b32, 4, directories[i].inode + 40 + 48);
        block_num = b32;

        if(block_num > 0){
            
            //iterate through each segment of the block
            for(int j = 0; j<block_size/4; j++){

                if(error_detected){
                    break;
                }
                current = block_size* block_num + j*4;
                pread(img_fd, &b32, 4, current);
                int subblock_num = b32;
                if(subblock_num > 0){
                     
                        current = block_size * subblock_num;
                        while(current < (block_size*subblock_num + block_size)){
                    
                         //
                        pread(img_fd, &b8, 1, current + 6);
                        name_len = b8;

                        pread(img_fd, &b16, 2, current + 4);
                        entry_len = b16;

                        pread(img_fd, &b32, 4, current);
                        file_inode = b32;

                        //sanity checks
                        if(entry_len<8 || entry_len > 1024){
                            fprintf(stderr, "Inode %d, block %x - bad direct: entry len = %d",directories[i].inode_num,subblock_num,entry_len);
                            error_detected = 1;
                            break;

                        } 
                        if (name_len + 8 > entry_len){
                            fprintf(stderr, "Inode %d, block %x - bad direct: len = %d, namelen = %d",directories[i].inode_num,subblock_num,entry_len, name_len);
                            error_detected = 1;
                            break;
                        }
                        if (file_inode > inode_count){
                            fprintf(stderr, "Inode %d, block %x - bad direct: Inode = %d",directories[i].inode_num,subblock_num,file_inode);
                            error_detected = 1;
                            break;
                        }
                        //skip to next entry if current entry is not valid
                        if(file_inode == 0){
                            current += entry_len;
                            entry_num ++;
                            continue;
                        }

                        //read name
                        name = malloc(name_len * sizeof(char));
                        char buffer;
                        for( int c = 0; c < name_len; c++){
                        pread(img_fd, &buffer, 1, current + 8 + c);
                        name[c] = buffer;
                        }
                        
                        //print gathered values
                        dprintf(directory_fd, "%d,", directories[i].inode_num);

                        dprintf(directory_fd, "%d,", entry_num);

                        dprintf(directory_fd, "%d,", entry_len);

                        dprintf(directory_fd, "%d,", name_len);

                        dprintf(directory_fd, "%d,", file_inode);

                        dprintf(directory_fd, "\"");
                        for ( int c = 0; c < name_len; c++){
                            dprintf(directory_fd, "%c", name[c]);
                        }
                        dprintf(directory_fd, "\"\n");

                        free(name);
                        
                        entry_num++;
                        current += entry_len;
                        
                    }


                }
                
            }
        }

        //read from double indirect 
        pread(img_fd, &b32, 4, directories[i].inode + 40 + 52);
        block_num = b32;

        if(block_num > 0){
            
            //iterate through each segment of the block
            for(int j = 0; j<block_size/4; j++){

                if(error_detected){
                    break;
                }
                current = block_size* block_num + j*4;
                pread(img_fd, &b32, 4, current);
                int subblock_num = b32;
                if(subblock_num > 0){

                        for(int k = 0; k<block_size /4; k++){
                            current = block_size * subblock_num + k*4;
                            
                            pread(img_fd, &b32, 4, current);
                            int subsubblock_num = b32;

                            if(subsubblock_num != 0){
                                current = subsubblock_num * block_size;
                                while(current < (block_size*subsubblock_num + block_size)){
                                
                                
                                pread(img_fd, &b8, 1, current + 6);
                                name_len = b8;

                                pread(img_fd, &b16, 2, current + 4);
                                entry_len = b16;

                                pread(img_fd, &b32, 4, current);
                                file_inode = b32;

                                //sanity checks
                                if(entry_len<8 || entry_len > 1024){
                                    fprintf(stderr, "Inode %d, block %x - bad direct: entry len = %d",directories[i].inode_num,subsubblock_num,entry_len);

                                } 
                                if (name_len + 8 > entry_len){
                                    fprintf(stderr, "Inode %d, block %x - bad direct: len = %d, namelen = %d",directories[i].inode_num,subsubblock_num,entry_len, name_len);
                                }
                                if (file_inode > inode_count){
                                    fprintf(stderr, "Inode %d, block %x - bad direct: Inode = %d",directories[i].inode_num,subsubblock_num,file_inode);
                                }
                                //skip to next entry if current entry is not valid
                                if(file_inode == 0){
                                    current += entry_len;
                                    entry_num ++;
                                    continue;
                                }

                                //read name
                                name = malloc(name_len * sizeof(char));
                                char buffer;
                                for( int c = 0; c < name_len; c++){
                                pread(img_fd, &buffer, 1, current + 8 + c);
                                name[c] = buffer;
                                }
                                
                                //print gathered values
                                dprintf(directory_fd, "%d,", directories[i].inode_num);

                                dprintf(directory_fd, "%d,", entry_num);

                                dprintf(directory_fd, "%d,", entry_len);

                                dprintf(directory_fd, "%d,", name_len);

                                dprintf(directory_fd, "%d,", file_inode);

                                dprintf(directory_fd, "\"");
                                for ( int c = 0; c < name_len; c++){
                                    dprintf(directory_fd, "%c", name[c]);
                                }
                                dprintf(directory_fd, "\"\n");

                                free(name);
                                
                                entry_num++;
                                current += entry_len;
                                }
                        }
                    }


                }
                
            }
        }


        //Triple indirect blocks

        pread(img_fd, &b32, 4, directories[i].inode + 40 + 56);
        block_num = b32;

        if(block_num > 0){
            
            //iterate through each segment of the block
            for(int j = 0; j<block_size/4; j++){

                if(error_detected){
                    break;
                }

                current = block_size* block_num + j*4;
                pread(img_fd, &b32, 4, current);
                int subblock_num = b32;
                if(subblock_num > 0){

                        for(int k = 0; k<block_size /4; k++){
                            current = block_size * subblock_num + k*4;
                            
                            pread(img_fd, &b32, 4, current);
                            int subsubblock_num = b32;

                            if(subsubblock_num != 0){
                                
                                for(int l = 0; l<block_size/4; l++){
                                    current = subsubblock_num * block_size;
                                    pread(img_fd, &b32, 4, current);
                                    int subsubsubblock_num = b32;
                                    if(subsubsubblock_num > 0){
                                        current = subsubsubblock_num * block_size;
                                    
                                    
                                    while(current < (block_size*subsubsubblock_num + block_size)){
                                    
                                    
                                    pread(img_fd, &b8, 1, current + 6);
                                    name_len = b8;

                                    pread(img_fd, &b16, 2, current + 4);
                                    entry_len = b16;

                                    pread(img_fd, &b32, 4, current);
                                    file_inode = b32;

                                    //sanity checks
                                    if(entry_len<8 || entry_len > 1024){
                                        fprintf(stderr, "Inode %d, block %x - bad direct: entry len = %d",directories[i].inode_num,subsubsubblock_num,entry_len);
                                        error_detected = 1;
                                        break;

                                    } 
                                    if (name_len + 8 > entry_len){
                                        fprintf(stderr, "Inode %d, block %x - bad direct: len = %d, namelen = %d",directories[i].inode_num,subsubsubblock_num,entry_len, name_len);
                                        error_detected = 1;
                                        break;
                                    }
                                    if (file_inode > inode_count){
                                        fprintf(stderr, "Inode %d, block %x - bad direct: Inode = %d",directories[i].inode_num,subsubsubblock_num,file_inode);
                                        error_detected = 1;
                                        break;
                                    }
                                    //skip to next entry if current entry is not valid
                                    if(file_inode == 0){
                                        current += entry_len;
                                        entry_num ++;
                                        continue;
                                    }

                                    //read name
                                    name = malloc(name_len * sizeof(char));
                                    char buffer;
                                    for( int c = 0; c < name_len; c++){
                                    pread(img_fd, &buffer, 1, current + 8 + c);
                                    name[c] = buffer;
                                    }
                                    
                                    //print gathered values
                                    dprintf(directory_fd, "%d,", directories[i].inode_num);

                                    dprintf(directory_fd, "%d,", entry_num);

                                    dprintf(directory_fd, "%d,", entry_len);

                                    dprintf(directory_fd, "%d,", name_len);

                                    dprintf(directory_fd, "%d,", file_inode);

                                    dprintf(directory_fd, "\"");
                                    for ( int c = 0; c < name_len; c++){
                                        dprintf(directory_fd, "%c", name[c]);
                                    }
                                    dprintf(directory_fd, "\"\n");

                                    free(name);
                                    
                                    entry_num++;
                                    current += entry_len;
                                }
                        }
                    }
                 }
                }
            }
                
            }
        }
    }
    close(directory_fd);
    //******************************** INDIRECT BLOCK INFO **********************************
    
    indirect_fd = creat("indirect.csv", S_IRWXU);

    int subblock_num;

    //single inidirect blocks
    for(int i = 0; i< i_index; i++){
        
        entry_num = 0;
        pread(img_fd, &b32, 4, used_inodes[i] + 40 + 48);
        block_num = b32;

        for(int j = 0; j< block_size/4; j++){
            pread(img_fd, &b32, 4, block_num * block_size + j*4);
            subblock_num = b32;

            //sanity check
             if(subblock_num > block_count){
                fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x", block_num, entry_num, subblock_num);
                continue;
            }
            if(subblock_num > 0){

                 

                dprintf(indirect_fd, "%x,", block_num);

                dprintf(indirect_fd, "%d,", entry_num);

                dprintf(indirect_fd, "%x\n", subblock_num);
                entry_num++;
            }
        }
    

    //double indirect blocks
    entry_num = 0;
    pread(img_fd, &b32, 4, used_inodes[i] + 40 + 52);
    block_num = b32;

    for(int j = 0; j<block_size/4; j++){
        pread(img_fd, &b32, 4, block_num * block_size + j * 4);

        subblock_num = b32;
        if(subblock_num > 0){
            entry_num = 0;
            for(int k = 0; k<block_size; k++){
                pread(img_fd, &b32, 4, subblock_num * block_size + k*4);
                int subsubblock_num = b32;

                //sanity check
                if(subblock_num > block_count){
                fprintf(stderr, "Indirect block %x - invalid entry[%d] = %x", block_num, entry_num, subblock_num);
                continue;
                }
                if(subsubblock_num>0){
                      //WHEN I PRINT DOUBLE INDIRECT BLOCKS MY OUTPUT NO LONGER MATCHES  
                    //dprintf(indirect_fd, "%x,", subblock_num);

                   // dprintf(indirect_fd, "%d,", entry_num);
                    //dprintf(indirect_fd, "%x\n", subsubblock_num);
                    entry_num++;
                }
            }
        }
    }

    //triple indirect blocks
     entry_num = 0;
    pread(img_fd, &b32, 4, used_inodes[i] + 40 + 56);
    block_num = b32;

    for(int j = 0; j<block_size/4; j++){
        pread(img_fd, &b32, 4, block_num * block_size + j * 4);

        subblock_num = b32;
        if(subblock_num > 0){
            entry_num = 0;
            for(int k = 0; k<block_size; k++){
                pread(img_fd, &b32, 4, subblock_num * block_size + k*4);
                int subsubblock_num = b32;
                if (subsubblock_num > 0){
                    entry_num=0;
                    for(int l = 0; l< block_size; l++){
                            pread(img_fd, &b32, 4, subsubblock_num * block_size + l*4);
                            int subsubsubblock_num = b32;
                            
                            if(subsubsubblock_num>0){
                                //WHEN I PRINT TRIPLE INDIRECT BLOCKS MY OUTPUT NO LONGER MATCHES  
                                //dprintf(indirect_fd, "%x,", subsubblock_num);

                               // dprintf(indirect_fd, "%d,", entry_num);
                               // dprintf(indirect_fd, "%x\n", subsubsubblock_num);
                                entry_num++;
                            }



                    }
                }
             
                }
                            
            }
        }
    }

}