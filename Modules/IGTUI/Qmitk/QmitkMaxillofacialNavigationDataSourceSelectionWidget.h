/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#ifndef QmitkMaxillofacialNavigationDataSourceSelectionWidget_h
#define QmitkMaxillofacialNavigationDataSourceSelectionWidget_h

#include "ui_QmitkMaxillofacialNavigationDataSourceSelectionWidgetControls.h"

#include "MitkIGTUIExports.h"
#include <mitkNavigationToolStorage.h>
#include <mitkNavigationDataSource.h>
#include <usServiceReference.h>

#include <QWidget.h>


class MitkIGTUI_EXPORT QmitkMaxillofacialNavigationDataSourceSelectionWidget : public QWidget
{
  Q_OBJECT // this is needed for all Qt objects that should have a MOC object (everything that derives from QObject)
public:

  static const std::string VIEW_ID;
  QmitkMaxillofacialNavigationDataSourceSelectionWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
  virtual ~QmitkMaxillofacialNavigationDataSourceSelectionWidget();
  
  /** @return Returns the currently selected NavigationDataSource. Returns null if no source is selected at the moment. */
  mitk::NavigationDataSource::Pointer GetSelectedNavigationDataSource();

  /** @return Returns the ID of the currently selected tool. You can get the corresponding NavigationData when calling GetOutput(id)
  *         on the source object. Returns -1 if there is no tool selected.
  */
  int GetSelectedToolID();

  /** @return Returns the NavigationTool of the current selected tool if a NavigationToolStorage is available. Returns NULL if
  *         there is no storage available or if no tool is selected.
  */
  mitk::NavigationTool::Pointer GetSelectedNavigationTool();

  /** @return Returns the NavigationToolStorage of the currently selected NavigationDataSource. Returns NULL if there is no
  *         source selected or if the source has no NavigationToolStorage assigned.
  */
  mitk::NavigationToolStorage::Pointer GetNavigationToolStorageOfSource();


signals:
  /** @brief This signal is emitted when a new navigation data source is selected.
  * @param n Holds the new selected navigation data source. Is null if the old source is deselected and no new source is selected.
  */
  void NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer n);
  void NavigationToolSelected();

protected slots:

void NavigationDataSourceSelected(us::ServiceReferenceU s);
//void NavigationToolSelected();

protected:

	/// \brief Creation of the connections
	virtual void CreateConnections();

	virtual void CreateQtPartControl(QWidget *parent);

	Ui::QmitkMaxillofacialNavigationDataSourceSelectionWidgetControls* m_Controls;

	mitk::NavigationToolStorage::Pointer m_CurrentStorage;
	mitk::NavigationDataSource::Pointer m_CurrentSource;
};
#endif // _QmitkMaxillofacialNavigationDataSourceSelectionWidget_H_INCLUDED
