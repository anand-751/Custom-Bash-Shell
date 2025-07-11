#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sstream>
#include <dirent.h>

using namespace std;

#ifdef _WIN32
    #define TERMINAL "start cmd"
    #define CALCULATOR "calc"
    #define TEXT_EDITOR "notepad"
#else
    #define TERMINAL "gnome-terminal"
    #define CALCULATOR "gnome-calculator"
    #define TEXT_EDITOR "gedit"
#endif

class Shell {
public:
    void run() {
        cout << "/help: Type /help to see the list of available commands\n" << endl;
        while (true) {
            cout << "mysh> ";
            string input;
            getline(cin, input);

            if (input.empty()) continue;

            vector<string> tokens = parseInput(input);

            if (handleBuiltinCommands(tokens)) continue;

            pid_t pid = fork();
            if (pid == 0) {
                executeCommand(tokens);
                exit(0);
            } else if (pid < 0) {
                perror("fork failed");
            } else {
                wait(nullptr);
            }
        }
    }

private:
    vector<string> parseInput(const string &input) {
        vector<string> tokens;
        string token;
        istringstream tokenStream(input);
        while (getline(tokenStream, token, ' ')) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void changeDirectory(const string &path) {
        if (chdir(path.c_str()) != 0) {
            perror("cd failed");
        }
    }

    void executeCommandWithPassword(const string &command) {
        char *password = getpass("Enter your sudo password: ");
        string fullCommand = "echo " + string(password) + " | sudo -S " + command;
        if (system(fullCommand.c_str()) != 0) {
            cerr << "Error: Unable to execute " << command << endl;
        }
    }

    void handleSystemCommand(const string &command) {
        if (command == "shutdown") {
            executeCommandWithPassword("shutdown now");
        } else if (command == "restart") {
            executeCommandWithPassword("reboot");
        } else {
            cerr << "Invalid system command.\n";
        }
    }

    void createDirectory(const string &dirName) {
        if (mkdir(dirName.c_str(), 0777) != 0) {
            perror("mkdir failed");
        }
    }

    void removeDirectory(const string &dirName) {
        if (rmdir(dirName.c_str()) != 0) {
            perror("rmdir failed");
        } else {
            cout << "Directory " << dirName << " removed successfully.\n";
        }
    }

    void openApplication(const string &appName) {
        string command;
        if (appName == "openCalculator") {
            command = CALCULATOR;
        } else if (appName == "openTextEditor") {
            command = TEXT_EDITOR;
        } else if (appName == "openTerminal") {
            command = TERMINAL;
        } else {
            cerr << "Error: Unknown application " << appName << endl;
            return;
        }

        if (system(command.c_str()) != 0) {
            cerr << "Error: Unable to open application " << appName << endl;
        }
    }

    void listFiles() {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(".")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                cout << ent->d_name << "  ";
            }
            cout << endl;
            closedir(dir);
        } else {
            perror("ls failed");
        }
    }

    void displayHelp() {
        cout << "Available commands:\n";
        cout << "  cd <path>        - Change directory to <path>\n";
        cout << "  mkdir <dir>      - Create a new directory named <dir>\n";
        cout << "  rmdir <dir>      - Remove a directory named <dir>\n";
        cout << "  shutdown         - Shut down the system\n";
        cout << "  restart          - Restart the system\n";
        cout << "  openCalculator   - Open the calculator application\n";
        cout << "  openTextEditor   - Open the text editor application\n";
        cout << "  openTerminal     - Open a new terminal window\n";
        cout << "  ls               - List files in the current directory\n";
        cout << "  /help            - Display this help message\n\n";
    }

    bool handleBuiltinCommands(const vector<string> &tokens) {
        if (tokens.empty()) return false;

        const string &cmd = tokens[0];

        if (cmd == "cd") {
            if (tokens.size() < 2) {
                cerr << "cd: missing argument\n";
            } else {
                changeDirectory(tokens[1]);
            }
            return true;
        } else if (cmd == "shutdown" || cmd == "restart") {
            handleSystemCommand(cmd);
            return true;
        } else if (cmd == "mkdir") {
            if (tokens.size() < 2) {
                cerr << "mkdir: missing directory name\n";
            } else {
                createDirectory(tokens[1]);
            }
            return true;
        } else if (cmd == "rmdir") {
            if (tokens.size() < 2) {
                cerr << "rmdir: missing directory name\n";
            } else {
                removeDirectory(tokens[1]);
            }
            return true;
        } else if (cmd == "openCalculator" || cmd == "openTextEditor" || cmd == "openTerminal") {
            openApplication(cmd);
            return true;
        } else if (cmd == "ls") {
            listFiles();
            return true;
        } else if (cmd == "/help") {
            displayHelp();
            return true;
        }

        return false;
    }

    void executeCommand(const vector<string> &tokens) {
        vector<char *> args;
        for (const string &token : tokens) {
            args.push_back(const_cast<char *>(token.c_str()));
        }
        args.push_back(nullptr);

        if (execvp(args[0], args.data()) == -1) {
            perror("execvp failed");
        }
    }
};

int main() {
    Shell shell;
    shell.run();
    return 0;
}
