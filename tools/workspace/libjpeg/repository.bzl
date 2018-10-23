# -*- mode: python -*-
# vi: set ft=python :

load(
    "@drake//tools/workspace:pkg_config.bzl",
    "pkg_config_repository",
)

def libjpeg_repository(
        name,
        licenses = ["restricted"],  # LGPL-2.1+
        modname = "libjpeg",
        **kwargs):
    pkg_config_repository(
        name = name,
        licenses = licenses,
        modname = modname,
        **kwargs
    )
