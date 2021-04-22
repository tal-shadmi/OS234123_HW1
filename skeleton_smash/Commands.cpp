#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <time.h>
#include <iomanip>
#include "Commands.h"

const char* SMASH_NAME = "smash";
const string WHITESPACE = " \n\r\t\f\v";

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

Command::Command(const char *cmd_line, bool isStopped) : commandParts(new char *[COMMAND_MAX_ARGS + 1]),
                                                         commandName(new char[COMMAND_ARGS_MAX_LENGTH + 1]),
                                                         startingTime(time(nullptr)) {
    char *s[];////////////////////////////////
    strcpy(s, cmd_line);
    strcpy(this->commandName, cmd_line);
    _removeBackgroundSign(s);
    this->commandPartsNum = _parseCommandLine(s, this->commandParts);// commandParts without '&' sign.
    this->onForeground = not _isBackgroundComamnd(cmd_line);
}

Command::~Command() {
    for (int i = 0; i < this->commandPartsNum; i++) {
        delete this->commandParts[i];
    }
    delete[] this->commandParts;
    delete[] this->commandName;
}

ostream &operator<<(ostream &os, const Command &command) {//add as a method to print to os
    time_t current = time(nullptr);
    double dif = difftime(current, command.startingTime);//////////////////////////////////////////////////////////////////////
    os << command.commandName << " : " << command.pid << " " << dif << " secs";
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

bool Command::IsStopped() {
    return this->isStopped;
}

void Command::SetIsStopped(bool stopped) {

    this->isStopped = stopped;
}

char *Command::GetCommandName() {
    return this->commandName;
}

time_t Command::GetRunningTime() {
    return this->runningTime;
}

bool Command::GetonForeground() {
    return this->onForeground;
}

/**
 * implementation for Built-in commands
 */

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line)
        : BuiltInCommand(cmd_line) {
}

void ChangePromptCommand::execute() {
    if (this->commandPartsNum > 1) {
        SmallShell::getInstance().setPromptName(this->commandParts[1]);
    } else {
        SmallShell::getInstance().setPromptName(SMASH_NAME);
    }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void ShowPidCommand::execute() {
    pid_t pid = SmallShell::getInstance().get_pid();
    cout << "smash pid is " << pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
    getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
    cout << cwd << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    this->plastPwd = plastPwd;
}

// TODO: handle errors properly
void ChangeDirCommand::execute() {//only changes made was to add "{" &  "}" in the right placed
    if (this->commandPartsNum != 2) {
        cout << "handle: \"smash error: cd: too many arguments\"" << endl;
    } else if (this->plastPwd == nullptr) {
        cout << "handle: \"smash error: cd: OLDPWD not set\"" << endl;
    } else if (string(this->commandParts[1]) == "-") {
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        if (chdir(*this->plastPwd)) {
            cout << "handle: \"chdir()system call failed (e.g., <path> argument points to a non-existing path)\""
                 << endl;
        }
        strcpy(*this->plastPwd, cwd);
        free(cwd);
    } else if (string(this->commandParts[1]) == "..") {
        char *cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        string path = string(this->commandParts[1]);
        unsigned int last_file_start_pos = path.find_last_of('/');
        string new_path = path.substr(0, last_file_start_pos);
        if (chdir(new_path.c_str()) == -1) {
            cout << "handle: \"chdir()system call failed (e.g., <path> argument points to a non-existing path)\""
                 << endl;
        }
        strcpy(*this->plastPwd, cwd);
        free(cwd);
    } else {
        if (chdir(this->commandParts[1]) == -1) {
            cout << "handle: \"chdir()system call failed (e.g., <path> argument points to a non-existing path)\""
                 << endl;
        }
    }
}

JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), job_list(jobs) {}

void JobsCommand::execute() {

    this->job_list->removeFinishedJobs();
    this->job_list->printJobsList();
}


QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) :
        BuiltInCommand(cmd_line), job_list(jobs) {}

void QuitCommand::execute() {
    if (strcmp(this->commandParts[1], "kill") == 0) {
        this->job_list->removeFinishedJobs();//TODO
        this->job_list->killAllJobs();//TODO
    }
    exit(0);
}

/**
 * implementation for External commands
 */

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    // TODO: fork is needed
    //build the array and alocate space for each string in the array
    char *argv[4];
    argv[0] = new char[5];
    strcpy(argv[0], "/bin/bash");
    argv[1] = new char[3];
    strcpy(argv[1], "-c");
    argv[2] = new char[this->cmd_line.size() + 1];
    argv[2] = const_cast<char *>(this->cmd_line.c_str());
    argv[3] = nullptr;
    pid_t pid = fork();// fork to differ father and son
    if (pid == 0) {
        if (not this->is_child) {
            setpgrp();
        }
        execv("/bin/bash", argv);
        exit(0);//if reached here all good , if an error was made it will send a signal smash
    } else {
        if (pid == -1) {//fork failed
            //error
        }

    }

}
/*
void ExternalCommand::execute() {
    // TODO: fork is needed

    char* argv[sizeof(this->commandParts) + 2];
    argv[0] = (char*)"bash";
    argv[1] = (char*)"-c";
    for (int i = 2; i < sizeof(this->commandParts) + 2; ++i) {
        argv[i] = this->commandParts[i - 2];
    }
    pid_t pid = fork();
    if(pid==0){//if Son
        setpgrp();
        execv("/bin/bash", argv);
    }
    else if(pid>0){//if father
        this->SetPid(pid);
        int job_id;
        SmallShell::getInstance().getJobList()->getLastJob(&job_id);
        SmallShell::getInstance().getJobList()->jobs_id_by_pid->insert(pair<pid_t,int>(pid,job_id));

        wait(nullptr);
    }
    else{
        //error
    }
}
*/


/*ListContentsCommand::ListContentsCommand(const char* &cmd_line,
                                         char** cur_dir)
        : BuiltInCommand(cmd_line), cur_dir(cur_dir) {
}

void ListContentsCommand::execute() {
    struct dirent **namelist;
    int n;
    n = scandir(this->cur_dir[1], &namelist, NULL, alphasort);
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            string content = namelist[i]->d_name;
            if (content!="." and content !=".."){
                cout << namelist[i]->d_name << endl;
            }
        }
    }
}
*/
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
/*int JobsList::JobEntry::getJobId() {
    return this->jobId;
}*/

/**
 * implementation of jobList
 */



void JobsList::addJob(Command *cmd, bool isStopped) {
    this->removeFinishedJobs();//changed to handle JobEntry diffrent struct
    int job_id = cmd->GetJobId();
    if (all_jobs.empty()) {
        job_id = 1;
    } else {
        job_id = all_jobs.rbegin()->first + 1;
    }
    //JobEntry *job_entry = new JobEntry(cmd,job_id);
    JobEntry job_entry(cmd, isStopped);
    cmd->set_time;
    this->all_jobs.insert(pair<int, JobEntry>(job_id, job_entry));
    this->pid_to_job_id.insert(pair<pid_t, int>
                                       (job_entry.getCommand()->GetPid(),
                                        job_id));
    if (isStopped) {
        this->stopped_jobs.insert(pair<int, JobEntry>(job_id, job_entry));
    }
}

void JobsList::printJobsList() {
    // delete all finished jobs:
    for (pair<int, JobEntry> element : this->all_jobs) {
        if (element.second.getCommand()->IsStopped()) {
            cout << element.first << " " << element.second.getCommand()->GetCommandName() << " : " <<
                 element.second.getCommand()->GetPid() << " " <<
                 difftime(time(nullptr), element.second.getCommand()->GetRunningTime())
                 << " (stopped)" << endl;
        } else {
            cout << element.first << " " << element.second.getCommand()->GetCommandName() << " : " <<
                 element.second.getCommand()->GetPid() << " "
                 << difftime(time(nullptr), element.second.getCommand()->GetRunningTime())
                 << endl;
        }
    }
};

void JobsList::killAllJobs() {

}

void JobsList::removeFinishedJobs() {
    pid_t stopped;// = waitpid(-1, nullptr, WNOHANG);
    while ((stopped = waitpid(-1, nullptr, WNOHANG)) > 0) {
        int job_to_remove = this->pid_to_job_id.find(stopped)->second;
        this->all_jobs.erase(job_to_remove);
        this->pid_to_job_id.erase(stopped);
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    return &this->all_jobs.find(jobId)->second;
}

void JobsList::removeJobById(int jobId) {
    pid_t pid = this->all_jobs.find(jobId)->second.getCommand()->GetPid();
    this->all_jobs.erase(jobId);
    this->pid_to_job_id.erase(pid);
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {
    *lastJobId = this->all_jobs.rbegin()->first;
    return &this->all_jobs.rbegin()->second;
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {
    *jobId = this->stopped_jobs.rbegin()->second.getCommand()->GetJobId();
    return &this->stopped_jobs.rbegin()->second;
}


/**
 * implementation for SmallShell
 */

SmallShell::SmallShell() {
// TODO: add your implementation
    this->jobs_list = new JobsList();
    this->promptName = "smash";
    this->pid = getpid();
    this->lastPath = nullptr;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    //string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    /* add missind commands , it's basicly the same just more elegant*/
    if (cmd_s.empty()) {
        return nullptr;
    }
    if (cmd_s.find('>') != string::npos) {//used string method to find if exixst in cmd
        //If no matches were found, the function returns string::npos.
        return new RedirectionCommand(cmd_line);
    } else if (cmd_s.find('|') != string::npos) {
        return new PipeCommand(cmd_line);
    } else if (cmd_s.find("chprompt") == 0) {//add chprompt
        return new ChangePromptCommand(cmd_line);
    } else if (cmd_s.find("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (cmd_s.find("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    } else if (cmd_s.find("cd") == 0) {
        return new ChangeDirCommand(cmd_line, (char **) this->lastPath.c_str());
    } else if (cmd_s.find("jobs") == 0) {
        return new JobsCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("kill") == 0) {
        return new KillCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("fg") == 0) {
        return new ForegroundCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("bg") == 0) {
        return new BackgroundCommand(cmd_line, this->jobs_list);
    } else if (cmd_s.find("quit") == 0) {
        return new QuitCommand(cmd_line, this->jobs_list);
    } else {
        return new ExternalCommand(cmd_line);
    }



/*
    if (firstWord == "showpid") {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord == "pwd") {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord == "cd") {
        return new ChangeDirCommand(cmd_line, (char **) this->lastPath.c_str());
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
    }*/
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    Command *cmd = CreateCommand(cmd_line);
    if (!cmd)return;//check if valid
    //this->jobs_list->removeFinishedJobs();
    this->jobs_list->addJob(cmd);
    cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPromptName() {
    return this->promptName;
}

void SmallShell::setPromptName(const char* newPromptName) {
    this->promptName = newPromptName;
}

pid_t SmallShell::get_pid() const {
    return this->pid;
}

JobsList *SmallShell::getJobList() {

    return this->jobs_list;
}

string SmallShell::getCurrDir() const {
    return this->path;
}

void SmallShell::add_to_job_list(Command *cmd) {

    this->jobs_list->addJob(cmd);

}

void SmallShell::set_foreground_cmd(Command *cmd) {
    this->foreground_cmd = cmd;
}

void SmallShell::stop_foreground() {

}

void SmallShell::kill_foreground() {

}





