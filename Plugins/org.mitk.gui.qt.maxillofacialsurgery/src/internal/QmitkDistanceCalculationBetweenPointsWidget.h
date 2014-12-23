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

#ifndef _QmitkDistanceCalculationBetweenPointsWidget_H_INCLUDED
#define _QmitkDistanceCalculationBetweenPointsWidget_H_INCLUDED

#include "ui_QmitkDistanceCalculationBetweenPointsWidget.h"
#include "QmitkStdMultiWidget.h"

/*!
 * \brief IGT Fiducial Registration Widget
 *
 * Widget used to set fiducial landmarks in the image and to confirm the corresponding landmarks on the world object (patient/phantom).
 *
 * SetImageFiducialsNode(), SetTrackerFiducialsNode() and SetMultiWidget() must be called, otherwise QmitkPointListWidget can not work.
 *
 *
 *
 * \sa IGT
*/
class QmitkDistanceCalculationBetweenPointsWidget : public QWidget
{
  Q_OBJECT // this is needed for all Qt objects that should have a MOC object (everything that derives from QObject)
public:

  QmitkDistanceCalculationBetweenPointsWidget(QWidget* parent);
  virtual ~QmitkDistanceCalculationBetweenPointsWidget();

  /*!
  \brief enumeration to specify the appearance of the widget.
  'FIDUCIALMODE' is likely to be used for (tracking) fiducial based registration purposes
  'LANDMARKMODE' can be used for any kind of landmark based registration (source landmarks -> target/reference landmarks)
  */
  
  void SetMultiWidget(QmitkStdMultiWidget* multiWidget); ///< Set the default stdMultiWidget (needed for the PointListwidget)
  void AddSliceNavigationController(mitk::SliceNavigationController* snc); ///< add the slice navigation controller to be used to move the crosshair to the actual point position

  void SetReferencePositionNode(mitk::DataNode::Pointer ReferenceNode); ///< specify data tree node for the image fiducials
  void SetMeasurementPositionNode(mitk::DataNode::Pointer MeasurementNode); ///< specify data tree node for the tracker fiducials
  mitk::DataNode::Pointer GetReferencePositionNode(); ///< returns data tree node for the image fiducials
  mitk::DataNode::Pointer GetMeasurementPositionNode(); ///< returns data tree node for the tracker fiducials
  void AddDistanceToList(double distance);
  void SetQualityDisplayText(QString text); ///< sets specific text on the UI (useful to display FRE/TRE...)
  /*!
  \brief Specify the name of the source landmarks. Will be used for label and button.

  Example: sourceLandmarkName="CT" will result in group box title "CT landmarks" and button text "Add CT landmark".
  */
  void SetReferenceLandmarkName(QString sourceLandmarkName);
  /*!
  \brief Specify the name of the source landmarks. Will be used for label and button.

  Example: targetLandmarkName="CT" will result in group box title "CT landmarks" and button text "Add CT landmark".
  */
  void SetMeasurementLandmarkName(QString targetLandmarkName);
  void AdjustButtonSpacing(); ///< Rearrange spacing when buttons are turned on or off

signals:
  void AddedReferencePosition();      ///< signal if a world instrument position was added to a tracking space fiducial
  void AddedMeasurementPosition();
  void DemandDistanceCalculation();

  protected slots:
  void OnSaveData();

protected:

  void CreateQtPartControl(QWidget *parent);

  void CreateConnections();

  void WriteData();

  Ui::QmitkDistanceCalculationBetweenPointsWidget* m_Controls;  ///< gui widget
  QmitkStdMultiWidget* m_MultiWidget;
  mitk::DataNode::Pointer m_ReferenceNode;
  mitk::DataNode::Pointer m_MeasurementNode;
  double *m_Distances;

  std::ostream* m_Stream; ///< the output stream

};
#endif // _QmitkDistanceCalculationBetweenPointsWidget_H_INCLUDED
