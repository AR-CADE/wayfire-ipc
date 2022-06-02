#ifndef _WAYFIRE_TASK_PROC_H
#define _WAYFIRE_TASK_PROC_H

#define PROCPATHLEN 64  // must hold /proc/2000222000/task/2000222000/cmdline

typedef struct task_t {
    char  state = 0;
} task_t;

#endif