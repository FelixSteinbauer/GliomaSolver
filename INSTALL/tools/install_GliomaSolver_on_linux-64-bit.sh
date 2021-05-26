#!/bin/bash

echo "==============================================="
echo "  Installing libraries for the GliomaSolver    "
echo "==============================================="

InstallDir=$(pwd)                       # ".../GliomaSolver/INSTALL"
SolverDir=$(dirname "${InstallDir}")    # ".../GliomaSolver"

echo "Shall this installer also install the libraries for the Bayesian Inference?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) BI="yes"; break;;         #if this variable is set to "yes" the respective libraries for Bayesian Inference will be installed
        No ) BI="no"; break;;
    esac
done

echo "Are we on the LRZ-Cluster?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) LRZ="yes"; break;;         #if this variable is set to "yes" this installer will not have root privileges
        No ) LRZ="no"; break;;
    esac
done


echo "--------------------------------------"
echo ">>> Getting prerequsites and external programms   <<<"
echo "--------------------------------------"



if [ $BI = "yes" ]          # only for Bayesian Inference
then

    if [ $LRZ = "yes" ]
    then

        module purge
        module load admin           #obligatroy
        module load lrz tempdir     #obligatory
        module load spack           #requirement for many libs
        
        module load intel-parallel-studio   #requires spack
        #"intel-parallel-studio: using intel wrappers for mpicc, mpif77, etc"
        #Intel paralell studio includes: gcc, fortran compiler, 

        #gawk and libtool is allready existent at LRZ
        module load texlive         #requires spack
        #pandoc is not available on the LRZ (TODO: workaround)


        #Alternativ to intel-parallel-studio:
        #module load gcc/8           #gcc version can be changed here
        #module load mpi.intel/2018_gcc
        #module load caf/gfortran    #requires gcc and mpi.intel       

    else
        sudo apt install gcc gfortran gawk libtool texlive-full pandoc pandoc-citeproc  
    fi

    #Explaination:    
    #gcc                                    # general prerequsites for building anything
    #gfortran                               # needed for mpich
    #gawk                                   # reliable math calculations with gawk (normal awk is shit)
    #libtool                                # needed for gsl
    #pandoc pandoc-citeproc texlive-full    # converting our results.md into a pdf (not available on LRZ)


else
    if [ $LRZ = "yes" ]
    then

        module purge
        module load lrz tempdir     #obligatory
        module load spack           #requirement for many libs
        
        module load intel-parallel-studio   #requires spack
        #"intel-parallel-studio: using intel wrappers for mpicc, mpif77, etc"
        #Intel paralell studio includes: gcc, fortran compiler, 

    else
        sudo apt install gcc gfortran gawk
    fi
fi

echo "--------------------------------------"
echo ">>> Downloading externa libraries   <<<"
echo "--------------------------------------"

cd "${SolverDir}"

#wget tdo.sk/~janka/GliomaSolverHome/libs/lib-linux-64-bit/lib.tgz #TODO: update janas package
wget https://syncandshare.lrz.de/dl/fi6xQsW6vgKxt7DEoQZzjnuS/lib.tgz
tar -zxf lib.tgz
rm lib.tgz
#contains: hypre-2.10.0b.tgz myVTK.tgz tbb40_20120613oss.tgz

if [ $LRZ != "yes" ]    #LRZ modules cover the content of this package
then
    wget tdo.sk/~janka/GliomaSolverHome/libs/lib-linux-64-bit/inference_libs.tgz
    tar -zxf inference_libs.tgz
    mv inference_libs/* lib/ && rm -r inference_libs
    rm inference_libs.tgz
    #contains: gsl-src.tgz  mpich-3.2.1-src.tgz
fi

echo " "
echo "--------------------------------------"
echo ">>> Installing libraries   <<<"
echo "--------------------------------------"

cd lib
LIB_BASE=$(pwd)                         # ".../GliomaSolver/lib"


echo "--------------------------------------"
echo " Unpacking vtk:"
echo "--------------------------------------"

#we use VTK 5 even on the LRZ because there were API changes which were not updated in our MRAG implementation
#https://vtk.org/Wiki/VTK/VTK_6_Migration/Replacement_of_SetInput

vtk=myVTK
tar -zxf ${vtk}.tgz
rm ${vtk}.tgz

if [ $LRZ = "yes" ]     #LRZ compatible installation
    then

    #mpicc is available through intel-paralell-studio
    module load tbb
    module load hypre

    if [ $BI = "yes" ]          # only for Bayesian Inference 
    then
        module load gsl
        #pi4u_lite (torc_lite, engine_tmcmc) has till to be used from a external source

    fi

else        #default linux installation

    echo "--------------------------------------"
    echo " Installing mpich:"
    echo "--------------------------------------"

    mpich_src=mpich-3.2.1-src   #archive name
    tar -zxf ${mpich_src}.tgz
    mkdir -p mpich-install

    cd "${mpich_src}"
    make clean
    ./configure --prefix="${LIB_BASE}"/mpich-install
    make
    make install
    export PATH="${LIB_BASE}"/mpich-install/bin:$PATH

    echo "---------------"
    echo "mpicc is set to"
    which mpicc
    echo "---------------"
    cd "${LIB_BASE}"
    rm ${mpich_src}.tgz
    rm -r ${mpich_src}
fi


echo "--------------------------------------"
echo " Installing tbb:"
echo "--------------------------------------"
#module load tbb #throws error on making that why also on LRZ the prepacked version is used

tbb=tbb40_20120613oss   #archive name
tar -zxf ${tbb}.tgz
rm ${tbb}.tgz

cd "${tbb}"
make clean
make
cd "${LIB_BASE}"


if [ $LRZ != "yes" ]
then


    echo "--------------------------------------"
    echo " Installing Hypre:"
    echo "--------------------------------------"

    hypre=hypre-2.10.0b     #archive name
    tar -zxf ${hypre}.tgz
    rm ${hypre}.tgz

    cd "${hypre}/src"
    make clean
    ./configure
    make
    make install
    cd "${LIB_BASE}"
fi

if [ $BI = "yes" ]          # only for Bayesian Inference 
then

    if [ $LRZ != "yes" ]    #default installation
    then
        echo "--------------------------------------"
        echo " Installing gsl:"
        echo "--------------------------------------"

        gsl_src=gsl-src
        tar -zxf ${gsl_src}.tgz
        mkdir -p gsl-install

        cd "${gsl_src}"
        ./autogen.sh
        ./configure --enable-maintainer-mode --disable-dynamic --prefix="${LIB_BASE}"/gsl-install
        make
        make install
        cd "${LIB_BASE}"
        rm ${gsl_src}.tgz
        #rm -r ${gsl_src}   #gonna throw a "rm: remove write-protected regular file 'gsl-src/gsl-config'?" hindering the flow
    fi

    #TODO: better way to get torc? maybe clone the current repo from https://github.com/cselab/pi4u ?
    #We only need the "torc_lite" subdirectory form this repo. However sparse checkjputs are not yet supported properly in git
    #git clone https://github.com/cselab/pi4u.git

    echo "--------------------------------------"
    echo " Installing torc:"
    echo "--------------------------------------"

    cd "${SolverDir}"/tools/pi4u_lite/torc_lite/
    autoreconf
    automake --add-missing
    ./configure CC=mpicc --prefix="${LIB_BASE}"/usr/torc --with-maxnodes=1024
    make
    make install

    export PATH="${LIB_BASE}"/usr/torc/bin:$PATH
    cd "${LIB_BASE}"

    echo "--------------------------------------"
    echo " Compiling Inference tools:"
    echo "--------------------------------------"

    cd "${SolverDir}"/tools/pi4u_lite/Inference

    cp tools.make/Makefile .
    cp tools.make/run_inference.sh .
    sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' Makefile
    sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' run_inference.sh

    if [ $LRZ = "yes" ]
    then
        cp tools.make/setup_lrz.sh .

        sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' setup_lrz.sh

        source setup_lrz.sh

    else
        UserName=$(hostname -s)
        cp tools.make/setup_linux.sh setup_${UserName}.sh
        
        sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' setup_${UserName}.sh

        source setup_${UserName}.sh
    fi

    make clean
    make

    cd "${LIB_BASE}"
fi


echo " "
echo "--------------------------------------"
echo ">>>       Creating Makefile       <<<"
echo "--------------------------------------"
cd "${SolverDir}"/makefile

cp tools.make/Makefile .

if [ $LRZ = "yes" ]
then
    cp tools.make/make.lrz .
    cp tools.make/setup_lrz.sh .

    #replace placeholders in files
    sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' make.lrz
    sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' setup_lrz.sh

    source setup_lrz.sh
else
    UserName=$(hostname -s)
    
    cp tools.make/make.linux make.${UserName}
    cp tools.make/setup_linux.sh setup_${UserName}.sh

    #replace placeholders in files
    sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' make.${UserName}
    sed -i 's|_USER_NAME_|'"${UserName}"'|g' Makefile
    sed -i 's|_USER_LIB_BASE_|'"${LIB_BASE}"'|g' setup_${UserName}.sh

    source setup_${UserName}.sh
fi

echo "  "
echo "==============================================="
echo ">>>     The Installation is completed       <<<"
echo "==============================================="
echo " "
echo "Please update your .bashrc or similar file to include path to the installed libraries."
echo " To do so, put the text between the following dashed lines into your .bashrc file:"
echo "---------------------------------------"

#cat setup_${UserName}.sh
echo "LIB_BASE=\"${LIB_BASE}\""
echo "export LD_LIBRARY_PATH=\$LIB_BASE/myVTK/lib/vtk-5.4/:\$LD_LIBRARY_PATH"
echo "export LD_LIBRARY_PATH=\$LIB_BASE/tbb40_20120613oss/build/linux_intel64_gcc_cc4.6.1_libc2.5_kernel2.6.18_release/:\$LD_LIBRARY_PATH"
    
if [ $LRZ != "yes" ]
then
    echo "export LD_LIBRARY_PATH=\$LIB_BASE/hypre-2.10.0b/src/hypre/lib/:\$LD_LIBRARY_PATH"
    echo "export PATH=\$LIB_BASE/mpich-install/bin:\$PATH"
fi

if [ $BI = "yes" ]          # only for Bayesian Inference 
then
    if [ $LRZ != "yes" ]
    then
        echo "export LD_LIBRARY_PATH=\$LIB_BASE/gsl-install/lib/:\$LD_LIBRARY_PATH"
    else
        echo "export LD_LIBRARY_PATH=/lrz/sys/libraries/gsl/2.3/lib:\$LD_LIBRARY_PATH"
    fi
    echo "export PATH=\$LIB_BASE/usr/torc/bin:\$PATH"
    #cat "${SolverDir}/tools/pi4u_lite/Inference/setup_${UserName}.sh"
fi
   

echo "---------------------------------------"
echo "To compile the GliomaSolver do"
echo "make clean && make -j 4"
echo "==============================================="

