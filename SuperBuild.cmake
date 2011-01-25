#PROJECT(MITK-super)
cmake_minimum_required(VERSION 2.8.2)
MESSAGE("In super build")
#-----------------------------------------------------------------------------
# Convenient macro allowing to download a file
#
MACRO(downloadFile url dest)
  FILE(DOWNLOAD ${url} ${dest} STATUS status)
  LIST(GET status 0 error_code)
  LIST(GET status 1 error_msg)
  IF(error_code)
    MESSAGE(FATAL_ERROR "error: Failed to download ${url} - ${error_msg}")
  ENDIF()
ENDMACRO()

INCLUDE(ExternalProject)

SET(ep_base "${CMAKE_BINARY_DIR}/CMakeExternals")
SET_PROPERTY(DIRECTORY PROPERTY EP_BASE ${ep_base})

SET(ep_install_dir ${ep_base}/Install)
SET(ep_build_dir ${ep_base}/Build)
SET(ep_source_dir ${ep_base}/Source)
#SET(ep_parallelism_level)
SET(ep_build_shared_libs ON)
SET(ep_build_testing OFF)

SET(ep_common_args
  -DCMAKE_INSTALL_PREFIX:PATH=${ep_install_dir}
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
  -DBUILD_TESTING:BOOL=${ep_build_testing}
  )

# Compute -G arg for configuring external projects with the same CMake generator:
IF(CMAKE_EXTRA_GENERATOR)
  SET(gen "${CMAKE_EXTRA_GENERATOR} - ${CMAKE_GENERATOR}")
ELSE()
  SET(gen "${CMAKE_GENERATOR}")
ENDIF()

# Use this value where semi-colons are needed in ep_add args:
set(sep "^^")

##

SET(ep_common_args 
  -DBUILD_SHARED_LIBS:BOOL=ON
  -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
)



#-----------------------------------------------------------------------------
# Prerequisites
#

if(MITK_USE_QT)
  set(vtk_QT_ARGS
      ${ep_common_args}
      -DDESIRED_QT_VERSION:STRING=4
      -DVTK_USE_GUISUPPORT:BOOL=ON
      -DVTK_USE_QVTK_QTOPENGL:BOOL=ON
      -DVTK_USE_QT:BOOL=ON
      -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
     )
endif(MITK_USE_QT)

# ----------------------------------------- 
# VTK
#

IF(NOT DEFINED VTK_DIR)
    SET(proj vtk)
    SET(VTK_DEPENDS ${proj})
    ExternalProject_Add(${proj}
      URL http://mitk.org/download/thirdparty/vtk-5.6.1.tar.gz
      INSTALL_COMMAND ""
      CMAKE_GENERATOR ${gen}
      CMAKE_ARGS
        ${ep_common_args}
        -DVTK_WRAP_TCL:BOOL=OFF
        -DVTK_WRAP_PYTHON:BOOL=OFF
        -DVTK_WRAP_JAVA:BOOL=OFF
        -DBUILD_SHARED_LIBS:BOOL=ON 
        -DVTK_USE_PARALLEL:BOOL=ON
        -DVTK_USE_SYSTEM_FREETYPE:BOOL=ON
        ${vtk_QT_ARGS}
      )
  SET(VTK_DIR ${ep_build_dir}/${proj})
ENDIF()
## ----------------------------------------
# DCMTK
IF(MITK_USE_DCMTK)

  IF(NOT DEFINED DCMTK_DIR)
  SET(proj DCMTK)
IF(UNIX)
  SET(DCMTK_CXX_FLAGS "-fPIC")
ENDIF(UNIX)

  ExternalProject_Add(${proj}
    URL http://mitk.org/download/thirdparty/dcmtk-3.6.0.tar.gz
      CMAKE_GENERATOR ${gen}
      CMAKE_ARGS
         ${ep_common_args}
         -DBUILD_SHARED_LIBS:BOOL=OFF
         -DCMAKE_CXX_FLAGS:STRING=${DCMTK_CXX_FLAGS}
         -DCMAKE_C_FLAGS:STRING=${DCMTK_CXX_FLAGS}
         -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/DCMTK-prefix/install
  )

  ENDIF()
ENDIF()     


# ----------------------------------------- 
# CTK
#
IF(MITK_USE_CTK)
IF(NOT DEFINED CTK_DIR)
    SET(proj CTK)
    # SET(CTK_DEPENDS ${proj})
    ExternalProject_Add(${proj}
      GIT_REPOSITORY git://github.com/commontk/CTK.git
      INSTALL_COMMAND ""
      CMAKE_GENERATOR ${gen}
      CMAKE_ARGS
        ${ep_common_args}
        -DDESIRED_QT_VERSION:STRING=4
        -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
        -DCTK_LIB_DICOM/Widgets:BOOL=ON
      )
  SET(CTK_DIR ${ep_build_dir}/${proj})
ENDIF()
ENDIF(MITK_USE_CTK)

## ----------------------------------------- 
# ITK
#
IF(NOT DEFINED ITK_DIR)
  SET(proj ITK)
  ExternalProject_Add(${proj}
     URL http://mitk.org/download/thirdparty/InsightToolkit-3.20.0.tar.gz 
     INSTALL_COMMAND ""
     CMAKE_GENERATOR ${gen}
     CMAKE_ARGS
       ${ep_common_args}
       -DBUILD_SHARED_LIBS:BOOL=ON 
       -DBUILD_TESTING:BOOL=OFF
       -DBUILD_EXAMPLES:BOOL=OFF
     )

  SET(ITK_DIR ${ep_build_dir}/${proj})

ENDIF()


#-----------------------------------------------------------------------------
# MITK Configure
#
SET(proj MITK-Configure)

ExternalProject_Add(${proj}
  DOWNLOAD_COMMAND ""
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    ${ctk_superbuild_boolean_args}
    -DMITK_USE_SUPERBUILD:BOOL=OFF
    -DWITH_COVERAGE:BOOL=${WITH_COVERAGE}
    -DCTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
    -DMITK_SUPERBUILD_BINARY_DIR:PATH=${MITK_BINARY_DIR}
    -DCMAKE_INSTALL_PREFIX:PATH=${ep_install_dir}
    -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    -DMITK_CXX_FLAGS:STRING=${MITK_CXX_FLAGS}
    -DMITK_C_FLAGS:STRING=${MITK_C_FLAGS}
    -DQT_QMAKE_EXECUTABLE:FILEPATH=${QT_QMAKE_EXECUTABLE}
    -DMITK_KWSTYLE_EXECUTABLE:FILEPATH=${MITK_KWSTYLE_EXECUTABLE}
    -DCTK_DIR:PATH=${CTK_DIR} # FindDCMTK expects DCMTK_DIR
    -DDCMTK_DIR:PATH=${CTK_DIR}/CMakeExternals/Install/ # FindDCMTK expects DCMTK_DIR
    -DVTK_DIR:PATH=${VTK_DIR}     # FindVTK expects VTK_DIR
    -DITK_DIR:PATH=${ITK_DIR}     # FindITK expects ITK_DIR
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  BINARY_DIR ${CMAKE_BINARY_DIR}/MITK-build
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  DEPENDS
    vtk
    ITK
  )


#-----------------------------------------------------------------------------
# MITK
#
#MESSAGE(STATUS SUPERBUILD_EXCLUDE_MITKBUILD_TARGET:${SUPERBUILD_EXCLUDE_MITKBUILD_TARGET})
IF(NOT DEFINED SUPERBUILD_EXCLUDE_MITKBUILD_TARGET OR NOT SUPERBUILD_EXCLUDE_MITKBUILD_TARGET)
  SET(proj MITK-build)
  ExternalProject_Add(${proj}
    DOWNLOAD_COMMAND ""
    CMAKE_GENERATOR ${gen}
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    BINARY_DIR MITK-build
    INSTALL_COMMAND ""
    DEPENDS
      "MITK-Configure"
    )
ENDIF()

#-----------------------------------------------------------------------------
# Custom target allowing to drive the build of MITK project itself
#
ADD_CUSTOM_TARGET(MITK
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/MITK-build
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/MITK-build
)
  