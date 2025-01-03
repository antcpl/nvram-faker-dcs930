#ifndef __NVRAM_FAKER_H__
#define __NVRAM_FAKER_H__

char *nvram_bufget(int useless, char *key);
void nvram_bufset(int useless,char *key, char *value);
void nvram_get(int useless, char * key);
void nvram_set(int useless, char * key);
void nvram_close(int useless);
void nvram_commit(int useless);
void nvram_init(int useless);
void nvram_clear(int useless);

#endif /* __NVRAM_FAKER_H__ */
