#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#define WHITESPACE " "
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

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
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

Command::Command(const char *cmd_line, bool isStopped) : commandParts(new char* [COMMAND_MAX_ARGS + 1]),
commandName(new char [COMMAND_ARGS_MAX_LENGTH+1]) {
    char* s;
    strcpy(s,cmd_line);
    strcpy(this->commandName, cmd_line);
    _removeBackgroundSign(s);
    this->commandPartsNum = _parseCommandLine(s,this->commandParts);// commandParts without '&' sign.
    this->onForeground = not _isBackgroundComamnd(cmd_line);
}

Command::~Command() {
    for (int i=0; i<sizeof(this->commandParts); i++){
        delete this->commandParts[i];
    }
    delete[] this->commandParts;
    delete[] this->commandName;
}

void Command::SetPid(pid_t pid){
    this->pid=pid;
}

bool Command::IsStopped() {
    return this->isStopped;
}

void Command::SetisStopped(bool stopped) {

    this->isStopped=stopped;
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

pid_t Command::GetPid() {
    return this->pid;
}

/**
 * implementation for Built-in commands
 */

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line){}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t pid = SmallShell::getInstance().get_pid();
    cout << "smash pid is " << pid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* cwd = new char[COMMAND_ARGS_MAX_LENGTH];
    getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
    cout << cwd << endl;
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd) : BuiltInCommand(cmd_line) {
    this->plastPwd = plastPwd;
}

// TODO: handle errors properly
void ChangeDirCommand::execute() {
    if (this->commandPartsNum != 2)
        cout << "handle: \"smash error: cd: too many arguments\"" << endl;
    else if (this->plastPwd == nullptr)
        cout << "handle: \"smash error: cd: OLDPWD not set\"" << endl;
    else if (string(this->commandParts[1]) == "-"){
        char* cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        if (chdir(*this->plastPwd)){
            cout << "handle: \"chdir()system call failed (e.g., <path> argument points to a non-existing path)\"" << endl;
        }
        strcpy(*this->plastPwd, cwd);
        free(cwd);
    }
    else if (string(this->commandParts[1]) == ".."){
        char* cwd = new char[COMMAND_ARGS_MAX_LENGTH];
        getcwd(cwd, COMMAND_ARGS_MAX_LENGTH);
        string path = string(this->commandParts[1]);
        unsigned int last_file_start_pos = path.find_last_of('/');
        string new_path = path.substr(0, last_file_start_pos);
        if (chdir(new_path.c_str()) == -1){
            cout << "handle: \"chdir()system call failed (e.g., <path> argument points to a non-existing path)\"" << endl;
        }
        strcpy(*this->plastPwd, cwd);
        free(cwd);
    }
    else {
        if (chdir(this->commandParts[1]) == -1){
            cout << "handle: \"chdir()system call failed (e.g., <path> argument points to a non-existing path)\"" << endl;
        }
    }
}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs):
BuiltInCommand(cmd_line), job_list(jobs) {}

void QuitCommand::execute() {
    if (strcmp(this->commandParts[1],"kill")==0) {
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
    char* argv[COMMAND_MAX_ARGS + 2] = {(char*)"bash", (char*)"-c", *this->commandParts};
    execv("/bin/bash", argv);
}

/**
 * implementation of jobList
 */
  JobsList::JobsList() :jobs (new map<int,JobEntry>),jobs_id_by_pid(new map<pid_t,int>) {}

  void JobsList::addJob(Command *cmd, bool isStopped) {

    int job_id ;
      //Command *new_cmd = new Command(new_cmd)
      if (jobs->empty())
    {
        job_id = 1;
    }else {
          job_id = jobs->rbegin()->key+1;
      }
      //JobEntry *job_entry = new JobEntry(cmd,job_id);
      pair<int,JobEntry>  element = new pair<int,JobEntry>(job_id,new JobEntry(cmd,job_id));
      this->jobs->insert(element);
  }


  void JobsList::printJobsList(){
      // delete all finished jobs:
      for (pair<int, JobEntry> element : *this->jobs){
          int process_status = waitpid();
      }
  };
  void JobsList::killAllJobs(){

  }
  void JobsList::removeFinishedJobs(){

  }
  JobEntry * JobsList::getJobById(int jobId){



      return this->jobs[jobId];

  }
  void JobsList::removeJobById(int jobId){

  }
  JobEntry * JobsList::getLastJob(int* lastJobId){

  }
  JobEntry * JobsList::getLastStoppedJob(int *jobId){

  }

/**
 * implementation for SmallShell
 */

SmallShell::SmallShell() {
// TODO: add your implementation
    this->jobs = new JobsList();
    this->promptName = "smash";
    this->pid = getpid();
    this->lastPath = nullptr;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
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
        return new JobsCommand(cmd_line, &this->jobs);
    }
    else if (firstWord == "kill"){
        return new KillCommand(cmd_line, &this->jobs);
    }
    else if (firstWord == "fg"){
        return new ForegroundCommand(cmd_line, &this->jobs);
    }
    else if (firstWord == "bg"){
        return new BackgroundCommand(cmd_line, &this->jobs);
    }
    else if (firstWord == "quit"){
        return new QuitCommand(cmd_line, &this->jobs);
    }
    else {
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    Command* cmd = CreateCommand(cmd_line);
    cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

string SmallShell::getPromptName() {
    return this->promptName;
}

void SmallShell::setPromptName(string &newPromptName) {
    this->promptName = newPromptName;
}

pid_t SmallShell::get_pid() const {
    return this->pid;
}