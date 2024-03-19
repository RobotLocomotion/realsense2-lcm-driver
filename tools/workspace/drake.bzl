# -*- mode: python -*-
# vi: set ft=python :

load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

# ================ USING A GITHUB COPY OF DRAKE ================

# Specify which revision of Drake to use.
#
# To change which commit Bazel will download, the steps are:
# 1. Update the first string below to the new commit.
# 2. Update the second string below to be the empty string.
# 3. Run `bazel build`; a checksum error will appear.
# 4. Update the second string below to the suggested new checksum.
# 5. Run `bazel build`; it should successfully download now.
#
(DRAKE_COMMIT, DRAKE_CHECKSUM) = (
    "1dd7381234b12e1ee580ac3014417d8dea489485",
    "d1f5ea29388523e5c7221b6c30fcf6f8c1b9fa3bfdd0290e178a531f664298a4",
)

# ================ IMPLEMENTATION DETAILS ================

def drake_repository():
    # Download a specific commit of Drake, from github.
    http_archive(
        name = "drake",
        sha256 = DRAKE_CHECKSUM or ("0" * 64),
        strip_prefix = "drake-{}".format(DRAKE_COMMIT),
        urls = [x.format(DRAKE_COMMIT) for x in [
            "https://github.com/RobotLocomotion/drake/archive/{}.tar.gz",
        ]],
    )
