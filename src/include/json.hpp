#ifndef _JSON_H
#define _JSON_H

#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <string>

struct json_parser_result
{
    std::string errors;
    bool error = false;
    Json::Value root;
};

class json
{
  public:
/*     static std::string json_array_to_string(Json::Value root)
 *   {
 *       if (root.isNull() || root.empty())
 *       {
 *           return "[]";
 *       }
 *
 *       std::string join = "";
 *       for (Json::ArrayIndex i = 0; i < root.size(); i++)
 *       {
 *           Json::Value value = root.get(i, "");
 *           if (value.isArray())
 *           {
 *               join += json_array_to_string(value);
 *           } else
 *           {
 *               join += (i == 0 ? "" : ", ") + (value.isString() ?
 *                   value.asString() :
 *                   json_to_string(root.get(i, "")));
 *           }
 *       }
 *
 *       return "[ " + join + " ]";
 *   } */

    static std::string json_to_string(Json::Value root,
        const std::string& ident = "  ")
    {
        if (root.isString())
        {
            return root.isNull() ? "" : root.asString();
        }

        Json::StreamWriterBuilder builder;
        builder["indentation"] = ident;
        builder["emitUTF8"]    = true;
        builder["enableYAMLCompatibility"] = true;
        return Json::writeString(builder, root.isNull() ? Json::objectValue : root);
    }

    static Json::Value string_to_json(const std::string & str)
    {
        auto parse_result = parse(str);

        if (parse_result.error)
        {
            // LOGE("Failed to parse json string %s", parse_error.errors.c_str());
            std::cerr << "Failed to parse json string: " << parse_result.errors <<
                std::endl;
        }

        return parse_result.root;
    }

    static json_parser_result parse(const std::string& str)
    {
        Json::CharReaderBuilder builder;
        Json::CharReader *reader = builder.newCharReader();
        json_parser_result parse_result;

        if (str.empty())
        {
            parse_result.error  = true;
            parse_result.errors = "String is empty .";
            return parse_result;
        }

        parse_result.error =
            !reader->parse(str.c_str(),
                str.c_str() + str.size(), &parse_result.root, &parse_result.errors);
        delete reader;

        return parse_result;
    }
};

#endif
