// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

module;

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

export module hydrocarbon:fields;
export import :enums;
export import :util;

export namespace hydro
{

// Represents the current state of downloading.
struct DownloadState {
    uint64_t bytes_received = 0;
    uint64_t bytes_left = 0;
    std::string_view buffer;
    uint64_t content_length = 0;
    Status status_code = Status::NoStatusCode;
};

using UserCallback = std::function<bool(const DownloadState &)>;

// Represents an unordered mapping of {key, value} query string pairs.
class Parameters {
public:
    std::string str;
    std::unordered_map<std::string, std::string> params;

    Parameters(std::string_view data = "")
    {
        str = std::string{data};
        if (data.empty())
            return;

        std::size_t pos = 0;
        while (pos < data.size()) {
            // skip leading & or ?
            while (pos < data.size() && (str[pos] == '?' || data[pos] == '&'))
                ++pos;

            if (pos >= data.size())
                break;

            // find current param end
            std::size_t amp = data.find('&', pos);
            std::size_t pair_end = (amp == std::string_view::npos) ? data.size() : amp;

            // find equals separator
            std::size_t eq = data.find('=', pos);
            if (eq != std::string_view::npos && eq < pair_end) {
                std::string_view name = data.substr(pos, eq - pos);
                std::string_view value = data.substr(eq + 1, pair_end - (eq + 1));

                if (!name.empty())
                    params.emplace(name, value);
            }

            pos = pair_end;
        }
    }

    std::optional<std::string> get(std::string_view key)
    {
        auto it = params.find(std::string{key});
        if (it == params.end())
            return std::nullopt;

        return it->second;
    }
};

// Represents an unordered multimapping of {key, value} header pairs.
class Header {
private:
    std::unordered_multimap<std::string, std::string> m_entries;

    // transform a key to lowercase
    static std::string lowercase_key(std::string_view key)
    {
        std::string lower_key{key};
        std::ranges::transform(lower_key, lower_key.begin(),
                       [](uint8_t c) { return std::tolower(c); });
        return lower_key;
    }

public:
    // Add a {key, value} pair to this set of headers.
    // The key and value strings are copied.
    void add(std::string_view key, std::string_view value)
    {
        m_entries.emplace(lowercase_key(key), value);
    }

    // Remove all entries with this key.
    // True on success, false if key not present, or erase failed for some reason.
    bool remove(std::string_view key)
    {
        std::string lower_key = lowercase_key(key);
        auto it = m_entries.find(lower_key);
        if (it == m_entries.end())
            return false;

        return m_entries.erase(lower_key) >= 1;
    }

    // Check if the headers contain the specified key.
    bool contains(std::string_view key) const
    {
        return m_entries.find(lowercase_key(key)) != m_entries.end();
    }

    // Get the first value for a key.
    std::optional<std::string> get(std::string_view key) const
    {
        auto [first, last] = m_entries.equal_range(lowercase_key(key));
        if (first == last)
            return std::nullopt;

        return first->second;
    }

    // Get all values for a key.
    std::vector<std::string> get_all(std::string_view key) const
    {
        std::vector<std::string> result;

        auto [first, last] = m_entries.equal_range(lowercase_key(key));
        for (auto it = first; it != last; ++it)
            result.push_back(it->second);

        return result;
    }

    // Get the underlying entry multimap.
    [[nodiscard]] const auto &entries() const
    {
        return m_entries;
    }

    // Get the number of headers.
    [[nodiscard]] std::size_t size() const
    {
        return m_entries.size();
    }
};

// Represents a URL and all of its relevant components.
class URL {
public:
    Protocol protocol = Protocol::HTTP;
    std::string hostname;
    std::string route = "/";
    uint16_t port = 80;
    std::string port_str = "80";
    Parameters params;

    URL(const char *url) : URL(std::string_view{url, std::strlen(url)}) {}

    URL(std::string url) : URL(std::string_view{url}) {};

    URL(std::string_view url)
    {
        if (url.empty())
            return;

        auto proto_end = url.find(":");

        // extract protocol
        if (proto_end != std::string_view::npos && url.substr(proto_end).starts_with("://")) {
            if (util::iequals(url.substr(0, proto_end), "https")) {
                protocol = Protocol::HTTPS;
                port = 443;
                port_str = "443";
            }
        } else {
            proto_end = std::string_view::npos;
        }

        const std::size_t hostname_start =
            (proto_end != std::string_view::npos) ? proto_end + 3 : 0;
        std::string_view remainder = url.substr(hostname_start);

        // remove #fragment
        if (auto hash = remainder.find('#'); hash != std::string_view::npos)
            remainder = remainder.substr(0, hash);

        auto route_begin = remainder.find('/');
        auto params_begin = remainder.find('?');

        // params, but no route
        if (params_begin != std::string_view::npos &&
            (route_begin == std::string_view::npos || params_begin < route_begin)) {
            hostname = std::string(remainder.substr(0, params_begin));

            params = Parameters(remainder.substr(params_begin));
        }
        // route
        else if (route_begin != std::string_view::npos) {
            hostname = std::string(remainder.substr(0, route_begin));
            remainder = remainder.substr(route_begin);

            // params
            auto qpos = remainder.find('?');
            if (qpos != std::string_view::npos) {
                route = std::string(remainder.substr(0, qpos));

                params = Parameters(std::string(remainder.substr(qpos)));
            } else {
                route = std::string(remainder);
            }
        }
        // no route or params
        else {
            hostname = std::string(remainder);
        }

        // port
        auto colon = hostname.find(':');
        if (colon != std::string::npos) {
            auto prev_port_str = port_str;
            port_str = std::string_view(hostname).substr(colon + 1);
            hostname = hostname.substr(0, colon);

            uint64_t t_port = 0;
            auto [ptr, ec] =
                std::from_chars(port_str.data(), port_str.data() + port_str.size(), t_port);
            if (ec == std::errc{} && t_port > 0 && t_port <= 65535 &&
                ptr == port_str.data() + port_str.size()) {
                port = uint16_t(t_port);
            } else {
                // invalid port_str, restore to prev value
                port_str = prev_port_str;
            }
        }
    }

    // <protocol>://<hostname>[:port]
    std::string hostpart() const
    {
        std::string portStr, protocolStr;

        switch (protocol) {
        case Protocol::NoProtocol:
            protocolStr = "";
            portStr = port_str;
            break;
        case Protocol::HTTP:
            protocolStr = "http://";
            portStr = port == 80 ? "" : std::format(":{}", port);
            break;
        case Protocol::HTTPS:
            protocolStr = "https://";
            portStr = port == 443 ? "" : std::format(":{}", port);
            break;
        }

        return std::format("{}{}{}", protocolStr, hostname, portStr);
    }
};

// Represents a response from a server.
class Response {
public:
    std::string body;
    Header headers;
    std::string status_line;
    Status status_code = Status::NoStatusCode;
    std::string reason;
    TransferMode transferMode = TransferMode::NoData;

    // Parse the status line e.g "HTTP/1.1 200 OK".
    static void parseStatusLine(std::string_view line, int &code, std::string &reason)
    {
        auto space1 = line.find(' ');
        if (space1 == std::string_view::npos)
            return;

        auto space2 = line.find(' ', space1 + 1);
        std::string_view code_str;

        // reason is present
        if (space2 != std::string_view::npos) {
            code_str = line.substr(space1 + 1, space2 - (space1 + 1));
            reason = line.substr(space2 + 1);
        }

        // no reason present
        else {
            code_str = line.substr(space1 + 1);
        }

        auto res = std::from_chars(code_str.data(), code_str.data() + code_str.size(), code);
        if (res.ec != std::errc{}) {
            code = 0;
            return;
        }
    }

    Response(std::string_view data = "")
    {
        if (data.empty())
            return;
        auto header_end = data.find("\r\n\r\n");
        std::string_view header_str =
            (header_end != std::string_view::npos) ? data.substr(0, header_end) : data;

        if (header_end != std::string_view::npos) {
            body = data.substr(header_end + 4);
        }

        std::size_t pos = 0;
        bool firstLine = true;

        while (true) {
            auto line_end = header_str.find("\r\n", pos);

            std::string_view line = (line_end != std::string_view::npos)
                                        ? header_str.substr(pos, line_end - pos)
                                        : header_str.substr(pos);

            if (line.empty())
                break;

            // status line
            if (firstLine) {
                status_line = line;
                int code = 0;
                parseStatusLine(line, code, reason);
                status_code = Status(code);
                firstLine = false;
            } else {
                auto colon = line.find(':');

                // key: value
                if (colon != std::string_view::npos) {
                    std::string_view key = line.substr(0, colon);
                    std::string_view value = line.substr(colon + 1);

                    while (!value.empty() && value.front() == ' ')
                        value.remove_prefix(1);

                    headers.add(key, value);
                }
            }

            if (line_end == std::string_view::npos)
                break;
            pos = line_end + 2;
        }

        // Server will never send a body for these statuses
        const auto code = static_cast<int>(status_code);
        if ((code >= 100 && code < 200) || code == 204 || code == 304) {
            transferMode = TransferMode::NoData;
            return;
        }

        // Chunked takes priority over sized
        const auto te = headers.get("transfer-encoding");
        const auto cl = headers.get("content-length");
        if (util::icontains(te.value_or(""), "chunked")) {
            transferMode = TransferMode::Chunked;
        } else if (cl.has_value()) {
            transferMode = TransferMode::Sized;
        } else {
            transferMode = TransferMode::UntilClose;
        }
    }

    // Header utilities.
    std::size_t contentLength()
    {
        const auto cl = headers.get("content-length");
        const auto str = cl.value_or("0");

        std::size_t res;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), res);
        if (ec != std::errc{}) {
            return 0;
        }

        return res;
    }
};

} // namespace hydro

export {

    template <>
    struct std::formatter<hydro::URL> : std::formatter<std::string> {
        auto format(const hydro::URL &u, std::format_context &ctx) const
        {
            return std::format_to(ctx.out(), "{}{}{}", u.hostpart(), u.route, u.params.str);
        }
    };
}