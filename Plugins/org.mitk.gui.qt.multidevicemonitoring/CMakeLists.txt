project(org_mitk_gui_qt_multidevicemonitoring)

include_directories(${CTK_INCLUDE_DIRS})

MACRO_CREATE_MITK_CTK_PLUGIN(
  EXPORT_DIRECTIVE MULTIDEVICEMONITORING_EXPORT
  EXPORTED_INCLUDE_SUFFIXES src
  MODULE_DEPENDS MitkQtWidgets MitkQtWidgetsExt MitkIGT MitkIGTUI MitkMaxillofacialLab
)