#include <stdio.h>
#include "nvram-faker.h"
 
int main(void)
{
    puts("This is a shared library test...");
    char *nvram_buffer;
    //nvram_buffer = nvram_bufget("friendly_name");
    nvram_bufset(0, "coucou", "les petits loups");
    nvram_bufset(0, "upnp_turn_on", "abcdef");
    nvram_bufset(0, "azezrgeshg", "qzregerg");


    //printf("%s\n", nvram_buffer);
    return 0;
}