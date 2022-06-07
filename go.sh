#!/usr/bin/env bash

die() { printf $'Error: %s\n' "$*" >&2; exit 1; }
root=$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)
PATH=${root:?}/go.sh.d${PATH:+:${PATH:?}}
project=${root##*/}
user=${USER:-nouser}
hostname=${HOSTNAME:-nohostname}

#---

ospray_source=${root:?}/ospray
ospray_repo=https://github.com/ospray/ospray.git
ospray_ref=v2.9.0
ospray_build=${root:?}/_build.ospray
ospray_stage=${root:?}/_stage.ospray
ospray_config=(
    -DCMAKE_POLICY_DEFAULT_CMP0074:STRING=NEW
    -DOSPRAY_ENABLE_APPS:BOOL=OFF
    -DINSTALL_IN_SEPARATE_DIRECTORIES:BOOL=OFF
)

declare -n ospray_git_source=ospray_source
declare -n ospray_git_repo=ospray_repo
declare -n ospray_git_ref=ospray_ref

ospray_cmake_source=${ospray_source:?}/scripts/superbuild
declare -n ospray_cmake_build=ospray_build
declare -n ospray_cmake_stage=ospray_stage
declare -n ospray_cmake_config=ospray_config

ospray_run_bindir=${ospray_stage:?}/bin
ospray_run_libdir=${ospray_stage:?}/lib:${ospray_stage:?}/lib64
ospray_run_incdir=${ospray_stage:?}/include


go-ospray() {
    go-ospray-"$@"
}

go-ospray-clean() {
    rm -rf \
        "${ospray_build:?}" \
        "${ospray_stage:?}"
}

go-ospray-git() {
    go-ospray-git-"$@"
}

go-ospray-git-clone() {
    git clone \
        "${ospray_git_repo:?}" \
        "${ospray_git_source:?}" \
        ${ospray_git_ref:+--branch "${ospray_git_ref:?}"}
}

go-ospray-git-checkout() {
    git \
        -C "${ospray_git_source:?}" \
        checkout \
        ${ospray_git_ref:+"${ospray_git_ref:?}"}
}

go-ospray-cmake() {
    go-ospray-cmake-"$@"
}

go-ospray-cmake-configure() {
    cmake \
        -H"${ospray_cmake_source:?}" \
        -B"${ospray_cmake_build:?}" \
        -DCMAKE_INSTALL_PREFIX:PATH="${ospray_cmake_stage:?}" \
        "${ospray_cmake_config[@]}"
}

# env: re, par
go-ospray-cmake--build() {
    cmake \
        --build "${ospray_cmake_build:?}" \
        --verbose \
        ${re+--clean-first} \
        ${par+--parallel}
}

go-ospray-cmake-build() {
    local re par
    go-ospray-cmake--build
}

go-ospray-cmake-parbuild() {
    local re par=
    go-ospray-cmake--build
}

go-ospray-cmake-rebuild() {
    local re= par
    go-ospray-cmake--build
}

go-ospray-cmake-reparbuild() {
    local re= par=
    go-ospray-cmake--build
}

go-ospray-cmake-install() {
    cmake \
        --install "${ospray_cmake_build:?}" \
        --verbose
}

go-ospray-run() {
    PATH=${ospray_run_bindir:?}${PATH:+:${PATH:?}} \
    CPATH=${ospray_run_incdir:?}${CPATH:+:${CPATH:?}} \
    LD_LIBRARY_PATH=${ospray_run_libdir:?}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    "$@"
}

go-ospray-go() {
    go-ospray-run go-"$@"
}

go-ospray-exec() {
    go-ospray-run exec "$@"
}

#---

vtk_source=${root:?}/vtk
vtk_repo=https://github.com/Kitware/VTK.git
vtk_ref=v9.1.0
vtk_build=${root:?}/_build.vtk
vtk_stage=${root:?}/_stage.vtk
vtk_config=(
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DVTK_ENABLE_LOGGING:BOOL=OFF
    -DVTK_ENABLE_WRAPPING:BOOL=OFF
    -DVTK_LEGACY_REMOVE:BOOL=ON
    -DVTK_OPENGL_USE_GLES:BOOL=OFF
    -DVTK_USE_SDL2:BOOL=OFF
    -DVTK_USE_RENDERING:BOOL=FALSE
    -DVTK_USEINFOVIS:BOOL=FALSE
    -DVTK_NO_PLATFORM_SOCKETS:BOOL=ON
    -DVTK_BUILD_TESTING=OFF
    -DVTK_BUILD_DOCUMENTATION=OFF
    -DVTK_REPORT_OPENGL_ERRORS=OFF
    -DVTK_ALL_NEW_OBJECT_FACTORY=OFF

    -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmDataModel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ChartsCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonArchive:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonColor:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonComputationalGeometry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonDataModel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonExecutionModel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonMath:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonMisc:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonPython:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonSystem:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_CommonTransforms:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_DomainsChemistry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_DomainsChemistryOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_DomainsMicroscopy:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_DomainsParallelChemistry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersAMR:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersExtraction:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersFlowPaths:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersGeneral:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersGeneric:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersGeometry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersHybrid:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersHyperTree:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersImaging:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersModeling:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersOpenTURNS:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelDIY2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelFlowPaths:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelGeometry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelImaging:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelMPI:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelStatistics:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersParallelVerdict:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersPoints:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersProgrammable:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersPython:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersReebGraph:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersSMP:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersSelection:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersSources:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersStatistics:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersTexture:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersTopology:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_FiltersVerdict:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_GeovisCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_GeovisGDAL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_GUISupportMFC:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_GUISupportQt:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtQuick:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_GUISupportQtSQL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingColor:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingFourier:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingGeneral:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingHybrid:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingMath:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingMorphological:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingSources:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingStatistics:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ImagingStencil:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InfovisBoost:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InfovisBoostGraphAlgorithms:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InfovisCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InfovisLayout:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InteractionImage:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InteractionStyle:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_InteractionWidgets:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOADIOS2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOAMR:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOAsynchronous:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOCGNSReader:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOCONVERGECFD:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOChemistry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOCityGML:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOEnSight:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOExodus:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOExport:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOExportGL2PS:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOExportPDF:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOFFMPEG:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOFides:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOGDAL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOGeoJSON:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOGeometry:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOH5Rage:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOH5part:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOHDF:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOIOSS:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOImage:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOImport:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOInfovis:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOLAS:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOLSDyna:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOLegacy:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOMINC:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOMPIImage:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOMPIParallel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOMotionFX:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOMovie:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOMySQL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IONetCDF:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOODBC:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOOMF:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOOggTheora:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOOpenVDB:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOPDAL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOPIO:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOPLY:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOParallel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOParallelExodus:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOParallelLSDyna:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOParallelNetCDF:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOParallelXML:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOParallelXdmf3:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOPostgreSQL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOSQL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOSegY:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOTRUCHAS:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOTecplotTable:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOVPIC:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOVeraOut:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOVideo:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOXML:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOXMLParser:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOXdmf2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_IOXdmf3:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ParallelCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ParallelDIY:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ParallelMPI:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ParallelMPI4Py:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingAnnotation:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingContext2D:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingContextOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingExternal:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingFFMPEGOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingFreeType:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingFreeTypeFontConfig:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingGL2PSOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingImage:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingLICOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingLOD:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingLabel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingMatplotlib:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingOpenGL2:STRING=WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingOpenVR:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingParallel:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingParallelLIC:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_PythonContext2D:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingQt:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingRayTracing:STRING=WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingSceneGraph:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingTk:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingUI:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingVR:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingVolume:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingVolumeAMR:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingVolumeOpenGL2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_RenderingVtkJS:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_TestingCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_TestingGenericBridge:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_TestingIOSQL:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_TestingRendering:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_cgns:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_cli11:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_diy2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_doubleconversion:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_eigen:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_exodusII:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_expat:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_exprtk:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_fides:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_fmt:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_freetype:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_gl2ps:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_glew:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_h5part:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_hdf5:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ioss:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_jpeg:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_jsoncpp:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_kissfft:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_libharu:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_libproj:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_libxml2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_loguru:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_lz4:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_lzma:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_mpi4py:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_netcdf:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ogg:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_pegtl:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_png:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_pugixml:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_sqlite:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_theora:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_tiff:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_utf8:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_verdict:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_vpic:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_vtkm:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_xdmf2:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_xdmf3:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_zfp:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_zlib:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_UtilitiesBenchmarks:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_DICOMParser:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_Java:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_kwiml:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_vtksys:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_mpi:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_metaio:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_opengl:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_Python:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_PythonInterpreter:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_octree:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ViewsContext2D:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ViewsCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ViewsInfovis:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_ViewsQt:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_WebCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_WebPython:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_WebGLExporter:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_WrappingPythonCore:STRING=DONT_WANT
    -DVTK_MODULE_ENABLE_VTK_WrappingTools:STRING=DONT_WANT

    -DVTK_DEFAULT_RENDER_WINDOW_HEADLESS:BOOL=ON
)

declare -n vtk_git_source=vtk_source
declare -n vtk_git_repo=vtk_repo
declare -n vtk_git_ref=vtk_ref

declare -n vtk_cmake_source=vtk_source
declare -n vtk_cmake_build=vtk_build
declare -n vtk_cmake_stage=vtk_stage
declare -n vtk_cmake_config=vtk_config

vtk_run_bindir=${vtk_stage:?}/bin
vtk_run_libdir=${vtk_stage:?}/lib
vtk_run_incdir=${vtk_stage:?}/include


go-vtk() {
    go-vtk-"$@"
}

go-vtk-clean() {
    rm -rf \
        "${vtk_build:?}" \
        "${vtk_stage:?}"
}

go-vtk-git() {
    go-vtk-git-"$@"
}

go-vtk-git-clone() {
    git clone \
        "${vtk_git_repo:?}" \
        "${vtk_git_source:?}" \
        ${vtk_git_ref:+--branch "${vtk_git_ref:?}"}
}

go-vtk-git-checkout() {
    git \
        -C "${vtk_git_source:?}" \
        checkout \
        ${vtk_git_ref:+"${vtk_git_ref:?}"}
}

go-vtk-cmake() {
    go-vtk-cmake-"$@"
}

go-vtk-cmake-configure() {
    cmake \
        -H"${vtk_cmake_source:?}" \
        -B"${vtk_cmake_build:?}" \
        -DCMAKE_INSTALL_PREFIX:PATH="${vtk_cmake_stage:?}" \
        "${vtk_cmake_config[@]}"
}

# env: re, par
go-vtk-cmake--build() {
    cmake \
        --build "${vtk_cmake_build:?}" \
        --verbose \
        ${re+--clean-first} \
        ${par+--parallel}
}

go-vtk-cmake-build() {
    local re par
    go-vtk-cmake--build
}

go-vtk-cmake-parbuild() {
    local re par=
    go-vtk-cmake--build
}

go-vtk-cmake-rebuild() {
    local re= par
    go-vtk-cmake--build
}

go-vtk-cmake-reparbuild() {
    local re= par=
    go-vtk-cmake--build
}

go-vtk-cmake-install() {
    cmake \
        --install "${vtk_cmake_build:?}" \
        --verbose
}

go-vtk-run() {
    PATH=${vtk_run_bindir:?}${PATH:+:${PATH:?}} \
    CPATH=${vtk_run_incdir:?}${CPATH:+:${CPATH:?}} \
    LD_LIBRARY_PATH=${vtk_run_libdir:?}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    vtk_ROOT=${vtk_stage:?} \
    "$@"
}

go-vtk-go() {
    go-vtk-run go-"$@"
}

go-vtk-exec() {
    go-vtk-run exec "$@"
}

#---

src_source=${root:?}/src
src_build=${root:?}/_build.src
src_stage=${root:?}/_stage.src
src_config=(
)

declare -n src_cmake_source=src_source
declare -n src_cmake_build=src_build
declare -n src_cmake_stage=src_stage
declare -n src_cmake_config=src_config

src_run_bindir=${src_stage:?}/bin
src_run_libdir=${src_stage:?}/lib
src_run_incdir=${src_stage:?}/include


go-src() {
    go-src-"$@"
}

go-src-clean() {
    rm -rf \
        "${src_build:?}" \
        "${src_stage:?}"
}

go-src-cmake() {
    go-src-cmake-"$@"
}

go-src-cmake-configure() {
    cmake \
        -H"${src_cmake_source:?}" \
        -B"${src_cmake_build:?}" \
        -DCMAKE_INSTALL_PREFIX:PATH="${src_cmake_stage:?}" \
        "${src_cmake_config[@]}"
}

# env: re, par
go-src-cmake--build() {
    cmake \
        --build "${src_cmake_build:?}" \
        --verbose \
        ${re+--clean-first} \
        ${par+--parallel}
}

go-src-cmake-build() {
    local re par
    go-src-cmake--build
}

go-src-cmake-parbuild() {
    local re par=
    go-src-cmake--build
}

go-src-cmake-rebuild() {
    local re= par
    go-src-cmake--build
}

go-src-cmake-reparbuild() {
    local re= par=
    go-src-cmake--build
}

go-src-cmake-install() {
    cmake \
        --install "${src_cmake_build:?}" \
        --verbose
}

go-src-run() {
    PATH=${src_run_bindir:?}${PATH:+:${PATH:?}} \
    CPATH=${src_run_incdir:?}${CPATH:+:${CPATH:?}} \
    LD_LIBRARY_PATH=${src_run_libdir:?}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}} \
    "$@"
}

go-src-go() {
    go-src-run go-"$@"
}

go-src-exec() {
    go-src-run exec "$@"
}

#---

test -f "${root:?}/env.sh" && source "${_:?}"
test -f "${root:?}/${hostname:+@${hostname:?}.env.sh}" && source "${_:?}"
test -f "${root:?}/${user:+${user:?}@.env.sh}" && source "${_:?}"
test -f "${root:?}/${user:+${hostname:+${user:?}@${hostname:?}.env.sh}}" && source "${_:?}"

go-"$@"
