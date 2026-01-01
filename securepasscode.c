#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>  // For _getch() on Windows
#else
    #include <termios.h>
    #include <unistd.h>
#endif

#define MAX_ACCOUNTS 100
#define MAX_LEN 256
#define FILENAME "passwords.txt"
#define KEY_SHIFT 10  // Simple Caesar cipher (NOT secure!)

typedef struct {
    char site[MAX_LEN];
    char username[MAX_LEN];
    char password[MAX_LEN];
} Account;

void encrypt(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] += KEY_SHIFT;
    }
}

void decrypt(char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] -= KEY_SHIFT;
    }
}

// Cross-platform hidden input
void hide_input(char *buf, int max_len) {
    buf[0] = '\0';
#ifdef _WIN32
    // Windows: use _getch() to hide input
    int i = 0;
    char ch;
    while ((ch = _getch()) != '\r' && ch != '\n') {  // Enter key
        if (ch == '\b' && i > 0) {  // Backspace
            i--;
            printf("\b \b");
        } else if (i < max_len - 1 && ch >= 32 && ch <= 126) {
            buf[i++] = ch;
            putchar('*');  // Show asterisk
        }
    }
    buf[i] = '\0';
    printf("\n");
#else
    // Linux/macOS: use termios
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    fgets(buf, max_len, stdin);
    buf[strcspn(buf, "\n")] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
#endif
}

int main() {
    char master[MAX_LEN] = {0};
    printf("Enter master password: ");
    hide_input(master, MAX_LEN);

    Account accounts[MAX_ACCOUNTS];
    int count = 0;
    int authenticated = 0;

    FILE *fp = fopen(FILENAME, "r");
    if (fp) {
        char temp[MAX_LEN];
        // Read first line: encrypted master password
        if (fgets(temp, MAX_LEN, fp)) {
            decrypt(temp);
            temp[strcspn(temp, "\n")] = '\0';
            if (strcmp(temp, master) == 0) {
                authenticated = 1;
                printf("Access granted!\n");

                // Load accounts (3 lines per account: site, username, password)
                while (count < MAX_ACCOUNTS &&
                       fgets(accounts[count].site, MAX_LEN, fp) &&
                       fgets(accounts[count].username, MAX_LEN, fp) &&
                       fgets(accounts[count].password, MAX_LEN, fp)) {
                    decrypt(accounts[count].site);
                    decrypt(accounts[count].username);
                    decrypt(accounts[count].password);

                    accounts[count].site[strcspn(accounts[count].site, "\n")] = '\0';
                    accounts[count].username[strcspn(accounts[count].username, "\n")] = '\0';
                    accounts[count].password[strcspn(accounts[count].password, "\n")] = '\0';
                    count++;
                }
            }
        }
        fclose(fp);

        if (!authenticated) {
            printf("Wrong master password!\n");
            return 1;
        }
    } else {
        printf("No password vault found. Creating a new one.\n");
        authenticated = 1;  // Allow creating new vault
    }

    // Main menu loop
    while (1) {
        printf("\n=== Password Manager ===\n");
        printf("1. Add new account\n");
        printf("2. View all accounts\n");
        printf("3. Save and exit\n");
        printf("Choice: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            while (getchar() != '\n');  // Clear input buffer
            continue;
        }
        getchar();  // Consume newline

        if (choice == 1) {
            if (count >= MAX_ACCOUNTS) {
                printf("Maximum accounts reached (%d).\n", MAX_ACCOUNTS);
                continue;
            }
            printf("Site (e.g. gmail.com): ");
            fgets(accounts[count].site, MAX_LEN, stdin);
            accounts[count].site[strcspn(accounts[count].site, "\n")] = '\0';

            printf("Username/Email: ");
            fgets(accounts[count].username, MAX_LEN, stdin);
            accounts[count].username[strcspn(accounts[count].username, "\n")] = '\0';

            printf("Password: ");
            hide_input(accounts[count].password, MAX_LEN);

            count++;
            printf("Account added successfully!\n");

        } else if (choice == 2) {
            if (count == 0) {
                printf("No accounts stored yet.\n");
            } else {
                printf("\n--- Stored Accounts (%d) ---\n", count);
                for (int i = 0; i < count; i++) {
                    printf("\n%d. Site: %s\n", i + 1, accounts[i].site);
                    printf("   Username: %s\n", accounts[i].username);
                    printf("   Password: %s\n", accounts[i].password);
                }
            }

        } else if (choice == 3) {
            fp = fopen(FILENAME, "w");
            if (!fp) {
                printf("Error: Could not save vault!\n");
                return 1;
            }

            char temp[MAX_LEN];
            strcpy(temp, master);
            encrypt(temp);
            fprintf(fp, "%s\n", temp);  // Save encrypted master

            for (int i = 0; i < count; i++) {
                strcpy(temp, accounts[i].site);
                encrypt(temp);
                fprintf(fp, "%s\n", temp);

                strcpy(temp, accounts[i].username);
                encrypt(temp);
                fprintf(fp, "%s\n", temp);

                strcpy(temp, accounts[i].password);
                encrypt(temp);
                fprintf(fp, "%s\n", temp);
            }
            fclose(fp);
            printf("Password vault saved successfully.\n");
            break;

        } else {
            printf("Invalid choice. Please try again.\n");
        }
    }

    // Best-effort memory cleanup
    memset(master, 0, sizeof(master));
    for (int i = 0; i < count; i++) {
        memset(accounts[i].password, 0, MAX_LEN);
    }

    return 0;
}
