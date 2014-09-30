MITK_CREATE_MODULE(ACVDMod #<-- module name
SUBPROJECTS
INCLUDE_DIRS Common DiscreteRemeshing VolumeProcessing#<-- sub-folders of module
INTERNAL_INCLUDE_DIRS ${INCLUDE_DIRS_INTERNAL}
PACKAGE_DEPENDS VTK
)

 