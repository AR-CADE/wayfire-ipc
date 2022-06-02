#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <wayfire/plugin.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/singleton-plugin.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/view.hpp>
#include <wayfire/workspace-manager.hpp>
#include <ipc/command.h>
#include <ipc/json.hpp>

#define PROCPATHLEN 64  // must hold /proc/2000222000/task/2000222000/cmdline

typedef struct task_t {
    char  state;
} task_t;

struct pre_unmapped_state {
    wl_event_loop *ev_loop;
    struct wl_event_source *event_source;
    int delay;
    wayfire_view &view;
};

static std::vector<pre_unmapped_state *> states;
static struct wl_listener display_destroy;

class wayfire_task_scanner
{
  public:

    wayfire_task_scanner()
    {
    }

    wayfire_task_scanner(const wayfire_task_scanner&) = delete;
    wayfire_task_scanner& operator =(const wayfire_task_scanner&) = delete;
    wayfire_task_scanner(wayfire_task_scanner&&) = delete;
    wayfire_task_scanner& operator =(wayfire_task_scanner&&) = delete;


    ~wayfire_task_scanner()
    {
    }


    task_t scan(pid_t pid) 
    {
        static __thread struct utlbuf_s ub = { NULL, 0 };    // buf for stat,statm,status
        static struct task_t t;    // buf for stat,statm,status

        if (file2str(std::string("/proc/" + std::to_string(pid)).c_str(), "status", &ub) != -1)
        {
                status2proc(ub.buf, t);
                std::cout << "state " << t.state  << "\n";
        }

        return t;
    }

    struct utlbuf_s {
        char *buf;     // dynamically grown buffer
        int   siz;     // current len of the above
    } utlbuf_s;

    static int file2str(const char *directory, const char *what, struct utlbuf_s *ub) 
    {
    #define buffGRW 1024
        char path[PROCPATHLEN];
        int fd, num, tot_read = 0, len;

        /* on first use we preallocate a buffer of minimum size to emulate
        former 'local static' behavior -- even if this read fails, that
        buffer will likely soon be used for another subdirectory anyway
        ( besides, with the calloc call we will never need use memcpy ) */
        if (ub->buf) 
            ub->buf[0] = '\0';
        else {
            ub->buf = (char *)calloc(1, (ub->siz = buffGRW));
            if (!ub->buf) {
                std::cout << "out of ressouces  in calloc (ub->buf == null) "  << "\n";
                return -1;
            }
        }
        len = snprintf(path, sizeof path, "%s/%s", directory, what);

        std::cout << "path = " << path  << "\n";

        
        if (len <= 0 || (size_t)len >= sizeof path) {
            std::cout << "out of ressouces (invalid len) " << len  << "\n";
            return -1;
        }
        
        if (-1 == (fd = open(path, O_RDONLY, 0))) {
            std::cout << "failed to open path " << path  << "\n";
            return -1;
        }
        
        while (0 < (num = read(fd, ub->buf + tot_read, ub->siz - tot_read))) {
            tot_read += num;
            if (tot_read < ub->siz) break;
            if (ub->siz >= std::numeric_limits<int>::max() - buffGRW) {
                tot_read--;
                break;
            }
            if (!(ub->buf = (char *)realloc(ub->buf, (ub->siz += buffGRW)))) {
                std::cout << "out of ressouces in realloc (ub->buf == null) " << len  << "\n";
                close(fd);
                return -1;
            }
        };
        ub->buf[tot_read] = '\0';
        close(fd);
        if (tot_read < 1) {
            std::cout << "out of ressouces (invalid tot_read) " << tot_read  << "\n";
            return -1;
        }
        return tot_read;
    #undef buffGRW
    }

typedef struct status_table_struct {
    unsigned char name[12];        // /proc/*/status field name
    unsigned char len;            // name length
#ifdef LABEL_OFFSET
    long offset;                  // jump address offset
#else
    void *addr;
#endif
} status_table_struct;

#ifdef LABEL_OFFSET
#define F(x) {#x, sizeof(#x)-1, (long)(&&case_##x-&&base)},
#else
#define F(x) {#x, sizeof(#x)-1, &&case_##x},
#endif
#define NUL  {"", 0, 0},

#define GPERF_TABLE_SIZE 128

// sometimes it's easier to do this manually, w/o gcc helping
#ifdef PROF
extern void __cyg_profile_func_enter(void*,void*);
#define ENTER(x) __cyg_profile_func_enter((void*)x,(void*)x)
#define LEAVE(x) __cyg_profile_func_exit((void*)x,(void*)x)
#else
#define ENTER(x)
#define LEAVE(x)
#endif


    static int status2proc (char *S, task_t &t) 
    {
        // 128 entries because we trust the kernel to use ASCII names
        static const unsigned char asso[] =
            {
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101,   6, 101,
            101, 101, 101, 101, 101,  45,  55,  25,  31,  50,
            50,  10,   0,  35, 101, 101,  21, 101,  30, 101,
            20,  36,   0,   5,   0,  40,   0,   0, 101, 101,
            101, 101, 101, 101, 101, 101, 101,  30, 101,  15,
                0,   1, 101,  10, 101,  10, 101, 101, 101,  25,
            101,  40,   0, 101,   0,  50,   6,  40, 101,   1,
            35, 101, 101, 101, 101, 101, 101, 101
            };

            static const status_table_struct table[GPERF_TABLE_SIZE] = {
            F(VmHWM)
            F(Threads)
            NUL NUL NUL
            F(VmRSS)
            F(VmSwap)
            NUL NUL NUL
            F(Tgid)
            F(VmStk)
            NUL NUL NUL
            F(VmSize)
            F(Gid)
            NUL NUL NUL
            F(VmPTE)
            F(VmPeak)
            NUL NUL NUL
            F(ShdPnd)
            F(Pid)
            NUL NUL NUL
            F(PPid)
            F(VmLib)
            NUL NUL NUL
            F(SigPnd)
            F(VmLck)
            NUL NUL NUL
            F(SigCgt)
            F(State)
            NUL NUL NUL
            F(CapPrm)
            F(Uid)
            NUL NUL NUL
            F(SigIgn)
            F(SigQ)
            NUL NUL NUL
            F(RssShmem)
            F(Name)
            NUL NUL NUL
            F(CapInh)
            F(VmData)
            NUL NUL NUL
            F(FDSize)
            NUL NUL NUL NUL
            F(SigBlk)
            NUL NUL NUL NUL
            F(CapEff)
            NUL NUL NUL NUL
            F(CapBnd)
            NUL NUL NUL NUL
            F(VmExe)
            NUL NUL NUL NUL
            F(Groups)
            NUL NUL NUL NUL
            F(RssAnon)
            NUL NUL NUL NUL
            F(RssFile)
            };

        #undef F
        #undef NUL

        ENTER(0x220);

            goto base;

            for(;;){
                char *colon;
                status_table_struct entry;

                // advance to next line
                S = strchr(S, '\n');
                if(!S) break;            // if no newline
                S++;

                // examine a field name (hash and compare)
            base:
                if(!*S) break;
                if((!S[0] || !S[1] || !S[2] || !S[3])) break;
                entry = table[(GPERF_TABLE_SIZE -1) & (asso[S[3]&127] + asso[S[2]&127] + asso[S[0]&127])];
                colon = strchr(S, ':');
                if(!colon) break;
                if(colon[1]!='\t') break;
                if(colon-S != entry.len) continue;
                if(memcmp(entry.name,S,colon-S)) continue;

                S = colon+2; // past the '\t'

        #ifdef LABEL_OFFSET
                goto *(&&base + entry.offset);
        #else
                goto *entry.addr;
        #endif  

            case_State:
                t.state = *S;
                continue;
            case_Name:
            case_ShdPnd:
            case_SigBlk:
            case_SigCgt:
            case_SigIgn:
            case_SigPnd:
            case_Tgid:
            case_Pid:
            case_PPid:
            case_Threads:
            case_Uid:
            case_Gid:
            case_VmData:
            case_VmExe:
            case_VmLck:
            case_VmLib:
            case_VmRSS:
            case_RssAnon: 
            case_RssFile: 
            case_RssShmem:
            case_VmSize:
            case_VmStk:
            case_VmSwap:
            case_Groups:
            case_CapBnd:
            case_CapEff:
            case_CapInh:
            case_CapPrm:
            case_FDSize:
            case_SigQ:
            case_VmHWM: 
            case_VmPTE:
            case_VmPeak: 
                continue;
            }

        LEAVE(0x220);
            return 0;
    }
#undef GPERF_TABLE_SIZE

};

class wayfire_autokill_singleton : public wf::plugin_interface_t
{

    static task_t scan(pid_t pid) 
    {
        static __thread struct utlbuf_s ub = { NULL, 0 };    // buf for stat,statm,status
        static struct task_t t;    // buf for stat,statm,status

        if (file2str(std::string("/proc/" + std::to_string(pid)).c_str(), "status", &ub) != -1)
        {
                status2proc(ub.buf, t);
                std::cout << "state " << t.state  << "\n";
        }

        return t;
    }

    struct utlbuf_s {
        char *buf;     // dynamically grown buffer
        int   siz;     // current len of the above
    } utlbuf_s;

    static int file2str(const char *directory, const char *what, struct utlbuf_s *ub) 
    {
    #define buffGRW 1024
        char path[PROCPATHLEN];
        int fd, num, tot_read = 0, len;

        /* on first use we preallocate a buffer of minimum size to emulate
        former 'local static' behavior -- even if this read fails, that
        buffer will likely soon be used for another subdirectory anyway
        ( besides, with the calloc call we will never need use memcpy ) */
        if (ub->buf) 
            ub->buf[0] = '\0';
        else {
            ub->buf = (char *)calloc(1, (ub->siz = buffGRW));
            if (!ub->buf) {
                std::cout << "out of ressouces  in calloc (ub->buf == null) "  << "\n";
                return -1;
            }
        }
        len = snprintf(path, sizeof path, "%s/%s", directory, what);

        std::cout << "path = " << path  << "\n";

        
        if (len <= 0 || (size_t)len >= sizeof path) {
            std::cout << "out of ressouces (invalid len) " << len  << "\n";
            return -1;
        }
        
        if (-1 == (fd = open(path, O_RDONLY, 0))) {
            std::cout << "failed to open path " << path  << "\n";
            return -1;
        }
        
        while (0 < (num = read(fd, ub->buf + tot_read, ub->siz - tot_read))) {
            tot_read += num;
            if (tot_read < ub->siz) break;
            if (ub->siz >= std::numeric_limits<int>::max() - buffGRW) {
                tot_read--;
                break;
            }
            if (!(ub->buf = (char *)realloc(ub->buf, (ub->siz += buffGRW)))) {
                std::cout << "out of ressouces in realloc (ub->buf == null) " << len  << "\n";
                close(fd);
                return -1;
            }
        };
        ub->buf[tot_read] = '\0';
        close(fd);
        if (tot_read < 1) {
            std::cout << "out of ressouces (invalid tot_read) " << tot_read  << "\n";
            return -1;
        }
        return tot_read;
    #undef buffGRW
    }

typedef struct status_table_struct {
    unsigned char name[12];        // /proc/*/status field name
    unsigned char len;            // name length
#ifdef LABEL_OFFSET
    long offset;                  // jump address offset
#else
    void *addr;
#endif
} status_table_struct;

#ifdef LABEL_OFFSET
#define F(x) {#x, sizeof(#x)-1, (long)(&&case_##x-&&base)},
#else
#define F(x) {#x, sizeof(#x)-1, &&case_##x},
#endif
#define NUL  {"", 0, 0},

#define GPERF_TABLE_SIZE 128

// sometimes it's easier to do this manually, w/o gcc helping
#ifdef PROF
extern void __cyg_profile_func_enter(void*,void*);
#define ENTER(x) __cyg_profile_func_enter((void*)x,(void*)x)
#define LEAVE(x) __cyg_profile_func_exit((void*)x,(void*)x)
#else
#define ENTER(x)
#define LEAVE(x)
#endif


    static int status2proc (char *S, task_t &t) 
    {
        // 128 entries because we trust the kernel to use ASCII names
        static const unsigned char asso[] =
            {
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
            101, 101, 101, 101, 101, 101, 101, 101,   6, 101,
            101, 101, 101, 101, 101,  45,  55,  25,  31,  50,
            50,  10,   0,  35, 101, 101,  21, 101,  30, 101,
            20,  36,   0,   5,   0,  40,   0,   0, 101, 101,
            101, 101, 101, 101, 101, 101, 101,  30, 101,  15,
                0,   1, 101,  10, 101,  10, 101, 101, 101,  25,
            101,  40,   0, 101,   0,  50,   6,  40, 101,   1,
            35, 101, 101, 101, 101, 101, 101, 101
            };

            static const status_table_struct table[GPERF_TABLE_SIZE] = {
            F(VmHWM)
            F(Threads)
            NUL NUL NUL
            F(VmRSS)
            F(VmSwap)
            NUL NUL NUL
            F(Tgid)
            F(VmStk)
            NUL NUL NUL
            F(VmSize)
            F(Gid)
            NUL NUL NUL
            F(VmPTE)
            F(VmPeak)
            NUL NUL NUL
            F(ShdPnd)
            F(Pid)
            NUL NUL NUL
            F(PPid)
            F(VmLib)
            NUL NUL NUL
            F(SigPnd)
            F(VmLck)
            NUL NUL NUL
            F(SigCgt)
            F(State)
            NUL NUL NUL
            F(CapPrm)
            F(Uid)
            NUL NUL NUL
            F(SigIgn)
            F(SigQ)
            NUL NUL NUL
            F(RssShmem)
            F(Name)
            NUL NUL NUL
            F(CapInh)
            F(VmData)
            NUL NUL NUL
            F(FDSize)
            NUL NUL NUL NUL
            F(SigBlk)
            NUL NUL NUL NUL
            F(CapEff)
            NUL NUL NUL NUL
            F(CapBnd)
            NUL NUL NUL NUL
            F(VmExe)
            NUL NUL NUL NUL
            F(Groups)
            NUL NUL NUL NUL
            F(RssAnon)
            NUL NUL NUL NUL
            F(RssFile)
            };

        #undef F
        #undef NUL

        ENTER(0x220);

            goto base;

            for(;;){
                char *colon;
                status_table_struct entry;

                // advance to next line
                S = strchr(S, '\n');
                if(!S) break;            // if no newline
                S++;

                // examine a field name (hash and compare)
            base:
                if(!*S) break;
                if((!S[0] || !S[1] || !S[2] || !S[3])) break;
                entry = table[(GPERF_TABLE_SIZE -1) & (asso[S[3]&127] + asso[S[2]&127] + asso[S[0]&127])];
                colon = strchr(S, ':');
                if(!colon) break;
                if(colon[1]!='\t') break;
                if(colon-S != entry.len) continue;
                if(memcmp(entry.name,S,colon-S)) continue;

                S = colon+2; // past the '\t'

        #ifdef LABEL_OFFSET
                goto *(&&base + entry.offset);
        #else
                goto *entry.addr;
        #endif  

            case_State:
                t.state = *S;
                continue;
            case_Name:
            case_ShdPnd:
            case_SigBlk:
            case_SigCgt:
            case_SigIgn:
            case_SigPnd:
            case_Tgid:
            case_Pid:
            case_PPid:
            case_Threads:
            case_Uid:
            case_Gid:
            case_VmData:
            case_VmExe:
            case_VmLck:
            case_VmLib:
            case_VmRSS:
            case_RssAnon: 
            case_RssFile: 
            case_RssShmem:
            case_VmSize:
            case_VmStk:
            case_VmSwap:
            case_Groups:
            case_CapBnd:
            case_CapEff:
            case_CapInh:
            case_CapPrm:
            case_FDSize:
            case_SigQ:
            case_VmHWM: 
            case_VmPTE:
            case_VmPeak: 
                continue;
            }

        LEAVE(0x220);
            return 0;
    }
#undef GPERF_TABLE_SIZE

    static void kill_view(const wayfire_view &view, bool zombieOnly = true)
    {
        if (view == nullptr)
        {
            return;    
        }

        wl_client * client = view->get_client();
        if (client != nullptr)
        {
            /* Get pid for native view */
            pid_t wl_pid = 0;
            wl_client_get_credentials(client, &wl_pid, 0, 0);

            if (wl_pid)
            {
                task_t task = scan(wl_pid);

                if (zombieOnly == true)
                {
                    switch (task.state) 
                    {
                        case 'R': // running
                        {
                            // let it run
                            std::cout << "task is runnig"  << "\n";
                            break;
                        }
                        case 'D': // uninterruptible sleep
                        {
                            // let it run
                            std::cout << "task is uninterruptible"  << "\n";
                            break;
                        }
                        case 'S': // sleeping
                        case 'Z': // zombie
                        case 'T': // stopped by job control signal
                        case 't': // stopped by debugger during trace
                        case 'I': // idle
                        {
                            // killall
                            std::cout << "killing the task"  << "\n";
                            kill(wl_pid, SIGTERM);
                            break;
                        }
                        default:
                            break;
                    }
                }
                else 
                {
                    kill(wl_pid, SIGTERM);
                }
            }    
        }
    }

    wf::signal_connection_t ping_timeout = [=] (wf::signal_data_t *data)
    {
        if (data == nullptr)
        {
            return;    
        }

        auto view = wf::get_signaled_view(data);

        if (view == nullptr)
        {
            return;    
        }

        kill_view(view);      

    };

    static void handle_display_destroy(struct wl_listener *listener,
                                        void *data) 
    {
        (void)listener;
        (void)data;

        for (auto it = states.begin(); it != states.end();)
        {   
            auto state = *it;

            if (state != nullptr)
            {
                auto event_source = state->event_source;

                if (event_source != nullptr) {
                    wl_event_source_remove(event_source);
                }

                free(state);
            } 

            it = states.erase(it);
        }

         wl_list_remove(&display_destroy.link);
    }

    static int handle_view_timeout(void *data) {
        struct pre_unmapped_state *state = (struct pre_unmapped_state *)data;

        auto view = state->view;
        
        if (view == nullptr)
        {
            return 0;    
        }

        for (auto it = states.begin(); it != states.end();)
        {   
            auto s = *it;

            if (s != nullptr)
            {
                if (s->view == view)
                {
                    auto event_source = s->event_source;

                    if (event_source != nullptr) {
                        wl_event_source_remove(event_source);
                    }

                    free(s);
                    it = states.erase(it);
                }

            } 
        }       

        kill_view(view, false);

        return 0;
    }

    wf::signal_connection_t view_pre_unmapped = [=] (wf::signal_data_t *data)
    {
        if (data == nullptr)
        {
            return;    
        }

        auto view = wf::get_signaled_view(data);

        if (view == nullptr)
        {
            return;    
        }

        view->ping();
        view->connect_signal("ping-timeout", &ping_timeout);


        struct pre_unmapped_state *state = (struct pre_unmapped_state *)malloc(sizeof(struct pre_unmapped_state));

        display_destroy.notify = handle_display_destroy;
        wl_display_add_destroy_listener(wf::get_core().display, &display_destroy);

        state->ev_loop = wf::get_core().ev_loop;
        state->view = view;
        state->delay = 1500;
        state->event_source =
        wl_event_loop_add_timer(state->ev_loop, handle_view_timeout, state);

        wl_event_source_timer_update(state->event_source, state->delay);

        states.push_back(state);
    };

    void init() override
    {
        output->connect_signal("view-pre-unmapped", &view_pre_unmapped);
    }

    void fini() override
    {
        output->disconnect_signal(&view_pre_unmapped);

        auto views = wf::get_core().get_all_views();

        for (wayfire_view view : views)
        {
            view->disconnect_signal(&view_pre_unmapped);
            view->disconnect_signal(&ping_timeout);
        }

        handle_display_destroy(nullptr, nullptr);
    }
};

DECLARE_WAYFIRE_PLUGIN(wayfire_autokill_singleton);
