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

#include "QmitkMultiDevicemonitoringRemeshingWidget.h"

// MITK

#include <mitkNodePredicateDataType.h>
#include <mitkVtkRepresentationProperty.h>
#include <mitkMessage.h>

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkGenericCell.h>


/* VIEW MANAGEMENT */
QmitkMultiDevicemonitoringRemeshingWidget::QmitkMultiDevicemonitoringRemeshingWidget(QWidget* parent)
: QWidget(parent), m_Controls(NULL),m_MultiWidget(NULL)
{
  CreateQtPartControl(this);
  m_remesher = mitk::ACVD::RemeshFilter::New();
  m_distanceFilter = vtkSmartPointer<vtkDistancePolyDataFilter>::New();

}

QmitkMultiDevicemonitoringRemeshingWidget::~QmitkMultiDevicemonitoringRemeshingWidget()
{
}

//#PUBLIC SETTER
void QmitkMultiDevicemonitoringRemeshingWidget::SetDataStorage(mitk::DataStorage::Pointer _DataStorage)
{
	// reset only if datastorage really changed
	if (m_DataStorage.GetPointer() != _DataStorage)
	{
			// set new storage
			m_DataStorage = _DataStorage;
			if (m_DataStorage.IsNotNull())
			{
				m_Controls->m_surfaceComboBox->SetDataStorage(m_DataStorage);
				m_Controls->m_surfaceComboBox->SetAutoSelectNewItems(false);
				m_Controls->m_surfaceComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));
			}		
	}
}

// #PUBLIC SETTER
void QmitkMultiDevicemonitoringRemeshingWidget::SetRenderer(vtkSmartPointer<vtkRenderWindow> _RenderWindow)
{
	m_renWin = _RenderWindow;
	mitk::BaseRenderer::Pointer renderer = mitk::BaseRenderer::GetInstance(_RenderWindow);
	
	m_renWinInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	m_renderer = renderer->GetVtkRenderer();

	
	//m_renderer->AddActor2D(m_scalarBar);
	
	//m_scalarBar->SetLookupTable(NULL);
	//m_scalarBar->SetTitle("Distance");
	//m_scalarBar->SetNumberOfLabels(4);
	
}

void QmitkMultiDevicemonitoringRemeshingWidget::CreateQtPartControl(QWidget* parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkMultiDevicemonitoringRemeshingWidget;
    m_Controls->setupUi(parent);

    this->CreateConnections();
	this->EnableWidgets(m_Controls->m_surfaceComboBox->GetSelectedNode().IsNotNull());
	this->OnAdvancedSettingsButtonToggled(false);
  }

  this->OnSelectedSurfaceChanged(m_Controls->m_surfaceComboBox->GetSelectedNode());
}
void QmitkMultiDevicemonitoringRemeshingWidget::CreateConnections()
{
  connect(m_Controls->m_surfaceComboBox, SIGNAL(OnSelectionChanged(const mitk::DataNode *)), this, SLOT(OnSelectedSurfaceChanged(const mitk::DataNode *)));
  connect(m_Controls->numVerticesSlider, SIGNAL(valueChanged(int)), this, SLOT(OnNumberOfVerticesChanged(int)));
  connect(m_Controls->numVerticesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnNumberOfVerticesChanged(int)));
  connect(m_Controls->gradationSlider, SIGNAL(valueChanged(double)), this, SLOT(OnGradationChanged(double)));
  connect(m_Controls->gradationSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnGradationChanged(double)));
  connect(m_Controls->advancedSettingsButton, SIGNAL(toggled(bool)), this, SLOT(OnAdvancedSettingsButtonToggled(bool)));
  connect(m_Controls->maxNumVerticesLineEdit, SIGNAL(editingFinished()), this, SLOT(OnMaxNumVerticesLineEditEditingFinished()));
  connect(m_Controls->edgeSplittingSlider, SIGNAL(valueChanged(double)), this, SLOT(OnEdgeSplittingChanged(double)));
  connect(m_Controls->edgeSplittingSpinBox, SIGNAL(valueChanged(double)), this, SLOT(OnEdgeSplittingChanged(double)));
  connect(m_Controls->subsamplingSlider, SIGNAL(valueChanged(int)), this, SLOT(OnSubsamplingChanged(int)));
  connect(m_Controls->subsamplingSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnSubsamplingChanged(int)));
  connect(m_Controls->optimizationLevelSlider, SIGNAL(valueChanged(int)), this, SLOT(OnOptimizationLevelChanged(int)));
  connect(m_Controls->optimizationLevelSpinBox, SIGNAL(valueChanged(int)), this, SLOT(OnOptimizationLevelChanged(int)));
  connect(m_Controls->remeshPushButton, SIGNAL(clicked()), this, SLOT(OnRemeshButtonClicked()));
}

void QmitkMultiDevicemonitoringRemeshingWidget::EnableWidgets(bool enable)
{
  m_Controls->m_surfaceComboBox->setEnabled(enable);
  m_Controls->numVerticesSlider->setEnabled(enable);
  m_Controls->numVerticesSpinBox->setEnabled(enable);
  m_Controls->gradationSlider->setEnabled(enable);
  m_Controls->gradationSpinBox->setEnabled(enable);
  m_Controls->maxNumVerticesLineEdit->setEnabled(enable);
  m_Controls->edgeSplittingSlider->setEnabled(enable);
  m_Controls->edgeSplittingSpinBox->setEnabled(enable);
  m_Controls->subsamplingSlider->setEnabled(enable);
  m_Controls->subsamplingSpinBox->setEnabled(enable);
  m_Controls->optimizationLevelSlider->setEnabled(enable);
  m_Controls->optimizationLevelSpinBox->setEnabled(enable);
  m_Controls->forceManifoldCheckBox->setEnabled(enable);
  m_Controls->boundaryFixingCheckBox->setEnabled(enable);
  m_Controls->remeshPushButton->setEnabled(enable);
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnAdvancedSettingsButtonToggled(bool toggled)
{
  m_Controls->maxNumVerticesLabel->setVisible(toggled);
  m_Controls->maxNumVerticesLineEdit->setVisible(toggled);
  m_Controls->edgeSplittingLabel->setVisible(toggled);
  m_Controls->edgeSplittingSlider->setVisible(toggled);
  m_Controls->edgeSplittingSpinBox->setVisible(toggled);
  m_Controls->subsamplingLabel->setVisible(toggled);
  m_Controls->subsamplingSlider->setVisible(toggled);
  m_Controls->subsamplingSpinBox->setVisible(toggled);
  m_Controls->optimizationLevelLabel->setVisible(toggled);
  m_Controls->optimizationLevelSlider->setVisible(toggled);
  m_Controls->optimizationLevelSpinBox->setVisible(toggled);
  m_Controls->forceManifoldCheckBox->setVisible(toggled);
  m_Controls->boundaryFixingCheckBox->setVisible(toggled);
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnEdgeSplittingChanged(double edgeSplitting)
{
  if (edgeSplitting != m_Controls->edgeSplittingSlider->value())
    m_Controls->edgeSplittingSlider->setValue(edgeSplitting);

  if (edgeSplitting != m_Controls->edgeSplittingSpinBox->value())
    m_Controls->edgeSplittingSpinBox->setValue(edgeSplitting);
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnGradationChanged(double gradation)
{
  if (gradation != m_Controls->gradationSlider->value())
    m_Controls->gradationSlider->setValue(gradation);

  if (gradation != m_Controls->gradationSpinBox->value())
    m_Controls->gradationSpinBox->setValue(gradation);
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnMaxNumVerticesLineEditEditingFinished()
{
  int maximum = m_Controls->maxNumVerticesLineEdit->text().toInt();

  if (m_Controls->numVerticesSpinBox->maximum() != maximum)
  {
    m_Controls->numVerticesSlider->setMaximum(maximum);
    m_Controls->numVerticesSpinBox->setMaximum(maximum);
  }
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnNumberOfVerticesChanged(int numVertices)
{
  if (numVertices != m_Controls->numVerticesSlider->value())
    m_Controls->numVerticesSlider->setValue(numVertices);

  if (numVertices != m_Controls->numVerticesSpinBox->value())
    m_Controls->numVerticesSpinBox->setValue(numVertices);
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnOptimizationLevelChanged(int optimizationLevel)
{
  if (optimizationLevel != m_Controls->optimizationLevelSlider->value())
    m_Controls->optimizationLevelSlider->setValue(optimizationLevel);

  if (optimizationLevel != m_Controls->optimizationLevelSpinBox->value())
    m_Controls->optimizationLevelSpinBox->setValue(optimizationLevel);
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnRemeshButtonClicked()
{
  mitk::DataNode::Pointer selectedNode = m_Controls->m_surfaceComboBox->GetSelectedNode();
  mitk::Surface::ConstPointer surface = static_cast<mitk::Surface*>(selectedNode->GetData());

  //mitk::Surface::Pointer surface2 = static_cast<mitk::Surface*>(selectedNode->GetData());

  int numVertices = m_Controls->numVerticesSpinBox->value();
  double gradation = m_Controls->gradationSpinBox->value();
  int subsampling = m_Controls->subsamplingSpinBox->value();
  double edgeSplitting = m_Controls->edgeSplittingSpinBox->value();
  int optimizationLevel = m_Controls->optimizationLevelSpinBox->value();
  bool forceManifold = m_Controls->forceManifoldCheckBox->isChecked();
  bool boundaryFixing = m_Controls->boundaryFixingCheckBox->isChecked();

  //mitk::Surface::Pointer remeshedSurface = mitk::ACVD::Remesh(surface, 0, numVertices, gradation, subsampling, edgeSplitting, optimizationLevel, forceManifold, boundaryFixing);

  
  
  mitk::Surface::Pointer mesh_surface = mitk::Surface::Pointer();
 
  mesh_surface = m_remesher->GenerateMesh(surface, 0, numVertices, gradation, subsampling, edgeSplitting, optimizationLevel, forceManifold, boundaryFixing);

  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetName(QString("%1 (%2, %3)").arg(selectedNode->GetName().c_str()).arg(mesh_surface->GetVtkPolyData()->GetNumberOfPoints()).arg(gradation).toStdString());
  newNode->SetProperty("material.representation", mitk::VtkRepresentationProperty::New(VTK_WIREFRAME));
  newNode->SetProperty("material.specularCoefficient", mitk::FloatProperty::New(0.0f));
  newNode->SetData(mesh_surface);

  m_DataStorage->Add(newNode, selectedNode);


  //CREATE SURFACE MAPPER
  
  /* m_mapper = vtkPolyDataMapper::New();
   m_mapper->SetInputData(surface2->GetVtkPolyData());

   vtkSmartPointer<vtkActor> m_actor = vtkSmartPointer<vtkActor>::New();
   m_actor->SetMapper(m_mapper);

   m_renderer->AddActor(m_actor);

   m_mapper->Update();*/

  }

void QmitkMultiDevicemonitoringRemeshingWidget::OnSelectedSurfaceChanged(const mitk::DataNode *node)
{
  if (node != NULL && node->GetData() != NULL)
  {
		  int numVertices = static_cast<int>(static_cast<mitk::Surface*>(node->GetData())->GetVtkPolyData()->GetNumberOfPoints());
		  int minimum = numVertices < 100 ? numVertices : 100;
		  int maximum = numVertices == minimum ? numVertices * 10 : numVertices;
		  int step = std::max(1, maximum / 10);

		  this->SetNumberOfVertices(minimum, maximum, step, numVertices);
		  this->EnableWidgets(true);
  }
  else
  {
    this->EnableWidgets(false);
    this->SetNumberOfVertices(0, 0, 0, 0);
  }
}

void QmitkMultiDevicemonitoringRemeshingWidget::OnSubsamplingChanged(int subsampling)
{
  if (subsampling != m_Controls->subsamplingSlider->value())
    m_Controls->subsamplingSlider->setValue(subsampling);

  if (subsampling != m_Controls->subsamplingSpinBox->value())
    m_Controls->subsamplingSpinBox->setValue(subsampling);
}

void QmitkMultiDevicemonitoringRemeshingWidget::SetNumberOfVertices(int minimum, int maximum, int step, int value)
{
  m_Controls->numVerticesSlider->setMinimum(minimum);
  m_Controls->numVerticesSlider->setMaximum(maximum);
  m_Controls->numVerticesSlider->setSingleStep(1);
  m_Controls->numVerticesSlider->setPageStep(step);

  m_Controls->numVerticesSpinBox->setMinimum(minimum);
  m_Controls->numVerticesSpinBox->setMaximum(maximum);
  m_Controls->numVerticesSpinBox->setSingleStep(step);
  m_Controls->numVerticesSpinBox->setValue(value);

  m_Controls->maxNumVerticesLineEdit->setText(QString("%1").arg(maximum));
}

void QmitkMultiDevicemonitoringRemeshingWidget::InitiateDistanceControl(mitk::DataNode::Pointer node)
{
	/*
	vtkSmartPointer<vtkCleanPolyData> clean1 = vtkSmartPointer<vtkCleanPolyData>::New();
	vtkSmartPointer<vtkCleanPolyData> clean2 = vtkSmartPointer<vtkCleanPolyData>::New();

	clean1->SetInputData(m_remesher->GetMeshSurface());


	vtkSmartPointer<vtkSphereSource> sphereSource2 = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource2->SetCenter(ToolNavigationPosition[0], ToolNavigationPosition[1], ToolNavigationPosition[2]);
	sphereSource2->SetRadius(0.1);
	sphereSource2->Update();
	clean2->SetInputData(sphereSource2->GetOutput());

	m_distanceFilter->SetInputConnection(0, clean1->GetOutputPort());
	m_distanceFilter->SetInputConnection(1, clean2->GetOutputPort());
	m_distanceFilter->Update();

	double minimum_distance = m_distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[0];

	//m_mapper = vtkPolyDataMapper::New();
	//m_mapper->SetInputConnection(m_distanceFilter->GetOutputPort());
	//m_mapper->SetInputData(m_distanceFilter->GetOutput());
	//m_mapper->SetScalarRange(m_distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[0],
		//m_distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[1]);

	//m_mapper->SetScalarRange(2,1000);
	
	//vtkSmartPointer<vtkActor> m_actor = vtkSmartPointer<vtkActor>::New();
	//m_actor->SetMapper(m_mapper);

	
	//vtkSmartPointer<vtkScalarBarActor> m_scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();

	//m_renderer->AddActor(m_actor);
	*/

	//Find closest point on surface
	
	mitk::Surface::Pointer surface = static_cast<mitk::Surface*>(node->GetData());

	//First option: point locator
	//m_Locator = mitk::PointLocator::New();
	//m_Locator->SetPoints(m_remesher->GetMeshSurface());

	//Second option: cell locator
	
	m_CellLocator = vtkSmartPointer<vtkCellLocator>::New();
	m_CellLocator->SetDataSet(surface->GetVtkPolyData());
	m_CellLocator->BuildLocator();

	return;
}

Distance_Data* QmitkMultiDevicemonitoringRemeshingWidget::RealTimeControlOfDistance(mitk::Point3D ToolNavigationPosition)
{
	Distance_Data *distance_data = new Distance_Data();

	double *Nav_pos = new double[3];
	Nav_pos[0] = ToolNavigationPosition[0];
	Nav_pos[1] = ToolNavigationPosition[1];
	Nav_pos[2] = ToolNavigationPosition[2];
	
	//First option: find the closest points to Navigation 3D point
	/*
	int closestPointId = m_Locator->FindClosestPoint(ToolNavigationPosition[0], ToolNavigationPosition[1], ToolNavigationPosition[2]);
	distance_data->closest_point = m_remesher->GetMeshSurface()->GetPoint(closestPointId);
	distance_data->distance = sqrt((distance_data->closest_point[0] - Nav_pos[0])*(distance_data->closest_point[0] - Nav_pos[0]) + (distance_data->closest_point[1] - Nav_pos[1])*(distance_data->closest_point[1] - Nav_pos[1]) + (distance_data->closest_point[2] - Nav_pos[2]) *(distance_data->closest_point[2] - Nav_pos[2]));
	std::cout << "1a->distance_data->closest_point: " << endl;
	std::cout << distance_data->closest_point[0] << "; " << distance_data->closest_point[1] << "; " << distance_data->closest_point[2] << endl;
	std::cout << "1b->Minimum distance: " << endl;
	std::cout << distance_data->distance << endl;
	std::cout << endl;
	*/
	//Second option: find the closest cell points to Navigation 3D point
	
	double* closestCellPoint = new double[3];//the coordinates of the closest point will be returned here

	
	double closestCellPointDistance2; //the squared distance to the closest point will be returned here
	vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
	vtkGenericCell* cell = vtkGenericCell::New();
	int subId; //this is rarely used (in triangle strips only, I believe)

	m_CellLocator->FindClosestPoint(Nav_pos, closestCellPoint, cell, cellId, subId, closestCellPointDistance2);

	distance_data->closest_point = closestCellPoint;
	distance_data->distance = sqrt(closestCellPointDistance2);

	cell->Delete();

	std::cout << "2a->distance_data->closest_point in cell: " << endl;
	std::cout << distance_data->closest_point[0] << "; " << distance_data->closest_point[1] << "; " << distance_data->closest_point[2] << endl;
	std::cout << "2b->Minimum distance: " << endl;
	std::cout << distance_data->distance << endl;
	std::cout << endl;
	
	return distance_data;
}