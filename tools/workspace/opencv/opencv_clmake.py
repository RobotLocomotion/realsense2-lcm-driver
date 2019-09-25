#!/usr/bin/python3 -B
# Copyright 2018 Toyota Research Institute.  All rights reserved.

'''Implements auto-generation of OpenCL related files for the OpenCV
build system.

It is roughly equivalent to cmake/cl2cpp.cmake in the opencv
distribution.
'''

import argparse
import hashlib
import io
import os


# Rather than escape every possibly bad C++ thing, we just hex encode
# the entire contents of the CL file for inclusion.
def hexify(data):
    return ":".join([bytes([c]).hex() for c in data])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--header', help='output header', required=True)
    parser.add_argument('--source', help='output source', required=True)
    parser.add_argument('--module', help='opencv module', required=True)
    parser.add_argument('cl_files', metavar='CL', type=str, nargs='+',
                        help='OpenCL source file')
    args = parser.parse_args()

    with open(args.header, 'w') as header, open(args.source, 'w') as source:
        source.write('''// This file is auto-generated. Do not edit!

#include <string.h>

#include <sstream>
#include <string>

#include "precomp.hpp"
#include "cvconfig.h"
#include "{header}"

#ifdef HAVE_OPENCL

namespace cv
{{
namespace ocl
{{
namespace {module}
{{

namespace {{
const char* unhexify(const std::string& data) {{
  std::ostringstream ostr;
  for (size_t i = 0; i < data.size(); i += 2) {{
     const char val = std::stoi(data.substr(i, 2), nullptr, 16);
     ostr.write(&val, 1);
  }}
  return ::strdup(ostr.str().c_str());
}}
}}

static const char* const moduleName = "{module}";

'''.format(header=os.path.basename(args.header), module=args.module))

        header.write('''// This file is auto-generated. Do not edit!

#include "opencv2/core/ocl.hpp"
#include "opencv2/core/ocl_genbase.hpp"
#include "opencv2/core/opencl/ocl_defs.hpp"

#ifdef HAVE_OPENCL

namespace cv
{{
namespace ocl
{{
namespace {module}
{{

'''.format(module=args.module))

        for cl_file in args.cl_files:
            cl_basename = os.path.splitext(os.path.basename(cl_file))[0]
            contents = open(cl_file, 'rb').read()
            hexified = hexify(contents)
            md5_hash = hashlib.md5(contents).hexdigest()

            source.write(
                'struct cv::ocl::internal::ProgramEntry '
                '{cl_file}_oclsrc={{moduleName, '
                '"{cl_basename}", '
                'unhexify("{hexified}"), '
                '"{md5_hash}", NULL}};\n'.format(
                    cl_file=cl_basename,
                    cl_basename=cl_basename,
                    hexified=hexified,
                    md5_hash=md5_hash))
            header.write(
                'extern struct cv::ocl::internal::ProgramEntry '
                '{cl_file}_oclsrc;\n'.format(cl_file=cl_basename))

        source.write('\n}}}\n#endif')
        header.write('\n}}}\n#endif')


if __name__ == '__main__':
    main()
