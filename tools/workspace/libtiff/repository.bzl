# -*- mode: python -*-
# vi: set ft=python :

load(
    "@drake//tools/workspace:pkg_config.bzl",
    "pkg_config_repository",
)

def libtiff_repository(
        name,
        licenses = ["notice"],  # libtiff
        modname = "libtiff-4",
        **kwargs):
    pkg_config_repository(
        name = name,
        licenses = licenses,
        modname = modname,
        **kwargs
    )
