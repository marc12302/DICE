#include <iostream>
#include <fstream>
#include <cstring>
#include <regex>
#include <unistd.h>
#include <termios.h>
#include <ctime>
#include <cstdlib>
#include <libgen.h>  // For dirname
#include <limits.h>  // For PATH_MAX

using namespace std;

void setStdinEcho(bool enable) {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    if (!enable) {
        tty.c_lflag &= ~ECHO;
    } else {
        tty.c_lflag |= ECHO;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    return std::string(result, (count > 0) ? count : 0);
}

string getDatabaseFilePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    string executablePath(result, (count > 0) ? count : 0);
    string directory = dirname(result);
    return directory + "/data_base.data";
}

void registerUser(const char email[], const char username[], const char password[]) {
    string filePath = getDatabaseFilePath();
    ofstream fout(filePath, ios::app);
    if (fout.is_open()) {
        fout << email << " " << username << " " << password << " 0" << endl;
        fout.flush();
        fout.close();
        cout << "[OK] Register completed\n";
    } else {
        cout << "[ERROR] Could not open file for writing\n";
    }
}

bool loginUser(const char username[], const char password[], float& deposit) {
    string filePath = getDatabaseFilePath();
    ifstream fin(filePath);
    if (fin.is_open()) {
        char fileEmail[100], fileUsername[100], filePassword[100];
        while (fin >> fileEmail >> fileUsername >> filePassword >> deposit) {
            if (strcmp(fileUsername, username) == 0 && strcmp(filePassword, password) == 0) {
                fin.close();
                return true;
            }
        }
        fin.close();
    } else {
        cout << "[ERROR] Could not open file for reading\n";
    }
    return false;
}

void saveUserData(const char username[], float deposit) {
    ifstream fin("data_base.data");
    ofstream fout("temp.data");
    if (fin.is_open() && fout.is_open()) {
        char fileEmail[100], fileUsername[100], filePassword[100];
        float fileDeposit;
        while (fin >> fileEmail >> fileUsername >> filePassword >> fileDeposit) {
            if (strcmp(fileUsername, username) == 0) {
                fout << fileEmail << " " << fileUsername << " " << filePassword << " " << deposit << endl;
            } else {
                fout << fileEmail << " " << fileUsername << " " << filePassword << " " << fileDeposit << endl;
            }
        }
        fin.close();
        fout.close();
        remove("data_base.data");
        rename("temp.data", "data_base.data");
    } else {
        cout << "[ERROR] Could not open file for updating\n";
    }
}

void getPassword(char password[], int maxLength) {
    int index = 0;
    char ch;
    setStdinEcho(false);
    while (true) {
        ch = getchar();
        if (ch == '\n') {
            password[index] = '\0';
            cout << endl;
            break;
        } else if (ch == 127 && index > 0) {
            cout << "\b \b";
            index--;
        } else if (index < maxLength - 1) {
            password[index] = ch;
            cout << '*';
            index++;
        }
    }
    setStdinEcho(true);
}

void addFunds(float& deposit) {
    float additionalFunds;
    cout << "[!] Your current deposit is: " << deposit << "$.\n";
    cout << "[?] How much money would you like to add to your deposit? ";
    cin >> additionalFunds;
    deposit += additionalFunds;
    cout << "[OK] Funds added successfully! Your new deposit is: " << deposit << "$.\n";
}

void gameDice(float& deposit, const char username[]) {
    srand(time(0));
    char choice;
    float initial_deposit = deposit;
    float profit;

    while (deposit > 0) {
        cout << "[.] Your current deposit is: " << deposit << "$\n";
        cout << "[?] Enter your bet: ";
        TRY_AGAIN:
        float bet;
        cin >> bet;

        if (bet > deposit) {
            cout << "[ERROR] Your bet is higher than your deposit.\n";
            cout << "[?] Would you like to add more funds? (y/n): ";
            cin >> choice;
            if (choice == 'y' || choice == 'Y') {
                addFunds(deposit);
                saveUserData(username, deposit);
                goto TRY_AGAIN;
            } else {
                cout << "[!] Bet cancelled. Returning to main menu.\n";
                return;
            }
        }

        profit = deposit - initial_deposit;

        int player_roll, casino_roll;
        int roll_modification;

        if (profit > 0) {
            roll_modification = rand() % 100;
            if (roll_modification < 20) {
                casino_roll = rand() % 6 + 2;
                player_roll = rand() % 6 + 1;
            } else {
                player_roll = rand() % 6 + 1;
                casino_roll = rand() % 6 + 1;
            }
        } else if (profit < 0) {
            roll_modification = rand() % 100;
            if (roll_modification < 10) {
                player_roll = rand() % 6 + 2;
                casino_roll = rand() % 6 + 1;
            } else {
                player_roll = rand() % 6 + 1;
                casino_roll = rand() % 6 + 1;
            }
        } else {
            player_roll = rand() % 6 + 1;
            casino_roll = rand() % 6 + 1;
        }

        cout << "[.] You rolled the dice and it was: " << player_roll << "\n";
        cout << "[.] The casino rolled the dice and it was: " << casino_roll << "\n";

        if (player_roll > casino_roll) {
            cout << "[ :) ] Congratulations, you won!\n";
            deposit += bet;
        } else if (player_roll < casino_roll) {
            cout << "[ :( ] Nooooo, you lost! Next time your luck could increase!\n";
            deposit -= bet;
        } else {
            cout << "[ :O ] Woww, it's a tie! Try again!\n";
        }

        saveUserData(username, deposit);

        cout << "[?] Do you want to play again? (y/n): ";
        cin >> choice;
        if (choice == 'n' || choice == 'N') {
            cout << "[.] Your deposit is: " << deposit;
            usleep(30000);
            break;
        }
    }

    if (deposit <= 0) {
        cout << "[!] Your deposit is empty. Game over!\n";
    }
}

bool emailIsValid(const char email[]) {
    const regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
    return regex_match(email, pattern);
}

void introLoading() {
    cout << "Loading";
    usleep(30000);
    cout << ".";
    usleep(30000);
    cout << ".";
    usleep(30000);
    cout << ".";
}

int main() {
    START:
    system("clear");
    char email[100], username[100], password[100], password2[100];
    char choice = '\0';

    int logged_in = 0;
    float deposit = 0;

    cout << "1. Register\n2. Login\n3. Exit\nType the digit to choose the option > ";
    cin >> choice;
    cin.ignore();

    switch (choice) {
        case '1': {
            system("clear");
            cout << "Registration\n";

            while (true) {
                cout << "Type your email: ";
                cin.getline(email, 100);
                if (emailIsValid(email)) break;
                cout << email << " is invalid. Type a valid email!\n";
                usleep(3000000);
            }

            cout << "Type your username: ";
            cin.getline(username, 100);

            while (true) {
                cout << "Type your password: ";
                getPassword(password, 100);

                cout << "Confirm your password: ";
                getPassword(password2, 100);

                if (strcmp(password, password2) == 0)
                    break;
                cout << "Passwords do not match! Please try again.\n";
                usleep(5000000);
            }

            registerUser(email, username, password);
            goto START;
            break;
        }

        case '2': {
//            START2:
            system("clear");
            cout << "Login\n";
            while (true) {
                cout << "Type your username: ";
                cin.getline(username, 100);

                cout << "Type your password: ";
                getPassword(password, 100);

                if (loginUser(username, password, deposit)) {
                    cout << "[OK] Successful login!\n";
                    logged_in = 1;
                    break;
                } else {
                    cout << "[ERROR] Invalid username or password. Please try again.\n";
                    usleep(5000000);
                }
            }
            break;
        }

        case '3':
            cout << "Thank you for playing!\n";
            return 0;

        default:
            cout << "Invalid choice. Exiting...\n";
            return 0;
    }

    if (logged_in) {
        system("clear");
        if (deposit <= 0) {
            cout << "[!] Your deposit is empty. Please add some money to start playing.\n";
            cout << "[?] Enter the amount to deposit: ";
            cin >> deposit;
            saveUserData(username, deposit);
        }

        introLoading();
        system("clear");
        cout << "[!] Welcome to DICE, " << username << '!';
        gameDice(deposit, username);
    }

    return 0;
}
