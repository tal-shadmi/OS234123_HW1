#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <time.h>
#include <iomanip>
#include "Commands.h"
#include <stdlib.h>
#include <algorithm>
#include <stdio.h>
#include <fcntl.h>

#define SMASH_NAME  "smash"
#define WHITESPACE  " \n\r\t\f\v"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

/**
 * helper functions for command line analyzing
 */

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

Command::Command(const char *cmd_line, bool isStopped) : command_parts(new char *[COMMAND_MAX_ARGS + 1]),
                                                         command_name(new char[COMMAND_ARGS_MAX_LENGTH + 1]),
                                                         starting_time(time(nullptr)) {
    char *s = new char[sizeof(cmd_line) / sizeof(cmd_line[0])];
    strcpy(s, cmd_line);
    _removeBackgroundSign(s);
    strcpy(this->command_name, cmd_line);
    this->command_parts_num = _parseCommandLine(s, this->command_parts); // commandParts without '&' sign.
    this->on_foreground = not _isBackgroundComamnd(cmd_line);
}

Command::~Command() {
    for (int i = 0; i < this->command_parts_num; i++) {
        delete this->command_parts[i];
    }
    delete[] this->command_parts;
    delete[] this->command_name;
}

// this function should be used for the JobsCommand execute() function
ostream &operator<<(ostream &os, const Command &command) {//add as a method to print to os
    time_t current = time(nullptr);
    double dif = difftime(current, command.starting_time); // Command has running_time parameter, maybe we should use that?
    os << command.command_name << " : " << command.pid << " " << dif << " secs";
    return os;
}

void Command::SetPid(pid_t pid) {
    this->pid = pid;
}

pid_t Command::GetPid() {
    return this->pid;
}

int Command::GetJobId() {
    return this->job_id;
}

void Command::SetJobId(int new_job_id) {
    this->job_id=new_job_id;
}

bool Command::IsStopped() {
    return this->is_stopped;
}

void Command::SetIsStopped(bool stopped) {

    this->is_stopped = stopped;
}

char *Command::GetCommandName() {
    return this->command_name;
}

bool Command::GetOnForeground() {
    return this->on_foreground;
}

void Command::SetOnForeground(bool on_foreground) {
    this->on_foreground = on_foreground;
}

void Command::SetTime() {
    this->starting_time = time(nullptr);
}

time_t Command::GetStartingTime() {
    return this->starting_time;
}

/**
 * implementation for PipeCommand commands
 */

PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line), is_background(_isBackgroundComamnd(cmd_line)) {}

void PipeCommand::execute() {
    string cmd(this->command_name);
    int pos = cmd.find('|');
    // find out if input is from stdout or stderr
    bool is_from_stdout = cmd[pos + 1] != '&';
    pid_t main_pid, pid1, pid2;
    int fd[2];
    pipe(fd);
    // main fork for the | process
    main_pid = fork();
    if (main_pid == -1){ // failed fork from smash
        close(fd[0]);
        close(fd[1]);
        perror("smash error: fork failed");
        return;
    }
    if (main_pid == 0){ // child of smash
        // set to be in the same group ID of smash
        setpgrp();
        // fork for the left side command
        pid1 = fork();
        if (pid1 == 0){
            if (is_from_stdout){
                dup2(fd[1], 1);
            } else{
                dup2(fd[1], 2);
            }
            close(fd[0]);
            close(fd[1]);
            SmallShell::getInstance().executeCommand(cmd.substr(0,pos).c_str());
            exit(0);
        }
        else if (pid1 == -1){
            close(fd[0]);
            close(fd[1]);
            perror("smash error: fork failed");
            exit(1);
        }
        // fork for the right side command
        pid2 = fork();
        if (pid2 == 0){
            dup2(fd[0], 0);
            close(fd[0]);
            close(fd[1]);
            if (is_from_stdout){
                SmallShell::getInstance().executeCommand(cmd.substr(pos + 1,cmd.length()).c_str());
            }
            else{
                SmallShell::getInstance().executeCommand(cmd.substr(pos + 2,cmd.length()).c_str());
            }
            exit(0);
        }
        else if (pid2 == -1){
            close(fd[0]);
            close(fd[1]);
            perror("smash error: fork failed");
            exit(1);
        }
        close(fd[0]);
        close(fd[1]);
        waitpid(pid1, nullptr, 0);
        waitpid(pid2, nullptr, 0);
        exit(0);
    }
    else{ // father (smash)
        close(fd[0]);
        close(fd[1]);
        this->SetPid(main_pid);
        SmallShell::getInstance().add_to_job_list(this);
        if (!this->is_background){
            // set smash foreground job to this job and wait
            SmallShell::getInstance().set_foreground_job(SmallShell::getInstance().getJobList()->pid_to_job_id.find(pid)->second);
            waitpid(main_pid, nullptr, WUNTRACED);
        }
    }
}

/**
 * implementation for RedirectionCommand commands
 */

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
    int fd;
    string cmd(this->command_name);
    int pos = cmd.find('>');
    bool is_override = cmd[pos + 1] != '>';
    if (is_override) {
        // opening file with writing access, name as stated in the command
        fd = open(_trim(cmd.substr(pos + 1,cmd.length())).c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    }
    else{
        // opening file with writing access, name as stated in the command and with appending option
        fd = open(_trim(cmd.substr(pos + 2,cmd.length())).c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
    }
    if (fd == -1){
        perror("smash error: open failed");
        return;
    }
    int monitor = dup(1);
    dup2(fd, 1);
    close(fd);
    SmallShell::getInstance().executeCommand(cmd.substr(0,pos).c_str());
    dup2(monitor, 1);
}

/**
 * implementation for Built-in commands
 */

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ChangePromptCommand::ChangePromptCommand(const char *cmd_line)
        : BuiltInCommand(cmd_line) {
}

void ChangePromptCommand::execute() {
    if (this->command_parts_num > 1) {
        SmallShell::getInstance().setPromptName(this->command_parts[1]);
    } else {
        SmallShell::getInstance().setPromptName(SMASH_NAME);
    }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t pid = SmallShell::getInstance().get_pid();
    cout <<SmallShell::getInstance().getPromptName() << " pid is " << pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
    getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
    cout << cwd << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line)  {
    this->plastPwd = plastPwd;
}

// TODO: handle errors properly
void ChangeDirCommand::execute() {//only changes made was to add "{" &  "}" in the right placed
    if (this->command_parts_num == 1){
        return;
    } else if (this->command_parts_num > 2) {
        perror("smash error: cd: too many arguments");
        return;
    } else if (string(this->command_parts[1]) == "-") {
        if (*this->plastPwd == nullptr) {
            perror("smash error: cd: OLDPWD not set\n");
            return;
        }
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        if (chdir(*this->plastPwd) == -1) {
            perror("smash error: chdir failed");
            return;
        }
        delete[] *this->plastPwd;
        *this->plastPwd = cwd ;
    } else if (string(this->command_parts[1]) == "..") {
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        string path = string(this->command_parts[1]);
        unsigned int last_file_start_pos = path.find_last_of('/');
        string new_path = path.substr(0, last_file_start_pos);
        if (chdir(new_path.c_str()) == -1) {
            perror("smash error: chdir failed");
            return;
        }
        delete[] *this->plastPwd;
        *this->plastPwd = cwd;
    } else {
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        if (chdir(this->command_parts[1]) == -1) {
            perror("smash error: chdir failed");
            return;
        }
        delete[] *this->plastPwd;
        *this->plastPwd = cwd;
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), job_list(jobs) {}

void JobsCommand::execute() {
    this->job_list->removeFinishedJobs();
    this->job_list->printJobsList();
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), job_list(jobs) {}

void KillCommand::execute() {

    string job_id(this->command_parts[2]);
    char *char_part_job_id = nullptr;
    auto job_id_n = strtol(job_id.data(), &char_part_job_id, 10);
    char *char_part_signal = nullptr;
    string signal(this->command_parts[1]);
    string empty_str("");
    auto signal_n = strtol(signal.data(), &char_part_signal, 10);
    auto element = this->job_list->all_jobs.find(job_id_n);
    if (job_id_n <= 0 ||element == this->job_list->all_jobs.end()){
        string error_msg = "smash error: kill: job-id ";
        error_msg+=job_id;
        error_msg+= " does not exist";
        perror(error_msg.c_str());
        return;
    }
    if (-signal_n < 0 || -signal_n > 31 || string(char_part_signal)!=""||string(char_part_job_id)!="") {
        perror("smash error: kill: invalid arguments");
        return;
    }
    pid_t pid = this->job_list->all_jobs.find(job_id_n)->second->getCommand()->GetPid();
    if (kill(pid, -signal_n) == -1) {
        perror("smash error: kill failed");
        return;
    }
    if (-signal_n == SIGSTOP || -signal_n == SIGTSTP){
        this->job_list->all_jobs.find(job_id_n)->second->getCommand()->SetIsStopped(true);
    }
    else if (-signal_n == SIGCONT){
        this->job_list->all_jobs.find(job_id_n)->second->getCommand()->SetIsStopped(false);
    }
    cout<<"signal number " <<-signal_n<< " was sent to pid "<<pid<<endl;
}

ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), job_list(jobs) {}

void ForegroundCommand::execute() {

    //TODO : check if negative number error
    int job_id_n;
    pid_t pid;
    if (this->command_parts_num > 1) {
        string job_id(this->command_parts[1]);
        char *char_part_job_id = nullptr;
        job_id_n = strtol(job_id.data(), &char_part_job_id, 10);
        auto element = this->job_list->all_jobs.find(job_id_n);
        if ((job_id_n <= 0 ||element == this->job_list->all_jobs.end())&&string(char_part_job_id)==""){
            string error_msg = "smash error: kill: job-id ";
            error_msg+=job_id;
            error_msg+= " does not exist";
            perror(error_msg.c_str());
            return;
        }
        if(this->command_parts_num>2||string(char_part_job_id)!=""){
            perror("smash error: fg: invalid arguments");
            return;
        }
        pid = element->second->getCommand()->GetPid();
    } else {
        if(this->job_list->all_jobs.empty()){
            perror("smash error: fg: jobs list is empty");
            return;
        }
        pid = this->job_list->getLastJob(&job_id_n)->getCommand()->GetPid();
    }
    SmallShell::getInstance().set_foreground_job(job_id_n);
    this->job_list->all_jobs.find(job_id_n)->second->getCommand()->SetOnForeground(true);
    this->job_list->stopped_jobs.remove(pid);
    cout << this->job_list->all_jobs.find(job_id_n)->second->getCommand()->GetCommandName() << " : " << pid << endl;
    if (kill(pid, SIGCONT) == -1){
        perror("smash error: kill failed");
        return;
    }
    waitpid(pid, nullptr, 0);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), job_list(jobs) {}

void BackgroundCommand::execute() {
    int job_id_n;
    pid_t pid;
    bool is_stopped;
    JobsList::JobEntry *job_entry;
    if (this->command_parts_num > 1) {
        string job_id(this->command_parts[1]);
        char *char_part_job_id = nullptr;
        auto job_id_n = strtol(job_id.data(), &char_part_job_id, 10);
        if (job_id_n <= 0 || string(char_part_job_id) !=""){
            perror("smash error: bg: invalid arguments");
            return;
        }
        auto job_it = this->job_list->all_jobs.find(job_id_n);
        if (job_it == this->job_list->all_jobs.end()) {
            string error_msg = "smash error: bg: job-id ";
            error_msg+=job_id;
            error_msg+= " does not exist";
            perror(error_msg.c_str());
            return;
        }
        is_stopped = job_it->second->getCommand()->IsStopped();
        if (!is_stopped) {
            string error_msg = "smash error: bg: job-id ";
            error_msg+=job_id;
            error_msg+= " is already running in the background";
            perror(error_msg.c_str());
            return;
        }
        job_entry = job_it->second;
        pid = job_it->second->getCommand()->GetPid();
    } else {
        job_entry = this->job_list->getLastStoppedJob(&job_id_n);
        if (job_entry == nullptr) {
           perror("smash error: bg: there is no stopped jobs to resume");
            return;
        }
        pid = job_entry->getCommand()->GetPid();
    }
    this->job_list->stopped_jobs.remove(pid);
    job_entry->set_stopped_status(false);
    if(kill(pid, SIGCONT) == -1){
        perror("smash error: kill failed");
        return;
    }
}

QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) :
        BuiltInCommand(cmd_line), job_list(jobs) {}

void QuitCommand::execute() {
    if (this->command_parts_num>1&&strcmp(this->command_parts[1], "kill") == 0) {
        this->job_list->removeFinishedJobs();
        this->job_list->killAllJobs();
        this->job_list->removeFinishedJobs();
    }
    exit(0);
}

CatCommand::CatCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void CatCommand::execute() {
    if (this->command_parts_num < 2){
        perror("smash error: cat: not enough arguments");
        return;
    }
    int fd;
    ssize_t read_status;
    for (int i=1; i<this->command_parts_num; i++){
        fd = open(this->command_parts[i], O_RDONLY);
        if (fd == -1){
            perror("smash error: open failed");
            return;
        }
        char c[500];
        while (true){
            read_status = read(fd, &c, 500);
            if (read_status == -1){
                perror("smash error: read failed");
                return;
            }
            if (!read_status){
                break;
            }
            if (write(1, &c, read_status) == -1){
                perror("smash error: write failed");
                return;
            }
        }
    }
    close(fd);
}

/**
 * implementation for External commands
 */

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line),
                                                         is_background(_isBackgroundComamnd(cmd_line)) {}

void ExternalCommand::execute() {
    char* argv[4];
    char *s = new char[sizeof(cmd_line) / sizeof(cmd_line[0])]; // needs decision on how to initialize s
    strcpy(s, this->command_name);
    _removeBackgroundSign(s);
    argv[0] = (char*)"/bin/bash";
    argv[1] = (char*)"-c";
    argv[2] = s;
    argv[3] = nullptr;

    pid_t pid = fork();// fork to differ father and child
    if (pid == 0) { // child
        setpgrp();
        execv(argv[0], argv);
        free(s); // added this to avoid memory leak, seems to work just fine
        exit(0);//if reached here all good, if an error was made it will send a signal smash
    } else { // father
        if (pid == -1) {
            perror("smash error: fork failed");
            return;
        }
        // TODO: maybe we should send a pointer to wait to get the exit status of the child
        // set child pid and add job to the list
        this->SetPid(pid);
        SmallShell::getInstance().add_to_job_list(this);
        if (!this->is_background){
            // set smash foreground job to this job and wait
            SmallShell::getInstance().set_foreground_job(SmallShell::getInstance().getJobList()->pid_to_job_id.find(pid)->second);
            waitpid(pid, nullptr, WUNTRACED);
        }
    }
}

/**
 * implementation of jobEntry
 */

Command *JobsList::JobEntry::getCommand() {
    return this->command;
}

bool JobsList::JobEntry::stopped() const {
    return this->is_stopped;
}

bool JobsList::JobEntry::operator!=(const JobEntry &job_entry) const {
    return this->command->GetPid() != job_entry.command->GetPid();
}

void JobsList::JobEntry::set_stopped_status(bool stopped) {
    this->is_stopped = stopped;
}

/**
 * implementation of jobList
 */

void JobsList::addJob(Command *cmd, bool isStopped) {
    // this->removeFinishedJobs(); //changed to handle JobEntry diffrent struct
    int job_id;
    if (this->all_jobs.empty()) {
        job_id = 1;
    } else {
        job_id = all_jobs.rbegin()->first + 1;
    }
    cmd->SetJobId(job_id);
    cmd->SetTime();
    JobEntry *job_entry = new JobEntry(cmd, isStopped);
    this->all_jobs.insert(*(new pair<int,JobEntry*>(job_id,job_entry)));
    this->pid_to_job_id.insert(pair<pid_t, int>
                                       (job_entry->getCommand()->GetPid(),
                                        job_id));
    if (isStopped) {
        this->stopped_jobs.push_back(job_entry->getCommand()->GetPid());
    }
}

void JobsList::printJobsList() {
    // delete all finished jobs:
    this->removeFinishedJobs();
    // print all current jobs:
    for (pair<int, JobEntry*> element : this->all_jobs) {
        cout << "[" << element.first << "]" << " " << element.second->getCommand()->GetCommandName() << " : " <<
             element.second->getCommand()->GetPid() << " "
             << difftime(time(nullptr), element.second->getCommand()->GetStartingTime()) << " secs";
        if (element.second->getCommand()->IsStopped()) {
            cout << "(stopped)";
        }
        cout << endl;
    }
};

void JobsList::killAllJobs() {
    int num_of_jobs = this->all_jobs.size();
    cout<<"smash: sending SIGKILL signal to "<<num_of_jobs<<" jobs:"<<endl;
    for_each(this->all_jobs.begin(), this->all_jobs.end(),
                  [](pair<int , JobEntry*> element){
                      if (kill(element.second->getCommand()->GetPid(),SIGKILL) == -1){
                          perror("smash error: kill failed");
                          return;
                      }
                      string command_name(element.second->getCommand()->GetCommandName());
                      cout<<element.second->getCommand()->GetPid()<<": "<<command_name<<endl;
    });
}

void JobsList::removeFinishedJobs() {
    pid_t stopped;// = waitpid(-1, nullptr, WNOHANG);
    if(this->all_jobs.empty())return;
    while ((stopped = waitpid(-1, nullptr, WNOHANG)) > 0) {
        int job_to_remove = this->pid_to_job_id.find(stopped)->second;
        this->all_jobs.erase(job_to_remove);
        this->pid_to_job_id.erase(stopped);
    }
    int job_id = SmallShell::getInstance().getForegroundJob();
    pid_t pid ;
    if(job_id==-1)return;
    else{
        pid = this->all_jobs.find(job_id)->second->getCommand()->GetPid();
        if(waitpid(pid, nullptr,WNOHANG)==-1){
            SmallShell::getInstance().set_foreground_job(-1);
            SmallShell::getInstance().getJobList()->removeJobById(job_id);
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    return this->all_jobs.find(jobId)->second;
}

void JobsList::removeJobById(int jobId) {
    pid_t pid = this->all_jobs.find(jobId)->second->getCommand()->GetPid();
    this->all_jobs.erase(jobId);
    this->pid_to_job_id.erase(pid);
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    if(lastJobId!= nullptr) {
        *lastJobId = this->all_jobs.rbegin()->first;
    }
    return this->all_jobs.rbegin()->second;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    if (this->stopped_jobs.empty()) {
        return nullptr;
    }
    *jobId = this->pid_to_job_id.find(this->stopped_jobs.back())->second;
    return this->all_jobs.find(*jobId)->second;
}

/**
 * implementation for SmallShell
 */

SmallShell::SmallShell() {
// TODO: add your implementation
    this->jobs_list = new JobsList();
    this->pid = getpid();
    this->job_on_foreground = -1 ;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    /* add missind commands , it's basicly the same just more elegant*/
    /*if (cmd_s.empty()) {
        return nullptr;
    }
    if (cmd_s.find('>') != string::npos) {//used string method to find if exixst in cmd
                                             //If no matches were found, the function returns string::npos.
        return new RedirectionCommand(cmd_line);
    } else if (cmd_s.find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    }
    else if (cmd_s.find("cat ") == 0 || cmd_s.find("cat\n") == 0){
        return new CatCommand(cmd_line);
    } else if (cmd_s.find("chprompt ") == 0 || cmd_s.find("chprompt\n") == 0) {//add chprompt
        return new ChangePromptCommand(cmd_line);
    } else if (cmd_s.find("showpid ") == 0 || cmd_s.find("showpid\n") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (cmd_s.find("pwd ") == 0 || cmd_s.find("pwd\n") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (cmd_s.find("cd ") == 0 || cmd_s.find("cd\n") == 0) {
        return new ChangeDirCommand(cmd_line, (char **) this->last_path.c_str());
    } else if (cmd_s.find("jobs\n") == 0) {
        return new JobsCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("kill ") == 0 || cmd_s.find("kill\n") == 0) {
        return new KillCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("fg ") == 0 || cmd_s.find("fg\n") == 0) {
        return new ForegroundCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("bg ") == 0 || cmd_s.find("bg\n") == 0) {
        return new BackgroundCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("quit ") == 0 || cmd_s.find("quit\n") == 0) {
        return new QuitCommand(cmd_line, this->jobs_list);
    } else {
        return new ExternalCommand(cmd_line);
    }*/

    if (cmd_s.empty()) {
        return nullptr;
    }
    else if (cmd_s.find('>') != string::npos) {//used string method to find if exixst in cmd
                                             //If no matches were found, the function returns string::npos.
        return new RedirectionCommand(cmd_line);
    }
    else if (cmd_s.find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    }
    else if (firstWord == "cat"){
        return new CatCommand(cmd_line);
    }
    else if (firstWord == "chprompt") {
        return new ChangePromptCommand(cmd_line);
    }
    else if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line, (char **) this->last_path.c_str());
    }
    else if (firstWord == "jobs"){
        return new JobsCommand(cmd_line,this->jobs_list);
    }
    else if (firstWord == "kill"){
        return new KillCommand(cmd_line, this->jobs_list);
    }
    else if (firstWord == "fg"){
        return new ForegroundCommand(cmd_line,this->jobs_list);
    }
    else if (firstWord == "bg"){
        return new BackgroundCommand(cmd_line, this->jobs_list);
    }
    else if (firstWord == "quit"){
        return new QuitCommand(cmd_line, this->jobs_list);
    }
    else {
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    Command *cmd = CreateCommand(cmd_line);
    if (!cmd)return;//check if valid
    this->jobs_list->removeFinishedJobs();
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPromptName() {
    return this->prompt_name;
}

void SmallShell::setPromptName(string newPromptName) {
    this->prompt_name = newPromptName;
}

pid_t SmallShell::get_pid() const {
    return this->pid;
}

JobsList *SmallShell::getJobList() {

    return this->jobs_list;
}

void SmallShell::add_to_job_list(Command *cmd) {

    this->jobs_list->addJob(cmd);
}

void SmallShell::set_foreground_job(int job_id) {
    this->job_on_foreground = job_id;
}

int SmallShell::getForegroundJob() {
    return this->job_on_foreground;
}

void SmallShell::stop_foreground() {
    this->jobs_list->removeFinishedJobs();
    if (this->getForegroundJob() == -1) return;
    pid_t foreground_pid = this->jobs_list->all_jobs.find(this->job_on_foreground)->second->getCommand()->GetPid();
    this->jobs_list->stopped_jobs.push_back(foreground_pid);
    this->jobs_list->all_jobs.find(this->job_on_foreground)->second->getCommand()->SetIsStopped(true);
    cout << "smash: process " << foreground_pid << " was stopped" << endl;
    if (kill(foreground_pid, SIGTSTP) == -1){
        perror("smash error: kill failed");
        return;
    }
}

void SmallShell::kill_foreground() {
    this->jobs_list->removeFinishedJobs();
    if (this->getForegroundJob() == -1) return;
    pid_t foreground_pid = this->jobs_list->all_jobs.find(this->job_on_foreground)->second->getCommand()->GetPid();
    cout << "smash: process " << foreground_pid <<" was killed"  << endl;
    if(kill(foreground_pid, SIGINT) == -1){
        perror("smash error: kill failed");
        return;
    }
}





