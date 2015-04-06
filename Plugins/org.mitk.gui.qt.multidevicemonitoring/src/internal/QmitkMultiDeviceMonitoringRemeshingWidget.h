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

#ifndef QmitkMultiDevicemonitoringRemeshingWidget_h
#define QmitkMultiDevicemonitoringRemeshingWidget_h

#include "ui_QmitkMultiDevicemonitoringRemeshingWidget.h"

//MITK

#include "mitkDataStorage.h"
#include "mitkDataNode.h"
#include "mitkNodePredicateBase.h"
#include "mitkPointSet.h"
#include <MITKMaxillofacialMeshLab.h>
#include "mitkPointLocator.h"
#include <mitkSurface.h>
#include <MitkQtWidgetsExports.h>

//VTK

#include "vtkCellLocator.h"
#include <vtkSmartPointer.h>
#include <vtkDistancePolyDataFilter.h>
#include <vtkProperty.h>

#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkScalarBarActor.h>

//QT

#include "QmitkStdMultiWidget.h"
#include <QThread>
#include <QWidget.h>

struct Distance_Data
{
	double* closest_point;
	double distance;
};

class QmitkMultiDevicemonitoringRemeshingWidget : public QWidget
{
  Q_OBJECT // this is needed for all Qt objects that should have a MOC object (everything that derives from QObject)
public:

  QmitkMultiDevicemonitoringRemeshingWidget(QWidget* parent);
  virtual ~QmitkMultiDevicemonitoringRemeshingWidget();
  void SetDataStorage(mitk::DataStorage::Pointer _DataStorage);
  void SetRenderer(vtkSmartPointer<vtkRenderWindow> _RenderWindow);
  void InitiateDistanceControl(mitk::DataNode::Pointer node);
  Distance_Data* RealTimeControlOfDistance(mitk::Point3D ToolNavigationPosition);
  
protected slots:
  void OnAdvancedSettingsButtonToggled(bool toggled);
  void OnEdgeSplittingChanged(double edgeSplitting);
  void OnGradationChanged(double gradation);
  void OnMaxNumVerticesLineEditEditingFinished();
  void OnNumberOfVerticesChanged(int numVertices);
  void OnOptimizationLevelChanged(int optimizationLevel);
  void OnRemeshButtonClicked();
  void OnSelectedSurfaceChanged(const mitk::DataNode *node);
  void OnSubsamplingChanged(int subsampling);
  

protected:

  void CreateQtPartControl(QWidget *parent);
  
  void CreateConnections();
  
  mitk::WeakPointer<mitk::DataStorage> m_DataStorage;
  

  Ui::QmitkMultiDevicemonitoringRemeshingWidget* m_Controls;  ///< gui widget
  QmitkStdMultiWidget* m_MultiWidget;
  mitk::DataNode::Pointer m_ImageFiducialsNode;
  mitk::DataNode::Pointer m_TrackerFiducialsNode;
  
  void EnableWidgets(bool enable);
  void SetNumberOfVertices(int minimum, int maximum, int step, int value);
  QThread m_Thread;
  mitk::ACVD::RemeshFilter::Pointer m_remesher;
  vtkSmartPointer<vtkDistancePolyDataFilter> m_distanceFilter;
  vtkSmartPointer<vtkRenderer> m_renderer;
  vtkSmartPointer<vtkRenderWindowInteractor> m_renWinInteractor;
  vtkSmartPointer<vtkRenderWindow> m_renWin;
  vtkSmartPointer<vtkPolyDataMapper> m_mapper;
  vtkSmartPointer<vtkActor> m_actor;
  vtkSmartPointer<vtkScalarBarActor> m_scalarBar;

  //mitk::PointLocator::Pointer m_Locator;
  vtkSmartPointer<vtkCellLocator> m_CellLocator;
};
#endif // _QmitkMultiDevicemonitoringRemeshing_H_INCLUDED
