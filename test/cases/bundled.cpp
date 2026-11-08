// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <string>

#include "cert.hpp"

import hydrocarbon;
import tests;

static void test_bundled_cert() {
    hydro::RequestOptions opts;
    opts.connect.cert = std::string{kCert};

    const hydro::URL url = "https://httpbin.org/get";
    auto ec = hydro::head(url, opts);
    tests::check_err("Bundled certificate HEAD request succeeds", ec, hydro::Error::Success);
}

int main() {
    test_bundled_cert();

    tests::exit();
}