#ifndef _SWAY_CRITERIA_H
#define _SWAY_CRITERIA_H


#include <cstdint>
#include <json/json.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <ipc/tools.hpp>
#include <wayfire/view.hpp>
#include <xwayland_support.h>

enum criteria_type
{
    CT_COMMAND                 = 1 << 0,
    CT_ASSIGN_OUTPUT           = 1 << 1,
    CT_ASSIGN_WORKSPACE        = 1 << 2,
    CT_ASSIGN_WORKSPACE_NUMBER = 1 << 3,
    CT_NO_FOCUS                = 1 << 4,
};

enum pattern_type
{
    PATTERN_PCRE2,
    PATTERN_FOCUSED,
};

struct pattern
{
    enum pattern_type match_type;
    pcre2_code *regex;
};

enum criteria_token
{
    T_ALL,
    T_APP_ID,
    T_CON_ID,
    T_CON_MARK,
    T_FLOATING,
#if HAVE_XWAYLAND && WIP
    T_CLASS,
    T_ID,
    T_INSTANCE,
    T_WINDOW_ROLE,
    T_WINDOW_TYPE,
#endif
    T_SHELL,
    T_TILING,
    T_TITLE,
    T_URGENT,
    T_WORKSPACE,
    T_PID,
    T_INVALID,
};

class criteria
{
  public:
    enum criteria_type type;
    char *raw     = nullptr; // entire criteria string (for logging)
    char *cmdlist = nullptr;
    char *target  = nullptr; // workspace or output name for `assign` criteria

    struct pattern *title    = nullptr;
    struct pattern *shell    = nullptr;
    struct pattern *app_id   = nullptr;
    struct pattern *con_mark = nullptr;
    uint32_t con_id = 0; // internal ID
#if HAVE_XWAYLAND && WIP
    struct pattern *class = nullptr;
    uint32_t id; // X11 window ID
    struct pattern *instance    = nullptr;
    struct pattern *window_role = nullptr;
    enum atom_name window_type;
#endif
    bool all = false;
    bool floating = false;
    bool tiling   = false;
    char urgent   = 0; // 'l' for latest or 'o' for oldest
    struct pattern *workspace = nullptr;
    pid_t pid = 0;

    criteria()
    {}
    criteria(char *raw, char **error)
    {
        this->parse(raw, error);
    }

    ~criteria()
    {
        this->destroy();
    }

    bool is_empty();
    // static bool is_equal(struct criteria *left, struct criteria *right);

    // static bool already_exists(struct criteria *criteria);

    void destroy();

    /**
     * Generate a criteria struct from a raw criteria string such as [class="foo"
     * instance="bar"] (brackets inclusive).
     *
     * The error argument is expected to be an address of a null pointer. If an error
     * is encountered, the function will return NULL and the pointer will be changed
     * to point to the error string. This string should be freed afterwards.
     */
    bool parse(char *raw, char **error);

    /**
     * Compile a list of criterias matching the given view.
     *
     * Criteria types can be bitwise ORed.
     */
    // static std::vector<struct criteria *> for_view(const wayfire_view& view, enum
    // criteria_type types);

    /**
     * Compile a list of containers matching the given criteria.
     */
    Json::Value get_containers();

  private:
    // The error pointer is used for parsing functions, and saves having to pass it
    // as an argument in several places.
    char *error = nullptr;
    bool parse_token(const char *name, char *value);
    bool matches_view(const Json::Value& view);
    bool has_container();
    bool matches_container(const Json::Value& container);

    bool generate_regex(pcre2_code **regex, char *value);
    bool pattern_create(struct pattern **pattern, char *value);
    static void pattern_destroy(struct pattern *pattern);
    static int regex_cmp(const char *item, const pcre2_code *regex);
#if HAVE_XWAYLAND && WIP
    static bool view_has_window_type(struct sway_view *view, enum atom_name name);
#endif
    static void unescape(char *value);
    static void skip_spaces(char **head);
    static enum criteria_token token_from_name(const char *name);
#if HAVE_XWAYLAND && WIP
    static enum atom_name parse_window_type(const char *type);
#endif
    static void node_for_each_matched_container(Json::Value obj,
        Json::Value *containers);
    static int lenient_strcmp(const char *a, const char *b);
};

#endif
