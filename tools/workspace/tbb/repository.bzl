# -*- mode: python -*-
# vi: set ft=python :

load(
    "@drake//tools/workspace:pkg_config.bzl",
    "pkg_config_repository",
)

def tbb_repository(
        name,
        # Note that as of Ubuntu 18.04, the license will be different.
        licenses = ["restricted"],  # GPL-2.0
        modname = "tbb",
        **kwargs):
    pkg_config_repository(
        name = name,
        licenses = licenses,
        modname = modname,
        **kwargs
    )
