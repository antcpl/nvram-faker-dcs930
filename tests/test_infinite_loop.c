#include <stdio.h>
#include "nvram-faker.h"
 
int main(void)
{
    puts("This is a shared library test...");
    char *nvram_buffer;
    while(1){
        nvram_buffer = nvram_bufget("friendly_name");
    }
    //printf("%s\n", nvram_buffer);
    return 0;
}