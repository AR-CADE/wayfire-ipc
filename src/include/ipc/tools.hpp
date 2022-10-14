#ifndef _WAYFIRE_IPC_TOOLS_H
#define _WAYFIRE_IPC_TOOLS_H

#include <cassert>
#include <cstddef>
#include <cstring>
#include <ipc.h>
#include <string>
#include <vector>
#include <wayfire/core.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/output.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/workspace-manager.hpp>

#define INVALID_WORKSPACE wf::point_t{-1, -1}

#define ASSERT_VALID_WORKSPACE(w, p) \
    assert(w != nullptr);assert(w->is_workspace_valid( \
    point));

class ipc_tools
{
  public:
    static wf::point_t closest_valid_ws(wf::point_t workspace)
    {
        auto grid =
            wf::get_core().get_active_output()->workspace->get_workspace_grid_size();
        workspace.x = wf::clamp(workspace.x, 0, grid.width - 1);
        workspace.y = wf::clamp(workspace.y, 0, grid.height - 1);
        return workspace;
    }

    static wf::point_t get_view_main_workspace(wayfire_view view)
    {
        auto og = wf::get_core().get_active_output()->get_screen_size();
        auto ws =
            wf::get_core().get_active_output()->workspace->get_current_workspace();

        auto wm = view->transform_region(view->get_wm_geometry());
        wf::point_t workspace = {
            ws.x + (int)std::floor((wm.x + wm.width / 2.0) / og.width),
            ws.y + (int)std::floor((wm.y + wm.height / 2.0) / og.height)
        };

        return closest_valid_ws(workspace);
    }

    static wf::point_t convert_workspace_index_to_coords(int index)
    {
        int i = 0;
        auto outputs = wf::get_core().output_layout->get_outputs();
        for (wf::output_t *output : outputs)
        {
            auto wsize = output->workspace->get_workspace_grid_size();
            for (int x = 0; x < wsize.width; x++)
            {
                for (int y = 0; y < wsize.height; y++)
                {
                    i++;
                    wf::point_t coord{x, y};

                    if (i == index)
                    {
                        return coord;
                    }
                }
            }
        }

        return INVALID_WORKSPACE;
    }

    static wf::point_t get_workspace_at_index(int index, wf::output_t *output)
    {
        assert(output != nullptr);
        auto wsize = output->workspace->get_workspace_grid_size();

        for (int x = 0; x < wsize.width; x++)
        {
            for (int y = 0; y < wsize.height; y++)
            {
                auto point = wf::point_t{x, y};
                auto i     = get_workspace_index(point, output);
                if (i == index)
                {
                    return point;
                }
            }
        }

        return INVALID_WORKSPACE;
    }

    static int get_workspace_id(uint32_t output_id, uint32_t workspace_index)
    {
        std::string id = std::to_string(output_id) + std::to_string(workspace_index);
        return std::stoi(id);
    }

    static int get_workspace_index(wf::point_t point, wf::output_t *output)
    {
        assert(output != nullptr);
        ASSERT_VALID_WORKSPACE(output->workspace, point);

        auto wsize = output->workspace->get_workspace_grid_size();
        return point.x + point.y * wsize.width + 1;
    }

    static std::vector<std::string> split_string(std::string s,
        const std::string & delimiter)
    {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            token     = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    static inline const char *argsep_next_interesting(const char *src,
        const char *delim)
    {
        const char *special    = strpbrk(src, "\"'\\");
        const char *next_delim = strpbrk(src, delim);
        if (!special)
        {
            return next_delim;
        }

        if (!next_delim)
        {
            return special;
        }

        return (next_delim < special) ? next_delim : special;
    }

    static char *argsep(char **stringp, const char *delim, char *matched)
    {
        char *start = *stringp;
        char *end   = start;
        bool in_string = false;
        bool in_char   = false;
        bool escaped   = false;
        char *interesting = nullptr;

        while ((interesting = (char*)argsep_next_interesting(end, delim)))
        {
            if (escaped && (interesting != end))
            {
                escaped = false;
            }

            if ((*interesting == '"') && !in_char && !escaped)
            {
                in_string = !in_string;
                end = interesting + 1;
            } else if ((*interesting == '\'') && !in_string && !escaped)
            {
                in_char = !in_char;
                end     = interesting + 1;
            } else if (*interesting == '\\')
            {
                escaped = !escaped;
                end     = interesting + 1;
            } else if (!in_string && !in_char && !escaped)
            {
                // We must have matched a separator
                end = interesting;
                if (matched)
                {
                    *matched = *end;
                }

                if (end - start)
                {
                    *(end++) = 0;
                    *stringp = end;
                    break;
                } else
                {
                    end = ++start;
                }
            } else
            {
                end++;
            }
        }

        if (!interesting)
        {
            *stringp = nullptr;
            if (matched)
            {
                *matched = '\0';
            }
        }

        return start;
    }

#if STRICT_C
    static char *join_args(char **argv, int argc)
    {
        if (!(argc > 0))
        {
            LOGE("argc should be positive");
            return nullptr;
        }

        int len = 0, i;
        for (i = 0; i < argc; ++i)
        {
            len += strlen(argv[i]) + 1;
        }

        char *res = (char*)malloc(len);
        len = 0;
        for (i = 0; i < argc; ++i)
        {
            strcpy(res + len, argv[i]);
            len += strlen(argv[i]);
            res[len++] = ' ';
        }

        res[len - 1] = '\0';
        return res;
    }

#else
    static std::string join_args(char **argv, int argc)
    {
        if (!(argc > 0))
        {
            LOGE("argc should be positive");
            return nullptr;
        }

        std::string res;

        for (int i = 0; i < argc; ++i)
        {
            if (i > 0)
            {
                res.append(" ");
            }

            res.append(std::string(argv[i]));
        }

        return res;
    }

#endif

    constexpr static const char whitespace[] = " \f\n\r\t\v";

    static char **split_args(const char *start, int *argc)
    {
        *argc     = 0;
        int alloc = 2;
        char **argv    = (char**)malloc(sizeof(char*) * alloc);
        bool in_token  = false;
        bool in_string = false;
        bool in_char   = false;
        bool in_brackets = false; // brackets are used for criteria
        bool escaped     = false;
        const char *end  = start;
        if (start)
        {
            while (*start)
            {
                if (!in_token)
                {
                    start    = (end += strspn(end, whitespace));
                    in_token = true;
                }

                if ((*end == '"') && !in_char && !escaped)
                {
                    in_string = !in_string;
                } else if ((*end == '\'') && !in_string && !escaped)
                {
                    in_char = !in_char;
                } else if ((*end == '[') && !in_string && !in_char && !in_brackets &&
                           !escaped)
                {
                    in_brackets = true;
                } else if ((*end == ']') && !in_string && !in_char && in_brackets &&
                           !escaped)
                {
                    in_brackets = false;
                } else if (*end == '\\')
                {
                    escaped = !escaped;
                } else if ((*end == '\0') ||
                           (!in_string && !in_char && !in_brackets &&
                            !escaped && strchr(whitespace, *end)))
                {
                    goto add_token;
                }

                if (*end != '\\')
                {
                    escaped = false;
                }

                ++end;
                continue;
add_token:
                if (end - start > 0)
                {
                    char *token = (char*)malloc(end - start + 1);
                    strncpy(token, start, end - start + 1);
                    token[end - start] = '\0';
                    argv[*argc] = token;
                    int tmp_argc = *argc;
                    if (++*argc + 1 == alloc)
                    {
                        char **tmp_argv = (char**)realloc(argv,
                            (alloc *= 2) *
                            sizeof(char*));
                        if (nullptr == tmp_argv)
                        {
                            free_argv(tmp_argc, argv);
                            return nullptr;
                        } else
                        {
                            argv = tmp_argv;
                        }
                    }
                }

                in_token = false;
                escaped  = false;
            }
        }

        argv[*argc] = nullptr;
        return argv;
    }

    static void free_argv(int argc, char **argv)
    {
        while (argc-- > 0)
        {
            free(argv[argc]);
        }

        free(argv);
    }

    static void strip_quotes(char *str)
    {
        bool in_str  = false;
        bool in_chr  = false;
        bool escaped = false;
        char *end    = strchr(str, 0);
        while (*str)
        {
            if ((*str == '\'') && !in_str && !escaped)
            {
                in_chr = !in_chr;
                goto shift_over;
            } else if ((*str == '\"') && !in_chr && !escaped)
            {
                in_str = !in_str;
                goto shift_over;
            } else if (*str == '\\')
            {
                escaped = !escaped;
                ++str;
                continue;
            }

            escaped = false;
            ++str;
            continue;
shift_over:
            memmove(str, str + 1, end-- - str);
        }

        *end = '\0';
    }

    static ipc_event_type event_to_type(const std::string & s)
    {
        // Events sent from sway to clients. Events have the highest bits set.
        if (s == IPC_I3_EVENT_WORKSPACE)
        {
            return IPC_I3_EVENT_TYPE_WORKSPACE;
        }

        if (s == IPC_SWAY_EVENT_OUTPUT)
        {
            return IPC_I3_EVENT_TYPE_OUTPUT;
        }

        if (s == IPC_I3_EVENT_WINDOW)
        {
            return IPC_I3_EVENT_TYPE_WINDOW;
        }

        if (s == IPC_I3_EVENT_BINDING)
        {
            return IPC_I3_EVENT_TYPE_BINDING;
        }

        if (s == IPC_I3_EVENT_SHUTDOWN)
        {
            return IPC_I3_EVENT_TYPE_SHUTDOWN;
        }

        if (s == IPC_I3_EVENT_TICK)
        {
            return IPC_I3_EVENT_TYPE_TICK;
        }

        // sway-specific event types
        if (s == IPC_SWAY_EVENT_INPUT)
        {
            return IPC_SWAY_EVENT_TYPE_INPUT;
        }

        //
        // wayfire specific event types
        //
        if (s == IPC_WF_EVENT_WORKSPACE_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_WORKSPACE_CHANGED;
        }

        if (s == IPC_WF_EVENT_OUTPUT_ADDED)
        {
            return IPC_WF_EVENT_TYPE_OUTPUT_ADDED;
        }

        if (s == IPC_WF_EVENT_OUTPUT_REMOVED)
        {
            return IPC_WF_EVENT_TYPE_OUTPUT_REMOVED;
        }

        if (s == IPC_WF_EVENT_OUTPUT_CONFIGURATION_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_OUTPUT_CONFIGURATION_CHANGED;
        }

        if (s == IPC_WF_EVENT_VIEW_PRE_MOVED_TO_OUTPUT)
        {
            return IPC_WF_EVENT_TYPE_VIEW_PRE_MOVED_TO_OUTPUT;
        }

        if (s == IPC_WF_EVENT_VIEW_ABOVE_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_ABOVE_CHANGED;
        }

        if (s == IPC_WF_EVENT_VIEW_HINTS_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_HINTS_CHANGED;
        }

        if (s == IPC_WF_EVENT_VIEW_MOVED_TO_OUTPUT)
        {
            return IPC_WF_EVENT_TYPE_VIEW_MOVED_TO_OUTPUT;
        }

        if (s == IPC_WF_EVENT_VIEW_MAPPED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_MAPPED;
        }

        if (s == IPC_WF_EVENT_VIEW_LAYER_ATTACHED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_LAYER_ATTACHED;
        }

        if (s == IPC_WF_EVENT_VIEW_LAYER_DETACHED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_LAYER_DETACHED;
        }

        if (s == IPC_WF_EVENT_VIEW_APP_ID_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_APP_ID_CHANGED;
        }

        if (s == IPC_WF_EVENT_VIEW_TITLE_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_TITLE_CHANGED;
        }

        if (s == IPC_WF_EVENT_VIEW_GEOMETRY_CHANGED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_GEOMETRY_CHANGED;
        }

        if (s == IPC_WF_EVENT_VIEW_TILED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_TILED;
        }

        if (s == IPC_WF_EVENT_VIEW_FOCUSED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_FOCUSED;
        }

        if (s == IPC_WF_EVENT_VIEW_UNMAPPED)
        {
            return IPC_WF_EVENT_TYPE_VIEW_UNMAPPED;
        }

        if (s == IPC_WF_EVENT_POINTER_BUTTON)
        {
            return IPC_WF_EVENT_TYPE_POINTER_BUTTON;
        }

        if (s == IPC_WF_EVENT_TABLET_BUTTON)
        {
            return IPC_WF_EVENT_TYPE_TABLET_BUTTON;
        }

        LOGE("Unsupported event type");
        return IPC_WF_EVENT_TYPE_UNSUPPORTED;
    }
};

#endif
