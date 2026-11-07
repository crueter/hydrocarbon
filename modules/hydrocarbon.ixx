// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

module;

#include <expected>
#include <format>
#include <string>
#include <string_view>

export module hydrocarbon;
export import :socket;

inline constexpr bool isRedirect(hydro::Status status)
{
    switch (status) {
    case hydro::Status::MovedPermanently:
    case hydro::Status::Found:
    case hydro::Status::SeeOther:
    case hydro::Status::TemporaryRedirect:
    case hydro::Status::PermanentRedirect:
        return true;
    default:
        return false;
    }
}

static inline hydro::URL resolveLocation(const hydro::URL &url, std::string_view location)
{
    // absolute URL
    if (location.find("://") != std::string::npos) {
        return hydro::URL(location);
    }

    // relative URL
    const auto hostpart = url.hostpart();
    const auto newUrl = std::format("{}{}", hostpart, location);
    return hydro::URL(newUrl);
}

// Downgrade POST/etc. to GET and remove body if the response code calls for such.
static inline void modifyMethod(std::string &method, std::string &body, hydro::Header &headers, hydro::Status status,
                                bool preserve)
{
    const auto dropBody = [&]() {
        body.clear();

        headers.remove("content-length");
        headers.remove("transfer-encoding");
    };

    switch (status) {
    case hydro::Status::MovedPermanently:
    case hydro::Status::Found:
        // move to GET and clear body
        if (!preserve && method != "GET" && method != "HEAD") {
            method = "GET";
            dropBody();
        }
        break;
    case hydro::Status::SeeOther:
        if (method != "HEAD") {
            method = "GET";
            dropBody();
        }
        break;
    case hydro::Status::TemporaryRedirect:
    case hydro::Status::PermanentRedirect:
        break;
    default:
        break;
    }
}

export namespace hydro
{

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

// single-shot perform that doesn't follow redirs
std::expected<Response, Error> performOnce(Socket &s, std::string_view method, const URL &url,
                                           const RequestOptions &opts = {});

std::expected<Response, Error> perform(std::string_view method, const URL &url,
                                       const RequestOptions &opts = {});

std::expected<Response, Error> get(const URL &url, const RequestOptions &opts = {});
std::expected<Response, Error> head(const URL &url, const RequestOptions &opts = {});
std::expected<Response, Error> post(const URL &url, const RequestOptions &opts = {});
std::expected<Response, Error> put(const URL &url, const RequestOptions &opts = {});
std::expected<Response, Error> patch(const URL &url, const RequestOptions &opts = {});
std::expected<Response, Error> options(const URL &url, const RequestOptions &opts = {});
std::expected<Response, Error> perform_delete(const URL &url, const RequestOptions &opts = {});

// single-shot perform that doesn't follow redirs
std::expected<Response, Error> performOnce(Socket &s, std::string_view method, const URL &url,
                                           const RequestOptions &opts)
{
    // connect
    {
        auto ec = s.connect(url, opts.connect);
        if (!ec)
            return std::unexpected(ec.error());
    }

    auto headers = opts.headers;

    // construct data to send
    const auto version = opts.version == Version::HTTP_1_0 ? "HTTP/1.0" : "HTTP/1.1";
    const auto line1 = std::format("{} {}{} {}", method, url.route, url.params.str, version);

    // default ua/host
    if (!headers.contains("user-agent")) {
        headers.add("user-agent", "Hydrocarbon/0.1.0");
    }

    // only send port if it's not the default
    if (!headers.contains("host")) {
        const bool isDefault = (url.protocol == Protocol::HTTPS) ? url.port == 443 : url.port == 80;
        headers.add("host",
                    isDefault ? url.hostname : std::format("{}:{}", url.hostname, url.port));
    }

    if (!opts.body.empty() && !headers.contains("content-length")) {
        headers.add("content-length", std::to_string(opts.body.size()));
    }

    // add headers
    std::string headerStr;
    for (const auto &[k, v] : headers.entries()) {
        headerStr += std::format("\r\n{}: {}", k, v);
    }

    std::string finalLine = "\r\nConnection: close\r\n\r\n";

    // send off request
    {
        auto ec = s.send("{}{}{}", line1, headerStr, finalLine);
        if (!ec)
            return std::unexpected(ec.error());
    }

    // send body if present
    if (!opts.body.empty()) {
        auto ec = s.send("{}", opts.body);
        if (!ec)
            return std::unexpected(ec.error());
    }

    // now read
    // HEAD responses never carry a body, no matter what the headers say
    const bool no_body = method == "HEAD";

    if (opts.callback) {
        return s.readCallback(opts.callback, no_body);
    } else {
        return s.readAll(no_body);
    }
}

std::expected<Response, Error> perform(std::string_view method, const URL &url,
                                       const RequestOptions &opts)
{
    std::string curMethod{method};
    hydro::URL curUrl{url};
    hydro::RequestOptions curOpts{opts};
    int count = 0;

    while (true) {
        Socket s;
        auto r = performOnce(s, curMethod, curUrl, curOpts);
        if (!r)
            return r;

        // handle redirects
        auto loc = r->headers.get("location");
        if (opts.redirect.follow && count < opts.redirect.maxRedirects && isRedirect(r->status_code) && loc.has_value()) {
            modifyMethod(curMethod, curOpts.body, curOpts.headers, r->status_code, opts.redirect.preserveMethod);

            curUrl = resolveLocation(curUrl, loc.value());
            ++count;
            continue;
        }

        return r.value();
    }
}

std::expected<Response, Error> get(const URL &url, const RequestOptions &opts)
{
    return perform("GET", url, opts);
}

std::expected<Response, Error> head(const URL &url, const RequestOptions &opts)
{
    return perform("HEAD", url, opts);
}

std::expected<Response, Error> post(const URL &url, const RequestOptions &opts)
{
    return perform("POST", url, opts);
}

std::expected<Response, Error> put(const URL &url, const RequestOptions &opts)
{
    return perform("PUT", url, opts);
}

std::expected<Response, Error> patch(const URL &url, const RequestOptions &opts)
{
    return perform("PATCH", url, opts);
}

std::expected<Response, Error> options(const URL &url, const RequestOptions &opts)
{
    return perform("OPTIONS", url, opts);
}

// annoying!
std::expected<Response, Error> perform_delete(const URL &url, const RequestOptions &opts)
{
    return perform("DELETE", url, opts);
}

} // namespace hydro
