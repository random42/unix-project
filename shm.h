#define SHM_H


void shm_push(void* shmptr, person* p);
void shm_pop(void* shmptr, pid_t pid);
void print_valid_shm(void* shmptr);
void print_all_shm(void* shmptr);
