#include "argparse.hpp"

#include <cassert>
#include <charconv>
#include <iostream>
#include <sstream>

int main()
{
    auto dummy_handler = [](const std::string &)
    {
    };

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add('a', "",       "", "Describe A", dummy_handler, "");
        a.add('a', "",       "", "Describe A\nOn multiple lines.", dummy_handler, true);
        a.add('a', "",       "", "Describe A", dummy_handler, "AA");
        a.help();
    }

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add({},  "long-a", "", "Describe A", dummy_handler, "");
        a.add({},  "long-a", "", "Describe A\nOn multiple lines.", dummy_handler, true);
        a.add({},  "long-a", "", "Describe A", dummy_handler, "AA");
        a.help();
    }

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add('a', "",       "", "Describe A", dummy_handler, "");
        a.add('a', "",       "", "Describe A\nOn multiple lines.", dummy_handler, true);
        a.add('a', "",       "", "Describe A", dummy_handler, "AA");
        a.add({},  "long-a", "", "Describe A", dummy_handler, "");
        a.add({},  "long-a", "", "Describe A\nOn multiple lines.", dummy_handler, true);
        a.add({},  "long-a", "", "Describe A", dummy_handler, "AA");
        a.help();
    }

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add('a', "long-a", "", "Describe A", dummy_handler, "");
        a.add('a', "long-a", "", "Describe A", dummy_handler, true);
        a.add('a', "long-a", "", "Describe A", dummy_handler, "AA");
        a.help();
    }

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add('a', "",       "ARG", "Describe A", dummy_handler, "");
        a.add('a', "",       "ARG", "Describe A", dummy_handler, true);
        a.add('a', "",       "ARG", "Describe A", dummy_handler, "AA");
        a.add({},  "long-a", "ARG", "Describe A", dummy_handler, "");
        a.add({},  "long-a", "ARG", "Describe A", dummy_handler, true);
        a.add({},  "long-a", "ARG", "Describe A", dummy_handler, "AA");
        a.help();
    }

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add('a', "long-a", "ARG", "Describe A", dummy_handler, "");
        a.add('a', "long-a", "ARG", "Describe A", dummy_handler, true);
        a.add('a', "long-a", "ARG", "Describe A", dummy_handler, "AA");
        a.help();
    }

    std::cout << "====" << std::endl;
    {
        ArgParse a;
        a.add('a', "long-a", "ARG", "", dummy_handler, "");
        a.add('a', "long-a", "ARG", "", dummy_handler, true);
        a.add('a', "long-a", "ARG", "", dummy_handler, "AA");
        a.help();
    }

    struct A
    {
        std::string opt;
        std::string arg;
    };

    std::vector<A> args;

    {
        ArgParse a;

        a.add(
            'a', "",
            "Describe a",
            [&]()
            {
                args.push_back({"a", ""});
            });

        a.add(
            'b', "",
            "Describe b",
            [&]()
            {
                args.push_back({"b", ""});
            });

        a.add(
            'c', "long-c-opt", "ARG",
            "Describe c",
            [&](const std::string & arg)
            {
                args.push_back({"c", arg});
            });

        a.add(
            'e', "long-e-opt",
            "Describe e",
            [&]()
            {
                args.push_back({"e", ""});
            });

        {
            std::vector<std::string> argv{};
            auto error = a.process(argv);
            assert(!error);
            assert(args.empty());
            assert(argv.empty());

            argv = {"-"};
            error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::InvalidOption);
            assert(args.empty());
            assert(argv.empty());

            argv = {"--"};
            error = a.process(argv);
            assert(!error);
            assert(args.empty());
            assert(argv.empty());

            argv = {"a"};
            error = a.process(argv);
            assert(!error);
            assert(args.empty());
            assert(argv.size() == 1);
            assert(argv[0] == "a");

            argv = {"--", "-a"};
            error = a.process(argv);
            assert(!error);
            assert(args.empty());
            assert(argv.size() == 1);
            assert(argv[0] == "-a");

            argv = {"-a"};
            error = a.process(argv);
            assert(!error);
            assert(args.size() == 1);
            assert(args[0].opt == "a");
            args.clear();
            assert(argv.empty());

            argv = {"A", "-a"};
            error = a.process(argv);
            assert(!error);
            assert(args.size() == 1);
            assert(args[0].opt == "a");
            args.clear();
            assert(argv.size() == 1);
            assert(argv[0] == "A");
        }

        {
            std::vector<std::string> argv{"-a-"};
            auto error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::UnrecognizedOption);
            assert(args.empty());
            assert(argv.empty());

            argv = {"-a", "-x"};
            error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::UnrecognizedOption);
            assert(error.message == "unrecognized option 'x'");
            assert(args.empty());
            assert(argv.empty());

            argv = {"-a", "--unknown"};
            error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::UnrecognizedOption);
            assert(error.message == "unrecognized option 'unknown'");
            assert(args.empty());
            assert(argv.empty());
        }

        {
            std::vector<std::string> argv{"-c"};
            auto error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::RequiresArgument);
            assert(error.message == "option 'c' requires an argument");
            assert(args.empty());
            assert(argv.empty());

            argv = {"-cC1", "-c", "C2"};
            error = a.process(argv);
            assert(!error);
            assert(args.size() == 2);
            assert(args[0].opt == "c");
            assert(args[0].arg == "C1");
            assert(args[1].opt == "c");
            assert(args[1].arg == "C2");
            args.clear();
            assert(argv.empty());
        }

        {
            std::vector<std::string> argv{"--long-"};
            auto error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::AmbiguousOption);
            assert(error.message == "option 'long-' is ambiguous");
            assert(args.empty());
            assert(argv.empty());

            argv = {"--long-c"};
            error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::RequiresArgument);
            assert(error.message == "option 'long-c-opt' requires an argument");
            assert(args.empty());
            assert(argv.empty());

            argv = {"--long-c=C1", "--long-c=", "operand", "--long-c", "C2"};
            error = a.process(argv);
            assert(!error);
            assert(args.size() == 3);
            assert(args[0].opt == "c");
            assert(args[0].arg == "C1");
            assert(args[1].opt == "c");
            assert(args[1].arg == "");
            assert(args[2].opt == "c");
            assert(args[2].arg == "C2");
            args.clear();
            assert(argv.size() == 1);
            assert(argv[0] == "operand");
        }

        {
            std::vector<std::string> argv{"--long-e=E"};
            auto error = a.process(argv);
            assert(error.kind == ArgParse::Error::Kind::UnexpectedArgument);
            assert(error.message == "option 'long-e-opt' does not allow an argument");
            assert(args.empty());
            assert(argv.empty());

            argv = {"--long-e", "E"};
            error = a.process(argv);
            assert(!error);
            assert(args.size() == 1);
            assert(args[0].opt == "e");
            args.clear();
            assert(argv.size() == 1);
            assert(argv[0] == "E");
        }
    }

    {
        ArgParse a;

        a.add(
            'a', "",
            "Describe a",
            [&]()
            {
                args.push_back({"a", ""});
            });

        a.add(
            'd', "", "OPTARG",
            "Describe d",
            [&](const std::string & arg)
            {
                args.push_back({"d", arg});
            },
            true);

        a.add(
            {}, "long-f", "OPTARG",
            "Describe f",
            [&](const std::string & arg)
            {
                args.push_back({"f", arg});
            },
            true);

        std::vector<std::string> argv{"-a"};
        auto error = a.process(argv);
        error = a.process(argv);
        assert(error.kind == ArgParse::Error::Kind::MissingOption);
        assert(error.message == "missing required option 'd'");
        assert(args.empty());
        assert(argv.empty());

        argv = {"-ad"};
        error = a.process(argv);
        assert(error.kind == ArgParse::Error::Kind::RequiresArgument);
        assert(error.message == "option 'd' requires an argument");
        assert(args.empty());
        assert(argv.empty());

        argv = {"-adx"};
        error = a.process(argv);
        assert(error.kind == ArgParse::Error::Kind::MissingOption);
        assert(error.message == "missing required option 'long-f'");
        assert(args.empty());
        assert(argv.empty());

        argv = {"-ad", "D", "-adD2", "--long-f", "F"};
        error = a.process(argv);
        assert(!error);
        assert(args.size() == 5);
        assert(args[0].opt == "a");
        assert(args[0].arg == "");
        assert(args[1].opt == "d");
        assert(args[1].arg == "D");
        assert(args[2].opt == "a");
        assert(args[2].arg == "");
        assert(args[3].opt == "d");
        assert(args[3].arg == "D2");
        assert(args[4].opt == "f");
        assert(args[4].arg == "F");
        args.clear();
        assert(argv.empty());
    }
}
