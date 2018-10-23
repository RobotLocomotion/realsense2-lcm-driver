# -*- python -*-
# vi: set ft=python :

# Reference external software libraries and tools per Drake's defaults.  Some
# software will come from the host system (Ubuntu or macOS); other software
# will be downloaded in source or binary form from github or other sites.
load(
    "@drake//tools/workspace:default.bzl",
    drake_add_default_repositories = "add_default_repositories",
)
load("//tools/workspace:mirrors.bzl", "DEFAULT_MIRRORS")
load("//tools/workspace/boost:repository.bzl", "boost_repository")
load("//tools/workspace/libjpeg:repository.bzl", "libjpeg_repository")
load("//tools/workspace/libtiff:repository.bzl", "libtiff_repository")
load("//tools/workspace/opencv:repository.bzl", "opencv_repository")
load("//tools/workspace/realsense2:repository.bzl", "realsense2_repository")
load("//tools/workspace/tbb:repository.bzl", "tbb_repository")

def add_default_repositories(excludes = [], mirrors = DEFAULT_MIRRORS):
    # N.B. We do *not* pass `mirrors = ` into the drake_add_default_... call.
    # We want to use its (wider) set of defaults for the packages it knows
    # about; we only use Anzu's mirrors list for Anzu-specific packages.
    drake_add_default_repositories(excludes = [
        "boost",
        "godotengine",  # TODO(jeremy-nimmer) Push our patches back into Drake.
        "snopt",
    ])
    if "boost" not in excludes:
        boost_repository(name = "boost")
    if "libjpeg" not in excludes:
        libjpeg_repository(name = "libjpeg")
    if "libtiff" not in excludes:
        libtiff_repository(name = "libtiff")
    if "opencv" not in excludes:
        opencv_repository(name = "opencv", mirrors = mirrors)
    if "realsense2" not in excludes:
        realsense2_repository(name = "realsense2")
    if "tbb" not in excludes:
        tbb_repository(name = "tbb")
