[license-badge]: https://img.shields.io/badge/License-Apache%202.0-blue.svg
[license-link]: https://opensource.org/licenses/Apache-2.0

[last-commit-badge]: https://img.shields.io/github/last-commit/TankerHQ/sdk-js.svg?label=Last%20commit&logo=github

<img src="https://cdn.jsdelivr.net/gh/TankerHQ/sdk-js@v1.10.1/src/public/tanker.png" alt="Tanker logo" width="180" />

[![License][license-badge]][license-link]
![Last Commit][last-commit-badge]

# A C++17 asynchronous HTTP client

[Overview](#overview) · [Contributing](#contributing) · [License](#license)

## Overview

Fetchpp is a C++17 asynchronous HTTP client written on top of Boost Beast and ASIO.

It closely follows ASIO API recommendations and style. This allows Fetchpp API to be used with callbacks, std::future, coroutines and more.

## Quickstart

```c++
#include <fetchpp/get.hpp>

#include <iostream>

int main()
{
  boost::asio::io_context ioc;

  fetchpp::async_get(ioc.get_executor(),
                     "http://httpbin.org/get",
                     [](auto err, auto response) {
                       if (err)
                         std::cerr << err.message() << std::endl;
                       std::cout << response.result_int() << std::endl;
                     });
  ioc.run();
  return 0;
}
```

## Contributing

We welcome feedback. Feel free to open any issue on the Github bug tracker.

## License

Fetchpp is licensed under the [Apache License, version 2.0](http://www.apache.org/licenses/LICENSE-2.0).
