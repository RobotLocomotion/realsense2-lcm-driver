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
            "librealsense2_2.10.3-0~realsense0.66_amd64.deb",
            "librealsense2-dev_2.10.3-0~realsense0.66_amd64.deb",
            "librealsense2-udev-rules_2.10.3-0~realsense0.66_amd64.deb",
        ],
        sha256s = [
            "e0149de58e7c8fe8e4ec27c530b53b263427dc6dc7c72698a991004ce8dd88c6",
            "ba31b0bafe8c22b64a0daa81e7efb6b278580f4959d1d81ae74659f68bf682da",
            "62d5c0a8a0957fb4f6d3fe02d3f38402564e14f12ae4ac8310046ccf2dd9a467",
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
