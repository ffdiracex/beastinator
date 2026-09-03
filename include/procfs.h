#ifndef PROCFS_H
#define PROCFS_H

#include <sys/types.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

//procfs error codes

typedef enum {
    PROCFS_OK               = 0,
    PROCFS_ERR_NO_PROC      = -1, // /proc not mounted
    PROCFS_ERR_NO_MEMORY    = -2, // inget minne kvar att använda
    PROCFS_ERR_PERMISSION   = -3, // otillåten aktion, i.e. vi behöver root eller en priviligerad användare
    PROCFS_ERR_NOT_FOUND    = -4, // ej hittat
    PROCFS_ERR_IO           = -5, // error med ut- och in-data.
    PROCFS_ERR_BUF_TOO_SMALL = -6, // buffert för liten
    PROCFS_ERR_NOT_IMPLEMENTED = -7, // en okänd funktion
} procfs_error_t;

// procfs flag codes
typedef enum
{
    PROCFS_FLAG_NONE        = 0,
    PROCFS_FLAG_INCLUDE_THREADS = 1 << 0, //include kernel threads
    PROCFS_FLAG_INCLUDE_ZOMBIE = 1 << 1, //include zombie process
    PROCFS_FLAG_INCLUDE_SYSTEM = 1 << 2, //include system process
    PROCFS_FLAG_VERBOSE         = 1 << 3,
    PROCFS_FLAG_USE_SYSCTL      = 1 << 4  //use sysctl instead of /proc
} procfs_flags_t;


typedef struct procfs_process
{
    int pid; // process id
    int ppid; //parent process id
    int pgid; // process group id
    int sid; // session id

    uid_t uid; // user id
    uid_t euid; //effective user id
    gid_t gid; // group id
    gid_t egid; //effective group id

    char state[16];  // running, sleeping, zombie, etc.
    char name[256]; //process name (command)
    char cmdline[4096]; // full command line
    char tty_name[64]; // tty device name
    char wchan[256]; // wait channel
    char emul[64]; // emulation (native, linux etc.)

    // RAM
    long rss; // resident set size
    long vsize; // virtual memory size
    long swap_size; // swap size

    //time
    long start_time; // process start time
    long user_time; // user CPU time
    long system_time; //system time (RTC)

    //scheduling
    int priority; //scheduling priority
    int nice; //nice value
    int num_threads; // number of threads

    // file descriptors
    int num_fds; // number of open file descriptors
    int *fds; // array of the file descriptor numbers
    char **fd_paths; // array of file descriptor paths

    //process flags
    unsigned int is_thread : 1; // is this a thread?
    unsigned int is_zombie : 1; // is this a zombie?
    unsigned int is_system : 1; // is this a system process?
    unsigned int has_tty   : 1; // does process have a TTY?
    unsigned int is_suid   : 1; // is this a SUID process?
    unsigned int is_sgid   : 1; // is this a SGID process?
    unsigned int is_traced : 1; //is this process being traced

    //pointer to next process (linked list impl)
    struct procfs_process *next;
}  procfs_process_t;

// proc state
typedef struct procfs_state
{
    int is_mounted; // is /proc mounted
    int num_processes; // total number of processes
    procfs_process_t *process_list; //linked list of processes
    procfs_flags_t flags; // current flags
    char mount_point[256]; // mount point, can vary
} procfs_state_t;

// function declarations

/*
* procfs_init - initialize procfs state
* @param state: Pointer to procfs_t to initialize
* @returns: PROCFS_OK on success, ERR_CODE on failure
*
*/
procfs_error_t procfs_init(procfs_state_t *state);

/*
* procfs_cleanup - clean up procfs state
* @param state: Pointer to procfs_state_t to cleanup
*  frees all allocated memory and resets state
*/
void procfs_cleanup(procfs_state_t *state);





#endif
