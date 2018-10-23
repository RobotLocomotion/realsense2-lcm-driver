# -*- mode: python -*-
# vi: set ft=python :

load(
    "@drake//tools/workspace:github.bzl",
    "github_archive",
)

def opencv_repository(
        name,
        mirrors = None):
    github_archive(
        name = name,
        repository = "opencv/opencv",
        commit = "3.4.0",
        sha256 = "678cc3d2d1b3464b512b084a8cca1fad7de207c7abdf2caa1fed636c13e916da",  # noqa
        build_file = "//tools/workspace/opencv:package.BUILD.bazel",
        mirrors = mirrors,
    )
