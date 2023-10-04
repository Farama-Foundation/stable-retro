import os
import subprocess
import sys
from distutils.spawn import find_executable

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

VERSION_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "retro/VERSION.txt",
)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
README = open(os.path.join(SCRIPT_DIR, "README.md")).read()


class CMakeBuild(build_ext):
    def run(self):
        suffix = super().get_ext_filename("")
        pyext_suffix = f"-DPYEXT_SUFFIX:STRING={suffix}"
        pylib_dir = ""
        if not self.inplace:
            pylib_dir = f"-DPYLIB_DIRECTORY:PATH={self.build_lib}"
        if self.debug:
            build_type = "-DCMAKE_BUILD_TYPE=Debug"
        else:
            build_type = ""
        python_executable = f"-DPYTHON_EXECUTABLE:STRING={sys.executable}"
        cmake_exe = find_executable("cmake")
        if not cmake_exe:
            try:
                import cmake
            except ImportError:
                subprocess.check_call([sys.executable, "-m", "pip", "install", "cmake"])
                import cmake
            cmake_exe = os.path.join(cmake.CMAKE_BIN_DIR, "cmake")
        subprocess.check_call(
            [
                cmake_exe,
                ".",
                "-G",
                "Unix Makefiles",
                build_type,
                pyext_suffix,
                pylib_dir,
                python_executable,
            ],
        )
        if self.parallel:
            jobs = f"-j{self.parallel:d}"
        else:
            import multiprocessing

            jobs = f"-j{multiprocessing.cpu_count():d}"
        make_exe = find_executable("make")
        if not make_exe:
            raise RuntimeError("Could not find Make executable. Is it installed?")
        subprocess.check_call([make_exe, jobs, "retro"])


platform_globs = [
    "*-%s/*" % plat
    for plat in [
        "Nes",
        "Snes",
        "Genesis",
        "Atari2600",
        "GameBoy",
        "Sms",
        "GameGear",
        "PCEngine",
        "GbColor",
        "GbAdvance",
        "32x",
        "Saturn",
    ]
]


setup(
    name="stable-retro",
    long_description=README,
    long_description_content_type="text/markdown",
    author="Farama Foundation",
    author_email="contact@farama.org",
    url="https://github.com/farama-foundation/stable-retro",
    version=open(VERSION_PATH).read().strip(),
    license="MIT",
    install_requires=["gymnasium>=0.27.1", "pyglet>=1.3.2,==1.*"],
    python_requires=">=3.6.0,<3.11",
    ext_modules=[Extension("retro._retro", ["CMakeLists.txt", "src/*.cpp"])],
    cmdclass={"build_ext": CMakeBuild},
    packages=[
        "retro",
        "retro.data",
        "retro.data.stable",
        "retro.data.experimental",
        "retro.data.contrib",
        "retro.scripts",
        "retro.import",
        "retro.examples",
    ],
    package_data={
        "retro": [
            "cores/*.json",
            "cores/*_libretro*",
            "VERSION.txt",
            "README.md",
            "LICENSES.md",
        ],
        "retro.data.stable": platform_globs,
        "retro.data.experimental": platform_globs,
        "retro.data.contrib": platform_globs,
    },
)
