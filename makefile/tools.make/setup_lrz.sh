# Load modules
module purge
#module load admin/1.0 lrz/default intel/17.0 mkl/2017
#module load gsl/2.3 blast gcc/4.9

module load admin           #obligatroy
module load lrz tempdir     #obligatory
module load spack           #requirement for many libs
        
module load intel-parallel-studio   #requires spack
#"intel-parallel-studio: using intel wrappers for mpicc, mpif77, etc"
#Intel paralell studio includes: gcc, fortran compiler, 

#gawk and libtool is allready existent at LRZ
module load texlive

#module load tbb #throws error on making
module load hypre
module load gsl


# Set up paths libraries
LIB_BASE="_USER_LIB_BASE_"

export LD_LIBRARY_PATH=/lrz/sys/libraries/gsl/2.4/lib/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/lrz/sys/libraries/hypre/2.11.2/lib/:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=/lrz/sys/intel/studio2018_p4/compilers_and_libraries_2018.5.274/linux/tbb/lib/intel64/gcc4.1:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LIB_BASE/tbb40_20120613oss/build/linux_intel64_gcc_cc4.6.1_libc2.5_kernel2.6.18_release/:$LD_LIBRARY_PATH


#we use VTK 5 because there were API changes which were not updated in our MRAG imlementation
#https://vtk.org/Wiki/VTK/VTK_6_Migration/Replacement_of_SetInput
#export LD_LIBRARY_PATH=/lrz/sys/spack/release/20.1/opt/haswell/vtk/8.1.2-gcc-xp3nkvw/include/vtk-8.1:$LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=/lrz/sys/tools/proot/images/centos/Freesurfer/freesurfer/lib/vtk/include/vtk-5.6/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$LIB_BASE/myVTK/lib/vtk-5.4/:$LD_LIBRARY_PATH

#vtk?
#/lrz/sys/applications/OpenFOAM/OpenFOAM-v1812+.impi.gcc/ThirdParty-v1812/ParaView-v5.6.0/VTK/Common/Core/
#/lrz/sys/graphics/paraview/5.5.0/paraview_gl/include/paraview-5.5
#/lrz/sys/tools/proot/images/centos/Freesurfer/freesurfer/lib/vtk/include/vtk-5.6/
#/lrz/sys/applications/OpenFOAM/OpenFOAM-6.0.impi.intel/ThirdParty-6/ParaView-5.8.0/VTK/Common/Core/
#/lrz/sys/spack/.tmp/test/mayavi/envs/x86_avx512/mayavi04/.spack-env/view/include/vtk-8.1/
#/lrz/sys/spack/release/20.1/opt/haswell/paraview/5.6.2-gcc-u6argwl/include/paraview-5.6/vtkPoints.h
#/lrz/sys/spack/release/20.1/opt/haswell/vtk/8.1.2-gcc-xp3nkvw/include/vtk-8.1/vtkPoints.h
#/lrz/sys/spack/release/19.1.1/opt/x86_avx2/paraview/5.4.1-gcc-jian24h/include/paraview-5.4/vtkPoints.h
#/lrz/sys/spack/release/19.1.1/opt/x86_avx2/vtk/8.0.1-gcc-u7phuwh/include/vtk-8.0/vtkPoints.h


#needed on LRZ for MAC login discompatiblity
export LANG=C
export LC_ALL=C



