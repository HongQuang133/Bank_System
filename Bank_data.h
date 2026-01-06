#pragma once

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>


#define SHM_NAME "/bank_shm_file"
#define SEM_NAME "/bank_semaphore"
#define MAX_USERS 1000
#define MAX_TRANS 10000
#define PORT 8080


enum Trans_type{
    T_DEPOSIT,
    T_WITHDRAW,
    T_TRANSFER,
    T_UNDO
};

enum Request_type {
    REQ_DEPOSIT,
    REQ_WITHDRAW,
    REQ_TRANSFER,
    REQ_UNDO,
    REQ_BALANCE,
    REQ_EXIT
};

struct User {
    int user_id;
    double balance;
};

struct Transaction {
    int trans_id;
    int user_from;
    int user_to;
    double amount;
    time_t timestamp;
    Trans_type type;
};

struct BankSHM{
    User users[MAX_USERS];
    Transaction trans[MAX_TRANS];
    int trans_count;
};
