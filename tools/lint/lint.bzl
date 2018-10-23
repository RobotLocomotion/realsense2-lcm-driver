# -*- python -*-

load(
    "@drake//tools/lint:lint.bzl",
    drake_bazel_lint = "bazel_lint",
    drake_cpplint = "cpplint",
    drake_pylint = "python_lint",
)

def add_lint_tests(
        cpplint_data = None,
        cpplint_extra_srcs = None,
        python_lint_ignore = None,
        python_lint_exclude = None,
        python_lint_extra_srcs = None,
        bazel_lint_ignore = None,
        bazel_lint_extra_srcs = None,
        bazel_lint_exclude = None):
    existing_rules = native.existing_rules().values()
    cpplint_data = (cpplint_data or []) + [
        "//:.clang-format",
    ]

    # TODO(jwnimmer-tri) Also enable pylint, possibly with Anzu-specific
    # configs.
    drake_cpplint(
        existing_rules = existing_rules,
        data = cpplint_data,
        extra_srcs = cpplint_extra_srcs,
    )
    drake_bazel_lint(
        ignore = bazel_lint_ignore,
        extra_srcs = bazel_lint_extra_srcs,
        exclude = bazel_lint_exclude,
    )
    drake_pylint(
        existing_rules = existing_rules,
        ignore = python_lint_ignore,
        exclude = python_lint_exclude,
        extra_srcs = python_lint_extra_srcs,
    )
