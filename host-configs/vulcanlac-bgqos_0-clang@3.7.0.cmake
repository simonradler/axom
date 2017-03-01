##################################
# uberenv host-config
#
# This is a generated file, edit at own risk.
##################################
# bgqos_0-clang@3.7.0
##################################

# cmake from uberenv
# cmake executable path: /usr/global/tools/CMake/bgqos_0/cmake-3.1.2/bin/cmake

#######
# using clang@3.7.0 compiler spec
#######

# c compiler used by spack
set("CMAKE_C_COMPILER" "/usr/local/bin/bgclang" CACHE PATH "")

# cpp compiler used by spack
set("CMAKE_CXX_COMPILER" "/usr/local/bin/bgclang++" CACHE PATH "")

# fortran compiler used by spack
# no fortran compiler

set("ENABLE_FORTRAN" "OFF" CACHE PATH "")

# hdf5 from uberenv
set("HDF5_DIR" "/usr/workspace/wsa/toolkit/thirdparty_libs/builds/2017_03_01_13_40_58/spack/opt/spack/bgqos_0/clang-3.7.0/hdf5-1.8.16-bosaqxj3xd5fhyovqnda3rgj2kjsj4ah" CACHE PATH "")

# conduit from uberenv
set("CONDUIT_DIR" "/usr/workspace/wsa/toolkit/thirdparty_libs/builds/2017_03_01_13_40_58/spack/opt/spack/bgqos_0/clang-3.7.0/conduit-0.2.1-oiiieme5mlcpao7pqwrk2mdquxnaguqm" CACHE PATH "")

# sparsehash headers from uberenv
set("SPARSEHASH_DIR" "/usr/workspace/wsa/toolkit/thirdparty_libs/builds/2017_03_01_13_40_58/spack/opt/spack/bgqos_0/clang-3.7.0/sparsehash-headers-2.0.2-jnxnoo3nsy2l6vutmvlvwri4tzgbqmro" CACHE PATH "")

# boost headers from uberenv
set("BOOST_DIR" "/usr/workspace/wsa/toolkit/thirdparty_libs/builds/2017_03_01_13_40_58/spack/opt/spack/bgqos_0/clang-3.7.0/boost-headers-1.58.0-qddl3bajxtossmhy4mazvjpah4zgx5aj" CACHE PATH "")

# python not build by uberenv

# lua not build by uberenv

# doxygen not built by uberenv

# sphinx not built by uberenv

# uncrustify not built by uberenv

# lcov and genhtml from uberenv
# lcov and genhtml not built by uberenv

##################################
# end uberenv host-config
##################################

##############################################################################
# !---------------------------------------------------------------------------
##############################################################################
# Options added manually to 
# lc bgq clang@3.7.0 host configs
##############################################################################

##############################################################################
# MPI - manually added for now
##############################################################################
set(ENABLE_MPI ON CACHE PATH "")
set(MPI_C_COMPILER "/usr/apps/gnu/clang/llnl/bin/mpiclang" CACHE PATH "")
set(MPI_CXX_COMPILER "/usr/apps/gnu/clang/llnl/bin/mpiclang++11-fastmpi" CACHE PATH "")
set(MPI_Fortran_COMPILER  "/usr/local/tools/compilers/ibm/mpif90-4.8.4-fastmpi" CACHE PATH "")

set(MPIEXEC "/usr/bin/srun" CACHE PATH "")
set(MPIEXEC_NUMPROC_FLAG "-n" CACHE PATH "")

##############################################################################
# !---------------------------------------------------------------------------
##############################################################################

