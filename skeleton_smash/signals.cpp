#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    SmallShell::getInstance().stop_foreground();//uses small shel methods
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell::getInstance().kill_foreground();//uses small shel methods
}

void alarmHandler(int sig_num) {
    cout << "smash: got an alarm" << endl;
   // SmallShell::getInstance()  func to print the time out jobs
    //uses small shel methods
}

