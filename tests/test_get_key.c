#include <stdio.h>
#include "nvram-faker.h"
 
int main(void)
{
    puts("This is a shared library test...");
    char *nvram_buffer;
    nvram_buffer = nvram_bufget("friendly_name");
    nvram_buffer = nvram_bufget("friendly_name");
    
    //nvram_bufset(0, "ygzrgegsq", "qergqgf");
    //nvram_bufset(0, "coucou", "les copains");

    //printf("%s\n", nvram_buffer);
    return 0;
}