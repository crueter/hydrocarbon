# Hydrocarbon

Pure synchronous, self-contained C++23 HTTP and HTTPS 1.1 library.

Contains code based off of [requests.h](https://github.com/danmig06/requests.h) and [cpp-httplib](https://github.com/yhirose/cpp-httplib). See [LICENSE](./LICENSE) and [COPYING](./COPYING.md).

## Features

- Clean, simple API (`hydro::get`, `hydro::post`, etc)
- Progress callback support
- C++23 `std::expected` API
- Built for C++20 modules
- No dependencies besides OpenSSL
- OS-agnostic, works on Windows and all UNIXes
- HTTP and HTTPS support

## Limitations

- Support for URL encoding/decoding is planned.
- Support for socket and R/W timeouts are planned.
- Support for operating systems beyond Windows and UNIX-likes is not planned.
  - Technically, at a minimum, you only need C++23, OpenSSL, and some networking methods: `recv`, `send`, `getaddrinfo`, `socket`, and `connect`.
- Support for disk I/O functionality is not planned.
- IPv6 support is not planned.
- Thread safety is not planned.
- The HTTP redirect spec is not fully supported.
- SIGPIPE isn't handled for HTTPS connections, usually this won't be an issue.

## Compiler Support

Your compiler and standard library must support `std::expected` and C++20 modules. Additionally, compiling the test suite requires `std::print` support.

- GCC 15+
- Clang 19+
  - 17+ might work but is not supported
- MSVC 2026+
- clang-cl with MSVC 2022+

AppleClang is unsupported as it lacks `clang-scan-deps`.

## Integration

Hydrocarbon can be used both as a system and a vendored target. It can be integrated simply via `find_package(hydrocarbon)` or `add_subdirectory(hydrocarbon)`, respectively.

Hydrocarbon creates a `hydro::carbon` target, so you can link to Hydrocarbon via `target_link_libraries(MyApp PRIVATE hydro::carbon)`. You usually shouldn't need Hydrocarbon within your header files, so internal targets generally won't need `PUBLIC` propagation.

In order for Hydrocarbon to properly compile and for `import hydrocarbon;` to work, you MUST turn on `CMAKE_CXX_SCAN_FOR_MODULES`, and use a supported generator. Ninja is recommended. For `clang-cl`, CMake 4.4 or newer is required and the Visual Studio generator is not supported.

### Adding Hydrocarbon to your project

You can use `FetchContent`, [CPMUtil](https://github.com/crueter/CPMUtil), a Git submodule, or any other dependency vendoring solution of your choice. You should probably use `find_package(hydrocarbon)` as well (CPMUtil will handle this for you).

### Caveats

As with all modules, you *must* ensure that the target you're importing the module into has more or less the same compiler flags as Hydrocarbon. If you set a bunch of additional flags e.g. `-fwrapv` or `-fno-rtti` to your source files, Hydrocarbon must also have these applied.

## Usage

Import Hydrocarbon via `import hydrocarbon;` in your C++ file.

Perform a request using any of:

- `hydro::get`
- `hydro::head`
- `hydro::post`
- `hydro::put`
- `hydro::patch`
- `hydro::options`
- `hydro::perform_delete`
  - Unfortunately `delete` is a reserved keyword.

All of these have the same method signature:

```cpp
hydro::<method>(const hydro::URL &url, const hydro::RequestOptions opts = {});
```

## URL

You can pass any well-formed string to these methods and they will construct the URL for you.

```cpp
auto ec = hydro::get("https://www.google.com");
```

You can also manually construct a `hydro::URL`, which contains the following fields:

```cpp
Protocol protocol = Protocol::HTTP;
std::string hostname;
std::string route = "/";
uint16_t port = 80;
std::string port_str = "80";
Parameters params;
```

## Options

There are three main structs used for connection configuration.

```cpp
struct ConnectOptions {
    // Whether or not to verify the server certificate.
    bool verify = true;

    // A custom certificate to load.
    std::string cert = "";

    // Disable SSL entirely.
    bool disableSsl = false;

    // Initial connection timeout.
    // (set to 0ms to disable)
    std::chrono::milliseconds connectionTimeout = 0ms;
};

struct RedirectOptions {
    bool follow = true;
    int maxRedirects = 5;

    // preserve method type for 301/302
    bool preserveMethod = false;
};

struct RequestOptions {
    Version version = Version::HTTP_1_1;
    Header headers{};
    std::string body = "";
    UserCallback callback{};
    ConnectOptions connect{};
    RedirectOptions redirect{};
};
```

### Headers

You can add headers using `<Options>.headers.add(key, value)`:

```cpp
hydro::RequestOptions opts;
opts.headers.add("my-custom-header", "my custom value");
```

You can remove them using `remove(key)`:

```cpp
opts.headers.remove("my-custom-header");
```

You can check if a header is present using `contains(key)`:

```cpp
bool customPresent = opts.headers.contains("my-custom-header");
```

You can get one or all header values of a specified key using `get(key)` or `get_all(key)`

```cpp
std::optional<std::string> value = opts.headers.get("my-custom-header");
std::vector<std::string> values = opts.headers.get_all("my-custom-header");
```

You can loop through all headers using `entries()`:

```cpp
for (const auto &[k, v] : opts.headers.entries()) {
    std::println("{}: {}", k, v);
}
```

And get the number of headers using `size()`:

```cpp
auto numHeaders = opts.headers.size();
```

Headers are always case-insensitive.

### Setting a Request Body

For POST, PUT, etc, you can add a body using `<Options>.body = value`:

```cpp
hydro::RequestOptions opts;
opts.body = "This is my request body";
```

### Disallow Redirects

You may choose to not follow redirects using `<Options>.redirect.follow = false`:

```cpp
hydro::RequestOptions opts;
opts.redirect.follow = false;
```

### Custom Certificate

You can load a custom certificate from memory as a PEM-encoded string (or from disk if you load it into a string beforehand) using `<Options>.connect.cert`. If you don't provide that, OpenSSL will attempt to load the system certificate store instead:

```cpp
std::string myCert = "...";

hydro::RequestOptions opts;
opts.connect.cert = myCert;
```

### Progress Callback

```cpp
// Represents the current state of downloading.
struct DownloadState {
    uint64_t bytes_received = 0;
    uint64_t bytes_left = 0;
    std::string_view buffer;
    uint64_t content_length = 0;
    Status status_code = Status::NoStatusCode;
};

using UserCallback = std::function<bool(const DownloadState &)>;
```

You may choose to opt into a progress callback via `<Options>.callback`. The progress callback takes in a `DownloadState` struct as defined above, and returns a boolean; returning `false` will cancel the request.

Example:

```cpp
const auto func = [](const hydro::DownloadState &state) {
    std::println("Received {}/{} bytes ({} remaining)", state.bytes_received,
                    state.content_length, state.bytes_left);

    return true;
};

hydro::RequestOptions opts;
opts.callback = func;
```

Note that for chunked transfers (which most servers use nowadays), `content_length` and `bytes_left` will always be zero. Additionally, if you specify a progress callback, the returned response will have no body. You are responsible for handling it.

## Handling Responses

Hydrocarbon's request methods return an [`std::expected`](https://en.cppreference.com/cpp/utility/expected) either containing a response or an error code. These error codes support `std::format`, so you can print/log them as-is. You can thus handle invalid responses simply:

```cpp
auto ec = hydro::get("https://www.google.com/");
if (!ec) {
    std::println("Error: {}", ec.error());
}
```

Otherwise, the response will contain these fields:

```cpp
std::string body;
Header headers;
std::string status_line;
Status status_code = Status::NoStatusCode;
std::string reason;
TransferMode transferMode = TransferMode::NoData;
```

Where:

- `body` contains the response body in full, **unless** you specified a progress callback.
- `headers` contains all of the request headers.
- `status_line` is the raw status line returned by the server (e.g. `HTTP/1.1 200 OK`)
- `status_code` is an enumeration corresponding to the numerical status code returned by the server.
- `reason` is the string status response, e.g. `OK` or `Not Found`.
- `transferMode` is either `Chunked`, `Sized`, `UntilClose`, or `NoData`.
  - Usually you don't need to worry about this

## Building/Testing

Standard CMake and CTest.

```cmake
cmake -S . -B build -GNinja
cmake --build build
ctest --test-dir build --output-on-failure
```
