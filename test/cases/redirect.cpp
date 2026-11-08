// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <format>
#include <string>

import hydrocarbon;
import tests;
import jacinth;

// WARNING: this is pure AI slop, HTTP redirection is the worst written spec of all time.

static void test_status_codes()
{
    struct Case {
        const char *name;
        const char *url;
        hydro::Status code;
    };
    const Case cases[] = {
        {"200", "https://httpbin.org/status/200", hydro::Status::Ok},
        {"301", "https://httpbin.org/status/301", hydro::Status::MovedPermanently},
        {"302", "https://httpbin.org/status/302", hydro::Status::Found},
        {"303", "https://httpbin.org/status/303", hydro::Status::SeeOther},
        {"304", "https://httpbin.org/status/304", hydro::Status::NotModified},
        {"305", "https://httpbin.org/status/305", hydro::Status::UseProxy},
        {"307", "https://httpbin.org/status/307", hydro::Status::TemporaryRedirect},
        {"308", "https://httpbin.org/status/308", hydro::Status::PermanentRedirect},
        {"404", "https://httpbin.org/status/404", hydro::Status::NotFound},
        {"500", "https://httpbin.org/status/500", hydro::Status::InternalServerError},
    };

    for (const auto &c : cases) {
        hydro::RequestOptions opts;
        opts.redirect.follow = false;
        auto ec = hydro::get(c.url, opts);
        tests::check_err(std::format("{} status request succeeds", c.name), ec,
                         hydro::Error::Success);
        if (!ec)
            continue;
        tests::check_match(std::format("status code {}", c.name), (int) ec->status_code,
                           (int) c.code);
    }
}

// --- resolveLocation: absolute vs relative Location (httpbin /redirect-to) ----

static void test_redirect_relative()
{
    // Location = /get  -> stays on https://httpbin.org/get
    auto ec = hydro::get("https://httpbin.org/redirect-to?url=/get&status_code=302");
    tests::check_err("relative redirect succeeds", ec, hydro::Error::Success);
    if (!ec)
        return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("relative -> method", doc.get<std::string>("method"), "GET");
    tests::check_match("relative -> url", doc.get<std::string>("url"), "https://httpbin.org/get");
}

static void test_redirect_absolute()
{
    // Location = http://httpbin.org/get  -> scheme/host come from the Location
    auto ec =
        hydro::get("https://httpbin.org/redirect-to?url=http://httpbin.org/get&status_code=302");
    tests::check_err("absolute redirect succeeds", ec, hydro::Error::Success);
    if (!ec)
        return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("absolute -> method", doc.get<std::string>("method"), "GET");
    tests::check_match("absolute -> url", doc.get<std::string>("url"), "http://httpbin.org/get");
}

static void test_redirect_relative_chain()
{
    // /relative-redirect/2 -> two relative hops then /get
    auto ec = hydro::get("https://httpbin.org/relative-redirect/2");
    tests::check_err("relative redirect chain succeeds", ec, hydro::Error::Success);
    if (!ec)
        return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("relative chain -> method", doc.get<std::string>("method"), "GET");
}

static void test_redirect_absolute_chain()
{
    // /absolute-redirect/2 -> two absolute hops then /get
    auto ec = hydro::get("https://httpbin.org/absolute-redirect/2");
    tests::check_err("absolute redirect chain succeeds", ec, hydro::Error::Success);
    if (!ec)
        return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("absolute chain -> method", doc.get<std::string>("method"), "GET");
}

// --- method/body downgrading (httpbin /redirect-to?url=/post) -----------------

static void test_downgrade(const char *name, const char *target, const char *status_code, bool preserve,
                    const char *expect_method, const char *expect_data)
{
    hydro::RequestOptions opts;
    opts.body = "{\"k\":\"v\"}";
    opts.headers.add("content-type", "application/json");
    opts.redirect.preserveMethod = preserve;

    auto ec = hydro::post(
        std::format("https://httpbin.org/redirect-to?url={}&status_code={}", target, status_code),
        opts);
    tests::check_err(std::format("{} succeeds", name), ec, hydro::Error::Success);
    if (!ec)
        return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match(std::format("{} method", name), doc.get<std::string>("method"),
                       expect_method);
    tests::check_match(std::format("{} body dropped/kept", name), doc.get<std::string>("data"),
                       expect_data);
}

static void test_downgrade_matrix()
{
    // 301/302: default downgrades POST->GET and drops the body;
    // preserveMethod=true keeps POST + body
    test_downgrade("301 default", "/get", "301", false, "GET", "");
    test_downgrade("301 preserve", "/post", "301", true, "POST", "{\"k\":\"v\"}");
    test_downgrade("302 preserve", "/post", "302", true, "POST", "{\"k\":\"v\"}");
    // 303: always downgrade to GET and drop the body
    test_downgrade("303", "/get", "303", false, "GET", "");
    // 307/308: preserve method AND body regardless of preserveMethod
    test_downgrade("307", "/post", "307", false, "POST", "{\"k\":\"v\"}");
    test_downgrade("308", "/post", "308", false, "POST", "{\"k\":\"v\"}");
}

// --- perform() loop controls -------------------------------------------------

static void test_redirect_follow_false()
{
    hydro::RequestOptions opts;
    opts.redirect.follow = false;

    auto ec = hydro::get("https://httpbin.org/redirect-to?url=/get&status_code=302", opts);
    tests::check_err("follow=false does not follow", ec, hydro::Error::Success);
    if (!ec)
        return;

    tests::check_match("follow=false keeps 302", (int) ec->status_code, (int) hydro::Status::Found);
    tests::check("follow=false keeps location header", ec->headers.contains("location"));
}

static void test_redirect_max_limit()
{
    // /redirect/5 does 5 hops; with maxRedirects=1 we should stop on a 30x.
    hydro::RequestOptions opts;
    opts.redirect.maxRedirects = 1;

    auto ec = hydro::get("https://httpbin.org/redirect/5", opts);
    tests::check_err("maxRedirects=1 stops early", ec, hydro::Error::Success);
    if (!ec)
        return;

    tests::check_match("maxRedirects=1 returns 302", (int) ec->status_code,
                       (int) hydro::Status::Found);
    tests::check("maxRedirects=1 keeps location header", ec->headers.contains("location"));
}

static void test_redirect_default_reaches_target()
{
    // /redirect/3 with the default maxRedirects=5 should reach /get (200).
    auto ec = hydro::get("https://httpbin.org/redirect/3");
    tests::check_err("default redirect reaches target", ec, hydro::Error::Success);
    if (!ec)
        return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("default -> status 200", (int) ec->status_code, (int) hydro::Status::Ok);
    tests::check_match("default -> method GET", doc.get<std::string>("method"), "GET");
}

int main()
{
    test_status_codes();
    test_redirect_relative();
    test_redirect_absolute();
    test_redirect_relative_chain();
    test_redirect_absolute_chain();
    test_downgrade_matrix();
    test_redirect_follow_false();
    test_redirect_max_limit();
    test_redirect_default_reaches_target();

    tests::exit();
}
