#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <json/value.h>
#include <string>
#include <vector>
#include <wayfire/core.hpp>
#include <wayfire/toplevel-view.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/plugins/common/util.hpp>
#include <wayfire/view.hpp>
#include <wayfire/output.hpp>
#include <wayfire/workspace-set.hpp>
#include "criteria.hpp"
#include <ipc/json.hpp>

// strcmp that also handles null pointers.
int criteria::lenient_strcmp(const char *a, const char *b)
{
    if (a == b)
    {
        return 0;
    } else if (!a)
    {
        return -1;
    } else if (!b)
    {
        return 1;
    } else
    {
        return strcmp(a, b);
    }
}

bool criteria::is_empty()
{
    return !this->title &&
           !this->shell &&
           !this->app_id &&
           !this->con_mark &&
           !this->con_id
#if HAVE_XWAYLAND && WIP
           && !this->class &&
           !this->id &&
           !this->instance &&
           !this->window_role &&
           this->window_type == ATOM_LAST
#endif
           && !this->floating &&
           !this->tiling &&
           !this->urgent &&
           !this->workspace &&
           !this->pid;
}

// Returns error string on failure or nullptr otherwise.
bool criteria::generate_regex(pcre2_code **regex, char *value)
{
    int errorcode;
    PCRE2_SIZE offset;

    *regex = pcre2_compile((PCRE2_SPTR)value, PCRE2_ZERO_TERMINATED,
        PCRE2_UTF | PCRE2_UCP, &errorcode, &offset, NULL);
    if (!*regex)
    {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));

        const char *fmt = "Regex compilation for '%s' failed: %s";
        int len = strlen(fmt) + strlen(value) + strlen(
            reinterpret_cast<char*>(buffer)) - 3;
        error = static_cast<char*>(malloc(len));
        snprintf(error, len, fmt, value, buffer);
        return false;
    }

    return true;
}

bool criteria::pattern_create(struct pattern **pattern, char *value)
{
    *pattern = static_cast<struct pattern*>(calloc(1, sizeof(struct pattern)));
    if (!*pattern)
    {
        LOGE("Failed to allocate pattern");
    }

    if (strcmp(value, "__focused__") == 0)
    {
        (*pattern)->match_type = PATTERN_FOCUSED;
    } else
    {
        (*pattern)->match_type = PATTERN_PCRE2;
        if (!generate_regex(&(*pattern)->regex, value))
        {
            return false;
        }
    }

    return true;
}

void criteria::pattern_destroy(struct pattern *pattern)
{
    if (pattern)
    {
        if (pattern->regex)
        {
            pcre2_code_free(pattern->regex);
            pattern->regex = nullptr;
        }

        free(pattern);
    }
}

void criteria::destroy()
{
    pattern_destroy(this->title);
    this->title = nullptr;
    pattern_destroy(this->shell);
    this->shell = nullptr;
    pattern_destroy(this->app_id);
    this->app_id = nullptr;
#if HAVE_XWAYLAND && WIP
    pattern_destroy(this->class);
    this->class = nullptr;
    pattern_destroy(this->instance);
    this->instance = nullptr;
    pattern_destroy(this->window_role);
    this->window_role = nullptr;
#endif
    pattern_destroy(this->con_mark);
    this->con_mark = nullptr;
    pattern_destroy(this->workspace);
    this->workspace = nullptr;

    if (this->target != nullptr)
    {
        free(this->target);
        this->target = nullptr;
    }

    if (this->cmdlist != nullptr)
    {
        free(this->cmdlist);
        this->cmdlist = nullptr;
    }

    if (this->raw != nullptr)
    {
        free(this->raw);
        this->raw = nullptr;
    }
}

int criteria::regex_cmp(const char *item, const pcre2_code *regex)
{
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    int result = pcre2_match(regex, (PCRE2_SPTR)item, strlen(
        item), 0, 0, match_data, NULL);
    pcre2_match_data_free(match_data);
    return result;
}

#if HAVE_XWAYLAND && WIP
bool criteria::view_has_window_type(struct sway_view *view, enum atom_name name)
{
    if (view->type != SWAY_VIEW_XWAYLAND)
    {
        return false;
    }

    struct wlr_xwayland_surface *surface = view->wlr_xwayland_surface;
    struct sway_xwayland *xwayland = &server.xwayland;
    xcb_atom_t desired_atom = xwayland->atoms[name];
    for (size_t i = 0; i < surface->window_type_len; ++i)
    {
        if (surface->window_type[i] == desired_atom)
        {
            return true;
        }
    }

    return false;
}

#endif

/* static int cmp_urgent(const wayfire_view& a, const wayfire_view& b) {
 *
 *   if (a->last_focus_timestamp < b->last_focus_timestamp) {
 *       return -1;
 *   } else if (a->last_focus_timestamp > b->last_focus_timestamp) {
 *       return 1;
 *   }
 *   return 0;
 *  } */

bool criteria::has_container()
{
    return /* criteria->con_mark || */ this->con_id;
}

bool criteria::matches_container(const Json::Value& container)
{
    if (this->con_mark)
    {
        //
        // mark is not supported
        //
    }

    if (this->con_id) // Internal ID
    {
        int id = container.get("id", Json::nullValue).asInt();

        if ((uint32_t)id != this->con_id)
        {
            return false;
        }
    }

    return true;
}

bool criteria::matches_view(const Json::Value& view)
{
    wf::output_t *focus  = wf::get_core().get_active_output();
    wayfire_view focused = focus ? focus->get_active_view() : nullptr;

    if (this->title)
    {
        Json::Value name_object = view.get("name", Json::nullValue);
        if (name_object.isNull() || !name_object.isString())
        {
            name_object = "";
        }

        std::string name = name_object.asString();
        const char *title_str = name.c_str();

        switch (this->title->match_type)
        {
          case PATTERN_FOCUSED:
        {
            std::string focused_title_str = focused->get_title();
            if (focused && lenient_strcmp(title_str, focused_title_str.c_str()))
            {
                return false;
            }

            break;
        }

          case PATTERN_PCRE2:
        {
            if (regex_cmp(title_str, this->title->regex) != 0)
            {
                return false;
            }

            break;
        }
        }
    }

    if (this->shell)
    {
        Json::Value view_shell_ojb = view.get("shell", Json::nullValue);
        if (view_shell_ojb.isNull() || !view_shell_ojb.isString())
        {
            view_shell_ojb = "";
        }

        std::string view_shell_str = view_shell_ojb.asString();
        const char *view_shell     = view_shell_str.c_str();

        switch (this->shell->match_type)
        {
          case PATTERN_FOCUSED:
        {
            Json::Value f = ipc_json::describe_view(focused);
            Json::Value focused_shell_ojb = f.get("shell", Json::nullValue);
            if (focused_shell_ojb.isNull() || !focused_shell_ojb.isString())
            {
                return false;
            }

            std::string focused_shell_str = focused_shell_ojb.asString();
            const char *focused_shell     = focused_shell_str.c_str();

            if (focused && strcmp(view_shell, focused_shell))
            {
                return false;
            }

            break;
        }

          case PATTERN_PCRE2:
        {
            if (regex_cmp(view_shell, this->shell->regex) != 0)
            {
                return false;
            }

            break;
        }
        }
    }

    if (this->app_id)
    {
        Json::Value app_id_obj = view.get("app_id", Json::nullValue);
        if (app_id_obj.isNull() || !app_id_obj.isString())
        {
            app_id_obj = "";
        }

        std::string view_app_id_str = app_id_obj.asString();
        const char *app_id_str = view_app_id_str.c_str();

        switch (this->app_id->match_type)
        {
          case PATTERN_FOCUSED:
        {
            std::string focused_app_id_str = focused->get_app_id();
            if (focused && lenient_strcmp(app_id_str, focused_app_id_str.c_str()))
            {
                return false;
            }

            break;
        }

          case PATTERN_PCRE2:
        {
            if (regex_cmp(app_id_str, this->app_id->regex) != 0)
            {
                return false;
            }

            break;
        }
        }
    }

    if (!matches_container(view))
    {
        return false;
    }

#if HAVE_XWAYLAND && WIP
    if (this->id) // X11 window ID
    {
        uint32_t x11_window_id = view_get_x11_window_id(view);
        if (!x11_window_id || (x11_window_id != this->id))
        {
            return false;
        }
    }

    if (this->class)
    {
        const char *class = view_get_class(view);
        if (!class)
        {
            class = "";
        }

        switch (this->class->match_type)
        {
          case PATTERN_FOCUSED:
            if (focused && lenient_strcmp(class, view_get_class(focused)))
            {
                return false;
            }

            break;

          case PATTERN_PCRE:
            if (regex_cmp(class, this->class->regex) != 0)
            {
                return false;
            }

            break;
        }
    }

    if (this->instance)
    {
        const char *instance = view_get_instance(view);
        if (!instance)
        {
            instance = "";
        }

        switch (this->instance->match_type)
        {
          case PATTERN_FOCUSED:
            if (focused && strcmp(instance, view_get_instance(focused)))
            {
                return false;
            }

            break;

          case PATTERN_PCRE:
            if (regex_cmp(instance, this->instance->regex) != 0)
            {
                return false;
            }

            break;
        }
    }

    if (this->window_role)
    {
        const char *window_role = view_get_window_role(view);
        if (!window_role)
        {
            window_role = "";
        }

        switch (this->window_role->match_type)
        {
          case PATTERN_FOCUSED:
            if (focused && strcmp(window_role, view_get_window_role(focused)))
            {
                return false;
            }

            break;

          case PATTERN_PCRE:
            if (regex_cmp(window_role, this->window_role->regex) != 0)
            {
                return false;
            }

            break;
        }
    }

    if (this->window_type != ATOM_LAST)
    {
        if (!view_has_window_type(view, this->window_type))
        {
            return false;
        }
    }

#endif

    if (this->floating)
    {
        Json::Value type_shell_ojb = view.get("type", Json::nullValue);
        if (type_shell_ojb.isNull() || !type_shell_ojb.isString())
        {
            return false;
        }

        if (!(type_shell_ojb.asString() == "floating_con"))
        {
            return false;
        }
    }

    if (this->tiling)
    {
#if 0
        Json::Value v = ipc_json::describe_view(view);
        Json::Value type_shell_ojb = v.get("type", Json::nullValue);
        if (type_shell_ojb.isNull() || !type_shell_ojb.isString())
        {
            return false;
        }

        if (type_shell_ojb.asString() == "floating_con")
        {
            return false;
        }

#endif

        //
        // unsupported
        //
        return false;
    }

    if (this->urgent)
    {
        Json::Value is_urgent = view.get("urgent", Json::nullValue);
        if (is_urgent.isNull() || !is_urgent.isBool() ||
            (is_urgent.asBool() == false))
        {
            return false;
        }

        std::vector<wayfire_view> urgent_views;

        for (auto& container : wf::get_core().get_all_views())
        {
            Json::Value c = ipc_json::describe_view(container);
            Json::Value u = c.get("urgent", Json::nullValue);

            if (!container || !u.isBool() || (u.asBool() == false))
            {
                continue;
            }

            urgent_views.push_back(container);
        }

        std::sort(urgent_views.begin(), urgent_views.end(),
            [] (wayfire_view a, wayfire_view b)
        {
            uint64_t a_last_focus_timestamp = get_focus_timestamp(a);
            uint64_t b_last_focus_timestamp = get_focus_timestamp(b);

            if (a_last_focus_timestamp < b_last_focus_timestamp)
            {
                return -1;
            } else if (a_last_focus_timestamp > b_last_focus_timestamp)
            {
                return 1;
            }

            return 0;
        });

        wayfire_view urgent_target;
        if (this->urgent == 'o') // oldest
        {
            urgent_target = urgent_views[0];
        } else // latest
        {
            urgent_target = urgent_views[urgent_views.size() - 1];
        }

        int id = view.get("id", Json::nullValue).asInt();
        if ((uint32_t)id != urgent_target->get_id())
        {
            return false;
        }
    }

    if (this->workspace)
    {
        int id = view.get("id", Json::nullValue).asInt();
        Json::Value workspace_name = Json::nullValue;

        for (const wayfire_view& v : wf::get_core().get_all_views())
        {
            if ((uint32_t)id != v->get_id())
            {
                wf::point_t point =
                    wf::get_core().get_active_output()->wset()->
                    get_view_main_workspace(
                        wf::toplevel_cast(v));
                Json::Value ws = ipc_json::describe_workspace(point,
                    v->get_output());
                workspace_name = ws.get("name", Json::nullValue);
            }
        }

        if (workspace_name.isNull() || (workspace_name.isString() == false))
        {
            return false;
        }

        std::string name = workspace_name.asString();

        switch (this->workspace->match_type)
        {
          case PATTERN_FOCUSED:
        {
            if (focused)
            {
                wf::point_t focused_ws =
                    focused->get_output()->wset()->get_view_main_workspace(wf::toplevel_cast(focused));
                Json::Value focused_workspace = ipc_json::describe_workspace(
                    focused_ws, focused->get_output());
                Json::Value focused_workspace_name = focused_workspace.get("name",
                    Json::nullValue);

                if (!focused_workspace_name.isNull() &&
                    focused_workspace_name.isString())
                {
                    std::string focused_workspace_name_str =
                        focused_workspace_name.asString();

                    if (strcmp(name.c_str(), focused_workspace_name_str.c_str()))
                    {
                        return false;
                    }
                }
            }

            break;
        }

          case PATTERN_PCRE2:
            if (regex_cmp(name.c_str(), this->workspace->regex) != 0)
            {
                return false;
            }

            break;
        }
    }

    if (this->pid)
    {
        int view_pid = view.get("pid", Json::nullValue).asInt();

        if (this->pid != view_pid)
        {
            return false;
        }
    }

    return true;
}

/* std::vector<struct criteria *> criteria_for_view(const wayfire_view& view, enum
 * criteria_type types) {
 *   std::vector<struct criteria *> criterias = config->criteria;
 *   std::vector<struct criteria *>matches;
 *   for (auto criteria : criterias) {
 *       if ((criteria->type & types) && criteria_matches_view(criteria, view)) {
 *           matches.push_back(criteria);
 *       }
 *   }
 *
 *   return matches;
 *  } */

/* struct match_data {
 *   struct criteria *criteria;
 *   Json::Value matches;
 *  }; */

void criteria::node_for_each_matched_container(Json::Value obj,
    Json::Value *containers)
{
    if (obj.isNull())
    {
        return;
    }

    std::string type  = obj.get("type", Json::nullValue).asString();
    Json::Value shell = obj.get("shell", Json::nullValue);

    if ((type == "workspace") || !shell.isNull())
    {
        containers->append(obj);
    }

    Json::Value nodes_obj = obj.get("nodes", Json::nullValue);
    if ((nodes_obj.isNull() == false) && nodes_obj.isArray())
    {
        for (const auto& node : nodes_obj)
        {
            node_for_each_matched_container(node, containers);
        }
    }

    Json::Value floating_nodes_obj = obj.get("floating_nodes", Json::nullValue);
    if ((floating_nodes_obj.isNull() == false) && floating_nodes_obj.isArray())
    {
        for (const auto& node : floating_nodes_obj)
        {
            node_for_each_matched_container(node, containers);
        }
    }
}

Json::Value criteria::get_containers()
{
    Json::Value matches = Json::arrayValue;

/*  struct match_data data = {
 *       .criteria = this,
 *       .matches = matches,
 *   }; */

    Json::Value containers;
    Json::Value root = ipc_json::get_tree();
    node_for_each_matched_container(root, &containers);

    for (const auto& container : containers)
    {
        std::string container_type =
            container.get("type", Json::nullValue).asString();

        if ((container_type == "con") || (container_type == "floating_con"))
        {
            if (matches_view(container))
            {
                matches.append(container);
            }
        } else if (has_container())
        {
            if (matches_container(container))
            {
                matches.append(container);
            }
        }
    }

    return matches;
}

#if HAVE_XWAYLAND && WIP
enum atom_name criteria::parse_window_type(const char *type)
{
    if (strcasecmp(type, "normal") == 0)
    {
        return NET_WM_WINDOW_TYPE_NORMAL;
    } else if (strcasecmp(type, "dialog") == 0)
    {
        return NET_WM_WINDOW_TYPE_DIALOG;
    } else if (strcasecmp(type, "utility") == 0)
    {
        return NET_WM_WINDOW_TYPE_UTILITY;
    } else if (strcasecmp(type, "toolbar") == 0)
    {
        return NET_WM_WINDOW_TYPE_TOOLBAR;
    } else if (strcasecmp(type, "splash") == 0)
    {
        return NET_WM_WINDOW_TYPE_SPLASH;
    } else if (strcasecmp(type, "menu") == 0)
    {
        return NET_WM_WINDOW_TYPE_MENU;
    } else if (strcasecmp(type, "dropdown_menu") == 0)
    {
        return NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
    } else if (strcasecmp(type, "popup_menu") == 0)
    {
        return NET_WM_WINDOW_TYPE_POPUP_MENU;
    } else if (strcasecmp(type, "tooltip") == 0)
    {
        return NET_WM_WINDOW_TYPE_TOOLTIP;
    } else if (strcasecmp(type, "notification") == 0)
    {
        return NET_WM_WINDOW_TYPE_NOTIFICATION;
    }

    return ATOM_LAST; // ie. invalid
}

#endif

enum criteria_token criteria::token_from_name(char *name)
{
    if (strcmp(name, "app_id") == 0)
    {
        return T_APP_ID;
    } else if (strcmp(name, "con_id") == 0)
    {
        return T_CON_ID;
    } else if (strcmp(name, "con_mark") == 0)
    {
        return T_CON_MARK;
#if HAVE_XWAYLAND && WIP
    } else if (strcmp(name, "class") == 0)
    {
        return T_CLASS;
    } else if (strcmp(name, "id") == 0)
    {
        return T_ID;
    } else if (strcmp(name, "instance") == 0)
    {
        return T_INSTANCE;
    } else if (strcmp(name, "window_role") == 0)
    {
        return T_WINDOW_ROLE;
    } else if (strcmp(name, "window_type") == 0)
    {
        return T_WINDOW_TYPE;
#endif
    } else if (strcmp(name, "shell") == 0)
    {
        return T_SHELL;
    } else if (strcmp(name, "title") == 0)
    {
        return T_TITLE;
    } else if (strcmp(name, "urgent") == 0)
    {
        return T_URGENT;
    } else if (strcmp(name, "workspace") == 0)
    {
        return T_WORKSPACE;
    } else if (strcmp(name, "tiling") == 0)
    {
        return T_TILING;
    } else if (strcmp(name, "floating") == 0)
    {
        return T_FLOATING;
    } else if (strcmp(name, "pid") == 0)
    {
        return T_PID;
    }

    return T_INVALID;
}

bool criteria::parse_token(char *name, char *value)
{
    enum criteria_token token = token_from_name(name);
    if (token == T_INVALID)
    {
        const char *fmt = "Token '%s' is not recognized";
        int len = strlen(fmt) + strlen(name) - 1;
        error = static_cast<char*>(malloc(len));
        snprintf(error, len, fmt, name);
        return false;
    }

    // Require value, unless token is floating or tiled
    if (!value && (token != T_FLOATING) && (token != T_TILING))
    {
        const char *fmt = "Token '%s' requires a value";
        int len = strlen(fmt) + strlen(name) - 1;
        error = static_cast<char*>(malloc(len));
        snprintf(error, len, fmt, name);
        return false;
    }

    char *endptr = nullptr;
    switch (token)
    {
      case T_TITLE:
        pattern_create(&this->title, value);
        break;

      case T_SHELL:
        pattern_create(&this->shell, value);
        break;

      case T_APP_ID:
        pattern_create(&this->app_id, value);
        break;

      case T_CON_ID:
        if (strcmp(value, "__focused__") == 0)
        {
            wf::output_t *focus = wf::get_core().get_active_output();
            wayfire_view view   = focus ? focus->get_active_view() : nullptr;
            this->con_id = view ? view->get_id() : 0;
        } else
        {
            this->con_id = strtoul(value, &endptr, 10);
            if (*endptr != 0)
            {
                error = strdup(
                    "The value for 'con_id' should be '__focused__' or numeric");
            }
        }

        break;

      case T_CON_MARK:
        pattern_create(&this->con_mark, value);
        break;

#if HAVE_XWAYLAND && WIP
      case T_CLASS:
        pattern_create(&this->class, value);
        break;

      case T_ID:
        this->id = strtoul(value, &endptr, 10);
        if (*endptr != 0)
        {
            error = strdup("The value for 'id' should be numeric");
        }

        break;

      case T_INSTANCE:
        pattern_create(&this->instance, value);
        break;

      case T_WINDOW_ROLE:
        pattern_create(&this->window_role, value);
        break;

      case T_WINDOW_TYPE:
        this->window_type = parse_window_type(value);
        break;

#endif
      case T_FLOATING:
        this->floating = true;
        break;

      case T_TILING:
        this->tiling = true;
        break;

      case T_URGENT:
        if ((strcmp(value, "latest") == 0) ||
            (strcmp(value, "newest") == 0) ||
            (strcmp(value, "last") == 0) ||
            (strcmp(value, "recent") == 0))
        {
            this->urgent = 'l';
        } else if ((strcmp(value, "oldest") == 0) ||
                   (strcmp(value, "first") == 0))
        {
            this->urgent = 'o';
        } else
        {
            error =
                strdup("The value for 'urgent' must be 'first', 'last', "
                       "'latest', 'newest', 'oldest' or 'recent'");
        }

        break;

      case T_WORKSPACE:
        pattern_create(&this->workspace, value);
        break;

      case T_PID:
        this->pid = strtoul(value, &endptr, 10);
        if (*endptr != 0)
        {
            error = strdup("The value for 'pid' should be numeric");
        }

        break;

      case T_INVALID:
        break;
    }

    if (error)
    {
        return false;
    }

    return true;
}

void criteria::skip_spaces(char **head)
{
    while (**head == ' ')
    {
        ++*head;
    }
}

// Remove escaping slashes from value
void criteria::unescape(char *value)
{
    if (!strchr(value, '\\'))
    {
        return;
    }

    char *copy     = static_cast<char*>(calloc(strlen(value) + 1, 1));
    char *readhead = value;
    char *writehead = copy;
    while (*readhead)
    {
        if ((*readhead == '\\') && (*(readhead + 1) == '"'))
        {
            // skip the slash
            ++readhead;
        }

        *writehead = *readhead;
        ++writehead;
        ++readhead;
    }

    strcpy(value, copy);
    free(copy);
}

/**
 * Parse a raw criteria string such as [class="foo" instance="bar"] into a criteria
 * struct.
 *
 * If errors are found, nullptr will be returned and the error argument will be
 * populated with an error string. It is up to the caller to free the error.
 */
bool criteria::parse(char *raw, char **error_arg)
{
    bool free_error_arg = false;
    bool free_error     = false;

    if (*error_arg)
    {
        free_error_arg = true;
    }

    if (error && (*error_arg != error))
    {
        free_error = true;
    }

    if (free_error_arg)
    {
        free(*error_arg);
    }

    if (free_error)
    {
        free(error);
    }

    *error_arg = nullptr;
    error = nullptr;

    char *head = raw;
    skip_spaces(&head);

    if (*head != '[')
    {
        *error_arg = strdup("No criteria");
        return false;
    }

    ++head;

#if HAVE_XWAYLAND && WIP
    criteria->window_type = ATOM_LAST; // default value
#endif
    char *name = nullptr, *value = nullptr;
    bool in_quotes = false;

    while (*head && *head != ']')
    {
        skip_spaces(&head);
        // Parse token name
        char *namestart = head;
        while ((*head >= 'a' && *head <= 'z') || *head == '_')
        {
            ++head;
        }

        name = static_cast<char*>(calloc(head - namestart + 1, 1));
        if (head != namestart)
        {
            memcpy(name, namestart, head - namestart);
        }

        // Parse token value
        skip_spaces(&head);
        value = nullptr;
        if (*head == '=')
        {
            ++head;
            skip_spaces(&head);
            if (*head == '"')
            {
                in_quotes = true;
                ++head;
            }

            char *valuestart = head;
            if (in_quotes)
            {
                while (*head && (*head != '"' || *(head - 1) == '\\'))
                {
                    ++head;
                }

                if (!*head)
                {
                    *error_arg = strdup("Quote mismatch in criteria");
                    free(name);
                    free(value);
                    destroy();
                    return false;
                }
            } else
            {
                while (*head && *head != ' ' && *head != ']')
                {
                    ++head;
                }
            }

            value = static_cast<char*>(calloc(head - valuestart + 1, 1));
            memcpy(value, valuestart, head - valuestart);
            if (in_quotes)
            {
                ++head;
                in_quotes = false;
            }

            unescape(value);
            LOGD("Found pair: ", name, "=", value);
        }

        if (!parse_token(name, value))
        {
            *error_arg = error;
            free(name);
            free(value);
            destroy();
            return false;
        }

        skip_spaces(&head);
        free(name);
        free(value);
        name  = nullptr;
        value = nullptr;
    }

    if (*head != ']')
    {
        *error_arg = strdup("No closing brace found in criteria");
        free(name);
        free(value);
        destroy();
        return false;
    }

    if (is_empty())
    {
        *error_arg = strdup("Criteria is empty");
        free(name);
        free(value);
        destroy();
        return false;
    }

    ++head;
    int len = head - raw;
    this->raw = static_cast<char*>(calloc(len + 1, 1));
    memcpy(this->raw, raw, len);
    return true;
}

/* bool criteria::is_equal(struct criteria *left, struct criteria *right) {
 *   if (left->type != right->type) {
 *       return false;
 *   }
 *   // XXX Only implemented for CT_NO_FOCUS for now.
 *   if (left->type == CT_NO_FOCUS) {
 *       return strcmp(left->raw, right->raw) == 0;
 *   }
 *   if (left->type == CT_COMMAND) {
 *       return strcmp(left->raw, right->raw) == 0
 *               && strcmp(left->cmdlist, right->cmdlist) == 0;
 *   }
 *   return false;
 *  } */

/* bool criteria_already_exists(struct criteria *criteria) {
 *   // XXX Only implemented for CT_NO_FOCUS and CT_COMMAND for now.
 *   // While criteria_is_equal also obeys this limitation, this is a shortcut
 *   // to avoid processing the list.
 *   if (criteria->type != CT_NO_FOCUS && criteria->type != CT_COMMAND) {
 *       return false;
 *   }
 *
 *   list_t *criterias = config->criteria;
 *   for (int i = 0; i < criterias->length; ++i) {
 *       struct criteria *existing = criterias->items[i];
 *       if (criteria_is_equal(criteria, existing)) {
 *           return true;
 *       }
 *   }
 *   return false;
 *  } */
