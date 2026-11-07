// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

module;

#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <utility>

export module hydrocarbon:enums;

export namespace hydro
{

// Basic API
enum class Status {
    NoStatusCode = 0,

    Continue = 100,
    SwitchingProtocols = 101,
    Processing = 102,
    EarlyHints = 103,

    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultiStatus = 207,
    AlreadyReported = 208,
    ImUsed = 226,

    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    TemporaryRedirect = 307,
    PermanentRedirect = 308,

    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeout = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    PayloadTooLarge = 413,
    UriTooLong = 414,
    UnsupportedMediaType = 415,
    RangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    ImATeapot = 418,
    MisdirectedRequest = 421,
    UnprocessableEntity = 422,
    Locked = 423,
    FailedDependency = 424,
    TooEarly = 425,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests = 429,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeout = 504,
    HTTPVersionNotSupported = 505,
    VariantAlsoNegotiates = 506,
    InsufficientStorage = 507,
    LoopDetected = 508,
    NotExtended = 510,
    NetworkAuthenticationRequired = 511
};

enum class Protocol { NoProtocol = 0, HTTP, HTTPS };

enum class Method { GET, POST, PUT, HEAD, OPTIONS };

enum class TransferMode {
    // Server will not send any data at all
    NoData = 0,

    // Explicit content-length
    Sized = 1,

    // Chunked transfers
    Chunked = 2,

    // Read until server closes connection
    UntilClose = 3,
};

enum class Version { HTTP_1_0 = -1, HTTP_1_1 = 0 };

// Connection errors.
enum class Error {
    // No error
    Success,

    // Failed DNS lookup
    DNSFailure,

    // Failed to create socket
    SocketCreate,

    // Connection to IP failed
    ConnectFailed,

    // Invalid TLS context
    TLSCtx,

    // Failed to initialize TLS
    TLSInit,

    // Failed to grab certificate
    TLSCert,

    // Failed to verify certificate
    TLSVerify,

    // Invalid certificate passed in
    InvalidCert,

    // Failed to send data
    SendFailed,

    // Failed to read data
    ReadFailed,

    // Headers are way too large to handle
    HeaderTooBig,

    // Malformed or otherwise invalid chunk data
    InvalidChunk,

    // Connection timeout
    Timeout,
};

using namespace std::chrono_literals;

// Connection options
struct ConnectOptions {
    // Whether or not to verify the server certificate.
    bool verify = true;

    // A custom certificate to load.
    std::string cert = "";

    // Disable SSL entirely.
    bool disableSsl = false;

    // R/W timeout.
    // std::chrono::milliseconds timeout = 0ms;

    // Initial connection timeout.
    std::chrono::milliseconds connectionTimeout = 0ms;
};

} // namespace hydro

// formatter specs
export {

    template <>
    struct std::formatter<hydro::Error> : std::formatter<std::string_view> {
        auto format(const hydro::Error e, std::format_context &ctx) const
        {
            std::string_view name;
            switch (e) {
            case hydro::Error::Success:
                name = "No error";
                break;
            case hydro::Error::DNSFailure:
                name = "DNS lookup failure";
                break;
            case hydro::Error::SocketCreate:
                name = "OS failed to create socket";
                break;
            case hydro::Error::ConnectFailed:
                name = "Failed to connect to host";
                break;
            case hydro::Error::TLSCtx:
                name = "Failed to initialize TLS context";
                break;
            case hydro::Error::TLSInit:
                name = "Failed to initialize TLS state";
                break;
            case hydro::Error::TLSCert:
                name = "Server returned no or invalid certificate";
                break;
            case hydro::Error::TLSVerify:
                name = "Failed to verify server certificate";
                break;
            case hydro::Error::InvalidCert:
                name = "Invalid custom certificate passed";
                break;
            case hydro::Error::SendFailed:
                name = "Connection closed or failed during send";
                break;
            case hydro::Error::ReadFailed:
                name = "Connection closed or failed during read";
                break;
            case hydro::Error::InvalidChunk:
                name = "Invalid or malformed chunk data received";
                break;
            case hydro::Error::HeaderTooBig:
                name = "Response header is too large";
                break;
            case hydro::Error::Timeout:
                name = "Connection timed out";
                break;
            }

            return std::formatter<std::string_view>::format(name, ctx);
        }
    };

    template <>
    struct std::formatter<hydro::Status> : std::formatter<std::string_view> {
        auto format(const hydro::Status s, std::format_context &ctx) const
        {
            std::string name;

            // This will not always match the status line, but is close enough.
            switch (s) {
            case hydro::Status::NoStatusCode:
                name = "0 Unknown";
                break;
            case hydro::Status::Continue:
                name = "100 Continue";
                break;
            case hydro::Status::SwitchingProtocols:
                name = "101 Switching Protocols";
                break;
            case hydro::Status::Processing:
                name = "102 Processing";
                break;
            case hydro::Status::EarlyHints:
                name = "103 Early Hints";
                break;
            case hydro::Status::Ok:
                name = "200 OK";
                break;
            case hydro::Status::Created:
                name = "201 Created";
                break;
            case hydro::Status::Accepted:
                name = "202 Accepted";
                break;
            case hydro::Status::NonAuthoritativeInformation:
                name = "203 Non-Authoritative Information";
                break;
            case hydro::Status::NoContent:
                name = "204 No Content";
                break;
            case hydro::Status::ResetContent:
                name = "205 Reset Content";
                break;
            case hydro::Status::PartialContent:
                name = "206 Partial Content";
                break;
            case hydro::Status::MultiStatus:
                name = "207 Multi-Status";
                break;
            case hydro::Status::AlreadyReported:
                name = "208 Already Reported";
                break;
            case hydro::Status::ImUsed:
                name = "226 IM Used";
                break;
            case hydro::Status::MultipleChoices:
                name = "300 Multiple Choices";
                break;
            case hydro::Status::MovedPermanently:
                name = "301 Moved Permanently";
                break;
            case hydro::Status::Found:
                name = "302 Found";
                break;
            case hydro::Status::SeeOther:
                name = "303 See Other";
                break;
            case hydro::Status::NotModified:
                name = "304 Not Modified";
                break;
            case hydro::Status::UseProxy:
                name = "305 Use Proxy";
                break;
            case hydro::Status::TemporaryRedirect:
                name = "307 Temporary Redirect";
                break;
            case hydro::Status::PermanentRedirect:
                name = "308 Permanent Redirect";
                break;
            case hydro::Status::BadRequest:
                name = "400 Bad Request";
                break;
            case hydro::Status::Unauthorized:
                name = "401 Unauthorized";
                break;
            case hydro::Status::PaymentRequired:
                name = "402 Payment Required";
                break;
            case hydro::Status::Forbidden:
                name = "403 Forbidden";
                break;
            case hydro::Status::NotFound:
                name = "404 Not Found";
                break;
            case hydro::Status::MethodNotAllowed:
                name = "405 Method Not Allowed";
                break;
            case hydro::Status::NotAcceptable:
                name = "406 Not Acceptable";
                break;
            case hydro::Status::ProxyAuthenticationRequired:
                name = "407 Proxy Authentication Required";
                break;
            case hydro::Status::RequestTimeout:
                name = "408 Request Timeout";
                break;
            case hydro::Status::Conflict:
                name = "409 Conflict";
                break;
            case hydro::Status::Gone:
                name = "410 Gone";
                break;
            case hydro::Status::LengthRequired:
                name = "411 Length Required";
                break;
            case hydro::Status::PreconditionFailed:
                name = "412 Precondition Failed";
                break;
            case hydro::Status::PayloadTooLarge:
                name = "413 Payload Too Large";
                break;
            case hydro::Status::UriTooLong:
                name = "414 URI Too Long";
                break;
            case hydro::Status::UnsupportedMediaType:
                name = "415 Unsupported Media Type";
                break;
            case hydro::Status::RangeNotSatisfiable:
                name = "416 Range Not Satisfiable";
                break;
            case hydro::Status::ExpectationFailed:
                name = "417 Expectation Failed";
                break;
            case hydro::Status::ImATeapot:
                name = "418 I'm a teapot";
                break;
            case hydro::Status::MisdirectedRequest:
                name = "421 Misdirected Request";
                break;
            case hydro::Status::UnprocessableEntity:
                name = "422 Unprocessable Entity";
                break;
            case hydro::Status::Locked:
                name = "423 Locked";
                break;
            case hydro::Status::FailedDependency:
                name = "424 Failed Dependency";
                break;
            case hydro::Status::TooEarly:
                name = "425 Too Early";
                break;
            case hydro::Status::UpgradeRequired:
                name = "426 Upgrade Required";
                break;
            case hydro::Status::PreconditionRequired:
                name = "428 Precondition Required";
                break;
            case hydro::Status::TooManyRequests:
                name = "429 Too Many Requests";
                break;
            case hydro::Status::RequestHeaderFieldsTooLarge:
                name = "431 Request Header Fields Too Large";
                break;
            case hydro::Status::UnavailableForLegalReasons:
                name = "451 Unavailable For Legal Reasons";
                break;
            case hydro::Status::InternalServerError:
                name = "500 Internal Server Error";
                break;
            case hydro::Status::NotImplemented:
                name = "501 Not Implemented";
                break;
            case hydro::Status::BadGateway:
                name = "502 Bad Gateway";
                break;
            case hydro::Status::ServiceUnavailable:
                name = "503 Service Unavailable";
                break;
            case hydro::Status::GatewayTimeout:
                name = "504 Gateway Timeout";
                break;
            case hydro::Status::HTTPVersionNotSupported:
                name = "505 HTTP Version Not Supported";
                break;
            case hydro::Status::VariantAlsoNegotiates:
                name = "506 Variant Also Negotiates";
                break;
            case hydro::Status::InsufficientStorage:
                name = "507 Insufficient Storage";
                break;
            case hydro::Status::LoopDetected:
                name = "508 Loop Detected";
                break;
            case hydro::Status::NotExtended:
                name = "510 Not Extended";
                break;
            case hydro::Status::NetworkAuthenticationRequired:
                name = "511 Network Authentication Required";
                break;
            default:
                name = std::to_string(std::to_underlying(s));
                break;
            }

            return std::formatter<std::string_view>::format(name, ctx);
        }
    };
}
