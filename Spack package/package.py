# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

# ----------------------------------------------------------------------------
# If you submit this package back to Spack as a pull request,
# please first remove this boilerplate and all FIXME comments.
#
# This is a template package file for Spack.  We've put "FIXME"
# next to all the things you'll want to change. Once you've handled
# them, you can save this file and test your package like this:
#
#     spack install mpipcl
#
# You can edit this file again by typing:
#
#     spack edit mpipcl
#
# See the Spack documentation for more information on packaging.
# ----------------------------------------------------------------------------

from spack_repo.builtin.build_systems.cmake import CMakePackage
from spack.package import *


class Mpipcl(CMakePackage):
    """FIXME: Put a proper description of your package here."""

    # FIXME: Add a proper url for your package's homepage here.
    homepage = "https://www.example.com"
    url = "https://github.com/mpi-advance/MPIPCL/archive/refs/tags/v1.3.0.tar.gz"
    git = "https://github.com/mpi-advance/MPIPCL.git"

    # FIXME: Add a list of GitHub accounts to
    # notify when the package is updated.
    # maintainers("github_user1", "github_user2")

    # FIXME: Add the SPDX identifier of the project's license below.
    # See https://spdx.org/licenses/ for a list. Upon manually verifying
    # the license, set checked_by to your Github username.
    license("UNKNOWN", checked_by="github_user1")

    version("testing", commit="b6dce22")
    version("1.3.0", sha256="ad65f5b87e57530927d1607fd7856f8d29bc34d4198490d34a70edefcd16f02a")
    version("1.2.0", sha256="6d28fdb452ed75a6c5623ac684aed39a71592524afca5804b39102d7a46bb6b8")
    version("1.1.2", sha256="37e842b0a1733df7e88df47cfbe95064077befbcc8bdfeb9fc0a9014c6e38a47")

    depends_on("c", type="build")

    # FIXME: Add dependencies if required.
    # depends_on("foo")
    depends_on("c", type="build")

    depends_on("mpi")
    depends_on("cmake @3.17:")

    variant("static_libs", default=False, description="Build static MPIPCL library")     
    variant("debug", default=False, description="Turn on debug statments inside library")
    variant("examples", default=False, description="Build Example programs")
    variant("Unique_names", default=False, description="Changes the types and names of functions to MPIP instead of MPIX")       

    def cmake_args(self):
        # FIXME: Add arguments other than
        # FIXME: CMAKE_INSTALL_PREFIX and CMAKE_BUILD_TYPE
        # FIXME: If not needed delete this function
        args = []
        if self.spec.satisfies("+static_libs"):
            args.append("-DDYNAMIC_LIBS=OFF")
        if self.spec.satisfies("+debug"):
            args.append("-DCMAKE_BUILD_TYPE=DEBUG") 
        if self.spec.satisfies("+examples"):
            args.append("-DBUILD_EXAMPLES=ON")
            args.append("-DEXAMPLES_TO_BIN=ON")
        return args
