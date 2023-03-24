#include <iostream>
#include <string>

#include "argparse/argparse.hpp"
#include "commands.h"

using namespace std;

#define VERSION "1.0"

int main(int argc, char **argv) {
    argparse::ArgumentParser argParser("sol_tool");
    argParser.add_description("flash sol tool, only support amf3 format");

    argparse::ArgumentParser findCommand("find");
    findCommand.add_description("use Everything Service to find recent updated sol");
    findCommand.add_argument("-c").scan<'d', int>().default_value(10).help("max result count");
    findCommand.add_argument("-t").default_value(false).implicit_value(true).help("show file update time");

    argparse::ArgumentParser showCommand("show");
    showCommand.add_description("show sol content");
    showCommand.add_argument("file").required().help("sol file path");

    argparse::ArgumentParser editCommand("edit");
    editCommand.add_description("edit sol with stdin data, only support existed value");
    editCommand.add_argument("file").required().help("original sol file path");
    editCommand.add_argument("new_file").default_value("").help("new sol file path");

    argParser.add_subparser(findCommand);
    argParser.add_subparser(showCommand);
    argParser.add_subparser(editCommand);

    try {
        argParser.parse_args(argc, argv);
    } catch (const exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << argParser;
        std::exit(1);
    }

    if (argParser.is_subcommand_used(findCommand)) {
        return find(findCommand.get<int>("-c"), findCommand.get<bool>("-t"));
    }

    if (argParser.is_subcommand_used(showCommand)) {
        return show(showCommand.get<string>("file"));
    }

    if (argParser.is_subcommand_used(editCommand)) {
        return edit(editCommand.get<string>("file"), editCommand.get<string>("new_file"));
    }

    cout << argParser;

    return 0;
}