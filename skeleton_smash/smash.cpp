#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[]) {

    struct sigaction sa{{0}};
    sa.sa_flags = SA_RESTART;


    sa.sa_handler = alarmHandler;
    if (sigaction(SIGALRM,&sa, nullptr)!= 0 ) {
        perror("smash error: failed to set alarm handler");//for alarm handeling
    }

    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        // std::cout << "smash> ";
        std::cout << smash.getPromptName() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}