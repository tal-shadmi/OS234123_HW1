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
#define NO_TIMEOUT_DURATION (-1)

class JobsList; //decalared

class Command {
protected:
    time_t starting_time;
    char **command_parts;
    int command_parts_num;
    char *command_name;
    pid_t pid = 0;
    int job_id = 0; //added job_id to Command class for easier handling with
                    // command method andjob list hierarchy
    bool is_stopped;
    bool on_foreground;
    // added on_timeout field for the command, true if on_timeout false if not
    // added timeout duration field for the command, holds duration of timeout if exists else -1
    bool on_timeout;
    int timeout_duration;

public:
    explicit Command(const char *cmd_line, bool is_stopped = false, bool on_timeout = false, int timeout_duration = NO_TIMEOUT_DURATION);
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
    // added SetOnTimeout function to change on_timeout field of command value to true
    void SetOnTimeout();
    // added SetTimeoutDuration function to set the duration until timeout for the command
    void SetTimeoutDuration(int duration);
    // added function to set command name
    void SetCommandName(char *cmd_line);
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
    ~PipeCommand() override = default;
    void execute() override;
};

class RedirectionCommand : public Command {

public:
    explicit RedirectionCommand(const char *cmd_line);
    ~RedirectionCommand() override = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    char **plastPwd;

public:
    explicit ChangeDirCommand(const char *cmd_line, char **plastPwd);
    ~ChangeDirCommand() override = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {

public:
    explicit GetCurrDirCommand(const char *cmd_line);
    ~GetCurrDirCommand() override = default;
    void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {

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

    JobsList *job_list;

public:
    explicit QuitCommand(const char *cmd_line, JobsList *jobs);
    ~QuitCommand() override = default;
    void execute() override;
};

class JobsCommand : public BuiltInCommand {

    JobsList *job_list;

public:
    explicit JobsCommand(const char *cmd_line, JobsList *jobs);
    ~JobsCommand() override = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {

    JobsList *job_list;

public:
    explicit KillCommand(const char *cmd_line, JobsList *jobs);
    ~KillCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {

    JobsList *job_list;

public:
    explicit ForegroundCommand(const char *cmd_line, JobsList *jobs);
    ~ForegroundCommand() override = default; //made default {made from simple type var}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {

    JobsList *job_list;

public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs);
    ~BackgroundCommand() override = default;
    void execute() override;
};

class CatCommand : public BuiltInCommand {

public:
    explicit CatCommand(const char *cmd_line);
    ~CatCommand() override = default;
    void execute() override;
};

class TimeoutCommand : public Command {

public:
    explicit TimeoutCommand(const char *cmd_line);
    ~TimeoutCommand() override = default ;
    void  execute() override ;
};


class JobsList {

public:
    class JobEntry {
        Command *command;
        bool is_stopped;
    public :
        explicit JobEntry(Command *cmd, bool is_stopped = false) : command(cmd), is_stopped(is_stopped) {};
        ~JobEntry() = default;
        Command *getCommand();
        bool stopped() const;
        bool operator!=(const JobEntry &job_entry) const;
        void set_stopped_status(bool stopped);
    };

    map<int,JobEntry> all_jobs; // map of all jobs - key: job id, data: JobEntry object
    list<pid_t> stopped_jobs; // list of stopped jobs
    unordered_map<pid_t, int> pid_to_job_id; // unordered map of job id with it's appropriate pid - key: process pid, data: job id

public:
    JobsList() = default;
    ~JobsList() = default;
    void addJob(Command *cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int *lastJobId);
    JobEntry * getLastStoppedJob(int *jobId);
};


class SmallShell {
public:
    class TimeOutKey {
    private:
        pid_t pid;
        time_t end_time;
    public:
        TimeOutKey(pid_t pid, time_t end_time) : pid(pid),end_time(end_time) {};
        ~TimeOutKey() = default;
        bool operator<(const TimeOutKey &key) const{
            if (this->end_time == key.end_time) {
                return this->pid < key.get_pid();
            }
            return this->end_time < key.get_end_time();

        }
        pid_t get_pid() const{
            return this->pid;
        }
        time_t get_end_time() const{
            return this->end_time;
        }
    };
    class TimeOutData {
    private:
        Command *command;
        string command_name;
    public:
        TimeOutData(Command *command, string &command_name) : command(command),command_name(command_name) {};
        ~TimeOutData() = default;

        Command* get_command() const{
            return this->command;
        }
        string get_command_name() const{
            return this->command_name;
        }
    };
private:
    struct time_out_command{
        pid_t pid;
        string command_name;
    };
    JobsList *jobs_list;
    string prompt_name = "smash";
    pid_t pid;
    string last_path;
    int job_on_foreground;
    map<TimeOutKey, TimeOutData> timeout_jobs;

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
    void add_to_job_list(Command *cmd);
    void add_to_time_out(Command *cmd , time_t duration);
    void set_foreground_job(int job_id);
    int getForegroundJob();
    void stop_foreground();
    void kill_foreground();
    void kill_time_out();
    pid_t get_pid() const;
    JobsList *getJobList();
    map<TimeOutKey, TimeOutData> *getTimeoutJobs();
};

#endif //SMASH_COMMAND_H_
