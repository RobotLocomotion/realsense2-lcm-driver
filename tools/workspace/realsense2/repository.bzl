# -*- mode: python -*-
# vi: set ft=python :

load(
    "@drake//tools/workspace:deb.bzl",
    "new_deb_archive",
)

# Use librealsense1 debs provided by
# https://software.intel.com/sites/products/realsense/intro/getting_started.html
#
# To update the URLs, add the apt repository as noted in the instructions then
#   apt -y install --print-uris librealsense2-dev

# The full list of debs:
# "librealsense2-dkms_1.2.0-0ubuntu5_all.deb",
# "librealsense2-udev-rules_2.10.3-0~realsense0.66_amd64.deb",
# "librealsense2_2.10.3-0~realsense0.66_amd64.deb",
# "librealsense2-utils_2.10.3-0~realsense0.66_amd64.deb",
# "librealsense2-dev_2.10.3-0~realsense0.66_amd64.deb",
# "librealsense2-dbg_2.10.3-0~realsense0.66_amd64.deb",

def realsense2_repository(
        name,
        # TODO(jeremy.nimmer@tri.global) Ideally we'd mate this mirrors list
        # with the mirrors.bzl design, once we are using that.
        realsense_mirrors = [
            "https://librealsense.intel.com/Debian/apt-repo/pool/bionic/main",  # noqa
        ],
        filenames = [
            "librealsense2_2.28.1-0~realsense0.1554_amd64.deb",
            "librealsense2-dev_2.28.1-0~realsense0.1554_amd64.deb",
            "librealsense2-udev-rules_2.28.1-0~realsense0.1554_amd64.deb",
        ],
        sha256s = [
            "11241508b4bdafcc46c906a8357ea1471d30fd56608745927f9710ea8a85c711",
            "5d19cc7d2da1aa351fbe7f0abc4ed77d9e37b692ce41583513a263dbb757c7c8",
            "a1342d6a4a6a483279b2a75646dbe69726c9743236e52ecac45882a1085d0ebc",
        ],
        build_file = "//tools/workspace/realsense2:package.BUILD.bazel",
        **kwargs):
    new_deb_archive(
        name = name,
        mirrors = realsense_mirrors,
        filenames = filenames,
        sha256s = sha256s,
        build_file = build_file,
    )
