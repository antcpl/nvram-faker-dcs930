#include <stdio.h>
#include "nvram-faker.h"
 
int main(void)
{
    puts("This is a shared library test...");
    char *nvram_buffer;
    //nvram_buffer = nvram_bufget("friendly_name");
    nvram_bufset(0, "upnp_turn_on", "1000000");
    //nvram_bufset(0, "coucou", "les copains");

    //printf("%s\n", nvram_buffer);
    return 0;
}