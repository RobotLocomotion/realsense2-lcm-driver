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
            "http://realsense-hw-public.s3.amazonaws.com/Debian/apt-repo/pool/xenial/main/libr/librealsense2",  # noqa
        ],
        filenames = [
            "librealsense2_2.16.1-0~realsense0.88_amd64.deb",
            "librealsense2-dev_2.16.1-0~realsense0.88_amd64.deb",
            "librealsense2-udev-rules_2.16.1-0~realsense0.88_amd64.deb",
        ],
        sha256s = [
            "7bc7c3835f46c3e2519801fb2b04c10123b8ac8b2fe4e682686ce09d849431b7",
            "3a8b14907b93df5239233cc79bcb56cf44f1683d8b306f51397797b459f573b2",
            "21ae912363597edf5ee8e1d131e2cdd5f9e8707392e4a6ba2cf8d9c1fe1caf4b",
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
