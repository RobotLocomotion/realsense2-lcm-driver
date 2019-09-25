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
    "0303727239528f1c2c075252681fdfd1e5da9362",
    "40c19927d3bda3143b61b5db6f55f61aa55e137156745997793532378c067a73",
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
