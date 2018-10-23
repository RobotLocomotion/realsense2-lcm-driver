#!/usr/bin/python3 -B
# Copyright 2018 Toyota Research Institute.  All rights reserved.

'''Implements auto-generation of OpenCL related files for the OpenCV
build system.

It is roughly equivalent to cmake/OpenCVCompilerOptimizations.cmake
in the opencv distribution.
'''

import argparse
import os


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--filename', required=True)
    parser.add_argument('--outdir', required=True,
                        help=("This must be a filename located in the " +
                              "output directory (it does not need to exist"))
    parser.add_argument('optimizations', metavar='OPT', type=str, nargs='+')
    args = parser.parse_args()

    code_str = """#include "precomp.hpp"
#include "{filename}.simd.hpp"
""".format(filename=args.filename)

    outdir = os.path.dirname(args.outdir)

    dispatch_modes = "BASELINE"
    declarations_file = os.path.join(
        outdir, "{filename}.simd_declarations.hpp".format(
            filename=args.filename))

    with open(declarations_file, "w") as declarations_fp:
        declarations_fp.write("""#define CV_CPU_SIMD_FILENAME "{filename}.simd.hpp"
""".format(filename=args.filename))

        for optimization in args.optimizations:
            source_name = os.path.join(
                outdir, "{filename}.{opt}.cpp".format(
                    filename=args.filename, opt=optimization.lower()))

            with open(source_name, "w") as optimization_fp:
                optimization_fp.write(code_str)

            declarations_fp.write("""
#define CV_CPU_DISPATCH_MODE {opt}
#include "opencv2/core/private/cv_cpu_include_simd_declarations.hpp"
""".format(opt=optimization))

            dispatch_modes = optimization + ", " + dispatch_modes

        declarations_fp.write("""
#define CV_CPU_DISPATCH_MODES_ALL {dispatch_modes}
""".format(dispatch_modes=dispatch_modes))


if __name__ == '__main__':
    main()
