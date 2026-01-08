#include "Bank_data.h"

int connect_to_server() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        std::cerr << "socket() failed\n";
        return -1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "connect() to server failed\n";
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

/* ===== Gửi request & nhận response ===== */
void send_request(const std::string& request) {
    int sock_fd = connect_to_server();
    if (sock_fd < 0) return;

    send(sock_fd, request.c_str(), request.size(), 0);

    char buffer[128] = {0};
    int n;
    while ((n = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[n] = '\0';
        std::cout << buffer;
    }
    close(sock_fd);
}

/* ===== Các chức năng ===== */

void deposit_menu(int user_id) {
    int amount;
    std::cout << "Amount: ";
    std::cin >> amount;

    std::ostringstream oss;
    oss << "1 " << user_id << " " << amount;

    send_request(oss.str());
}

void withdraw_menu(int user_id) {
    int amount;
    std::cout << "Amount: ";
    std::cin >> amount;

    std::ostringstream oss;
    oss << "2 " << user_id << " " << amount;

    send_request(oss.str());
}

void transfer_menu(int user_from) {
    int user_to, amount;
    std::cout << "to User: ";
    std::cin >> user_to;
    while(user_to <= 0 || user_to > 1000) {
        std::cout << "Invalid User. Please try again.\n";
        std::cout << "to User: ";
        std::cin >> user_to;
    }
    std::cout << "Amount: ";
    std::cin >> amount;
    
    std::ostringstream oss;
    oss << "3 " << user_from << " " << user_to << " " << amount;

    send_request(oss.str());
}

void undo_menu(int user_id) {
    std::ostringstream oss;
    oss << "4 " << user_id;
    send_request(oss.str());
}

void balance_menu(int user_id) {
    std::ostringstream oss;
    oss << "5 " << user_id;
    send_request(oss.str());
}
void history_menu(int user_id) {
    std::ostringstream oss;
    oss << "6 " << user_id;
    send_request(oss.str());
}

/* ===== Hiển thị menu ===== */
void show_menu() {
    std::cout << "\n===== MENU =====\n";
    std::cout << "1. Deposit\n";
    std::cout << "2. Withdraw\n";
    std::cout << "3. Transfer\n";
    std::cout << "4. Undo\n";
    std::cout << "5. Balance\n";
    std::cout << "6. History\n";
    std::cout << "0. Exit\n";
    std::cout << "Choose: ";
}

int main() {
    int user_id;
    /* ===== Nhập & kiểm tra user ID ===== */
    do {
        std::cout << "Enter user ID (1-" << MAX_USERS << "): ";
        std::cin >> user_id;
        if(user_id <= 0) {
            std::cout << "Invalid ID. Please try again\n";
        }
    } while (user_id <= 0 || user_id > MAX_USERS);

    std::cout << "Login success. Welcome user " << user_id << "\n";

    while (true) {
        show_menu();

        int choice;
        std::cin >> choice;

        if (choice == 0) {
            std::cout << "Bye.\n";
            break;
        }

        switch (choice) {
            case 1:
                deposit_menu(user_id);
                break;
            case 2:
                withdraw_menu(user_id);
                break;
            case 3:
                transfer_menu(user_id);
                break;
            case 4:
                undo_menu(user_id);
                break;
            case 5:
                balance_menu(user_id);
                break;
            case 6:
                history_menu(user_id);
                break;
            default:
                std::cout << "Invalid choice\n";
        }
    }

    return 0;
}