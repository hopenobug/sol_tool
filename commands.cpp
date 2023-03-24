#include "commands.h"
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <filesystem>

#undef UNICODE
#include "everything/Everything.h"
#include "sol/sol.h"

#include "amf/deserializer.hpp"
#include "amf/serializationcontext.hpp"

#include "amf/types/amfarray.hpp"
#include "amf/types/amfbool.hpp"
#include "amf/types/amfdate.hpp"
#include "amf/types/amfdouble.hpp"
#include "amf/types/amfinteger.hpp"
#include "amf/types/amfstring.hpp"


using namespace std;

static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    rtrim(s);
    ltrim(s);
}

typedef void (*ProcessItemFunc)(amf::AmfItemPtr ptr, const string &path);

static unordered_map<string, amf::AmfItemPtr> dataMap;

#define DEF_PRINT_VALUE_FUNC(type)                                                                                               \
    static void print##type(amf::AmfItemPtr ptr, const string &path) { cout << path << " " << ptr->as##type() << endl; }

#define DEF_SET_VALUE_FUNC(type)                                                                                                 \
    static void set##type(amf::AmfItemPtr ptr, const string &path) {                                                             \
        auto it = dataMap.find(path);                                                                                            \
        if (it != dataMap.end()) {                                                                                               \
            ptr->as##type() = it->second->as##type();                                                                            \
        }                                                                                                                        \
    }

DEF_PRINT_VALUE_FUNC(String)
DEF_PRINT_VALUE_FUNC(Integer)
DEF_PRINT_VALUE_FUNC(Double)
DEF_PRINT_VALUE_FUNC(Date)
DEF_PRINT_VALUE_FUNC(Bool)

DEF_SET_VALUE_FUNC(String)
DEF_SET_VALUE_FUNC(Integer)
DEF_SET_VALUE_FUNC(Double)
DEF_SET_VALUE_FUNC(Date)
DEF_SET_VALUE_FUNC(Bool)

static ProcessItemFunc onString = printString;
static ProcessItemFunc onInteger = printInteger;
static ProcessItemFunc onDouble = printDouble;
static ProcessItemFunc onDate = printDate;
static ProcessItemFunc onBool = printBool;

static void traverseItem(amf::AmfItemPtr ptr, string path) {
    cout << fixed << setprecision(4);

    // TODO support more types
    switch (ptr->valueType) {
    case amf::AMF_ARRAY: {
        auto &arr = ptr->as<amf::AmfArray>();
        for (int i = 0; i < arr.dense.size(); ++i) {
            traverseItem(arr.dense[i], path + "[" + to_string(i) + "]");
        }
        for (auto &&[key, value] : arr.associative) {
            traverseItem(value, path + "." + key);
        }
    } break;
    case amf::AMF_STRING:
        path += "__string";
        onString(ptr, path);
        break;
    case amf::AMF_INTEGER:
        path += "__integer";
        onInteger(ptr, path); // TODO support multiline string
        break;
    case amf::AMF_DOUBLE:
        path += "__double";
        onDouble(ptr, path);
        break;
    case amf::AMF_DATE:
        path += "__date";
        onDate(ptr, path);
        break;
    case amf::AMF_TRUE:
        path += "__bool";
        onBool(ptr, path);
    default:
        break;
    }
}

static std::string getMTime(const string &filename) {
    auto tp = chrono::clock_cast<chrono::system_clock>(filesystem::last_write_time(filename));
    auto t = chrono::system_clock::to_time_t(tp);
    auto now = localtime(&t);

    char buf[100];
    strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", now);
    return buf;
}

int find(int maxCount, bool printTime) {
    if (maxCount <= 0) {
        cerr << "Invalid count" << endl;
        return 1;
    }

    if (Everything_GetBuildNumber() == 0) {
        cerr << "Everything Service is not running" << endl;
        return 1;
    }

    Everything_SetSort(EVERYTHING_SORT_DATE_MODIFIED_DESCENDING);
    Everything_SetMax(maxCount);
    Everything_SetSearch("file: nopath: *.sol");

    Everything_Query(TRUE);

    for (DWORD i = 0; i < Everything_GetNumResults(); i++) {
        string path = Everything_GetResultPath(i);
        std::replace(path.begin(), path.end(), '\\', '/');
        std::string filename = path + "/" + Everything_GetResultFileName(i);

        std::string line = filename;

        if (printTime) {
            line += " " + getMTime(filename);
        }
        cout << line << endl;
    }

    Everything_CleanUp();
    return 0;
}

int show(string filename) {
    trim(filename);
    try {
        Sol sol;
        sol.load(filename);

        for (auto &key : sol.keys) {
            traverseItem(sol.items[key], key);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}

static void readSolData() {
    string line;
    while (getline(cin, line)) {
        auto i = line.find(" ");
        if (i == string::npos) {
            continue;
        }

        string path = line.substr(0, i);
        string valueStr = line.substr(i + 1);

        if (path.ends_with("__string")) {
            dataMap[path] = amf::AmfItemPtr(new amf::AmfString(valueStr));
        } else if (path.ends_with("__integer")) {
            dataMap[path] = amf::AmfItemPtr(new amf::AmfInteger(stoi(valueStr)));
        } else if (path.ends_with("__double")) {
            dataMap[path] = amf::AmfItemPtr(new amf::AmfDouble(stod(valueStr)));
        } else if (path.ends_with("__date")) {
            dataMap[path] = amf::AmfItemPtr(new amf::AmfDate(stoll(valueStr)));
        } else if (path.ends_with("__bool")) {
            dataMap[path] = amf::AmfItemPtr(new amf::AmfBool((valueStr == "0" || valueStr == "false") ? false : true));
        }
    }
}

static inline void setEditFunc() {
    onString = setString;
    onInteger = setInteger;
    onDouble = setDouble;
    onDate = setDate;
    onBool = setBool;
}

int edit(string filename, string newFilename) {
    trim(filename);
    if (newFilename.empty()) {
        newFilename = filename;
    }

    try {
        readSolData();

        Sol sol;
        sol.load(filename);

        if (!dataMap.empty()) {
            setEditFunc();

            for (auto &key : sol.keys) {
                traverseItem(sol.items[key], key);
            }
        }

        sol.save(newFilename);
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}