#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

// Authors : completely inspired from the zcutlip and decidedlygray very good project which can be found here : https://github.com/zcutlip/nvram-faker

// Modified to fit the needs of our project by me and colleagues : Antoine CHAPEL, Enzo MAZZOLENI, Loup LOBET 

// Fitted to match exactly the DCS-93X D-Link nvram (tested and working from DCS-932 to DCS-934)

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

//called at each line by ini_parse_file in the ini project
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
        LOG_PRINTF("kv is NULL\n");
        return 0;
    }
    
    // this condition is to manage two things : increase the array size when needed and manage the multi-line values in nvram 
    if(kv_count>=2){ 
        if(strcmp(kv[kv_count-2],name)==0){ 
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
            }else
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
        else if(kv_count >= key_value_pair_len)
        {
            old_kv_len=key_value_pair_len;
            key_value_pair_len=(key_value_pair_len * 2);
            new_kv=(char **)malloc(key_value_pair_len * sizeof(char **));
            if(NULL == new_kv)
            {
                LOG_PRINTF("Failed to reallocate key value array.\n");
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
        LOG_PRINTF("Failed to allocate memory for key value array. Terminating.\n");
        exit(1);
    }
    
    ret = ini_parse(INI_FILE_PATH,ini_handler,(void *)&key_value_pairs);
    if (0 != ret)
    {
        LOG_PRINTF("ret from ini_parse was: %d\n",ret);
        LOG_PRINTF("INI parse failed. Terminating\n");
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

    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_bufget called with key : %s\n"RED_OFF, key);

    pid_t ppid = getppid();
    if(ppid!=-1){
        LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_bufget called by process : %d\n"RED_OFF, ppid);
    }
    
    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Opening the file %s\n"RED_OFF, INI_FILE_PATH);

    int fd = open(INI_FILE_PATH, O_RDONLY);
    if (fd == -1) {
        printf("Fail open the file to lock it at location : %s\n", INI_FILE_PATH);
        return NULL;
    }

    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Locking the file %s\n"RED_OFF, INI_FILE_PATH);

    if (flock(fd, LOCK_EX) == -1) {
        perror("Fail to lock the file");
        close(fd);
        return NULL;
    }

    //to test if the file lock is working correclty
    // while(1){
    //     printf("I locked the file\n");
    // }

    LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : initializing the memory map and the key research\n"RED_OFF);
    //call to initialize ini to get the latest file version
    initialize_ini();
    //from now on the key value pair array is in our memory 


    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Unlocking the file %s\n"RED_OFF, INI_FILE_PATH);
    if (flock(fd, LOCK_UN) == -1) {
        perror("Fail to unlock the file");
        close(fd);
        return NULL;
    }

    close(fd);

    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Iterating over the structure to look for the key \n"RED_OFF);

    //we can now make a get and try to find the key we want
    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            LOG_PRINTF("%s=%s\n",key,key_value_pairs[i+1]);
            found = 1;
            value=key_value_pairs[i+1];
            break;
        }
    }

    ret = NULL;
    if(!found)
    {
            LOG_PRINTF( RED_ON"%s=Unknown\n"RED_OFF,key);
    }else
    {
            ret=strdup(value);
    }


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
        LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : nvram_bufset called by process : %d\n"RED_OFF, ppid);
    }
    
    //locking the file
    int fd = open(INI_FILE_PATH, O_RDONLY);
    if (fd == -1) {
        printf("Fail open the file to lock it at location : %s\n", INI_FILE_PATH);
        return;
    }
    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Opening the file %s\n"RED_OFF, INI_FILE_PATH);

    if (flock(fd, LOCK_EX) == -1) {
        perror("Fail to lock the file");
        close(fd);
        return;
    }
    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Locking the file %s\n"RED_OFF, INI_FILE_PATH);

    LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : initializing the memory map and the key research\n"RED_OFF);
    //call to initialize ini to get the latest file version
    initialize_ini();
    //from now on the key value pair array is in our memory 

    LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : iterating over the structure to find if the key is already present\n"RED_OFF);
    //we can now make a get and try to find the key we want
    for(i=0;i<kv_count;i+=2)
    {
        if(strcmp(key,key_value_pairs[i]) == 0)
        {
            LOG_PRINTF(GREEN_ON"%s=%s\n"GREEN_OFF,key,key_value_pairs[i+1]);
            found = 1;
            break;
        }
    }

    if(!found)
    {
        LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : The key has not been found\n"RED_OFF);
        if(kv_count >= key_value_pair_len)
        {
            old_kv_len=key_value_pair_len;
            key_value_pair_len=(key_value_pair_len * 2);
            new_kv=(char **)malloc(key_value_pair_len * sizeof(char **));
            if(NULL == new_kv)
            {
                LOG_PRINTF("Failed to reallocate key value array.\n");
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
        LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : The key has been found\n"RED_OFF);
        //this is the easy way, OK for non multiline values
        char * old_ptr=key_value_pairs[i+1];
        key_value_pairs[i+1]=strdup(value);
        free(old_ptr);
    
        
        LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : We replaced the old value by the new one\n"RED_OFF);
    }

    FILE* file;

    LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : Opening the file again to get another type of file descriptor\n"RED_OFF);

    file = fopen(INI_FILE_PATH, "w");
    if (!file)
        return ;
    
    for(i=0; i<kv_count; i+=2){
        char * ptr1=strchr(key_value_pairs[i+1], '\n');
        char * ptr2=strrchr(key_value_pairs[i+1], '\n');
        // LOG_PRINTF(GREEN_ON"[+] NVRAM DEBUG : %p \n"GREEN_OFF, ptr1);
        // LOG_PRINTF(GREEN_ON"[+] NVRAM DEBUG : %p \n"GREEN_OFF, ptr2);

        if(ptr1!=NULL && ptr2!= NULL && ptr1!=ptr2){
            // LOG_PRINTF(GREEN_ON"[+] NVRAM DEBUG : I am the holy condition \n"GREEN_OFF);
        //this is the hard path, we have a multi line value
        //we must add a space at the begining of each line for the parser to work 
            char *line = strtok(key_value_pairs[i+1], "\n");
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
    LOG_PRINTF(RED_ON"[+] NVRAM_DEBUG : %s file modified\n"RED_OFF,INI_FILE_PATH);


    //unlocking the file 
    if (flock(fd, LOCK_UN) == -1) {
        perror("Fail to unlock the file");
        close(fd);
        return;
    }
    close(fd);
    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Unlocking the file %s\n"RED_OFF, INI_FILE_PATH);


    for(i=0;i<(DEFAULT_KV_PAIR_LEN/2)-1;i++){
        //LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Here is the freeing counter %d\n"RED_OFF,i);
        free(key_value_pairs[i]);
        free(key_value_pairs[i+1]);
        key_value_pairs[i]=NULL;
        key_value_pairs[i+1]=NULL;
    }
    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : we are out of the loop\n"RED_OFF);
    free(key_value_pairs);
    key_value_pairs=NULL;
    kv_count=0;

    LOG_PRINTF(RED_ON"[+] NVRAM DEBUG : Every data has been freed\n"RED_OFF);

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

