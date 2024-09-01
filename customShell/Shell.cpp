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

#ifdef _WIN32
    #define TERMINAL "start cmd"
    #define CALCULATOR "calc"
    #define TEXT_EDITOR "notepad"
#else
    #define TERMINAL "gnome-terminal"
    #define CALCULATOR "gnome-calculator"
    #define TEXT_EDITOR "gedit"
#endif

// Function to split the input into command and arguments
std::vector<std::string> parseInput(const std::string &input) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, ' ')) {
        tokens.push_back(token);
    }
    return tokens;
}

// Function to change directory
void changeDirectory(const std::string &path) {
    if (chdir(path.c_str()) != 0) {
        perror("cd failed");
    }
}

// Function to handle system shutdown/restart with password prompt
void executeCommandWithPassword(const std::string &command) {
    char *password = getpass("Enter your sudo password: ");
    std::string fullCommand = "echo " + std::string(password) + " | sudo -S " + command;
    if (system(fullCommand.c_str()) != 0) {
        std::cerr << "Error: Unable to execute " << command << std::endl;
    }
}

void handleSystemCommand(const std::string &command) {
    if (command == "shutdown") {
        executeCommandWithPassword("shutdown now");
    } else if (command == "restart") {
        executeCommandWithPassword("reboot");
    } else {
        std::cerr << "Invalid system command.\n";
    }
}

// Function to create a directory
void createDirectory(const std::string &dirName) {
    if (mkdir(dirName.c_str(), 0777) != 0) {
        perror("mkdir failed");
    }
}

// Function to remove a directory
void removeDirectory(const std::string &dirName) {
    if (rmdir(dirName.c_str()) != 0) {
        perror("rmdir failed");
    } else {
        std::cout << "Directory " << dirName << " removed successfully.\n";
    }
}

void openApplication(const std::string &appName) {
    std::string command;
    if (appName == "openCalculator") {
        command = CALCULATOR;
    } else if (appName == "openTextEditor") {
        command = TEXT_EDITOR;
    } else if (appName == "openTerminal") {
        command = TERMINAL;
    } else {
        std::cerr << "Error: Unknown application " << appName << std::endl;
        return;
    }

    if (system(command.c_str()) != 0) {
        std::cerr << "Error: Unable to open application " << appName << std::endl;
    }
}

void listFiles() {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::cout << ent->d_name << "  ";
        }
        std::cout << std::endl;
        closedir(dir);
    } else {
        perror("ls failed");
    }
}

void displayHelp() {
    std::cout << "Available commands:\n";
    std::cout << "  cd <path>        - Change directory to <path>\n";
    std::cout << "  mkdir <dir>      - Create a new directory named <dir>\n";
    std::cout << "  rmdir <dir>      - Remove a directory named <dir>\n";
    std::cout << "  shutdown         - Shut down the system\n";
    std::cout << "  restart          - Restart the system\n";
    std::cout << "  openCalculator   - Open the calculator application\n";
    std::cout << "  openTextEditor   - Open the text editor application\n";
    std::cout << "  openTerminal     - Open a new terminal window\n";
    std::cout << "  ls               - List files in the current directory\n";
    std::cout << "  /help            - Display this help message\n";
    std::cout<<std::endl;
}

// Function to handle custom built-in commands
bool handleBuiltinCommands(const std::vector<std::string> &tokens) {
    if (tokens[0] == "cd") {
        if (tokens.size() < 2) {
            std::cerr << "cd: missing argument\n";
        } else {
            changeDirectory(tokens[1]);
        }
        return true;
    } else if (tokens[0] == "shutdown" || tokens[0] == "restart") {
        handleSystemCommand(tokens[0]);
        return true;
    } else if (tokens[0] == "mkdir") {
        if (tokens.size() < 2) {
            std::cerr << "mkdir: missing directory name\n";
        } else {
            createDirectory(tokens[1]);
        }
        return true;
    } else if (tokens[0] == "rmdir") {
        if (tokens.size() < 2) {
            std::cerr << "rmdir: missing directory name\n";
        } else {
            removeDirectory(tokens[1]);
        }
        return true;
    } else if (tokens[0] == "openCalculator" || tokens[0] == "openTextEditor" || tokens[0] == "openTerminal") {
        openApplication(tokens[0]);
        return true;
    } else if (tokens[0] == "ls") {
        listFiles();
        return true;
    } else if (tokens[0] == "/help") {
        displayHelp();
        return true;
    }

    return false;
}

// Function to execute external commands
void executeCommand(const std::vector<std::string> &tokens) {
    std::vector<char *> args;
    for (const std::string &token : tokens) {
        args.push_back(const_cast<char *>(token.c_str()));
    }
    args.push_back(nullptr); // execvp expects a null-terminated array

    if (execvp(args[0], args.data()) == -1) {
        perror("execvp failed");
    }
}

int main() {
    std::cout << "/help: Type /help to see the list of available commands\n";
    std::cout<<std::endl;
    while (true) {
        std::cout << "mysh> "; // Display prompt
        std::string input;
        std::getline(std::cin, input); // Get input from user

        if (input.empty()) continue;

        std::vector<std::string> tokens = parseInput(input);

        // Handle built-in commands
        if (handleBuiltinCommands(tokens)) {
            continue;
        }

        // Fork a child process to execute the external command
        pid_t pid = fork();
        if (pid == 0) { // Child process
            executeCommand(tokens);
            exit(0);
        } else if (pid < 0) {
            perror("fork failed");
        } else { // Parent process
            wait(nullptr); // Wait for the child process to finish
        }
    }
    return 0;
}
