# -*- python -*-
# Copyright 2018 Toyota Research Institute.  All rights reserved.

"""
Implements the OpenCV build system for bazel.
"""

# How to upgrade the OpenCV version:
#
# 1. Get the desired release zip file, point WORKSPACE to it.
#
# 2. Update KNOWN_OPTIMIZATIONS if desired, from
#    OpenCVCompilerOptimizations.cmake The list here doesn't have to
#    be complete, but it does need to enumerate everything we might
#    possibly want to turn on at some point.
#
# 3. Run cmake on the opencv distro normally, examine the generated
#    cvconfig.h and apply any differences manually to the version here.
#
# 4. Update modules in opencv.BUILD with new source exclusions or
#    optimizations.  Typically, both of these will be discovered just
#    by trying to build and seeing what things don't compile or link.
#
# 5. If you question whether SIMD or OpenCL optimizations are working,
#    you can use //program/opencv_validation to verify through manual
#    inspection that they are being activated correctly.

KNOWN_OPTIMIZATIONS = [
    "SSE1",
    "SSE2",
    "SSE3",
    "SSE4_1",
    "SSE4_2",
    "AVX",
    "AVX2",
    "FP16",
]

# The following options are used to build the OpenCV libraries.  They
# should not be exported to consumer software.
OPENCV_COPTS = [
    "-D__OPENCV_BUILD",
    "-Wno-narrowing",
    "-Wno-unused-function",
    "-Wno-unused-but-set-variable",
    "-Wno-unused-variable",
    "-Wno-deprecated-declarations",
    "-Wno-class-memaccess",

    # We need optimization to always be on, or else things won't even
    # compile.
    "-O2",
]

# The debug symbols for opencv are enormous, and we don't really
# expect to be debugging it.  Thus, omit them for now.
NOCOPTS = ["^-g$$"]

# These headers are depended upon by many things.
LOCAL_HEADERS = [
    "cvconfig.h",
    "cv_cpu_config.h",
    "custom_hal.hpp",
]

def opencv_base(config, use_cuda = True):
    """Create the base rules for opencv, all of which are private.  Rules
    for individual modules, (as created by 'opencv_module'), will depend
    on these.

    @param opencv_config - a python dictionary with the following fields
       'opts' - a list of supported compiler optimizations from the set
                KNOWN_OPTIMIZATIONS

       'modules' - a list of known modules, this must have an entry
                   for every 'opencv_module' invocation
    """

    native.genrule(
        name = "modules",
        outs = ["include/opencv2/opencv_modules.hpp"],
        cmd = """cat > $@ <<EOF
""" + "".join([
                  "#define HAVE_OPENCV_{module}\n".format(module = module.upper())
                  for module in sorted(config["modules"])
              ]) +
              """
EOF
""",
    )

    # NOTE(josh.pieper): I haven't figure out what this is for yet,
    # but the sample builds I created all created an effectively empty
    # file.
    native.genrule(
        name = "custom_hal",
        outs = ["custom_hal.hpp"],
        cmd = "touch $@",
    )

    # NOTE: The OpenCV build system also has a concept of "dispatched"
    # CPU functions, which are compiled, and then disabled at runtime.
    # We do not bother supporting that, and instead only support the
    # "baseline"

    cv_cpu_config_content = """
#ifndef OPENCV_CV_CPU_CONFIG_H_INCLUDED
#define OPENCV_CV_CPU_CONFIG_H_INCLUDED

""" + "".join(["""
#define CV_CPU_COMPILE_{opt} 1
#define CV_CPU_BASELINE_COMPILE_{opt} 1
""".format(opt = opt.upper()) for opt in config["opts"]]) + """

#define CV_CPU_BASELINE_FEATURES 0 , {features}

#endif
""".format(features = " , ".join([
        "CV_CPU_" + opt.upper()
        for opt in config["opts"]
    ]))

    native.genrule(
        name = "cv_cpu_config",
        outs = ["cv_cpu_config.h"],
        cmd = "cat > $@ <<EOF\n" + cv_cpu_config_content + "\nEOF\n",
    )

    native.genrule(
        name = "version_string",
        outs = ["modules/core/version_string.inc"],
        cmd = """cat > $@ <<EOF
"TRI"
EOF
""",
    )

    native.genrule(
        name = "cvconfig",
        outs = ["cvconfig.h"],
        cmd = """cat > $@ <<EOF
#ifndef OPENCV_CVCONFIG_H_INCLUDED
#define OPENCV_CVCONFIG_H_INCLUDED

/* OpenCV compiled as static or dynamic libs */
#define BUILD_SHARED_LIBS

/* OpenCV intrinsics optimized code */
#define CV_ENABLE_INTRINSICS

/* OpenCV additional optimized code */
/* #undef CV_DISABLE_OPTIMIZATION */

/* Compile for 'real' NVIDIA GPU architectures */
#define CUDA_ARCH_BIN " 30 35 37 50 52 60 61"

/* Create PTX or BIN for 1.0 compute capability */
/* #undef CUDA_ARCH_BIN_OR_PTX_10 */

/* NVIDIA GPU features are used */
#define CUDA_ARCH_FEATURES " 30 35 37 50 52 60 61"

/* Compile for 'virtual' NVIDIA PTX architectures */
#define CUDA_ARCH_PTX ""

/* AVFoundation video libraries */
/* #undef HAVE_AVFOUNDATION */

/* V4L capturing support */
/* #undef HAVE_CAMV4L */

/* V4L2 capturing support */
#define HAVE_CAMV4L2

/* Carbon windowing environment */
/* #undef HAVE_CARBON */

/* AMD's Basic Linear Algebra Subprograms Library*/
/* #undef HAVE_CLAMDBLAS */

/* AMD's OpenCL Fast Fourier Transform Library*/
/* #undef HAVE_CLAMDFFT */

/* Clp support */
/* #undef HAVE_CLP */

/* Cocoa API */
/* #undef HAVE_COCOA */

/* C= */
/* #undef HAVE_CSTRIPES */

/* NVidia Cuda Basic Linear Algebra Subprograms (BLAS) API*/
#define HAVE_CUBLAS

/* NVidia Cuda Runtime API*/
{HAVE_CUDA}

/* NVidia Cuda Fast Fourier Transform (FFT) API*/
#define HAVE_CUFFT

/* IEEE1394 capturing support */
/* #undef HAVE_DC1394 */

/* IEEE1394 capturing support - libdc1394 v2.x */
/* #define HAVE_DC1394_2 */

/* DirectX */
/* #undef HAVE_DIRECTX */
/* #undef HAVE_DIRECTX_NV12 */
/* #undef HAVE_D3D11 */
/* #undef HAVE_D3D10 */
/* #undef HAVE_D3D9 */

/* DirectShow Video Capture library */
/* #undef HAVE_DSHOW */

/* Eigen Matrix & Linear Algebra Library */
#define HAVE_EIGEN

/* FFMpeg video library */
#define HAVE_FFMPEG

/* Geospatial Data Abstraction Library */
/* #undef HAVE_GDAL */

/* GStreamer multimedia framework */
/* #define HAVE_GSTREAMER */

/* GTK+ 2.0 Thread support */
#define HAVE_GTHREAD

/* GTK+ 2.x toolkit */
#define HAVE_GTK

/* Halide support */
/* #undef HAVE_HALIDE */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Intel Perceptual Computing SDK library */
/* #undef HAVE_INTELPERC */

/* Intel Integrated Performance Primitives */
/* #undef HAVE_IPP */
/* #undef HAVE_IPP_ICV */
/* #undef HAVE_IPP_IW */

/* Intel IPP Async */
/* #undef HAVE_IPP_A */

/* JPEG-2000 codec */
/* #undef HAVE_JASPER */

/* IJG JPEG codec */
#define HAVE_JPEG

/* libpng/png.h needs to be included */
#define HAVE_LIBPNG_PNG_H

/* GDCM DICOM codec */
/* #undef HAVE_GDCM */

/* V4L/V4L2 capturing support via libv4l */
/* #undef HAVE_LIBV4L */

/* Microsoft Media Foundation Capture library */
/* #undef HAVE_MSMF */

/* NVidia Video Decoding API*/
/* #undef HAVE_NVCUVID */

/* NVidia Video Encoding API*/
/* #undef HAVE_NVCUVENC */

/* OpenCL Support */
{HAVE_OPENCL}
/* #undef HAVE_OPENCL_STATIC */
/* #undef HAVE_OPENCL_SVM */

/* OpenEXR codec */
/* #undef HAVE_OPENEXR */

/* OpenGL support*/
/* #define HAVE_OPENGL */

/* OpenNI library */
/* #undef HAVE_OPENNI */

/* OpenNI library */
/* #undef HAVE_OPENNI2 */

/* PNG codec */
#define HAVE_PNG

/* Posix threads (pthreads) */
#define HAVE_PTHREAD

/* parallel_for with pthreads */
#define HAVE_PTHREADS_PF

/* Qt support */
/* #undef HAVE_QT */

/* Qt OpenGL support */
/* #undef HAVE_QT_OPENGL */

/* QuickTime video libraries */
/* #undef HAVE_QUICKTIME */

/* QTKit video libraries */
/* #undef HAVE_QTKIT */

/* Intel Threading Building Blocks */
/* #undef HAVE_TBB */

/* TIFF codec */
#define HAVE_TIFF

/* Unicap video capture library */
/* #undef HAVE_UNICAP */

/* Video for Windows support */
/* #undef HAVE_VFW */

/* V4L2 capturing support in videoio.h */
/* #undef HAVE_VIDEOIO */

/* Win32 UI */
/* #undef HAVE_WIN32UI */

/* XIMEA camera support */
/* #undef HAVE_XIMEA */

/* Xine video library */
/* #undef HAVE_XINE */

/* Define if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* gPhoto2 library */
/* #undef HAVE_GPHOTO2 */

/* VA library (libva) */
/* #undef HAVE_VA */

/* Intel VA-API/OpenCL */
/* #undef HAVE_VA_INTEL */

/* Intel Media SDK */
/* #undef HAVE_MFX */

/* Lapack */
/* #undef HAVE_LAPACK */

/* Library was compiled with functions instrumentation */
/* #undef ENABLE_INSTRUMENTATION */

/* OpenVX */
/* #undef HAVE_OPENVX */

#if defined(HAVE_XINE)         || \
    defined(HAVE_GSTREAMER)    || \
    defined(HAVE_QUICKTIME)    || \
    defined(HAVE_QTKIT)        || \
    defined(HAVE_AVFOUNDATION) || \
    /*defined(HAVE_OPENNI)     || too specialized */ \
    defined(HAVE_FFMPEG)       || \
    defined(HAVE_MSMF)
#define HAVE_VIDEO_INPUT
#endif

#if /*defined(HAVE_XINE)       || */\
    defined(HAVE_GSTREAMER)    || \
    defined(HAVE_QUICKTIME)    || \
    defined(HAVE_QTKIT)        || \
    defined(HAVE_AVFOUNDATION) || \
    defined(HAVE_FFMPEG)       || \
    defined(HAVE_MSMF)
#define HAVE_VIDEO_OUTPUT
#endif

/* OpenCV trace utilities */
#define OPENCV_TRACE


#endif // OPENCV_CVCONFIG_H_INCLUDED
EOF
""".format(
            HAVE_CUDA = ("#define HAVE_CUDA" if use_cuda else ""),
            HAVE_OPENCL = ("#define HAVE_OPENCL" if use_cuda else ""),
        ),
    )

    native.cc_library(
        name = "headers",
        hdrs = [
            "include/opencv2/opencv_modules.hpp",
        ] + native.glob(["include/**/*.h", "include/**/*.hpp"]),
        strip_include_prefix = "include",
        deps = ["@eigen", "@tbb"] + (["@cuda"] if use_cuda else []),
        visibility = ["//visibility:public"],
    )

    # These headers are not exposed to external users, but are used
    # within the build process.
    native.cc_library(
        name = "internal_headers",
        hdrs = LOCAL_HEADERS,
    )

def opencv_module(
        name,
        config,
        simd_sources = None,
        opencl = False,
        cuda = False,
        srcs_excludes = None,
        **kwargs):
    """Declare an OpenCV module with @p name.

    @param config has the same semantics as for 'opencv_base'

    @param simd_sources, if present, is a dictionary, mapping source
    files to a list of available optimized versions.

    @param opencl, if True, expands all opencl sources in this module

    @param cuda, if True, builds all CUDA sources in this module
    """

    if name not in config["modules"]:
        fail("'{name}' not in config['modules']".format(name = name))

    enabled_optimizations = config["opts"]

    # A dictionary mapping optimization types, to lists of source
    # files that need to be included for that optimization level.
    optimized_sources = {}

    # The set of output files from any enabled SIMD sources.
    simd_outs = []

    module_copts = []
    if "copts" in kwargs:
        module_copts += kwargs.pop("copts")

    module_deps = []
    if "deps" in kwargs:
        module_deps += kwargs.pop("deps")

    native.cc_library(
        name = "{name}_deps".format(name = name),
        deps = module_deps,
    )

    for simd_source in (simd_sources or {}).keys():
        declaration_file = "modules/{name}/{source}.simd_declarations.hpp".format(
            name = name,
            source = simd_source,
        )

        simd_outs.append(declaration_file)

        available_optimizations = simd_sources[simd_source]
        this_file_optimizations = depset(transitive = [
            depset(enabled_optimizations),
            depset(available_optimizations),
        ]).to_list()

        cpp_outs = []
        for opt in this_file_optimizations:
            cpp_file = "modules/{name}/{simd_source}.{opt}.cpp".format(
                name = name,
                simd_source = simd_source,
                opt = opt.lower(),
            )
            cpp_outs.append(cpp_file)
            optimized_sources.setdefault(opt, []).append(cpp_file)

        native.genrule(
            name = "opencv_{name}_{simd_source}_simd".format(
                name = name,
                simd_source = simd_source,
            ),
            outs = [declaration_file] + cpp_outs,
            tools = ["//:opencv_optimizations"],
            cmd = ("$(location //:opencv_optimizations) " +
                   "--outdir $(location {declaration}) ".format(declaration = declaration_file) +
                   "--filename {simd_source} ".format(simd_source = simd_source) +
                   "".join([opt.upper() for opt in this_file_optimizations])),
        )

    # The set of output files from any enabled opencl directories.
    opencl_outs = []

    # The set of runtime files needed by opencv for this module.
    opencl_runtime = []

    if opencl:
        opencl_outs = [
            "modules/{name}/opencl_kernels_{name}.cpp".format(name = name),
            "modules/{name}/opencl_kernels_{name}.hpp".format(name = name),
        ]

        native.genrule(
            name = "opencl_kernels_{name}".format(name = name),
            outs = opencl_outs,
            srcs = native.glob(["modules/{name}/src/opencl/*.cl".format(name = name)]),
            tools = ["//:opencv_clmake"],
            cmd = ("$(location //:opencv_clmake) " +
                   "--source $(location {}) ".format(opencl_outs[0]) +
                   "--header $(location {}) ".format(opencl_outs[1]) +
                   "--module {name} ".format(name = name) +
                   "$(SRCS)"),
        )

        opencl_runtime = native.glob(
            [
                "modules/{name}/src/opencl/runtime/**/*.cpp".format(name = name),
                "modules/{name}/src/opencl/runtime/**/*.hpp".format(name = name),
            ],
        )

    native.cc_library(
        name = "{name}_headers".format(name = name),
        deps = [
            ":headers",
            ":internal_headers",
            ":{name}_deps".format(name = name),
        ],
        hdrs = native.glob([
            "modules/{name}/include/**/*.hpp".format(name = name),
            "modules/{name}/include/**/*.h".format(name = name),
        ]),
        strip_include_prefix = "modules/{name}/include".format(name = name),
        visibility = ["//visibility:public"],
        textual_hdrs = (kwargs.pop("textual_hdrs") if "textual_hdrs" in kwargs else []),
    )

    cuda_deps = []
    if cuda:
        cu_files = native.glob(["modules/{name}/src/cuda/*.cu".format(name = name)])
        cu_cc_files = [x + ".cc" for x in cu_files]

        # GRRR. Our current bazel setup requires that the CUDA files
        # end in '.cc', thus we rename them.
        native.genrule(
            name = "{name}_cuda_rename".format(name = name),
            outs = cu_cc_files,
            srcs = cu_files,
            cmd = "for a in $(SRCS); do cp $$a $(GENDIR)/$$a.cc; done",
        )

        native.cc_library(
            name = "{name}_cuda".format(name = name),
            srcs = native.glob([
                "modules/{name}/src/cuda/*.hpp".format(name = name),
                "modules/{name}/src/cuda/*.h".format(name = name),
                "modules/{name}/src/*.hpp".format(name = name),
                "modules/{name}/src/*.h".format(name = name),
            ]) + cu_cc_files,
            copts = [
                "-x",
                "cuda",
                "-Iexternal/opencv/modules/{name}/src/cuda".format(name = name),
                "-Iexternal/opencv/modules/{name}/src".format(name = name),
            ] + OPENCV_COPTS + module_copts,
            features = NOCOPTS,
            deps = [
                ":{name}_headers".format(name = name),
            ] + module_deps,
        )

        cuda_deps.append("{name}_cuda".format(name = name))

    for opt in optimized_sources.keys():
        native.cc_library(
            name = "{name}_opt_{opt}".format(name = name, opt = opt.lower()),
            deps = [":{name}_headers".format(name = name)],
            srcs = (LOCAL_HEADERS +
                    native.glob([
                        "modules/{name}/src/*.hpp".format(name = name),
                        "modules/{name}/src/*.h".format(name = name),
                    ]) +
                    optimized_sources[opt]),
            copts = [
                "-DCV_CPU_DISPATCH_MODE={opt}".format(opt = opt.upper()),
                "-Iexternal/opencv/modules/{name}/src".format(name = name),
            ] + OPENCV_COPTS + module_copts,
            features = NOCOPTS,
        )

    native.cc_binary(
        name = "libopencv_{name}.so".format(name = name),
        deps = [
            "{name}_headers".format(name = name),
        ] + [
            "{name}_opt_{opt}".format(name = name, opt = opt.lower())
            for opt in optimized_sources.keys()
        ] + cuda_deps,
        srcs = (native.glob(
            [
                "modules/{name}/src/*.cpp".format(name = name),
                "modules/{name}/src/*.hpp".format(name = name),
                "modules/{name}/src/*.h".format(name = name),
                # TODO(jeremy.nimmer) Parameterize the KAZE special case, instead
                # of hard-coding it.
                "modules/{name}/src/kaze/*.h".format(name = name),
                "modules/{name}/src/kaze/*.cpp".format(name = name),
                "modules/{name}/src/utils/*.hpp".format(name = name),
                "modules/{name}/src/utils/*.cpp".format(name = name),
            ],
            exclude = [
                "modules/{name}/src/*.{opt}.cpp".format(
                    name = name,
                    opt = opt.lower(),
                )
                for opt in KNOWN_OPTIMIZATIONS
            ] + (srcs_excludes or []),
        ) + opencl_runtime + LOCAL_HEADERS + opencl_outs + simd_outs),
        copts = [
            "-I$(GENDIR)/external/opencv/modules/{name}".format(name = name),
            "-Iexternal/opencv/modules/{name}/src".format(name = name),
        ] + OPENCV_COPTS + module_copts,
        features = NOCOPTS,
        linkshared = 1,
        visibility = ["//visibility:public"],
        **kwargs
    )

    native.cc_library(
        name = name,
        srcs = [":libopencv_{name}.so".format(name = name)],
        deps = [":{name}_headers".format(name = name)],
        linkopts = ["-lpthread", "-lz"],
        visibility = ["//visibility:public"],
    )

def _flatten(value):
    return [
        inner
        for outer in value
        for inner in outer
    ]

def opencv_python(name, config, modules, runtime_deps_modules):
    """Create the opencv python bindings.

    @param config has the same semantics as for 'opencv_base'

    @param modules list of modules to include in python bindings
    """

    # This function is roughly cribbed from
    # modules/python/bindings/CMakeLists.txt

    blacklist = [
        "modules/**/*.h",
        "modules/core/**/cuda/**",
        "modules/cuda*/**",
        "modules/cudev/**",
        "modules/core/**/hal/*",
        "modules/**/utils/*",
        "modules/**/.inl.h*",
        "modules/**/*_inl.h*",
        "modules/**/*.details.h*",
        "modules/**/detection_based_tracker.hpp",
        "modules/**/opencl/**",
        "modules/**/openvx/**",
        "modules/**/ovx.hpp",
    ]

    opencv_hdrs = _flatten(
        [
            native.glob(
                ["modules/" + module + "/include/**/*.hpp"],
                exclude = blacklist,
            )
            for module in modules
        ],
    )

    opencv_userdef_hdrs = _flatten(
        [
            native.glob(["modules/" + module + "/misc/python/pyopencv*.hpp"])
            for module in modules
        ],
    )

    native.genrule(
        name = "python_gen_headers",
        outs = ["headers.txt"],
        cmd = """cat > $@ <<EOF
""" + "\n".join(opencv_hdrs) +
              """
EOF
""",
    )

    cv2_generated_hdrs = [
        "pyopencv_generated_include.h",
        "pyopencv_generated_funcs.h",
        "pyopencv_generated_types.h",
        "pyopencv_generated_type_reg.h",
        "pyopencv_generated_type_publish.h",
        "pyopencv_generated_ns_reg.h",
    ]

    cv2_generated_files = cv2_generated_hdrs + [
        "pyopencv_signatures.json",
    ]

    native.genrule(
        name = "python_gen",
        outs = cv2_generated_files,
        srcs = [
            ":python_gen_headers",
        ] + opencv_hdrs,
        tools = [
            "modules/python/src2/gen2.py",
            "modules/python/src2/hdr_parser.py",
        ],
        cmd = "cd external/opencv && /usr/bin/python3 ../../$(location :modules/python/src2/gen2.py) ../../$(GENDIR)/external/opencv ../../$(location :python_gen_headers)",
    )

    native.genrule(
        name = "python_custom_headers",
        outs = ["pyopencv_custom_headers.h"],
        cmd = """cat > $@ <<EOF
""" + "\n".join([
            '#include "{}"'.format(x)
            for x in opencv_userdef_hdrs
        ]) + """
EOF
""",
    )

    # Compile the C++ sources into a shared library python module.  Note that
    # upstream would call this library "cv2.so" so that it could be directly
    # imported.  However, we give it a different name so that the "install
    # everything into once place" trick below will work -- we reserve cv2 as
    # the module (folder) name, not the module (shared library) name.
    native.cc_binary(
        name = "libcv2.so",
        srcs = [
            "modules/python/src2/cv2.cpp",
            "modules/python/src2/pycompat.hpp",
            "pyopencv_custom_headers.h",
        ] + cv2_generated_hdrs + opencv_userdef_hdrs,
        copts = OPENCV_COPTS,
        features = NOCOPTS,
        deps = [":" + module for module in modules] + [
            "@numpy",
            "@python",
        ],
        linkopts = ["-Wl,-rpath=$$ORIGIN"],
        linkshared = 1,
    )

    # Assemble the all of the python pieces into once place.  This is the only
    # robust way to placate both the effective PYTHONPATH (sys.path) and
    # effective LD_LIBRARY_PATH (the loader paths searched while loading shared
    # libraries).  By copying everything we need using genrules, we are assured
    # that all relevant files are in the same folder tree (not genfiles vs out,
    # for example), will only ever be found in one place, and are all located
    # at "." relative to each other.
    shlibs = []
    for libname in modules + runtime_deps_modules:
        shlib = "libopencv_{}.so".format(libname)
        shlibs.append(shlib)
        native.genrule(
            name = "install_{}_genrule".format(shlib),
            srcs = [shlib],
            outs = ["install/{}".format(shlib)],
            cmd = "cp $< $@",
        )
    shlibs.append("cv2.so")
    native.genrule(
        name = "install_libcv2_genrule",
        srcs = ["libcv2.so"],
        outs = ["install/cv2.so"],
        cmd = "cp $< $@",
    )

    # Provide an importable library based on the genfiles copies.
    native.py_library(
        name = name,
        data = [
            "install/{}".format(x)
            for x in shlibs
        ],
        imports = ["install"],
        visibility = ["//visibility:public"],
    )

def opencv_all(name, config):
    native.cc_library(
        name = name,
        deps = [":" + x for x in config["modules"]],
        visibility = ["//visibility:public"],
    )

    native.filegroup(
        name = "{name}_so_data".format(name = name),
        srcs = [":libopencv_{}.so".format(x) for x in config["modules"]],
        visibility = ["//visibility:public"],
    )
