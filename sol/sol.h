#pragma once

#include "amf/utils/amfitemptr.hpp"
#include <map>
#include <vector>

class Sol {
  public:
    void load(const std::string &path);
    void save(const std::string &path = "");
    amf::AmfItem &root();

    std::string filename;
    std::string name;
    std::map<std::string, amf::AmfItemPtr> items;
    std::vector<std::string> keys;
};