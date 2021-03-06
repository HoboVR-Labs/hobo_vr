# SPDX-License-Identifier: GPL-2.0-only

# Copyright (C) 2020-2021 Oleg Vorobiov <oleg.vorobiov@hobovrlabs.org>
# Copyright (C) 2020-2021 Josh Miklos <josh.miklos@hobovrlabs.org>

from setuptools import setup, find_packages
from virtualreality import __version__

with open("pypi_readme.md", "r") as f:
    long_description = f.read()

setup(
    name="virtualreality",
    version=__version__,
    author="Okawo <okawo.198@gmail.com>, SimLeek <simulator.leek@gmail.com>",
    author_email="",
    description="python side of hobo_vr",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/okawo80085/hobo_vr",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Education",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: Healthcare Industry",
        "Intended Audience :: Information Technology",
        "Intended Audience :: Manufacturing",
        "Intended Audience :: Science/Research",
        "Intended Audience :: Telecommunications Industry",
        "License :: OSI Approved :: GNU General Public License v2 (GPLv2)",
        "Operating System :: OS Independent",
        "Programming Language :: C++",
        "Programming Language :: Python :: 3.7",
        "Topic :: Games/Entertainment",
        "Topic :: Multimedia :: Graphics :: Viewers",
        "Topic :: Scientific/Engineering :: Human Machine Interfaces",
        "Topic :: System :: Hardware :: Hardware Drivers",
    ],
    install_requires=[
        "numpy",
        "docopt",
        "aioconsole",
    ],
    extras_require={},
    entry_points={},
    python_requires=">=3.7",
)
