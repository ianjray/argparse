# argparse
C++ Command Line Argument Parser

Features a `help()` method which pretty-prints the options and their descriptions.

## Example

```shell
$ ./example --help
example [OPTIONS...]
  -h, --help     Print this message and exit.
  -v, --verbose  Multiple -v options increase the verbosity.

$ ./example foo
usage: example [OPTIONS...]

$ ./example -z
example: unrecognized option 'z'

$ ./example --ver=foo
example: option 'verbose' does not allow an argument

$ ./example -vvv
verbosity: 3
```

```c++
#include "argparse.h"

#include <iostream>

auto main(int _argc, char *const *_argv) -> int
{
    int v{};
    ArgParse ap;

    ap.add('h', "help", "Print this message and exit.",
        [&]() {
            std::cout << "example [OPTIONS...]" << std::endl;
            ap.help();
            exit(EXIT_SUCCESS);
        });

    ap.add('v', "verbose", "Multiple -v options increase the verbosity.",
        [&]() {
            v++;
        });

    auto argv = std::vector<std::string>(_argv + 1, _argv + _argc);
    auto err = ap.process(argv);
    if (err) {
        std::cerr << "example: " << err.message << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!argv.empty()) {
        std::cerr << "usage: example [OPTIONS...]" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "verbosity: " << v << std::endl;
}
```
