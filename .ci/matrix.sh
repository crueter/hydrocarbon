#!/bin/sh -e

# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

# Windows: msvc, clang-cl, msys2 gcc (amd64 only) + clang
# Linux: ubuntu, arch; gcc, clang, clang-19
# macOS: appleclang, homebrew clang

matrix() {
	printf '"name": "%s"' "$1"
}

msys() {
	matrix "$1"

	printf ', "msystem": "%s"' "$2"
}

container() {
	matrix "$1"

	printf ', "container": "%s"' "$2"
}

WIN="$(matrix "Windows")"
WIN2022="$(matrix "Windows 2022")"

UBUNTU_LTS="$(container "Ubuntu 26.04" ubuntu:26.04)"
UBUNTU_STABLE="$(container "Ubuntu 26.10" ubuntu:26.10)"

ARCH="$(container "Arch Linux" ghcr.io/pkgforge-dev/archlinux:latest)"
MACOS="$(matrix "macOS")"

UCRT64="$(msys "MSYS2 UCRT64" UCRT64)"
CLANGARM64="$(msys "MSYS2 CLANGARM64" CLANGARM64)"

CL='msvc clang-cl'
CLANG_CL='clang-cl'
GCC='gcc'
GCC_CLANG='gcc clang'
CLANG='clang'
APPLE_CLANG='brew-clang'

# broken, appleclang does not yet support modules
# APPLE_CLANG='appleclang brew-clang'

# runs-on=arch
WIN2022_RO="windows-2022=amd64"
WIN_RO="windows-latest=amd64 windows-11-arm=aarch64"
UBUNTU_RO="ubuntu-26.04=amd64 ubuntu-26.04-arm=aarch64"

ARCH_RO="$UBUNTU_RO"
UCRT_RO="windows-latest=amd64"
CLANGARM_RO="windows-11-arm=aarch64"
MACOS_RO="macos-latest=aarch64"

n=0

# data, runner, arch, program, refl
target() {
	if [ "$n" -gt 0 ]; then
		printf ", "
	fi

	printf '{%s, "runs-on": "%s", "arch": "%s", "program": "%s"}' "$1" "$2" "$3" "$4"
	n=$((n + 1))
}

programs() {
	data="$1"
	runs="$2"
	progs="$3"

	for o in $runs; do
		runner="${o%%=*}"
		arch="${o##*=}"

		for p in $progs; do
			target "$data" "$runner" "$arch" "$p"
		done
	done
}

printf '['

# windows
programs "$WIN" "$WIN_RO" "$CL"
programs "$WIN2022" "$WIN2022_RO" "$CLANG_CL"

# msys
programs "$UCRT64" "$UCRT_RO" "$GCC"
programs "$UCRT64" "$UCRT_RO" "$CLANG"
programs "$CLANGARM64" "$CLANGARM_RO" "$CLANG"

# ubuntu
programs "$UBUNTU_LTS" "$UBUNTU_RO" "$GCC_CLANG"
programs "$UBUNTU_STABLE" "$UBUNTU_RO" "$GCC_CLANG"

# arch
programs "$ARCH" "$ARCH_RO" "$GCC_CLANG"

# macos
programs "$MACOS" "$MACOS_RO" "$APPLE_CLANG"

echo ']'
