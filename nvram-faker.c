#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

#include "nvram-faker.h"
//include before ini.h to override ini.h defaults
#include "nvram-faker-internal.h"
#include "ini.h"

#define RED_ON "\033[22;31m"
#define RED_OFF "\033[22;00m"
#define GREEN_ON "\033[22;32m"
#define GREEN_OFF "\033[22;00m"
#define DEFAULT_KV_PAIR_LEN 1024

static int kv_count=0;
static int key_value_pair_len=DEFAULT_KV_PAIR_LEN;
static char **key_value_pairs=NULL;

static int ini_handler(void *user, const char *section, const char *name,const char *value)
{

    int old_kv_len;
    char **kv;
    char **new_kv;
    int i;
    
    if(NULL == user || NULL == section || NULL == name || NULL == value)
    {
        DEBUG_PRINTF("bad parameter to ini_handler\n");
        return 0;
    }
    kv = *((char ***)user);
    if(NULL == kv)
    {
        DEBUG_PRINTF("kv is NULL\n");
        return 0;
    }
    
    // tricky way of working, for multiline values, ini_parse_file call ini handler one time per line with the same key each time. 
    // from the index 2 we have to check if the precedent key has the same value as the one we are working on this iteration
    if(kv_count>=2){
        // if the current key equals to the precedent key, then we have to treat the multi line
        if(strcmp(kv[kv_count-2],name)==0){
            // if the precedent value didn't has \n then we have to allocate 3 bytes more to add this \n
            if(kv[kv_count-1][strlen(kv[kv_count-1])-1]!='\n'){
                int new_size = strlen(kv[kv_count-1])+strlen(value)+3; 
                char * old_value = kv[kv_count-1];
                kv[kv_count-1] = malloc(new_size);
                char * first_line ="\n\0";
                strcpy(kv[kv_count-1],old_value);
                strcat(kv[kv_count-1],first_line);
                strcat(kv[kv_count-1],value);
                char * return_line = "\n\0";
                strcat(kv[kv_count-1],return_line);
                free(old_value);
            }
            //if the precedent value already has a \n then only need to allocate 2 bytes more for the \n\0
            else
            {
                int new_size = strlen(kv[kv_count-1])+strlen(value)+2;
                char * old_value = kv[kv_count-1];
                kv[kv_count-1] = malloc(new_size);
                strcpy(kv[kv_count-1],old_value);
                strcat(kv[kv_count-1],value);
                char * return_line = "\n\0";
                strcat(kv[kv_count-1],return_line);
                free(old_value);
            }
        }
        // if the array isn't big enough, we allocate more space 
        else if(kv_count >= key_value_pair_len)
        {
            old_kv_len=key_value_pair_len;
            key_value_pair_len=(key_value_pair_len * 2);
            new_kv=(char **)malloc(key_value_pair_len * sizeof(char **));
            if(NULL == new_kv)
            {
                DEBUG_PRINTF("Failed to reallocate key value array.\n");
                return 0;
            }
            for(i=0;i<old_kv_len;i++)
            {
                new_kv[i]=kv[i];
            }
            free(*(char ***)user);
            kv=new_kv;
            *(char ***)user=kv;
            kv[kv_count++]=strdup(name);
            kv[kv_count++]=strdup(value);
        }
        //otherwise just a new value to add to the array 
        else{
            kv[kv_count++]=strdup(name);
            kv[kv_count++]=strdup(value);
        }
    }
    else{
        kv[kv_count++]=strdup(name);
        kv[kv_count++]=strdup(value);
    }
   
    return 1;
}

void initialize_ini(void)
{
    int ret;
    DEBUG_PRINTF("Initializing.\n");
    if (NULL == key_value_pairs)
    {
        key_value_pairs=malloc(key_value_pair_len * sizeof(char **));
    }
    if(NULL == key_value_pairs)
    {
        DEBUG_PRINTF("Failed to allocate memory for key value array. Terminating.\n");
        exit(1);
    }
    
    ret = ini_parse(INI_FILE_PATH,ini_handler,(void *)&key_value_pairs);
    if (0 != ret)
    {
        DEBUG_PRINTF("ret from ini_parse was: %d\n",ret);
        DEBUG_PRINTF("INI parse failed. Terminating\n");
        free(key_value_pairs);
        key_value_pairs=NULL;
        exit(1);
    }else
    {
        DEBUG_PRINTF("ret from ini_parse was: %d\n",ret);
    }
    
    return;
}


char *nvram_bufget(int useless, char *key)
{
    int i;
    int found=0;
    char *value;
    char *ret;

    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_bufget called with key : %s\n"RED_OFF, key);

    pid_t ppid = getppid();
    if(ppid!=-1){
        DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_bufget called by process : %d\n"RED_OFF, ppid);
    }
    
    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Opening the file %s\n"RED_OFF, INI_FILE_PATH);

    int fd = open(INI_FILE_PATH, O_RDONLY);
    if (fd == -1) {
        printf("Fail open the file to lock it at location : %s\n", INI_FILE_PATH);
        return NULL;
    }

    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Locking the file %s\n"RED_OFF, INI_FILE_PATH);

    if (flock(fd, LOCK_EX) == -1) {
        perror("Fail to lock the file");
        close(fd);
        return NULL;
    }

    //to test if the file lock is working correclty
    // while(1){
    //     printf("I locked the file\n");
    // }

    DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : initializing the memory map and the key research\n"RED_OFF);
    //call to initialize ini to get the latest file version
    initialize_ini();

    //from now on the key value pair array is in our memory 

    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Unlocking the file %s\n"RED_OFF, INI_FILE_PATH);
    if (flock(fd, LOCK_UN) == -1) {
        perror("Fail to unlock the file");
        close(fd);
        return NULL;
    }

    close(fd);

    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Iterating over the structure to look for the key \n"RED_OFF);

    //we can now make a get and try to find the key we want
    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            DEBUG_PRINTF("%s=%s\n",key,key_value_pairs[i+1]);
            found = 1;
            value=key_value_pairs[i+1];
            break;
        }
    }

    ret = NULL;
    if(!found)
    {
            DEBUG_PRINTF( RED_ON"%s=Unknown\n"RED_OFF,key);
    }else
    {
            ret=strdup(value);
    }

    // freeing the space of the array 
    for(i=0;i<(DEFAULT_KV_PAIR_LEN/2)-1;i++){
        free(key_value_pairs[i]);
        free(key_value_pairs[i+1]);
        key_value_pairs[i]=NULL;
        key_value_pairs[i+1]=NULL;
    }
    free(key_value_pairs);
    key_value_pairs=NULL;
    kv_count=0;

    return ret;
}


void nvram_bufset(int useless,char *key,char *value)
{
    int i=0;
    int found=0;
    char **new_kv;
    int old_kv_len;

    pid_t ppid = getppid();
    if(ppid!=-1){
        DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_bufset called by process : %d\n"RED_OFF, ppid);
    }
    
    //locking the file
    int fd = open(INI_FILE_PATH, O_RDONLY);
    if (fd == -1) {
        printf("Fail open the file to lock it at location : %s\n", INI_FILE_PATH);
        return;
    }
    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Opening the file %s\n"RED_OFF, INI_FILE_PATH);

    if (flock(fd, LOCK_EX) == -1) {
        perror("Fail to lock the file");
        close(fd);
        return;
    }
    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Locking the file %s\n"RED_OFF, INI_FILE_PATH);

    DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : initializing the memory map and the key research\n"RED_OFF);
    //call to initialize ini to get the latest file version
    initialize_ini();
    //from now on the key value pair array is in our memory 

    DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : iterating over the structure to find if the key is already present\n"RED_OFF);
    //we can now make a get and try to find the key we want
    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            DEBUG_PRINTF(GREEN_ON"%s=%s\n"GREEN_OFF,key,key_value_pairs[i+1]);
            found = 1;
            break;
        }
    }

    // if the key has not been found then we have to add the value 
    if(!found)
    {
        DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : The key has not been found\n"RED_OFF);
        // check to see if we have enough space
        if(kv_count >= key_value_pair_len)
        {
            old_kv_len=key_value_pair_len;
            key_value_pair_len=(key_value_pair_len * 2);
            new_kv=(char **)malloc(key_value_pair_len * sizeof(char **));
            if(NULL == new_kv)
            {
                DEBUG_PRINTF("Failed to reallocate key value array.\n");
                return;
            }
            for(i=0;i<old_kv_len;i++)
            {
                new_kv[i]=key_value_pairs[i];
            }
            free(key_value_pairs);
            key_value_pairs=new_kv;
        }
        key_value_pairs[kv_count++]=strdup(key);
        key_value_pairs[kv_count++]=strdup(value);

    }
    else
    {
        DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : The key has been found\n"RED_OFF);
        //this is the easy way, OK for non multiline values
        char * old_ptr=key_value_pairs[i+1];
        key_value_pairs[i+1]=strdup(value);
        free(old_ptr);
    
        
        DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : We replaced the old value by the new one\n"RED_OFF);
    }


    // We just modified the array in the memory of the process, now we have to do the same in the file 
    FILE* file;

    DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : Opening the file again to get another type of file descriptor\n"RED_OFF);

    file = fopen(INI_FILE_PATH, "w");
    if (!file)
        return ;
    
    // here we need a special format for the multiline values
    for(i=0; i<kv_count; i+=2){
        // get the first occurency of the \n
        char * ptr1=strchr(key_value_pairs[i+1], '\n');
        // get the last occurency of the \n
        char * ptr2=strrchr(key_value_pairs[i+1], '\n');

        // if both are different, then we have a multi line value
        if(ptr1!=NULL && ptr2!= NULL && ptr1!=ptr2  ){
        //this is the hard path, we have a multi line value
        //we must add a space at the begining of each line for the parser to work on the next parsing
            char *line = strtok(key_value_pairs[i+1], "\n");
            //spare the multi line values into one line pieces, add a space at the beginning and repeat until the end
            fprintf(file, "%s:%s\n",key_value_pairs[i],line);
            while(line!=NULL){
                line = strtok(NULL, "\n");
                if(line!=NULL)  fprintf(file," %s\n",line);
            }
        }
        else
        {
            //this is the easy way no multi line value 
            fprintf(file, "%s:%s\n",key_value_pairs[i],key_value_pairs[i+1]);
        }
    }

    fclose(file);
    DEBUG_PRINTF(RED_ON"[+] NVRAM_DEBUG : %s file modified\n"RED_OFF,INI_FILE_PATH);


    //unlocking the file 
    if (flock(fd, LOCK_UN) == -1) {
        perror("Fail to unlock the file");
        close(fd);
        return;
    }
    close(fd);
    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Unlocking the file %s\n"RED_OFF, INI_FILE_PATH);


    for(i=0;i<(DEFAULT_KV_PAIR_LEN/2)-1;i++){
        free(key_value_pairs[i]);
        free(key_value_pairs[i+1]);
        key_value_pairs[i]=NULL;
        key_value_pairs[i+1]=NULL;
    }
    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : we are out of the loop\n"RED_OFF);
    free(key_value_pairs);
    key_value_pairs=NULL;
    kv_count=0;

    DEBUG_PRINTF(RED_ON"[+] NVRAM DEBUG : Every data has been freed\n"RED_OFF);

    return; 
}

void nvram_get(int useless, char * key){return; }
void nvram_set(int useless, char * key){return; }
void nvram_close(int useless){return; }
void nvram_commit(int useless){return; }

void nvram_init(int useless){   

    pid_t ppid = getppid();
    if(ppid!=-1){
            LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_init called from  \n"RED_OFF);
    } 
    return; 
}
void nvram_clear(int useless){
    pid_t ppid = getppid();
    if(ppid!=-1){
            LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_clear called from  \n"RED_OFF);
    } 
    return; 
}

void nvram_renew(int useless, char * file){
    pid_t ppid = getppid();
    if(ppid!=-1){
            LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_renew called from  \n"RED_OFF);
    } 

    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_renew called for file %s\n"RED_OFF, file);

    return; 
}

