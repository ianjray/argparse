#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

/// Command line argument parser.
/// @discussion Provides methods to describe the set of supported options, render help, and process command line
/// arguments.
/// @see https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
class ArgParse
{
    class Impl;
    std::unique_ptr<Impl> pimpl;

public:
    ArgParse();

    ~ArgParse();
    ArgParse(const ArgParse&) = delete;
    auto operator=(const ArgParse&) -> ArgParse & = delete;
    ArgParse(ArgParse&&) = delete;
    auto operator=(ArgParse&&) -> ArgParse & = delete;

    /// Add an option.
    /// @param shortName    Option character (or NUL if not used).
    /// @param longName     Option name (or empty string if not used).
    /// @param description  Usage description.
    /// @param callback     Function called to process this option.
    /// @param defaultValue Default value (or empty string if none).
    void add(char shortName,
             const char *longName,
             const char *description,
             std::function<void()> callback,
             const char *defaultValue = "");

    /// Add an option which has an option-argument.
    /// @param shortName    Option character (or NUL if not used).
    /// @param longName     Option name (or empty string if not used).
    /// @param parameter    Parameter name.
    /// @param description  Usage description.
    /// @param callback     Function called to process this option.
    /// @param defaultValue Default value (or empty string if none).
    void add(char shortName,
             const char *longName,
             const char *parameter,
             const char *description,
             std::function<void(const std::string &)> callback,
             const char *defaultValue = "");

    /// Add an option which has an option-argument and is required.
    /// @param shortName    Option character (or NUL if not used).
    /// @param longName     Option name (or empty string if not used).
    /// @param parameter    Parameter name.
    /// @param description  Usage description.
    /// @param callback     Function called to process this option.
    /// @param required     True if option is required.
    void add(char shortName,
             const char *longName,
             const char *parameter,
             const char *description,
             std::function<void(const std::string &)> callback,
             bool required);

    /// Render description to stdout.
    void help() const;

    struct Error {
        enum class Kind
        {
            /// Parsing successful.
            None,
            /// An invalid option '-' was given.
            InvalidOption,
            /// An ambigious long option was given.
            AmbiguousOption,
            /// An unrecognized option was given.
            UnrecognizedOption,
            /// Option-argument of the form '-o ARG', '-oARG', '--long ARG', or '--long=ARG' was not given.
            RequiresArgument,
            /// An unexpected option-argument of the form --long=ARG was given.
            UnexpectedArgument,
            /// A required option was not given.
            MissingOption
        };

        /// Error kind.
        Kind kind;

        /// Descriptive message.
        std::string message;

        Error();
        Error(Kind kind, const std::string &option);

        /// @return true If object describes a non-None error.
        operator bool() const;
    };

    /// Parse argument list.
    /// @discussion Parses a command line argument list @c argv to identify options.
    /// Options begin with either short delimiter "-" or long delimiter "--".
    /// Options and other arguments may be intermixed.
    /// Long options partially match if there is no ambiguity.
    /// Use special delimiter "--" to terminate argument processing.
    /// An error is returned if any invalid or unrecognized options are found, or if any arguments are missing, or if
    /// any required options are missing. Otherwise the callback functions are called and success is returned.
    /// @see add
    /// @return Error Descriptive message or Error::Kind::None if parsing successful.
    auto process(std::vector<std::string> &argv) -> Error;
};
