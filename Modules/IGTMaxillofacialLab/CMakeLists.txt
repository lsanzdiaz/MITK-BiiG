MITK_CREATE_MODULE(IGTMaxillofacialLab #<-- module name
  SUBPROJECTS
  INCLUDE_DIRS IGTMaxillofacialLabFiles #<-- sub-folders of module
  INTERNAL_INCLUDE_DIRS ${INCLUDE_DIRS_INTERNAL} 
  DEPENDS MitkIGT MitkQtWidgetsExt MitkIGTUI
)