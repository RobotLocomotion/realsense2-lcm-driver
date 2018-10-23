# -*- mode: python -*-
# vi: set ft=python :

def _impl(repository_ctx):
    repository_ctx.symlink("/usr/include/boost", "boost")
    repository_ctx.symlink(
        Label("@realsense2_lcm_driver//tools/workspace/boost:package.BUILD.bazel"),  # noqa
        "BUILD.bazel",
    )

boost_repository = repository_rule(
    local = True,
    implementation = _impl,
)
