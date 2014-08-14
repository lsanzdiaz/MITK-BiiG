MITK_CREATE_MODULE(MaxillofacialLab #<-- module name
  SUBPROJECTS
  INCLUDE_DIRS MaxillofacialLabFiles #<-- sub-folders of module
  INTERNAL_INCLUDE_DIRS ${INCLUDE_DIRS_INTERNAL} 
  DEPENDS MitkQtWidgetsExt
)