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

#include "QmitkMaxillofacialRemeshingWidget.h"

// MITK

#include <MITKMaxillofacialMeshLab.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkVtkRepresentationProperty.h>
#include <mitkPointLocator.h>

#include <itkCommand.h>
#include <itkPoint.h>
#include <mitkMessage.h>

// Qt
#include <QIntValidator>

// VTK
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkLine.h>
#include <vtkDistancePolyDataFilter.h>
#include <vtkCleanPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataReader.h>
#include <vtkSphereSource.h>
#include <vtkScalarBarActor.h>
#include <vtkIntersectionPolyDataFilter.h>
#include <vtkArray.h>
#include <vtkPointLocator.h>

// C++ Standard Library
#include <algorithm>
#include <limits>



/* VIEW MANAGEMENT */
QmitkMaxillofacialRemeshingWidget::QmitkMaxillofacialRemeshingWidget(QWidget* parent)
: QWidget(parent), m_Controls(NULL),m_MultiWidget(NULL)
{
  CreateQtPartControl(this);
  m_remesher = mitk::ACVD::RemeshFilter::New();
  m_distanceFilter = vtkSmartPointer<vtkDistancePolyDataFilter>::New();

}

QmitkMaxillofacialRemeshingWidget::~QmitkMaxillofacialRemeshingWidget()
{
}

//#PUBLIC SETTER
void QmitkMaxillofacialRemeshingWidget::SetDataStorage(mitk::DataStorage::Pointer _DataStorage)
{
	// reset only if datastorage really changed
	if (m_DataStorage.GetPointer() != _DataStorage)
	{
			// set new storage
			m_DataStorage = _DataStorage;
			if (m_DataStorage.IsNotNull())
			{
				m_Controls->surfaceComboBox->SetDataStorage(m_DataStorage);
				m_Controls->surfaceComboBox->SetAutoSelectNewItems(false);
				m_Controls->surfaceComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));
			}		
	}
}

// #PUBLIC SETTER
void QmitkMaxillofacialRemeshingWidget::SetRenderer(vtkSmartPointer<vtkRenderWindow> _RenderWindow)
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

void QmitkMaxillofacialRemeshingWidget::CreateQtPartControl(QWidget* parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkMaxillofacialRemeshingWidget;
    m_Controls->setupUi(parent);

    this->CreateConnections();
	this->EnableWidgets(m_Controls->surfaceComboBox->GetSelectedNode().IsNotNull());
	this->OnAdvancedSettingsButtonToggled(false);
  }

  this->OnSelectedSurfaceChanged(m_Controls->surfaceComboBox->GetSelectedNode());
}
void QmitkMaxillofacialRemeshingWidget::CreateConnections()
{
  connect(m_Controls->surfaceComboBox, SIGNAL(OnSelectionChanged(const mitk::DataNode *)), this, SLOT(OnSelectedSurfaceChanged(const mitk::DataNode *)));
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

void QmitkMaxillofacialRemeshingWidget::EnableWidgets(bool enable)
{
	m_Controls->surfaceComboBox->setEnabled(enable);
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

void QmitkMaxillofacialRemeshingWidget::OnAdvancedSettingsButtonToggled(bool toggled)
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

void QmitkMaxillofacialRemeshingWidget::OnEdgeSplittingChanged(double edgeSplitting)
{
  if (edgeSplitting != m_Controls->edgeSplittingSlider->value())
    m_Controls->edgeSplittingSlider->setValue(edgeSplitting);

  if (edgeSplitting != m_Controls->edgeSplittingSpinBox->value())
    m_Controls->edgeSplittingSpinBox->setValue(edgeSplitting);
}

void QmitkMaxillofacialRemeshingWidget::OnGradationChanged(double gradation)
{
  if (gradation != m_Controls->gradationSlider->value())
    m_Controls->gradationSlider->setValue(gradation);

  if (gradation != m_Controls->gradationSpinBox->value())
    m_Controls->gradationSpinBox->setValue(gradation);
}

void QmitkMaxillofacialRemeshingWidget::OnMaxNumVerticesLineEditEditingFinished()
{
  int maximum = m_Controls->maxNumVerticesLineEdit->text().toInt();

  if (m_Controls->numVerticesSpinBox->maximum() != maximum)
  {
    m_Controls->numVerticesSlider->setMaximum(maximum);
    m_Controls->numVerticesSpinBox->setMaximum(maximum);
  }
}

void QmitkMaxillofacialRemeshingWidget::OnNumberOfVerticesChanged(int numVertices)
{
  if (numVertices != m_Controls->numVerticesSlider->value())
    m_Controls->numVerticesSlider->setValue(numVertices);

  if (numVertices != m_Controls->numVerticesSpinBox->value())
    m_Controls->numVerticesSpinBox->setValue(numVertices);
}

void QmitkMaxillofacialRemeshingWidget::OnOptimizationLevelChanged(int optimizationLevel)
{
  if (optimizationLevel != m_Controls->optimizationLevelSlider->value())
    m_Controls->optimizationLevelSlider->setValue(optimizationLevel);

  if (optimizationLevel != m_Controls->optimizationLevelSpinBox->value())
    m_Controls->optimizationLevelSpinBox->setValue(optimizationLevel);
}

void QmitkMaxillofacialRemeshingWidget::OnRemeshButtonClicked()
{
  mitk::DataNode::Pointer selectedNode = m_Controls->surfaceComboBox->GetSelectedNode();
  mitk::Surface::ConstPointer surface = static_cast<mitk::Surface*>(selectedNode->GetData());
  int numVertices = m_Controls->numVerticesSpinBox->value();
  double gradation = m_Controls->gradationSpinBox->value();
  int subsampling = m_Controls->subsamplingSpinBox->value();
  double edgeSplitting = m_Controls->edgeSplittingSpinBox->value();
  int optimizationLevel = m_Controls->optimizationLevelSpinBox->value();
  bool forceManifold = m_Controls->forceManifoldCheckBox->isChecked();
  bool boundaryFixing = m_Controls->boundaryFixingCheckBox->isChecked();

  //mitk::Surface::Pointer remeshedSurface = mitk::ACVD::Remesh(surface, 0, numVertices, gradation, subsampling, edgeSplitting, optimizationLevel, forceManifold, boundaryFixing);

  
  
  mitk::Surface::Pointer mesh_surface = mitk::Surface::Pointer();
  //sgroup = m_remesher->GenerateMeshWithMargin(surface, 0, numVertices, gradation, subsampling, edgeSplitting, optimizationLevel, forceManifold, boundaryFixing);
  mesh_surface = m_remesher->GenerateMesh(surface, 0, numVertices, gradation, subsampling, edgeSplitting, optimizationLevel, forceManifold, boundaryFixing);

  mitk::DataNode::Pointer newNode = mitk::DataNode::New();
  newNode->SetName(QString("%1 (%2, %3)").arg(selectedNode->GetName().c_str()).arg(mesh_surface->GetVtkPolyData()->GetNumberOfPoints()).arg(gradation).toStdString());
  newNode->SetProperty("material.representation", mitk::VtkRepresentationProperty::New(VTK_WIREFRAME));
  //newNode->SetProperty("material.specularCoefficient", mitk::FloatProperty::New(0.0f));
  newNode->SetData(mesh_surface);

  m_DataStorage->Add(newNode, selectedNode);
  
  }

void QmitkMaxillofacialRemeshingWidget::OnSelectedSurfaceChanged(const mitk::DataNode *node)
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

void QmitkMaxillofacialRemeshingWidget::OnSubsamplingChanged(int subsampling)
{
  if (subsampling != m_Controls->subsamplingSlider->value())
    m_Controls->subsamplingSlider->setValue(subsampling);

  if (subsampling != m_Controls->subsamplingSpinBox->value())
    m_Controls->subsamplingSpinBox->setValue(subsampling);
}

void QmitkMaxillofacialRemeshingWidget::SetNumberOfVertices(int minimum, int maximum, int step, int value)
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

double* QmitkMaxillofacialRemeshingWidget::CreateDistanceMatrix(mitk::Point3D ToolNavigationPosition)
{
	/*mitk::Point3D P = ToolNavigationPosition;
	unsigned int indexOfminimumDistance = m_remesher->CreateControlDistanceMatrix(P, m_remesher->GetMeshSurface());
	//Create line between tool point and closest point in surface
	std::cout << "Index of minimum distance: " << endl;
	std::cout << indexOfminimumDistance << endl;*/
	//double *S = m_remesher->GetMeshSurface()->GetPoint(indexOfminimumDistance);
	double *S = new double[3];

	double *Nav_pos = new double[3];
	Nav_pos[0] = ToolNavigationPosition[0];
	Nav_pos[1] = ToolNavigationPosition[1];
	Nav_pos[2] = ToolNavigationPosition[2];

	
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

	//Find closest point on surface

	m_Locator = mitk::PointLocator::New();
	m_Locator->SetPoints(m_remesher->GetMeshSurface());
	int distance;
	int closestPointId = m_Locator->FindClosestPoint(ToolNavigationPosition[0], ToolNavigationPosition[1], ToolNavigationPosition[2]);
	
	std::cout << "Index of closest point: " << endl;
	std::cout << closestPointId << endl;

	return S;
}

Distance_Data* QmitkMaxillofacialRemeshingWidget::ControlOfDistance(mitk::Point3D ToolNavigationPosition)
{
	Distance_Data *distance_data = new Distance_Data();

	double *Nav_pos = new double[3];
	Nav_pos[0] = ToolNavigationPosition[0];
	Nav_pos[1] = ToolNavigationPosition[1];
	Nav_pos[2] = ToolNavigationPosition[2];


	/*
	vtkSmartPointer<vtkCleanPolyData> clean1 = vtkSmartPointer<vtkCleanPolyData>::New();
	vtkSmartPointer<vtkCleanPolyData> clean2 = vtkSmartPointer<vtkCleanPolyData>::New();

	clean1->SetInputData(m_remesher->GetMeshSurface());
	

	vtkSmartPointer<vtkSphereSource> sphereSource1 = vtkSmartPointer<vtkSphereSource>::New();
	sphereSource1->SetCenter(ToolNavigationPosition[0], ToolNavigationPosition[1], ToolNavigationPosition[2]);
	sphereSource1->SetRadius(0.1);
	sphereSource1->Update();
	clean2->SetInputData(sphereSource1->GetOutput());

	m_distanceFilter->SetInputConnection(0, clean1->GetOutputPort());
	m_distanceFilter->SetInputConnection(1, clean2->GetOutputPort());
	m_distanceFilter->SignedDistanceOff();
	
	m_distanceFilter->Update();
	
	vtkSmartPointer<vtkPolyData> pd = m_distanceFilter->GetOutput();
	
	double minimum_distance = m_distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[0];

	std::cout << "New minimum distance: " << endl;
	std::cout << minimum_distance << endl;
	*/
	//std::cout << "New point array: " << endl;
	//std::cout << m_distanceFilter->GetOutput(1)->GetPointData()->GetArray("Points") << endl;
	//Try with VTK Point Locator
	//vtkPointLocator::FindClosestPoint
	/*vtkSmartPointer<vtkPointLocator> Loc = vtkSmartPointer<vtkPointLocator>::New();
	Loc->SetDataSet(m_remesher->GetMeshSurface());
	vtkSmartPointer<vtkIdList> closest_points = vtkSmartPointer<vtkIdList>::New();
	Loc->FindPointsWithinRadius(minimum_distance, Nav_pos, closest_points);
	double* point = Loc->GetDataSet()->GetPoint(closest_points->GetId(0));*/
	
	int closestPointId = m_Locator->FindClosestPoint(ToolNavigationPosition[0], ToolNavigationPosition[1], ToolNavigationPosition[2]);
	std::cout << "Index of closest point: " << endl;
	std::cout << closestPointId << endl;
	//distance_data->distance = m_distanceFilter->GetOutput()->GetPointData()->GetScalars()->GetRange()[0];
	distance_data->closest_point = m_remesher->GetMeshSurface()->GetPoint(closestPointId);
	
	distance_data->distance = sqrt((distance_data->closest_point[0] - Nav_pos[0])*(distance_data->closest_point[0] - Nav_pos[0]) + (distance_data->closest_point[1] - Nav_pos[1])*(distance_data->closest_point[1] - Nav_pos[1]) + (distance_data->closest_point[2] - Nav_pos[2]) *(distance_data->closest_point[2] - Nav_pos[2]));
  return distance_data;
}