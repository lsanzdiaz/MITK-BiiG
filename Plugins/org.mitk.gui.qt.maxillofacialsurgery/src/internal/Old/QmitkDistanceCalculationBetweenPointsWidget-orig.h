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

#ifndef QmitkDistanceCalculationBetweenPointsWidget_h
#define QmitkDistanceCalculationBetweenPointsWidget_h

#include "ui_QmitkDistanceCalculationBetweenPointsWidget.h"
#include "QmitkStdMultiWidget.h"
#include "MitkIGTUIExports.h"


class QmitkDistanceCalculationBetweenPointsWidget : public QWidget
{
  Q_OBJECT // this is needed for all Qt objects that should have a MOC object (everything that derives from QObject)
public:

  QmitkDistanceCalculationBetweenPointsWidget(QWidget* parent);
  virtual ~QmitkDistanceCalculationBetweenPointsWidget();

  void SetMultiWidget(QmitkStdMultiWidget* multiWidget); ///< Set the default stdMultiWidget (needed for the PointListwidget)
  void AddSliceNavigationController(mitk::SliceNavigationController* snc); ///< add the slice navigation controller to be used to move the crosshair to the actual point position

  void SetReferencePositionNode(mitk::DataNode::Pointer imageFiducialsNode); ///< specify data tree node for the image fiducials
  void SetMeasurementPositionNode(mitk::DataNode::Pointer trackerFiducialsNode); ///< specify data tree node for the tracker fiducials
  mitk::DataNode::Pointer GetReferencePositionNode(); ///< returns data tree node for the image fiducials
  mitk::DataNode::Pointer GetMeasurementPositionNode(); ///< returns data tree node for the tracker fiducials
  mitk::DataNode::Pointer m_ReferencePositionNode;
  mitk::DataNode::Pointer m_MeasurementPositionNode;

  signals:
  void OnAddReferencePosition();      
  void OnAddMeasurementPosition();      
  void OnCalculateDistance();      

protected:

  void CreateQtPartControl(QWidget *parent);

  void CreateConnections();

  Ui::QmitkDistanceCalculationBetweenPointsWidget* m_Controls;  ///< gui widget
  QmitkStdMultiWidget* m_MultiWidget;
  

};
#endif // _QmitkDistanceCalculationBetweenPointsWidget_h
