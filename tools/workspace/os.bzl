# -*- python -*-
# vi: set ft=python :

load(
    "@drake//tools/workspace:execute.bzl",
    "execute_and_return",
)

def _os_impl(repo_ctx):
    # Find the Ubuntu version number.
    result = execute_and_return(repo_ctx, ["/usr/bin/lsb_release", "-sr"])
    if result.error:
        fail(result.error)
    ubuntu_release = result.stdout.strip()

    repo_ctx.file("BUILD.bazel", "")

    constants = """
UBUNTU_RELEASE = "{ubuntu_release}"
    """.format(
        ubuntu_release = ubuntu_release,
    )
    repo_ctx.file("os.bzl", constants)

os_repository = repository_rule(
    implementation = _os_impl,
)
