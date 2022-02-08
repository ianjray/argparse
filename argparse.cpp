#include "argparse.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{

const std::string shortDelimiter{"-"};
const std::string longDelimiter{"--"};

/// @return bool True if @c str begins with @c prefix.
bool hasPrefix(const std::string &str, const std::string &prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

/// @return std::string String containing character @c c if non-NUL, otherwise the empty string.
std::string to_string(char c)
{
    std::string s{};
    if (c) {
        s = c;
    }
    return s;
}

std::vector<std::string> split_on_delimiter(const std::string &str, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss{str};
    std::string item;
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

} // namespace

class ArgParse::Impl
{
    /// Describes an option.
    struct Option {
        std::string shortName;
        std::string longName;
        std::string parameter;
        std::string description;
        std::function<void()> callback;
        std::function<void(const std::string &)> callback_arg;
        std::string defaultValue;
        bool required;
        bool has;

        /// @return std::string Option name, preferring @c longName if available.
        const std::string &name() const
        {
            if (longName.size()) {
                return longName;
            }
            return shortName;
        }
    };

    /// The set of supported options.
    /// @see add
    std::vector<Option> options_;

    /// @brief Lookup short name.
    /// @discussion Find option having short name @c name.
    std::tuple<Error, Option> lookupShortName(const std::string &name)
    {
        for (auto &option : options_) {
            if (option.shortName == name) {
                option.has = true;
                return {Error{}, option};
            }
        }

        return {Error{Error::Kind::UnrecognizedOption, name}, {}};
    }

    /// @brief Lookup long name.
    /// @discussion Find option having long name @c name.
    /// Unambigious partial matches are supported.
    std::tuple<Error, Option> lookupLongName(const std::string &name)
    {
        int n{};
        for (const auto &option : options_) {
            if (hasPrefix(option.longName, name)) {
                n++;
            }
        }

        if (n > 1) {
            return {Error{Error::Kind::AmbiguousOption, name}, {}};
        }

        if (n == 1) {
            for (auto &option : options_) {
                if (hasPrefix(option.longName, name)) {
                    option.has = true;
                    return {Error{}, option};
                }
            }
        } // UNREACHABLE

        return {Error{Error::Kind::UnrecognizedOption, name}, {}};
    }

public:
    Impl() : options_{}
    {
    }

    void add(char shortName,
             const char *longName,
             const char *description,
             std::function<void()> callback,
             const char *defaultValue)
    {
        options_.push_back({to_string(shortName), longName, {}, description, callback, {}, defaultValue, {}, {}});
    }

    void add(char shortName,
             const char *longName,
             const char *parameter,
             const char *description,
             std::function<void(const std::string &)> callback_arg,
             const char *defaultValue)
    {
        options_.push_back(
            {to_string(shortName), longName, parameter, description, {}, callback_arg, defaultValue, {}, {}});
    }

    void add(char shortName,
             const char *longName,
             const char *parameter,
             const char *description,
             std::function<void(const std::string &)> callback_arg,
             bool required)
    {
        options_.push_back(
            {to_string(shortName), longName, parameter, description, {}, callback_arg, {}, required, {}});
    }

    void help() const
    {
        const std::string argumentSeparator{"="};
        const size_t columnSeparatorWidth = 2;

        size_t firstColumnWidth{};
        for (const auto &option : options_) {
            if (option.shortName.size()) {
                size_t w{};
                if (option.longName.empty()) {
                    if (option.parameter.size()) {
                        w = argumentSeparator.size() + option.parameter.size();
                    }
                }
                firstColumnWidth = std::max(firstColumnWidth, shortDelimiter.size() + option.shortName.size() + w);
            }
        }

        size_t secondColumnWidth{};
        for (const auto &option : options_) {
            if (option.longName.size()) {
                size_t w{};
                if (option.parameter.size()) {
                    w = argumentSeparator.size() + option.parameter.size();
                }
                secondColumnWidth = std::max(secondColumnWidth, longDelimiter.size() + option.longName.size() + w);
            }
        }

        if (firstColumnWidth && secondColumnWidth) {
            firstColumnWidth += columnSeparatorWidth;
        }

        for (const auto &option : options_) {
            // Indent.
            std::cout << "  ";

            if (firstColumnWidth) {
                std::stringstream ss;
                if (option.shortName.size()) {
                    ss << shortDelimiter << option.shortName;
                    if (option.longName.empty()) {
                        if (option.parameter.size()) {
                            ss << argumentSeparator << option.parameter;
                        }
                    } else {
                        ss << ", ";
                    }
                }
                std::cout << std::left << std::setw(static_cast<int>(firstColumnWidth)) << ss.str();
            }

            if (secondColumnWidth) {
                std::stringstream ss;
                if (option.longName.size()) {
                    ss << longDelimiter << option.longName;
                    if (option.parameter.size()) {
                        ss << argumentSeparator << option.parameter;
                    }
                }
                std::cout << std::left << std::setw(static_cast<int>(secondColumnWidth)) << ss.str();
            }

            if (option.description.size()) {
                auto first{true};
                for (const auto &line : split_on_delimiter(option.description, '\n')) {
                    if (first) {
                        first = false;
                    } else {
                        std::cout << std::endl << "  " << std::setw(static_cast<int>(firstColumnWidth + secondColumnWidth)) << " ";
                    }
                    std::cout << "  " << line;
		}
            }

            if (option.required) {
                std::cout << "  (required)";
            } else if (option.defaultValue.size()) {
                std::cout << "  (default: '" << option.defaultValue << "')";
            }

            std::cout << std::endl;
        }
    }

    Error process(std::vector<std::string> &argv)
    {
        for (auto &option : options_) {
            option.has = false;
        }

        struct OptionVectorElement {
            Option option;
            std::string arg;
        };

        std::vector<OptionVectorElement> optv;

        for (auto it = argv.begin(); it != argv.end();) {
            auto str{*it};

            // §4 All options should be preceded by the '-' delimiter character.
            // §9 All options should precede operands on the command line.
            if (!hasPrefix(str, shortDelimiter)) {
                // Extension: allow mixing of options and non-options.
                it++;
                continue;
            }

            it = argv.erase(it);

            // §10 The first -- argument that is not an option-argument should be accepted as a delimiter indicating the
            // end of options.
            if (str == longDelimiter) {
                break;
            }

            if (hasPrefix(str, longDelimiter)) {
                // Extension: Long options begin with the '--' delimiter string.
                str.erase(str.begin(), str.begin() + static_cast<int>(longDelimiter.size()));

                std::string split{"="};
                std::string arg{};

                auto off = str.find(split);
                auto hasParameter = off != std::string::npos;
                if (hasParameter) {
                    arg = str.substr(off + split.size());
                    str = str.substr(0, off);
                }

                auto [err, option] = lookupLongName(str);
                if (err) {
                    return err;

                } else if (option.parameter.size()) {
                    // §7 Option-arguments should not be optional.
                    if (hasParameter) {
                        optv.push_back({option, arg});

                    } else if (it != argv.end()) {
                        optv.push_back({option, *it});
                        it = argv.erase(it);

                    } else {
                        return Error{Error::Kind::RequiresArgument, option.longName};
                    }

                } else if (hasParameter) {
                    return Error{Error::Kind::UnexpectedArgument, option.longName};

                } else {
                    optv.push_back({option, ""});
                }

            } else {
                // §4 All options should be preceded by the '-' delimiter character.
                str.erase(str.begin());

                // §5 One or more options without option-arguments, followed by at most one option that takes an
                // option-argument, should be accepted when grouped behind one '-' delimiter.
                if (str.empty()) {
                    return Error{Error::Kind::InvalidOption, ""};
                }

                while (!str.empty()) {
                    std::string name = {str.front()};
                    str.erase(str.begin());

                    auto [err, option] = lookupShortName(name);
                    if (err) {
                        return err;

                    } else if (option.parameter.size()) {
                        if (str.size()) {
                            optv.push_back({option, str});

                        } else if (it != argv.end()) {
                            optv.push_back({option, *it});
                            it = argv.erase(it);

                        } else {
                            return Error{Error::Kind::RequiresArgument, name};
                        }
                        break;
                    }

                    optv.push_back({option, ""});
                }
            }
        }

        for (const auto &option : options_) {
            if (option.required && !option.has) {
                return Error{Error::Kind::MissingOption, option.name()};
            }
        }

        for (const auto &member : optv) {
            if (member.option.callback) {
                member.option.callback();
            } else {
                member.option.callback_arg(member.arg);
            }
        }

        return Error{};
    }
};

ArgParse::Error::Error() : ArgParse::Error{Error::Kind::None, ""}
{
}

ArgParse::Error::Error(Error::Kind _kind, const std::string &name) : kind{_kind}, message{}
{
    std::stringstream ss;
    switch (kind) {
        case Error::Kind::None:
            break;
        case Error::Kind::InvalidOption:
            ss << "invalid option '" << name << "'";
            break;
        case Error::Kind::AmbiguousOption:
            ss << "option '" << name << "' is ambiguous";
            break;
        case Error::Kind::UnrecognizedOption:
            ss << "unrecognized option '" << name << "'";
            break;
        case Error::Kind::RequiresArgument:
            ss << "option '" << name << "' requires an argument";
            break;
        case Error::Kind::UnexpectedArgument:
            ss << "option '" << name << "' does not allow an argument";
            break;
        case Error::Kind::MissingOption:
            ss << "missing required option '" << name << "'";
            break;
    }
    message = ss.str();
}

ArgParse::Error::operator bool() const
{
    return kind != Error::Kind::None;
}

ArgParse::ArgParse() : pimpl{std::make_unique<Impl>()}
{
}

ArgParse::~ArgParse() = default;

void ArgParse::add(char shortName,
                   const char *longName,
                   const char *description,
                   std::function<void()> callback,
                   const char *defaultValue)
{
    pimpl->add(shortName, longName, description, callback, defaultValue);
}

void ArgParse::add(char shortName,
                   const char *longName,
                   const char *parameter,
                   const char *description,
                   std::function<void(const std::string &)> callback,
                   const char *defaultValue)
{
    pimpl->add(shortName, longName, parameter, description, callback, defaultValue);
}

void ArgParse::add(char shortName,
                   const char *longName,
                   const char *parameter,
                   const char *description,
                   std::function<void(const std::string &)> callback,
                   bool required)
{
    pimpl->add(shortName, longName, parameter, description, callback, required);
}

void ArgParse::help() const
{
    pimpl->help();
}

ArgParse::Error ArgParse::process(std::vector<std::string> &argv)
{
    return pimpl->process(argv);
}
