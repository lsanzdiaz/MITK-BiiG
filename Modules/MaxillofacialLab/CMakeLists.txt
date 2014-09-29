MITK_CREATE_MODULE(MaxillofacialLab #<-- module name
  DEPENDS MitkCore MitkQtWidgetsExt MitkACVDMod
  PACKAGE_DEPENDS VTK|vtkIOPLY+vtkIOMINC
  INCLUDE_DIRS MaxillofacialLabFiles #<-- sub-folders of module
  INTERNAL_INCLUDE_DIRS ${INCLUDE_DIRS_INTERNAL} 
)