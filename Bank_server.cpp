#include "Bank_data.h"

BankSHM* init_shm() {
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR | O_EXCL, 0666);
    if(fd = -1) {
        perror("shm_open (create)");
        return nullptr;
    }

    if (ftruncate(fd, sizeof(BankSHM)) == -1) {
        perror("ftruncate");
        close(fd);
        return nullptr;
    }

    BankSHM* shm = (BankSHM*)mmap(nullptr, sizeof(BankSHM), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (shm == MAP_FAILED) {
        perror("mmap");
        return nullptr;
    }

    for (int i = 0; i < MAX_USERS; i++) {
        shm->users[i].user_id = i;
        shm->users[i].balance = 0;
    }

    shm->trans_count = 0;
    
    std::cout << "Server: Shared memory initialized. \n";
    return shm;
}

sem_t* create_semaphore() {
    sem_t* sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);

    if (sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    std::cout << "Server: Semaphore created. \n";
    return sem;
}


