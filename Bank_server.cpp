#include "Bank_data.h"

class Sem_manager {
    private:
        sem_t* sem;
    public:

        Sem_manager() {
            sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
            if (sem == SEM_FAILED){
                std::cerr << "Failed open semaphore. \n";
            }
        }

        ~Sem_manager() {
            sem_close(sem);
        }

        void lock() {
            sem_wait(sem);
        }

        void unlock() {
            sem_post(sem);
        }
};

class BankServer {
    private: 
        BankSHM *bank;
        Sem_manager &sem;
        int shm_fd;
        int sock_fd;

    public:
        void init_SHM();
        void initUsers();
        void initSocket();

    private:
        void init_SHM(){
            shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);

            if (shm_fd < 0) {
                std::cerr << "shm_open() failed. \n";
            }

            ftruncate(shm_fd, sizeof(BankSHM));

            bank = (BankSHM*)mmap(0, sizeof(BankSHM), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

            if (bank->trans_count == 0) {
                bank->trans_count =0;
            }
        }

        void initUsers() {
            for(int i  = 0; i < MAX_USERS; i++) {
                bank->users[i].user_id = i;
                bank->users[i].balance = 0;
            }
        }

        void initSocket() {
            sock_fd = socket(AF_INET, SOCK_STREAM, 0);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(PORT);

            bind(sock_fd, (sockaddr*)&addr, sizeof(addr));
            listen(sock_fd, 10);
        }
        
};

class BankService {
    private:
        BankSHM *bank;
        Sem_manager &sem;
        Transaction *t;

    public:
        BankService(BankSHM *bank, Sem_manager &sem, Transaction) : bank(bank), sem(sem){}

        bool deposit(int user_id, int amount) {
            sem.lock();
            bank->users[user_id].balance += amount;
            addTransaction(user_id, amount, bank->trans[user_id].type = T_DEPOSIT);
            sem.unlock();
            return true;
        }
        
        bool withdraw(int user_id, int amount) {
            sem.lock();
            if(amount > bank->users[user_id].balance) {
                return false;
            }
            else {
                bank->users[user_id].balance -= amount;
                addTransaction(user_id, amount, bank->trans[user_id].type = T_WITHDRAW);
            }
            sem.unlock();
        }

    private:
        void addTransaction(int user_id, int amount, int type) {
            if(bank->trans_count >= MAX_TRANS) return;
            Transaction &t = bank->trans[bank->trans_count++];
            t = {user_id, amount, type, time_t(nullptr), false};
        }
};


