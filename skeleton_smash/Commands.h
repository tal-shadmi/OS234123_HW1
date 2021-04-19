#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <time.h>
#include <unistd.h>
#include <map>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {

protected:
  char ** commandParts;
  int commandPartsNum;
  char *commandName;
  pid_t pid = -1;
  time_t runningTime = 0;
  time_t startingTime;
  bool isStopped = false;
  bool onForeground;


 public:
  explicit Command(const char* cmd_line , bool isStopped = false);
  virtual ~Command();
  virtual void execute() = 0;
  pid_t GetPid();
  void SetPid(pid_t pid);
  bool IsStopped();
  void SetisStopped(bool stopped);
  char* GetCommandName();
  time_t GetRunningTime();
  bool GetonForeground();


  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  explicit BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  explicit ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  explicit PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
  char** plastPwd;
 public:
  explicit ChangeDirCommand(const char* cmd_line, char** plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  explicit GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  explicit ShowPidCommand(const char* cmd_line);
  ~ShowPidCommand() override = default;
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
 private:
    JobsList *job_list;
// TODO: Add your data members public:
 public:
  explicit QuitCommand (const char* cmd_line, JobsList* jobs);
  ~QuitCommand() override = default;
  void execute() override;
};

class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
   Command *command;
   int jobId;
   public :
     JobEntry(Command* cmd,int job_id);
     ~JobEntry() = default;
     Command* getCommand();
     int getJobId();
     /*
    * TODO: need to add signals table and running status (finished execution or not)
    */
  };
  map<int,JobEntry> *all_jobs;
  map<pid_t,int> *jobs_id_by_pid;
  list<JobEntry> *stopped_jobs;

 public:
  JobsList();
  ~JobsList() = default;
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  explicit JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  explicit KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  explicit ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class CatCommand : public BuiltInCommand {
 public:
  CatCommand(const char* cmd_line);
  virtual ~CatCommand() {}
  void execute() override;
};


class SmallShell {
 private:
  // TODO: Add your data members
  JobsList *jobs;
  string promptName;
  int pid;
  string path;
  string lastPath;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell() = default;
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  string getPromptName();
  void setPromptName(string &newPromptName);
  pid_t get_pid() const;
  JobsList * getJobList();
};

#endif //SMASH_COMMAND_H_
