#include "Bank_data.h"

void init_user() {
    for (int i = 1; i <= MAX_USERS; i++) {
        bank->users[i].user_id = i;
        bank->users[i].balance = 0;
    }
}

void initBankSHM() {

    bool first_time = false;

    bank_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if(bank_fd >= 0) {
        first_time = true;
    }
    else {
        bank_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    }

    ftruncate(bank_fd, sizeof(BankSHM));

    bank = (BankSHM*)mmap(0, sizeof(BankSHM), PROT_READ | PROT_WRITE, MAP_SHARED, bank_fd, 0);

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);

    if(first_time) {
        bank->trans_count = 0;
        init_user();
    }
}


bool init_socket() {

    struct sockaddr_in server_addr;

    sock_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_server_fd < 0) {
        std::cerr << "socket() is failed. \n";
        return false;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(sock_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return false;
    }

    if (bind(sock_server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "bind() is failed. \n";
        return false;
    }

    if (listen(sock_server_fd, 10) < -1) {
        std::cerr << "listen() is failed. \n";
        return false;
    }

    return true;
}   

void add_transaction(int user_id, int user_from, int user_to, int amount, Trans_type type, int undo_target) {
    if (bank->trans_count >= MAX_TRANS) return;

    Transaction &t = bank->trans[bank->trans_count++];
    t.user_id = user_id;
    t.user_from = user_from;
    t.user_to = user_to;
    t.amount = amount;
    t.type = type;
    t.timestamp = time(nullptr);
    t.undone = false;
    t.undo_target = undo_target;
}

void deposit(int user_id, int amount) {
    sem_wait(sem);
    bank->users[user_id].balance += amount;
    add_transaction(user_id, -1, -1, amount, T_DEPOSIT, -1);
    sem_post(sem);
}

bool withdraw(int user_id, int amount) {
    sem_wait(sem);
    if (bank->users[user_id].balance < amount) {
        std::cerr << "Your's balance not insufficent!\n";
        sem_post(sem);
        return false;
    } else {
        bank->users[user_id].balance -= amount;
        add_transaction(user_id, -1, -1, amount, T_WITHDRAW, -1);
        sem_post(sem);
        return true;
    }
}

bool transfer(int user_from, int user_to, int amount) {
    sem_wait(sem);
    if(amount > bank->users[user_from].balance) {
        std::cerr << "Yours's balance insufficient \n";
        sem_post(sem);
        return false;
    } else {
        bank->users[user_from].balance -= amount;
        bank->users[user_to].balance += amount;
        add_transaction(-1, user_from, user_to, amount, T_TRANSFER, -1);
        sem_post(sem);
        return true;
    }
}

bool undo(int user_id) {
    sem_wait(sem);

    for (int i = bank->trans_count -1; i >= 0; i--) {
        Transaction &t = bank->trans[i];
        if(t.type == T_UNDO) continue;
        if(t.undone) continue;

        bool related = (t.user_id == user_id) || (t.user_from == user_id) || (t.user_to == user_id);

        if(!related) continue;

        
        switch(t.type) {

            case T_DEPOSIT: 
                bank->users[user_id].balance -= t.amount;
                break;

            case T_WITHDRAW:
                bank->users[user_id].balance += t.amount;
                break;

            case T_TRANSFER:
                    if (bank->users[t.user_to].balance < t.amount) {
                        sem_post(sem);
                        return false;
                    }
                    bank->users[t.user_from].balance += t.amount;
                    bank->users[t.user_to].balance -= t.amount;
                    break;
            default:
                break;
        }
        t.undone = true;
        add_transaction(t.user_id, t.user_from, t.user_to, t.amount, T_UNDO, i);

        sem_post(sem);
        return true;
    }
    sem_post(sem);
    return false;
}

void print_transaction(const Transaction &t) {
    if(t.type != T_UNDO) {
        switch(t.type) {
            case T_DEPOSIT: 
                std::cout << "DEPOSIT | User: " << t.user_id;
                break;
            case T_WITHDRAW:
                std::cout << "WITHDRAW | User: " << t.user_id;
                break;
            case T_TRANSFER:
                std::cout << "TRANSFER | From User: " << t.user_from << " to User: " << t.user_to;
                break;
            default:
                break;
        }
        std::cout << " | Amount: " << t.amount << " | Time: " << ctime(&t.timestamp);
        return;
    }
    const Transaction &un = bank->trans[t.undo_target];
    std::cout << "UNDO -> ";
    switch(un.type) {
        case T_DEPOSIT: {
            std::cout << "DEPOSIT | User: " << un.user_id;
            break;
        }
        case T_WITHDRAW: {
            std::cout << "WITHDRAW | User: " << un.user_id;
            break;
        }
        case T_TRANSFER: {
            std::cout << "TRANSFER| From User: " << un.user_from << " to User: " << un.user_to;
            break;
         }
        default:
            break;
    }
    std::cout << " | Amount: " << un.amount << " | Original time: " << ctime(&un.timestamp) << " | Undo time: " << ctime(&t.timestamp);
}

void print_user_history(int user_id,int client_fd) {
    sem_wait(sem);
    for (int i = 0; i < bank->trans_count; i++) {
        Transaction &t = bank->trans[i];

        if(t.user_id == user_id || t.user_from == user_id || t.user_to == user_id) {
            std::ostringstream oss;
            oss << "TYPE: ";
            switch (t.type) {

                case T_DEPOSIT: 
                    oss << "DEPOSIT";
                break;

                case T_WITHDRAW:
                    oss << "WITHDRAW"; 
                break;

                case T_TRANSFER:
                    oss << "TRANSFER";
                break;

                case T_UNDO:
                    oss << "UNDO";
                break;
            }

            oss << " | AMOUNT: " << t.amount << "| TIME " << ctime(&t.timestamp);

            write(client_fd, oss.str().c_str(), oss.str().size());
        }
    }
    sem_post(sem);
}

double get_balance(int user_id) {
    double bal = bank->users[user_id].balance;
    return bal;
}

void print_all_transactions() {
    sem_wait(sem);
    std::cout << "\n ====== ALL TRANSACTION ======\n";
    
    for (int i = 0; i < bank->trans_count; i++) {
        Transaction &t = bank->trans[i];
        std::cout << "[ " << i << " ]";

        if(t.type != T_UNDO) {
        switch(t.type) {
            case T_DEPOSIT: 
                std::cout << "DEPOSIT | User: " << t.user_id;
                break;
            case T_WITHDRAW:
                std::cout << "WITHDRAW | User: " << t.user_id;
                break;
            case T_TRANSFER:
                std::cout << "TRANSFER | From User: " << t.user_from << " to User: " << t.user_to;
                break;
            default:
                break;
        }
        std::cout << " | Amount: " << t.amount << " | Time: " << ctime(&t.timestamp);
        continue;
        }
        const Transaction &un = bank->trans[t.undo_target];
        std::cout << "UNDO -> ";
        switch(un.type) {
            case T_DEPOSIT: {
                std::cout << "DEPOSIT | User: " << un.user_id;
                break;
            }
            case T_WITHDRAW: {
                std::cout << "WITHDRAW | User: " << un.user_id;
                break;
            }
            case T_TRANSFER: {
                std::cout << "TRANSFER| From User: " << un.user_from << " to User: " << un.user_to;
                break;
            }
            default:
                break;
        }
    }
    std::cout << "\n ======END====== \n";
    sem_post(sem);
}

void handle_client(int client_fd) {
    char buffer[128] = {0};
    int n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) return;

    buffer[n] = '\0';
  
    int cmd;
    std::istringstream iss(buffer);
    iss >> cmd;
    std::string response;

    switch(cmd) {

        case 1: {
            int user_id, amount;
            iss >> user_id >> amount;
            deposit(user_id, amount);
            Transaction &t = bank->trans[bank->trans_count -1];
            print_transaction(t);
            response = "DEPOSIT SUCCESS\n";
            break;
        }

        case 2: {
            int user_id, amount;
            iss >> user_id >> amount;
            if(withdraw(user_id, amount) == true) {
                Transaction &t = bank->trans[bank->trans_count -1];
                print_transaction(t);
                response = "WITHDRAW SUCCESS\n";
            }
            else {
                response = "WITHDRAW FAILED\n";
            }
            break;
        }

        case 3: {
            int user_from, user_to, amount;
            iss >> user_from >> user_to >> amount;
            if(transfer(user_from, user_to, amount)) {
                Transaction &t = bank->trans[bank->trans_count -1];
                print_transaction(t);
                response = "TRANSFER SUCCESS\n";
            }
            else {
                response = "TRANSFER FAILED\n";
            }
            break;
        }

        case 4: {
            int user_id;
            iss >> user_id;
            if(undo(user_id)) {
                Transaction &t = bank->trans[bank->trans_count -1];
                print_transaction(t);
                response = "UNDO SUCCESS\n";
            }
            else {
                response = "UNDO FAILED\n";
            }
            break;
        }

        case 5: {
            int user_id;
            iss >> user_id;
            std::ostringstream oss;
            double bal = get_balance(user_id);
            oss << "BALANCE: " << bal;
            response = oss.str();
            break;  
        }

        case 6: {
            int user_id;
            iss >> user_id;

            response = "----- TRANSACTION HISTORY ----- \n";
            print_user_history(user_id, client_fd);
            break;
        }
      
    }
    write(client_fd, response.c_str(), response.size());
}

void cleanup(int) {
    std::cout <<"\n Server shutting down...\n";
    munmap(bank, sizeof(BankSHM));
    close(bank_fd);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    close(sock_server_fd);
    exit(0);
}

void sig_print_all(int) {
    print_all_transactions();
}

int main() {
    signal(SIGINT, cleanup);
    signal(SIGUSR1, sig_print_all);
    initBankSHM();
    if(!init_socket()) {
        std::cerr << "Socket init failed. Exit. \n";
        exit(0);
    }
    
    std::cout << "Bank server running...\n";

    while(1){
        int client_fd = accept(sock_server_fd, nullptr, nullptr);
        if(client_fd < 0) {
            std::cerr << "accept() failed. \n";
            return -1;
        }

        handle_client(client_fd);
        close(client_fd);
    }

    return 0;
}

