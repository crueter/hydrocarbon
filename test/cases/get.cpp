// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <chrono>
#include <string>
#include <print>
#include <format>

import hydrocarbon;
import tests;
import jacinth;

static void test_http() {
    const hydro::URL url = "http://httpbin.org/get?param1=value1&param2=value2";
    auto ec = hydro::get(url);
    tests::check_err("httpbin.org/get request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("returned method", doc.get<std::string>("method"), "GET");
    tests::check_match("returned URL", doc.get<std::string>("url"), std::format("{}", url));
    tests::check_match("returned status code", ec->status_code, hydro::Status::Ok);

    const auto args = doc.get("args");
    tests::check_match("param1", args.get<std::string>("param1"), "value1");
    tests::check_match("param2", args.get<std::string>("param2"), "value2");
}

static void test_get() {
    const hydro::URL url = "https://httpbin.org/get?param1=value1&param2=value2";
    auto ec = hydro::get(url);
    tests::check_err("HTTPS httpbin.org/get request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("HTTPS returned method", doc.get<std::string>("method"), "GET");
    tests::check_match("HTTPS returned URL", doc.get<std::string>("url"), std::format("{}", url));
    tests::check_match("HTTPS returned status code", ec->status_code, hydro::Status::Ok);

    const auto args = doc.get("args");
    tests::check_match("HTTPS param1", args.get<std::string>("param1"), "value1");
    tests::check_match("HTTPS param2", args.get<std::string>("param2"), "value2");
}

static void test_drip() {
    std::string body;
    const auto func = [&body](const hydro::DownloadState &state) {
        std::println("Received {}/{} bytes ({} remaining)", state.bytes_received,
                        state.content_length, state.bytes_left);

        body += state.buffer;

        return true;
    };

    static std::size_t bytes = 10;
    const auto url = std::format("https://httpbin.org/drip?delay=0.5&numbytes={}&duration=1", bytes);
    auto ec = hydro::get(url, {.callback = func});

    tests::check_err("httpbin.org/drip request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    tests::check_match("drip body size", body.size(), bytes);
}

static void test_cancel() {
    using namespace std::chrono_literals;
    using sc = std::chrono::steady_clock;

    sc::time_point t0{0ms};

    const auto func = [&t0](const hydro::DownloadState &state) {
        // first byte
        if (state.bytes_received == 1) {
            t0 = sc::now();
        }

        const auto diff = std::chrono::steady_clock::now() - t0;
        const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(diff);
        std::println("Received {}/{} bytes ({} remaining) (current time {}, cancelling at 1000ms)", state.bytes_received,
                        state.content_length, state.bytes_left, dur);

        return (std::chrono::steady_clock::now() - t0) < 1s;
    };

    static std::size_t bytes = 10;
    const auto url = std::format("https://httpbin.org/drip?delay=0.5&numbytes={}&duration=2", bytes);
    auto ec = hydro::get(url, {.callback = func});

    tests::check_err("cancelled httpbin.org/drip request succeeds", ec, hydro::Error::Success);

    if (!ec) return;
}

int main() {
    test_http();
    test_get();
    test_drip();
    test_cancel();

    tests::exit();
}