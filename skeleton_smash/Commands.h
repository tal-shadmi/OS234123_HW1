#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <time.h>
#include <unistd.h>
#include <map>
#include <unordered_map>
#include <dirent.h>

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class JobsList; //decalared

class Command {
protected:
    time_t starting_time = 0;
    char **command_parts;
    int command_parts_num;
    char *command_name;
    pid_t pid = 0;
    int job_id = 0; //added job_id to Command class for easier handling with
                    // command method andjob list hierarchy
    bool is_stopped = false;
    bool on_foreground;


public:
    explicit Command(const char *cmd_line, bool isStopped = false);

    virtual ~Command();

    virtual void execute() = 0;

    friend ostream &operator<<(ostream &os, Command const &command);// so we could print

    void SetPid(pid_t pid);

    pid_t GetPid();

    int GetJobId();

    void SetJobId(int new_job_id);

    bool IsStopped();

    void SetIsStopped(bool stopped);

    char *GetCommandName();

    bool GetOnForeground();

    void SetOnForeground(bool on_foreground);

    void SetTime();

    time_t GetStartingTime();

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    explicit BuiltInCommand(const char *cmd_line);
    ~BuiltInCommand() override = default;//made default {made from simple type var}
};

class ExternalCommand : public Command {
    bool is_background;
    string cmd_line;
public:
    explicit ExternalCommand(const char *cmd_line);
    ~ExternalCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class PipeCommand : public Command {
    bool is_background;
public:
    explicit PipeCommand(const char *cmd_line);
    virtual ~PipeCommand() = default;
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);
    virtual ~RedirectionCommand() = default;
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
    char **plastPwd;
public:
    explicit ChangeDirCommand(const char *cmd_line, char **plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    explicit GetCurrDirCommand(const char *cmd_line);
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand { //added new class to handle chprompt
public:
    explicit ChangePromptCommand(const char* cmd_line);
    ~ChangePromptCommand() override = default;
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    explicit ShowPidCommand(const char *cmd_line);
    ~ShowPidCommand() override = default;
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
private:
    JobsList *job_list;
// TODO: Add your data members public:
public:
    explicit QuitCommand(const char *cmd_line, JobsList *jobs);
    ~QuitCommand() override = default;
    void execute() override;
};

class JobsCommand : public BuiltInCommand {
private:
    JobsList *job_list; //added pointer to job_list
public:
    explicit JobsCommand(const char *cmd_line, JobsList *jobs);
    ~JobsCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
private:
    JobsList *job_list; //added pointer to job_list
public:
    explicit KillCommand(const char *cmd_line, JobsList *jobs);
    virtual ~KillCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
private:
    JobsList *job_list;
public:
    explicit ForegroundCommand(const char *cmd_line, JobsList *jobs);
    virtual ~ForegroundCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
private:
    JobsList *job_list; //added pointer to job_list
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs);
    virtual ~BackgroundCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class CatCommand : public BuiltInCommand {
public:
    CatCommand(const char *cmd_line);
    virtual ~CatCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
        Command *command;
        bool is_stopped; //changed becasue Command already hold job_id , to make easier to iterate both maps
    public :
        explicit JobEntry(Command *cmd, bool is_stopped = false) :
                command(cmd), is_stopped(is_stopped) {};//add inline
        ~JobEntry() = default;
        Command *getCommand();
        bool stopped() const; //handle new bool
        bool operator!=(const JobEntry &job_entry) const; //to compare to job_entry
        void set_stopped_status(bool stopped); //handle new bool
        /*
       * TODO: need to add signals table and running status (finished execution or not)
       */
    };

    map<int,JobEntry*> all_jobs;
    list<pid_t> stopped_jobs;
    unordered_map<pid_t, int> pid_to_job_id;

    /*
     * changed all_jobs and stopped_jobs to map sorted by job_id
     * last stop_will be the last job added to stopped_jobs
     * unordered_map for pid_to_job_id , uses hash to store node so its easier to iterate
     *
     * */

public:
    JobsList() = default; //made default {made from simple type var}
    ~JobsList() = default;
    void addJob(Command *cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry *getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry *getLastJob(int *lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};


class SmallShell {
private:
    JobsList *jobs_list; // changed from jobs -> jobs_list
    string prompt_name = "smash";//add default starting name
    pid_t pid;
    string last_path;
    int job_on_foreground; //curr job on foreground , for ctrl+c / ctrl+z handlers


    SmallShell();

public:
    Command *CreateCommand(const char *cmd_line);
    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
                                    // Instantiated on first use.
        return instance;
    }

    ~SmallShell() = default;
    void executeCommand(const char *cmd_line);
    string getPromptName();
    void setPromptName(string newPromptName);
    void add_to_job_list(Command *cmd); // just to make it easier to add new jobs
    void set_foreground_job(int job_id);
    int getForegroundJob();
    void stop_foreground();
    void kill_foreground();
    pid_t get_pid() const;
    JobsList *getJobList();
};

#endif //SMASH_COMMAND_H_
