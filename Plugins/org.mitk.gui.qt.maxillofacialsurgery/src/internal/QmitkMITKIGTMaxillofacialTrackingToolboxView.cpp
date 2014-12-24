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

// Blueberry
#include <berryISelectionService.h>
#include <berryIWorkbenchWindow.h>

// Qmitk
#include "QmitkMITKIGTMaxillofacialTrackingToolboxView.h"

#include <QmitkNDIConfigurationWidget.h>
#include <QmitkUpdateTimerWidget.h>
#include <QmitkToolSelectionWidget.h>
#include <QmitkToolTrackingStatusWidget.h>

// Qt
#include <QMessageBox>
#include <qfiledialog.h>

// MITK
#include <mitkNavigationToolStorageDeserializer.h>
#include <mitkTrackingDeviceSourceConfigurator.h>
#include <mitkTrackingVolumeGenerator.h>

#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNavigationToolStorageSerializer.h>


// vtk
#include <vtkSphereSource.h>

//for exceptions
#include <mitkIGTException.h>
#include <mitkIGTIOException.h>

//Tracking
#include "mitkVirtualTrackingDevice.h"
#include "mitkTrackingDeviceSource.h"

//STL Reader

#include <mitkSTLFileReader.h>

const std::string QmitkMITKIGTMaxillofacialTrackingToolboxView::VIEW_ID = "org.mitk.views.QMITKMITKIGTMaxillofacialTrackingToolbox";

QmitkMITKIGTMaxillofacialTrackingToolboxView::QmitkMITKIGTMaxillofacialTrackingToolboxView()
	: QmitkAbstractView()
, m_Controls( 0 )
{
  //Initialize tracking elements
  m_TrackingTimer = new QTimer(this);
  m_tracking = false;
  
  //Initialize logging elements
  m_logging = false;
  m_loggedFrames = 0;
 
  
  //Initialize registration elements
  m_ImageFiducialsDataNode = NULL;
  m_TrackerFiducialsDataNode = NULL;
  
  //Initialize PointSetRecording elements  
  m_PSRecordingPointSet = NULL;
  m_PointSetRecording = false;

  //Initialize PointSetRecording elements  
  m_DistanceControl = false;
  m_DistanceLinePointSet = NULL;
  m_PermanentRegistration = false;
  
  //Initialize camera view elements
  m_VirtualTracking = false;
  
  //Initialize elements for reference marker 
  m_ThereIsAReference = false;
  m_Reference_Orientation_Inverse = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
  m_Original_Reference_Orientation_Inverse = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
  m_TotalOrientationTransform = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();

 

  //Initialize distance calculation elements
  m_ReferencePositionDataNode = NULL;
  m_ReferencePositionDataNode = NULL;
}

QmitkMITKIGTMaxillofacialTrackingToolboxView::~QmitkMITKIGTMaxillofacialTrackingToolboxView()
{
//remove the tracking volume
this->GetDataStorage()->Remove(m_TrackingVolumeNode);
//remove the tool storage
m_toolStorage->UnRegisterMicroservice();
}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::CreateQtPartControl( QWidget *parent )
{
  // build up qt view, unless already done
  if ( !m_Controls )
  {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls = new Ui::QmitkMITKIGTMaxillofacialTrackingToolboxViewControls;
    m_Controls->setupUi( parent );

    //Tracking device connections
    connect( m_Controls->m_Connect, SIGNAL(clicked()), this, SLOT(OnConnect()) );
    connect( m_Controls->m_Disconnect, SIGNAL(clicked()), this, SLOT(OnDisconnect()) );
    connect( m_Controls->m_StartTracking, SIGNAL(clicked()), this, SLOT(OnStartTracking()) );
    connect( m_Controls->m_StopTracking, SIGNAL(clicked()), this, SLOT(OnStopTracking()) );
    connect( m_TrackingTimer, SIGNAL(timeout()), this, SLOT(UpdateTrackingTimer()));
	
	//Tracking tool connections
	//connect(m_Controls->m_LoadTools, SIGNAL(clicked()), this, SLOT(OnLoadTools()));
	connect(m_Controls->m_AddSingleTool, SIGNAL(clicked()), this, SLOT(OnAddSingleTool()));
	//connect(m_Controls->m_AutoDetectTools, SIGNAL(clicked()), this, SLOT(OnAutoDetectTools()));
	connect(m_Controls->m_ResetTools, SIGNAL(clicked()), this, SLOT(OnResetTools()));
	connect(m_Controls->m_NavigationToolCreationWidget, SIGNAL(NavigationToolFinished()), this, SLOT(OnAddSingleToolFinished()));
	connect(m_Controls->m_NavigationToolCreationWidget, SIGNAL(Canceled()), this, SLOT(OnAddSingleToolCanceled()));
	connect(m_Controls->m_configurationWidget, SIGNAL(TrackingDeviceSelectionChanged()), this, SLOT(OnTrackingDeviceChanged()));

	//added the option to set whether a tool a reference marker or not
	connect(m_Controls->m_ToolIsReferenceFramework, SIGNAL(toggled(bool)), this, SLOT(OnSetAsReferenceFramework(bool)));

	//added the option of performing virtual tracking
	connect(m_Controls->m_PerformVirtualTracking, SIGNAL(toggled(bool)), this, SLOT(OnPerformVirtualTracking(bool)));

	//create connections for Registration tab
	connect(m_Controls->m_UsePermanentRegistrationToggle, SIGNAL(toggled(bool)), this, SLOT(OnApplyRegistration(bool)));
	connect(m_Controls->m_TrackingDeviceSelectionWidget, SIGNAL(NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupNavigation()));
	connect(m_Controls->m_TrackingDeviceSelectionWidget, SIGNAL(NavigationToolSelected()), this, SLOT(OnSetupTool()));
	

	//allow to choose a surface that will be permanently associated with the selected tool
	connect(m_Controls->m_RegisterToolWithSurface, SIGNAL(toggled(bool)), this, SLOT(OnRegisterToolWithSurface(bool)));

	//create connection for saving tool data before registration
	connect(m_Controls->m_AcceptToolData, SIGNAL(clicked()), this, SLOT(OnAcceptToolData()));
	//connect(m_Controls->m_UseAsPointerButton, SIGNAL(clicked()), this, SLOT(OnInstrumentSelected()));
	//connect(m_Controls->m_UseAsObjectmarkerButton, SIGNAL(clicked()), this, SLOT(OnObjectmarkerSelected()));
	connect(m_Controls->m_RegistrationWidget, SIGNAL(AddedTrackingFiducial()), this, SLOT(OnAddRegistrationTrackingFiducial()));
	connect(m_Controls->m_RegistrationWidget, SIGNAL(PerformFiducialRegistration()), this, SLOT(OnCalculateRegistration()));
	
	
	
	connect(m_Controls->m_PointSetRecordCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnPointSetRecording(bool)));
	connect(m_Controls->m_DistanceControlCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnDistanceControl(bool)));
	connect(m_Controls->m_DistanceCalculationBetweenPointsWidget, SIGNAL(AddedReferencePosition()), this, SLOT(OnAddReferencePosition()));
	connect(m_Controls->m_DistanceCalculationBetweenPointsWidget, SIGNAL(AddedMeasurementPosition()), this, SLOT(OnAddMeasurementPosition()));
	connect(m_Controls->m_DistanceCalculationBetweenPointsWidget, SIGNAL(DemandDistanceCalculation()), this, SLOT(OnCalculateDistanceBetweenPoints()));
	connect(m_Controls->m_TrajectoryControlToolSelectionWidget, SIGNAL(NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupToolForTrajectoryControl()));
	connect(m_Controls->m_TrajectoryControlToolSelectionWidget, SIGNAL(NavigationToolSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupTool()));


	connect(m_Controls->m_RegistrationSurfaceComboBox, SIGNAL(OnSelectionChanged(const mitk::DataNode *)), this, SLOT(m_RegistrationSurfaceChanged(const mitk::DataNode *)));
	connect(m_Controls->m_ToolToSurfaceRegistration, SIGNAL(toggled(bool)), this, SLOT(OnToolToSurfaceRegistration(bool)));
	connect(m_Controls->m_GeneralRegistration, SIGNAL(toggled(bool)), this, SLOT(OnGeneralRegistration(bool)));

	//create connection for saving transform into a file
	connect(m_Controls->m_ChooseTransformFile, SIGNAL(clicked()), this, SLOT(OnChooseTransformFileClicked()));

    //create connections for logging tab
	connect(m_Controls->m_ChooseFile, SIGNAL(clicked()), this, SLOT(OnChooseFileClicked()));
    connect(m_Controls->m_StartLogging, SIGNAL(clicked()), this, SLOT(StartLogging()));
    connect(m_Controls->m_StopLogging, SIGNAL(clicked()), this, SLOT(StopLogging()));
    connect(m_Controls->m_csvFormat, SIGNAL(clicked()), this, SLOT(OnToggleFileExtension()));
    connect(m_Controls->m_xmlFormat, SIGNAL(clicked()), this, SLOT(OnToggleFileExtension()));


	//initialize widgets
    m_Controls->m_configurationWidget->EnableAdvancedUserControl(false);
    m_Controls->m_TrackingToolsStatusWidget->SetShowPositions(true);
    m_Controls->m_TrackingToolsStatusWidget->SetTextAlignment(Qt::AlignLeft);
	
	
	CreateBundleWidgets(parent);
   
	//Tracking volume connections. TO-DO REMOVE
	connect(m_Controls->m_VolumeSelectionBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(OnTrackingVolumeChanged(QString)));
	connect(m_Controls->m_ShowTrackingVolume, SIGNAL(clicked()), this, SLOT(OnShowTrackingVolumeChanged()));

	//initialize tracking volume node. TO-DO REMOVE
    m_TrackingVolumeNode = mitk::DataNode::New();
    m_TrackingVolumeNode->SetName("TrackingVolume");
    m_TrackingVolumeNode->SetOpacity(0.15);
    m_TrackingVolumeNode->SetBoolProperty("Backface Culling",true);
    mitk::Color red;
    red.SetRed(1);
    m_TrackingVolumeNode->SetColor(red);
    GetDataStorage()->Add(m_TrackingVolumeNode);

    //initialize buttons
    m_Controls->m_Connect->setEnabled(true);
    m_Controls->m_Disconnect->setEnabled(false);
    m_Controls->m_StartTracking->setEnabled(false);
    m_Controls->m_StopTracking->setEnabled(false);
    //m_Controls->m_AutoDetectTools->setVisible(false); //only visible if tracking device is Aurora
	//m_Controls->m_LoadTools->setEnabled(false);
	//m_Controls->m_LoadTools->setVisible(false);

    //Update List of available models for selected tool.
    std::vector<mitk::TrackingDeviceData> Compatibles = mitk::GetDeviceDataForLine( m_Controls->m_configurationWidget->GetTrackingDevice()->GetType());
    m_Controls->m_VolumeSelectionBox->clear();
    for(int i = 0; i < Compatibles.size(); i++)
    {
      m_Controls->m_VolumeSelectionBox->addItem(Compatibles[i].Model.c_str());
    }

    //initialize tool storage
    m_toolStorage = mitk::NavigationToolStorage::New(GetDataStorage());
    m_toolStorage->SetName("MaxillofacialTrackingToolbox Default Storage");
    m_toolStorage->RegisterAsMicroservice("no tracking device");

    //set home directory as default path for logfile
    m_Controls->m_LoggingFileName->setText(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + "logfile.csv");
  
	//set home directory as default path for logfile
	m_Controls->m_TransformFileName->setText(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + "transform.csv");


	m_Controls->m_ToolForPositioningComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_ToolForPositioningComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_ToolForPositioningComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));

	//The TargetSurfaceComboBox includes all the surface models that are currently in the DataStorage.
	m_Controls->m_TargetSurfaceComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_TargetSurfaceComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_TargetSurfaceComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));

	m_Controls->m_RegistrationSurfaceComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_RegistrationSurfaceComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_RegistrationSurfaceComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));

	m_Controls->m_ControlSurfaceComboBox->SetDataStorage(m_toolStorage->GetDataStorage());
	m_Controls->m_ControlSurfaceComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_ControlSurfaceComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface")); 

	m_Controls->m_MovingSurfaceComboBox->SetDataStorage(m_toolStorage->GetDataStorage());
	m_Controls->m_MovingSurfaceComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_MovingSurfaceComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));
	
	connect(m_Controls->m_CalculateSurfaceToSurfaceDisalignmentBtn, SIGNAL(clicked()), this, SLOT(OnSurfaceToSurfaceDisalignmentControl()));

	//The RemeshingWidget must consider all the surface models that are currently in the DataStorage.
	m_Controls->m_RemeshingWidget->SetDataStorage(this->GetDataStorage());
	
  }
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::LoadModel(std::string filename)
{
	mitk::STLFileReader::Pointer stlReader = mitk::STLFileReader::New();
	try
	{
		stlReader->SetFileName(filename.c_str());
		stlReader->Update();
	}
	catch (...)
	{
	}

	if (stlReader->GetOutput() == NULL);
	else
	{
		mitk::DataNode::Pointer newNode = mitk::DataNode::New();
		newNode->SetName(filename);
		newNode->SetData(stlReader->GetOutput());
		this->GetDataStorage()->Add(newNode);
		newNode->SetBoolProperty("updateDataOnRender", false);
	}
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnPreloadSettings()
{
	std::string ToolName;
	std::string ToolDefinitionFile;
	std::string ToolRepresentationObject;
	//TO-DO: Read xml
	
	//Set Tracking device values
	std::string CalibrationFile = "J:/Optitrack/Calibration/Calibration.cal";
	
	m_Controls->m_configurationWidget->SetOptitrackCalibrationFile(CalibrationFile);
	
	
	OnResetTools();
	//Set Tool1 values
	ToolName = "Pointer";
	ToolDefinitionFile = "J:/Optitrack/Tool_definition/PunteroPolaris.txt";
	ToolRepresentationObject = "J:/Optitrack/Tool_definition/ndi_polaris_tool.stl";
	
	this->OnAddSingleTool();
	this->m_Controls->m_NavigationToolCreationWidget->PreloadToolSettings(ToolName, ToolDefinitionFile, ToolRepresentationObject);
	
	this->m_Controls->m_NavigationToolCreationWidget->AddToolFinished();
	this->OnAddSingleToolFinished();
	
	
	ToolName = "RigidBody";
	ToolDefinitionFile = "J:/Optitrack/RigidBody_definition/RigidBody.txt";
	ToolRepresentationObject = "J:/Optitrack/RigidBody_definition/RigidBody.stl";


	this->OnAddSingleTool();
	
	this->m_Controls->m_NavigationToolCreationWidget->PreloadToolSettings(ToolName, ToolDefinitionFile, ToolRepresentationObject);


	this->m_Controls->m_NavigationToolCreationWidget->AddToolFinished();
	this->OnAddSingleToolFinished();
	m_Controls->m_ShowTrackingVolume->setChecked(false);

	this->OnConnect();
	this->OnStartTracking();
	
	
	m_InstrumentNavigationData = m_TrackingDeviceSource->GetOutput(0);
	
	InitializeRegistration();


	//Open surface model
	std::string m_SurfaceName = "J:/External_resources/Mandibula/Jaw.stl";
	LoadModel(m_SurfaceName);
	
 	GlobalReinit();

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnLoadTools()
{
  //read in filename
  QString filename = QFileDialog::getOpenFileName(NULL,tr("Open Tool Storage"), "/", tr("Tool Storage Files (*.IGTToolStorage)"));
  if (filename.isNull()) return;

  //read tool storage from disk
  std::string errorMessage = "";
  mitk::NavigationToolStorageDeserializer::Pointer myDeserializer = mitk::NavigationToolStorageDeserializer::New(GetDataStorage());
  // try-catch block for exceptions
  try
  {
    this->ReplaceCurrentToolStorage(myDeserializer->Deserialize(filename.toStdString()),filename.toStdString());
  }
  catch(mitk::IGTException)
  {
   std::string errormessage = "Error during deserializing. Problems with file,please check the file?";
   QMessageBox::warning(NULL, "IGTPlayer: Error", errormessage.c_str());
   return;
  }

  if(m_toolStorage->isEmpty())
    {
    errorMessage = myDeserializer->GetErrorMessage();
    MessageBox(errorMessage);
    return;
    }

  //update label
  Poco::Path myPath = Poco::Path(filename.toStdString()); //use this to seperate filename from path
  QString toolLabel = QString("Loaded Tools: ") + QString::number(m_toolStorage->GetToolCount()) + " Tools from " + myPath.getFileName().c_str();
  m_Controls->m_toolLabel->setText(toolLabel);

  //update tool preview
  m_Controls->m_TrackingToolsStatusWidget->RemoveStatusLabels();
  m_Controls->m_TrackingToolsStatusWidget->PreShowTools(m_toolStorage);

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnResetTools()
{
  this->ReplaceCurrentToolStorage(mitk::NavigationToolStorage::New(GetDataStorage()),"MaxillofacialTrackingToolbox Default Storage");
  m_Controls->m_TrackingToolsStatusWidget->RemoveStatusLabels();
  QString toolLabel = QString("Loaded Tools: <none>");
  m_Controls->m_toolLabel->setText(toolLabel);
  
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnConnect()
  {

	if (this->GetRenderWindowPart() != NULL)
	{
		m_Controls->m_RemeshingWidget->SetRenderer(this->GetRenderWindowPart()->GetQmitkRenderWindow("3d")->GetVtkRenderWindow());
	}
  
	//check if the Virtual Tracking option is enabled. If so, perform virtual tracking
	if (m_VirtualTracking)
	{
		OnVirtualConnect();
		return;
	}

  
	//check if everything is ready to start tracking	
if (this->m_toolStorage.IsNull())
  {
    MessageBox("Error: No Tools Loaded Yet!");
    return;
  }
  else if (this->m_toolStorage->GetToolCount() == 0)
  {
    MessageBox("Error: No Way To Track Without Tools!");
    return;
  }

  //build the IGT pipeline
  mitk::TrackingDevice::Pointer trackingDevice = this->m_Controls->m_configurationWidget->GetTrackingDevice();
  trackingDevice->SetData(m_TrackingDeviceData);

  //Get Tracking Volume Data
  mitk::TrackingDeviceData data = mitk::DeviceDataUnspecified;

  QString qstr =  m_Controls->m_VolumeSelectionBox->currentText();
  if ( (! qstr.isNull()) || (! qstr.isEmpty()) ) {
    std::string str = qstr.toStdString();
    data = mitk::GetDeviceDataByName(str); //Data will be set later, after device generation
  }

  //Create Navigation Data Source with the factory class
  mitk::TrackingDeviceSourceConfigurator::Pointer myTrackingDeviceSourceFactory = mitk::TrackingDeviceSourceConfigurator::New(this->m_toolStorage,trackingDevice);

  m_TrackingDeviceSource = myTrackingDeviceSourceFactory->CreateTrackingDeviceSource();
  

  m_ToolVisualizationFilter = mitk::NavigationDataObjectVisualizationFilter::New();
  
  for (unsigned int i = 0; i<m_TrackingDeviceSource->GetNumberOfIndexedOutputs(); i++)
  {
	  mitk::NavigationTool::Pointer currentTool = this->m_toolStorage->GetToolByName(m_TrackingDeviceSource->GetOutput(i)->GetName());
	  std::cout << "Tool Name" << m_TrackingDeviceSource->GetOutput(i)->GetName() << endl;
	  if (currentTool.IsNull())
	  {
		  MessageBox("Error: did not find corresponding tool in tracking device after initialization.");
		  return;
	  }
	  m_ToolVisualizationFilter->SetInput(i, m_TrackingDeviceSource->GetOutput(i));
	  m_ToolVisualizationFilter->SetRepresentationObject(i, currentTool->GetDataNode()->GetData());
  }
  
  if ( m_TrackingDeviceSource.IsNull() )
  {
    MessageBox(std::string("Cannot connect to device: ") + myTrackingDeviceSourceFactory->GetErrorMessage());
    return;
  }

  //First check if the created object is valid
  if (m_TrackingDeviceSource.IsNull())
  {
    MessageBox(myTrackingDeviceSourceFactory->GetErrorMessage());
    return;
  }

  MITK_INFO << "Number of tools: " << m_TrackingDeviceSource->GetNumberOfOutputs();

  //The tools are maybe reordered after initialization, e.g. in case of auto-detected tools of NDI Aurora
  mitk::NavigationToolStorage::Pointer toolsInNewOrder = myTrackingDeviceSourceFactory->GetUpdatedNavigationToolStorage();
  if ((toolsInNewOrder.IsNotNull()) && (toolsInNewOrder->GetToolCount() > 0))
    {
    //so delete the old tools in wrong order and add them in the right order
    //we cannot simply replace the tool storage because the new storage is
    //not correctly initialized with the right data storage
    m_toolStorage->DeleteAllTools();
    for (int i=0; i < toolsInNewOrder->GetToolCount(); i++) 
	{
		m_toolStorage->AddTool(toolsInNewOrder->GetTool(i));	
	}
    }

  m_MaxillofacialTrackingLab = new MITKMaxillofacialTrackingLab();

  //connect to device
  try
    {
    m_TrackingDeviceSource->Connect();
    //Microservice registration:
    m_TrackingDeviceSource->RegisterAsMicroservice();
    m_toolStorage->UnRegisterMicroservice();
    m_toolStorage->RegisterAsMicroservice(m_TrackingDeviceSource->GetMicroserviceID());
    m_toolStorage->LockStorage();

	
    }
  catch (...) //todo: change to mitk::IGTException
    {
    MessageBox("Error on connecting the tracking device.");
    return;
    }

  //enable/disable Buttons
  m_Controls->m_Disconnect->setEnabled(true);
  m_Controls->m_StartTracking->setEnabled(true);
  m_Controls->m_StopTracking->setEnabled(false);
  m_Controls->m_Connect->setEnabled(false);
  DisableOptionsButtons();
  DisableTrackingConfigurationButtons();
  m_Controls->m_configurationWidget->ConfigurationFinished();

  m_Controls->m_TrackingControlLabel->setText("Status: connected");
  }

  /*OnVirtualConnect allows for the connection in virtual mode instead of a real tracking device*/
void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnVirtualConnect()
{
	//build the Virtual IGT pipeline instead of the IGT pipeline for a real tracker device
	
	mitk::VirtualTrackingDevice::Pointer VirtualTrackingDevice = mitk::VirtualTrackingDevice::New();

	//Add virtual tool to virtual tracking device

	mitk::ScalarType bounds[] = { 0.0, 200.0, 0.0, 200.0, 0.0, 200.0 };
	VirtualTrackingDevice->SetBounds(bounds);
	mitk::NavigationTool::Pointer NavigationTool = mitk::NavigationTool::New();
	mitk::TrackingTool::Pointer tool = VirtualTrackingDevice->AddTool("Virtual tool");
	NavigationTool->SetTrackingTool(tool);
	NavigationTool->SetIdentifier("Virtual Navigation Tool");

	//Associate DataNode to virtual tool

	mitk::DataNode::Pointer newNode = mitk::DataNode::New();
	mitk::Surface::Pointer sphere = mitk::Surface::New();
	double scale[] = { 5.0, 5.0, 5.0 };
	sphere->GetGeometry()->SetSpacing(scale); //scale it a little that so we can see something
	vtkSphereSource *vtkData = vtkSphereSource::New();
	vtkData->SetRadius(0.5f);
	vtkData->SetCenter(0.0, 0.0, 0.0);
	vtkData->Update();
	sphere->SetVtkPolyData(vtkData->GetOutput());
	vtkData->Delete();
	newNode->SetData(sphere);
	newNode->SetName("Virtual Tool");
	newNode->SetColor(0.3, 0.3, 0.3);
	NavigationTool->SetDataNode(newNode);
	
	
	//Tracking device data

	VirtualTrackingDevice->SetData(m_TrackingDeviceData);
	
	try
	{
		//Get Tracking Volume Data
		mitk::TrackingDeviceData data = mitk::DeviceDataUnspecified;

		QString qstr = m_Controls->m_VolumeSelectionBox->currentText();
		if ((!qstr.isNull()) || (!qstr.isEmpty())) {
			std::string str = qstr.toStdString();
			data = mitk::GetDeviceDataByName(str); //Data will be set later, after device generation
		}
	}
	
	catch (...)
	{
		MessageBox("Error on getting tracking volume data.");
		return;
	}

	
	//Create Virtual Navigation Data Source and associate it to the virtual tracking device
	m_TrackingDeviceSource = mitk::TrackingDeviceSource::New();
	
	
	m_TrackingDeviceSource->SetTrackingDevice(VirtualTrackingDevice); //Here we set the tracking device to the source of the pipeline.
	
	//Create ToolVisualizationFilter and set the output of m_TrackingDeviceSource as an input

	m_ToolVisualizationFilter = mitk::NavigationDataObjectVisualizationFilter::New();

	m_ToolVisualizationFilter->SetInput(0, m_TrackingDeviceSource->GetOutput());

	//Set the created sphere node as tool representation object
	m_ToolVisualizationFilter->SetRepresentationObject(0, NavigationTool->GetDataNode()->GetData());

	//Number of tools
	MITK_INFO << "Number of tools: " << m_TrackingDeviceSource->GetNumberOfOutputs();
	
	//remove real navigation tools from toolStorage
	this->m_toolStorage->DeleteAllTools();
	
	//Add virtual navigation tool
	m_toolStorage->AddTool(NavigationTool);

	
	//connect to device
	try
	{
		m_TrackingDeviceSource->Connect();
		//Microservice registration:
		m_TrackingDeviceSource->RegisterAsMicroservice();

		m_toolStorage->UnRegisterMicroservice();
		m_toolStorage->RegisterAsMicroservice(m_TrackingDeviceSource->GetMicroserviceID());
		m_toolStorage->LockStorage();
	}
	catch (...) //todo: change to mitk::IGTException
	{
		MessageBox("Error on connecting the tracking device.");
		return;
	}

	//enable/disable Buttons
	m_Controls->m_Disconnect->setEnabled(true);
	m_Controls->m_StartTracking->setEnabled(true);
	m_Controls->m_StopTracking->setEnabled(false);
	m_Controls->m_Connect->setEnabled(false);

	m_Controls->m_PerformVirtualTracking->setEnabled(false);

	DisableOptionsButtons();
	DisableTrackingConfigurationButtons();
	m_Controls->m_configurationWidget->ConfigurationFinished();

	m_Controls->m_TrackingControlLabel->setText("Status: connected");
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnDisconnect()
  {
  if (m_tracking) this->OnStopTracking();

  m_TrackingDeviceSource->Disconnect();
  m_TrackingDeviceSource->UnRegisterMicroservice();
  m_toolStorage->UnLockStorage();

  //enable/disable Buttons
  m_Controls->m_Disconnect->setEnabled(false);
  m_Controls->m_StartTracking->setEnabled(false);
  m_Controls->m_StopTracking->setEnabled(false);
  m_Controls->m_Connect->setEnabled(true);
  EnableOptionsButtons();
  EnableTrackingConfigurationButtons();
  m_Controls->m_configurationWidget->Reset();
  m_Controls->m_TrackingControlLabel->setText("Status: disconnected");
  m_Controls->m_PerformVirtualTracking->setEnabled(true);

  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnStartTracking()
{
  try
    {
    m_TrackingDeviceSource->StartTracking();
    }
  catch (...) //todo: change to mitk::IGTException
    {
    MessageBox("Error while starting the tracking device!");
    return;
    }

  m_TrackingTimer->start(1000/(m_Controls->m_UpdateRate->value()));
  m_Controls->m_TrackingControlLabel->setText("Status: tracking");

  //connect the tool visualization widget
  for(int i=0; i<m_TrackingDeviceSource->GetNumberOfOutputs(); i++)
  {
    m_Controls->m_TrackingToolsStatusWidget->AddNavigationData(m_ToolVisualizationFilter->GetOutput(i));
	//std::cout << "AddNavigationData, tool : " << i << endl;
  }
  m_Controls->m_TrackingToolsStatusWidget->ShowStatusLabels();


  this->OnTrackingVolumeChanged(m_Controls->m_VolumeSelectionBox->currentText());

  //enable/disable Buttons
  m_Controls->m_Disconnect->setEnabled(true);
  m_Controls->m_StartTracking->setEnabled(false);
  m_Controls->m_StopTracking->setEnabled(true);
  m_Controls->m_Connect->setEnabled(false);

  m_tracking = true;

  this->GlobalReinit();
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnStopTracking()
{
  if (!m_tracking) return;
  m_TrackingTimer->stop();
  m_TrackingDeviceSource->StopTracking();
  m_Controls->m_TrackingControlLabel->setText("Status: connected");
  if (m_logging) StopLogging();
  m_Controls->m_TrackingToolsStatusWidget->RemoveStatusLabels();
  m_Controls->m_TrackingToolsStatusWidget->PreShowTools(m_toolStorage);
  m_tracking = false;

  //enable/disable Buttons
  m_Controls->m_Disconnect->setEnabled(true);
  m_Controls->m_StartTracking->setEnabled(true);
  m_Controls->m_StopTracking->setEnabled(false);
  m_Controls->m_Connect->setEnabled(false);

  this->GlobalReinit();
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnPerformVirtualTracking(bool on)
{
	if (on)
	{
		m_VirtualTracking = true;
	}
	else
	{
		m_VirtualTracking = false;
	}
	OnTrackingDeviceChanged();
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnTrackingDeviceChanged()
{
  mitk::TrackingDeviceType Type;

  if (m_VirtualTracking)
  {
	  Type = mitk::TrackingSystemNotSpecified;
	  MessageBox("The virtual tracking option has been enabled. This option allows the tracking of only one tool");
	  m_Controls->m_TrackingToolsGroupBox->setEnabled(false);
	  m_Controls->m_TrackingControlsGroupBox->setEnabled(true);
  }

  else
  {
	  if (m_Controls->m_configurationWidget->GetTrackingDevice().IsNotNull())
	  {
		  Type = m_Controls->m_configurationWidget->GetTrackingDevice()->GetType();
		  //enable controls because device is valid
		  m_Controls->m_TrackingToolsGroupBox->setEnabled(true);
		  m_Controls->m_TrackingControlsGroupBox->setEnabled(true);
	  }
	  else
	  {
		  Type = mitk::TrackingSystemNotSpecified;
		  MessageBox("Error: This tracking device is not included in this project. Please make sure that the device is installed and activated in your MITK build.");
		  m_Controls->m_TrackingToolsGroupBox->setEnabled(false);
		  m_Controls->m_TrackingControlsGroupBox->setEnabled(false);
		  return;
	  }

	  // Code to enable/disable device specific buttons
	  if (Type == mitk::NDIAurora) //Aurora
	  {
		  //m_Controls->m_AutoDetectTools->setVisible(true);
		  m_Controls->m_AddSingleTool->setEnabled(false);
	  }
	  else //Polaris or Microntracker
	  {
		  //m_Controls->m_AutoDetectTools->setVisible(false);
		  m_Controls->m_AddSingleTool->setEnabled(true);
	  }
  }

  // Code to select appropriate tracking volume for current type
  //First clear volume
  m_Controls->m_VolumeSelectionBox->clear();

  //In the case of virtual device, set tracking volume to a rectangle 200mmx200mmx200mm
  if (m_VirtualTracking)
  {
	  m_Controls->m_VolumeSelectionBox->addItem("Virtual Tracking Volume");
      return;
  }
  std::vector<mitk::TrackingDeviceData> Compatibles = mitk::GetDeviceDataForLine(Type);
  
  for(int i = 0; i < Compatibles.size(); i++)
  {
    m_Controls->m_VolumeSelectionBox->addItem(Compatibles[i].Model.c_str());
  }


  this->OnPreloadSettings();
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnTrackingVolumeChanged(QString qstr)
{
 if (qstr.isNull()) return;
  if (qstr.isEmpty()) return;
  if (m_Controls->m_ShowTrackingVolume->isChecked())
  {
    mitk::TrackingVolumeGenerator::Pointer volumeGenerator = mitk::TrackingVolumeGenerator::New();

    std::string str = qstr.toStdString();

    mitk::TrackingDeviceData data = mitk::GetDeviceDataByName(str);
    m_TrackingDeviceData = data;

    volumeGenerator->SetTrackingDeviceData(data);
    volumeGenerator->Update();

    mitk::Surface::Pointer volumeSurface = volumeGenerator->GetOutput();

    m_TrackingVolumeNode->SetData(volumeSurface);

    GlobalReinit();
  }
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnShowTrackingVolumeChanged()
{
 if (m_Controls->m_ShowTrackingVolume->isChecked())
    {
    OnTrackingVolumeChanged(m_Controls->m_VolumeSelectionBox->currentText());
    GetDataStorage()->Add(m_TrackingVolumeNode);
    }
  else
    {
    GetDataStorage()->Remove(m_TrackingVolumeNode);
    GlobalReinit();
    }
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAutoDetectTools()
{
if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() == mitk::NDIAurora)
    {
    DisableTrackingConfigurationButtons();
    mitk::NDITrackingDevice::Pointer currentDevice = dynamic_cast<mitk::NDITrackingDevice*>(m_Controls->m_configurationWidget->GetTrackingDevice().GetPointer());
    currentDevice->OpenConnection();
    currentDevice->StartTracking();
    mitk::NavigationToolStorage::Pointer autoDetectedStorage = mitk::NavigationToolStorage::New(this->GetDataStorage());
    for (int i=0; i<currentDevice->GetToolCount(); i++)
      {
      //create a navigation tool with sphere as surface
      std::stringstream toolname;
      toolname << "AutoDetectedTool" << i;
      mitk::NavigationTool::Pointer newTool = mitk::NavigationTool::New();
      newTool->SetSerialNumber(dynamic_cast<mitk::NDIPassiveTool*>(currentDevice->GetTool(i))->GetSerialNumber());
      newTool->SetIdentifier(toolname.str());
      newTool->SetTrackingDeviceType(mitk::NDIAurora);
      mitk::DataNode::Pointer newNode = mitk::DataNode::New();
      mitk::Surface::Pointer mySphere = mitk::Surface::New();
      vtkSphereSource *vtkData = vtkSphereSource::New();
      vtkData->SetRadius(3.0f);
      vtkData->SetCenter(0.0, 0.0, 0.0);
      vtkData->Update();
      mySphere->SetVtkPolyData(vtkData->GetOutput());
      vtkData->Delete();
      newNode->SetData(mySphere);
      newNode->SetName(toolname.str());
      newTool->SetDataNode(newNode);
      autoDetectedStorage->AddTool(newTool);
      }
    //save detected tools
    this->ReplaceCurrentToolStorage(autoDetectedStorage,"Autodetected NDI Aurora Storage");
    //update label
    QString toolLabel = QString("Loaded Tools: ") + QString::number(m_toolStorage->GetToolCount()) + " Tools (Auto Detected)";
    m_Controls->m_toolLabel->setText(toolLabel);
    //update tool preview
    m_Controls->m_TrackingToolsStatusWidget->RemoveStatusLabels();
    m_Controls->m_TrackingToolsStatusWidget->PreShowTools(m_toolStorage);
    currentDevice->StopTracking();
    currentDevice->CloseConnection();

    EnableTrackingConfigurationButtons();

    if (m_toolStorage->GetToolCount()>0)
      {
      //ask the user if he wants to save the detected tools
      QMessageBox msgBox;
      switch(m_toolStorage->GetToolCount())
        {
        case 1:
          msgBox.setText("Found one tool!");
          break;
        default:
          msgBox.setText("Found " + QString::number(m_toolStorage->GetToolCount()) + " tools!");
        }
      msgBox.setInformativeText("Do you want to save this tools as tool storage, so you can load them again?");
      msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
      msgBox.setDefaultButton(QMessageBox::No);
      int ret = msgBox.exec();

      if (ret == 16384) //yes
        {
        //ask the user for a filename
        QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save File"),"/",tr("*.IGTToolStorage"));
        //check for empty filename
        if(fileName == "") {return;}
        mitk::NavigationToolStorageSerializer::Pointer mySerializer = mitk::NavigationToolStorageSerializer::New();

        //when Serialize method is used exceptions are thrown, need to be adapted
        //try-catch block for exception handling in Serializer
        try
        {
        mySerializer->Serialize(fileName.toStdString(),m_toolStorage);
        }
        catch(mitk::IGTException)
        {
        std::string errormessage = "Error during serialization. Please check the Zip file.";
        QMessageBox::warning(NULL, "IGTPlayer: Error", errormessage.c_str());}
        return;
        }
        else if (ret == 65536) //no
        {
        return;
        }
      }

    }
}

/**
\brief This method sets up the navigation pipeline during initialization.
*/

void QmitkMITKIGTMaxillofacialTrackingToolboxView::CreateBundleWidgets(QWidget* parent)
{
	//initialize registration widget
	m_Controls->m_RegistrationWidget->HideStaticRegistrationRadioButton(true);
	m_Controls->m_RegistrationWidget->HideContinousRegistrationRadioButton(true);
	m_Controls->m_RegistrationWidget->HideUseICPRegistrationCheckbox(true);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnSetupNavigation()
{

	mitk::DataStorage* ds = this->GetDataStorage();

	if (ds == NULL)
	{
		MITK_WARN << "IGTSurfaceTracker: Error", "can not access DataStorage. Navigation not possible";
		return;
	}
	InitializeRegistration();

	InitializeDistanceControl();

	//Initialize elements for surface orientation
	m_SurfaceGeometricalTransform = new SurfaceGeometricalTransform[m_ToolVisualizationFilter->GetNumberOfOutputs()];
	
	for (int i = 0; i < m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
	{
		m_SurfaceGeometricalTransform[i].node_name = "-";
		m_SurfaceGeometricalTransform[i].surface_node = NULL;
		m_SurfaceGeometricalTransform[i].SurfaceRelated = false;
		m_SurfaceGeometricalTransform[i].SurfaceToToolTransform = NULL;
		m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse = NULL;

	}

	//Initialize tool ListBox, reference ListBox, and associated surface ListBox

	for (int i = 0; i < m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
	{
		QListWidgetItem * item = new QListWidgetItem();
		item->setText("-");
		m_Controls->m_Surfaces->addItem(item);

	}

	for (int i = 0; i < m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
	{
		QString tool_name = m_toolStorage->GetTool(i)->GetToolName().c_str();
		QListWidgetItem *tool_item = new QListWidgetItem;
		tool_item->setText(tool_name);
		m_Controls->m_ToolList->addItem(tool_item);
	}

	for (int i = 0; i < m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
	{
		QListWidgetItem *reference_item = new QListWidgetItem;
		reference_item->setText("-");
		m_Controls->m_IsReferenceFramework->addItem(reference_item);
	}

	

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnSetupToolForTrajectoryControl()
{

	mitk::DataStorage* ds = this->GetDataStorage();

	if (ds == NULL)
	{
		MITK_WARN << "IGTSurfaceTracker: Error", "can not access DataStorage. Navigation not possible";
		return;
	}
	InitializeDistanceControl();

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::InitializeDistanceControl()
{
	mitk::DataStorage* ds = this->GetDataStorage();
	if (ds == NULL)
		return;

	// let the registration widget know about the slice navigation controllers
	// in the active render window part (crosshair updates)
	foreach(QmitkRenderWindow* renderWindow, this->GetRenderWindowPart()->GetQmitkRenderWindows().values())
	{
		m_Controls->m_DistanceCalculationBetweenPointsWidget->AddSliceNavigationController(renderWindow->GetSliceNavigationController());
	}

	if (m_ReferencePositionDataNode.IsNull())
	{
		m_ReferencePositionDataNode = mitk::DataNode::New();
		mitk::PointSet::Pointer ifPS = mitk::PointSet::New();

		m_ReferencePositionDataNode->SetData(ifPS);

		mitk::Color color;
		color.Set(1.0f, 0.0f, 0.0f);
		m_ReferencePositionDataNode->SetName("Reference Points");
		m_ReferencePositionDataNode->SetColor(color);
		m_ReferencePositionDataNode->SetBoolProperty("updateDataOnRender", false);

		ds->Add(m_ReferencePositionDataNode);
	}
	m_Controls->m_DistanceCalculationBetweenPointsWidget->SetReferencePositionNode(m_ReferencePositionDataNode);

	if (m_MeasurementPositionDataNode.IsNull())
	{
		m_MeasurementPositionDataNode = mitk::DataNode::New();
		mitk::PointSet::Pointer tfPS = mitk::PointSet::New();
		m_MeasurementPositionDataNode->SetData(tfPS);

		mitk::Color color;
		color.Set(0.0f, 1.0f, 0.0f);
		m_MeasurementPositionDataNode->SetName("Measurement Points");
		m_MeasurementPositionDataNode->SetColor(color);
		m_MeasurementPositionDataNode->SetBoolProperty("updateDataOnRender", false);

		ds->Add(m_MeasurementPositionDataNode);
	}

	m_Controls->m_DistanceCalculationBetweenPointsWidget->SetMeasurementPositionNode(m_MeasurementPositionDataNode);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnSetupTool()
{
	
	if (m_ThereIsAReference == true)
	{
		std::cout << m_Reference_Index << std::endl;
		std::cout << m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID() << std::endl;

		if (m_Reference_Index == m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID())
		{
			std::cout << "Tool is reference framework" << std::endl;
			m_Controls->m_ToolIsReferenceFramework->setChecked(true);
			
			m_ReferenceFrameworkChecked = true;
		}
		else
		{
			std::cout << "Tool is a reference framework" << std::endl;
			m_Controls->m_ToolIsReferenceFramework->setChecked(false);
			m_ReferenceFrameworkChecked = false;
		}
	}
		
	else
	{
		m_Controls->m_ToolIsReferenceFramework->setChecked(false);
		m_ReferenceFrameworkChecked = false;
	}

	QListWidgetItem *surface_item = m_Controls->m_Surfaces->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID());

	if (surface_item->text().toStdString() == "-")
	{
		m_Controls->m_RegisterToolWithSurface->setChecked(false);
		m_Controls->m_RegistrationSurfaceComboBox->setEnabled(false);
	}
	else
	{
		m_Controls->m_RegisterToolWithSurface->setChecked(true);
		m_Controls->m_RegistrationSurfaceComboBox->setEnabled(true);
	}

}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnRegisterToolWithSurface(bool on)
{
	if (on)
	{
		m_Controls->m_RegistrationSurfaceComboBox->setEnabled(true);
	}

	else
	{
		m_Controls->m_RegistrationSurfaceComboBox->setEnabled(false);
	}

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnSetAsReferenceFramework(bool on)
{
	if (on)
	{
		m_ReferenceFrameworkChecked = true;
	}

	else
	{
		m_ReferenceFrameworkChecked = false;
	}

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAcceptToolData()
{

	m_ThereIsAReference = false;
	m_Reference_Index = NULL;

	QListWidgetItem *item = new QListWidgetItem();

	for (int index = 0; index < m_toolStorage->GetToolCount(); index++)
	{

		item = m_Controls->m_IsReferenceFramework->item(index);

		if (m_ReferenceFrameworkChecked)
		{
			if (index == m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID())
			{
				item->setText("Reference framework");
				m_ThereIsAReference = true;
				m_Reference_Index = index;
			}
			else
			{
				item->setText("-");
			}
		}
		else
		{
			if (index == m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID())
			{
				item->setText("-");
			}
			
		}

		if (item->text().compare("Reference framework") == 0)
		{
			m_ThereIsAReference = true;
			m_Reference_Index = index;
		}
	}

	if (m_Controls->m_RegisterToolWithSurface->isChecked())
	{
		QString surface_name = QString::fromStdString(m_Controls->m_RegistrationSurfaceComboBox->GetSelectedNode()->GetProperty("name")->GetValueAsString());
		QListWidgetItem *surface_item = m_Controls->m_Surfaces->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID());
		surface_item->setText(surface_name);
		surface_item = NULL;
	}

	
	item = NULL;
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnCalculateRegistration()
{

	int index = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID();

	//Check for initialization
	if (!CheckRegistrationInitialization()) return;

	m_MaxillofacialTrackingLab->CalculateRegistration(m_ImageFiducialsDataNode, m_TrackerFiducialsDataNode);

	m_Controls->m_RegistrationWidget->SetQualityDisplayText("FRE: " + QString::number(m_MaxillofacialTrackingLab->GetRegistrationFRE()) + " mm");

	vtkMatrix4x4 * matrix = m_MaxillofacialTrackingLab->GetVTKRegistrationTransform()->GetMatrix();

	//Transform Tracker Fiducials to the position on the Image World 
	m_TrackerFiducialsDataNode->GetData()->GetGeometry()->Compose(matrix);
	m_TrackerFiducialsDataNode->GetData()->GetTimeGeometry()->Update();
	mitk::RenderingManager::GetInstance()->RequestUpdateAll();


	m_Controls->m_UsePermanentRegistrationToggle->setEnabled(true);

	//save transform to file
	std::ofstream myfile;

	std::cout << "Matrix: " << *matrix << std::endl;

	std::string filename = this->m_Controls->m_TransformFileName->text().toStdString().c_str();
	try
	{
		std::ofstream myfile(filename);
		myfile << *matrix;
		myfile.close();
	}
	catch (...)
	{
		std::string warningmessage = "Transform file has not being specified or does not exist. Registration will not be saved on file.";
		QMessageBox::warning(NULL, "IGTPlayer: Error", warningmessage.c_str());
	}

	SaveSurfaceToToolRegistrationTransform();
}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::SaveSurfaceToToolRegistrationTransform()
{
	int index = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID();

	if (m_Controls->m_ToolToSurfaceRegistration->isChecked())
	{
		m_SurfaceGeometricalTransform[index].surface_node = m_Controls->m_RegistrationSurfaceComboBox->GetSelectedNode();
		m_SurfaceGeometricalTransform[index].node_name = m_SurfaceGeometricalTransform[index].surface_node->GetName();
		m_SurfaceGeometricalTransform[index].SurfaceRelated = true;
		itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer  matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
		m_MaxillofacialTrackingLab->GetITKRegistrationTransform()->GetInverse(matrix);
		m_SurfaceGeometricalTransform[index].SurfaceToToolTransform = matrix;
	}

	else

	{
		m_GeneralRegistrationTransform = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
		m_GeneralRegistrationTransform = m_MaxillofacialTrackingLab->GetITKRegistrationTransform();
	}
}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnToolToSurfaceRegistration(bool on)
{
	m_Controls->m_GeneralRegistration->setChecked(!on);
}
void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnGeneralRegistration(bool on)
{
	m_Controls->m_ToolToSurfaceRegistration->setChecked(!on);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnApplyRegistration(bool on)
{
	
	if (on)
	{
		//some initial checks
		if (!CheckRegistrationInitialization())
		{
			m_Controls->m_UsePermanentRegistrationToggle->setChecked(false);
			return;
		}

		for (unsigned int i = 0; i<m_TrackingDeviceSource->GetNumberOfOutputs(); i++)
		{
			if (m_ThereIsAReference) // All the tools and surfaces must be represented with respect to the reference framework
			{
				std::cout << "Reference Index : " << m_Reference_Index << std::endl;
				m_Original_Reference_Orientation = m_ToolVisualizationFilter->GetOutput(m_Reference_Index)->GetAffineTransform3D();
				m_Original_Reference_Orientation->GetInverse(m_Original_Reference_Orientation_Inverse);

				if (i == m_Reference_Index)
				{
					m_Total_Reference_Orientation = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
					m_Total_Reference_Orientation->SetIdentity();

					//We set the full orientation and the registration now, since later we won't allow any changes in position or orientation for this tool
					m_Total_Reference_Orientation->Compose(m_Original_Reference_Orientation);
					m_Total_Reference_Orientation->Compose(m_GeneralRegistrationTransform);
					m_ToolVisualizationFilter->SetOffset(i, m_Total_Reference_Orientation);
					
				}
				else
				{
					m_TotalOrientationTransform->SetIdentity();
					m_TotalOrientationTransform->Compose(m_GeneralRegistrationTransform);
					m_ToolVisualizationFilter->SetOffset(i, m_TotalOrientationTransform);

					if (m_SurfaceGeometricalTransform[i].SurfaceRelated)
					{
						itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
						matrix->SetIdentity();
						matrix->Compose(m_SurfaceGeometricalTransform[i].SurfaceToToolTransform);
						matrix->Compose(m_GeneralRegistrationTransform);
						m_SurfaceGeometricalTransform[i].surface_node->GetData()->GetGeometry()->SetIndexToWorldTransform(matrix);
						m_SurfaceGeometricalTransform[i].surface_node->GetData()->Modified();
						m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
						m_ToolVisualizationFilter->GetOutput(i)->GetAffineTransform3D()->GetInverse(m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse);
					}

				}
			}

			else // If there is no reference framework, we give priority to the tool that we are using for the registration
			{

				if (m_SurfaceGeometricalTransform[i].SurfaceRelated)
				{
					itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
					matrix->SetIdentity();
					matrix->Compose(m_SurfaceGeometricalTransform[i].SurfaceToToolTransform);
					matrix->Compose(m_GeneralRegistrationTransform);
					m_SurfaceGeometricalTransform[i].surface_node->GetData()->GetGeometry()->SetIndexToWorldTransform(matrix);
					m_SurfaceGeometricalTransform[i].surface_node->GetData()->Modified();
					m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
					m_ToolVisualizationFilter->GetOutput(i)->GetAffineTransform3D()->GetInverse(m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse);
				}

				m_ToolVisualizationFilter->SetOffset(i, m_GeneralRegistrationTransform);
			}

			m_ToolVisualizationFilter->Update();
		}


		//From this point, we won't visualize the changes in orientation or position for the tool that represents the reference marker. Instead, the changes will
		//be applied to the rest of the context (inverted)

		if (m_ThereIsAReference)
		{
			m_ToolVisualizationFilter->SetTransformOrientation(m_Reference_Index, false);
			m_ToolVisualizationFilter->SetTransformPosition(m_Reference_Index, false);
		}

		//TO-DO fix this.
		if (m_PSRecordingPointSet.IsNotNull())
		{
			m_PSRecordingPointSet->GetGeometry()->Compose(m_MaxillofacialTrackingLab->GetITKRegistrationTransform());
		}
	
		mitk::RenderingManager::GetInstance()->RequestUpdateAll();
		
		//Permanent registration must be applied
		m_PermanentRegistration = true;
		
	}

	else //if off = disable the permanent registration
	{
		//stop permanent registration
		m_PermanentRegistration = false;
		
		for (unsigned int i = 0; i<m_TrackingDeviceSource->GetNumberOfOutputs(); i++)
		{
		   m_ToolVisualizationFilter->SetOffset(i, NULL);
		}

		m_ToolVisualizationFilter->Update();

		//Undo registration in Trajectory PointSet TO-DO

		if (m_PSRecordingPointSet.IsNotNull())
		{
			m_PSRecordingPointSet->GetGeometry()->Compose(m_MaxillofacialTrackingLab->GetITKRegistrationTransformInverse());
		}

		//Undo registration in distance line TO-DO

		if (m_DistanceLinePointSet.IsNotNull())
		{
			m_DistanceLinePointSet->GetGeometry()->Compose(m_MaxillofacialTrackingLab->GetITKRegistrationTransformInverse());
		}
	}
}

bool QmitkMITKIGTMaxillofacialTrackingToolboxView::CheckRegistrationInitialization()
{
	// a couple of variables that we need in this method
	std::string warningMessage = "";
	bool initializationErrorDetected = false;
	mitk::PointSet::Pointer imageFiducials, trackerFiducials;

	// check some initialization stuff
	if (m_ImageFiducialsDataNode.IsNull() || m_TrackerFiducialsDataNode.IsNull())
	{
		warningMessage = "Initialization not finished!";
		MITK_WARN << warningMessage;
		QMessageBox::warning(NULL, "Registration not possible", warningMessage.c_str());
		return false;
	}
	else
	{
		imageFiducials = dynamic_cast<mitk::PointSet*>(m_ImageFiducialsDataNode->GetData());
		trackerFiducials = dynamic_cast<mitk::PointSet*>(m_TrackerFiducialsDataNode->GetData());
	}

	if (imageFiducials.IsNull() || trackerFiducials.IsNull())
	{
		warningMessage = "Fiducial data objects not found. \n"
			"Please set 3 or more fiducials in the image and with the tracking system.\n\n"
			"Registration is not possible";
		initializationErrorDetected = true;
	}
	else if ((imageFiducials->GetSize() < 3) || (trackerFiducials->GetSize() < 3) || (imageFiducials->GetSize() != trackerFiducials->GetSize()))
	{
		warningMessage = "Not enough fiducial pairs found. At least 3 fiducial must exist for the image and the tracking system respectively.";
		initializationErrorDetected = true;
	}

	// finaly: if an err was detected, give a warning and an error popup, then return false
	if (initializationErrorDetected)
	{
		MITK_WARN << warningMessage;
		QMessageBox::warning(NULL, "Registration not possible", warningMessage.c_str());
		return false;
	}
	//if no error was detected simply return true
	else { return true; }

}
void QmitkMITKIGTMaxillofacialTrackingToolboxView::InitializeRegistration()
{
	mitk::DataStorage* ds = this->GetDataStorage();
	if (ds == NULL)
		return;

	// let the registration widget know about the slice navigation controllers
	// in the active render window part (crosshair updates)
	foreach(QmitkRenderWindow* renderWindow, this->GetRenderWindowPart()->GetQmitkRenderWindows().values())
	{
		m_Controls->m_RegistrationWidget->AddSliceNavigationController(renderWindow->GetSliceNavigationController());
	}

	if (m_ImageFiducialsDataNode.IsNull())
	{
		m_ImageFiducialsDataNode = mitk::DataNode::New();
		mitk::PointSet::Pointer ifPS = mitk::PointSet::New();

		m_ImageFiducialsDataNode->SetData(ifPS);

		mitk::Color color;
		color.Set(1.0f, 0.0f, 0.0f);
		m_ImageFiducialsDataNode->SetName("Image Fiducials");
		m_ImageFiducialsDataNode->SetColor(color);
		m_ImageFiducialsDataNode->SetBoolProperty("updateDataOnRender", false);

		ds->Add(m_ImageFiducialsDataNode);
	}

	m_Controls->m_RegistrationWidget->SetImageFiducialsNode(m_ImageFiducialsDataNode);

	if (m_TrackerFiducialsDataNode.IsNull())
	{
		m_TrackerFiducialsDataNode = mitk::DataNode::New();
		mitk::PointSet::Pointer tfPS = mitk::PointSet::New();
		m_TrackerFiducialsDataNode->SetData(tfPS);

		mitk::Color color;
		color.Set(0.0f, 1.0f, 0.0f);
		m_TrackerFiducialsDataNode->SetName("Tracking Fiducials");
		m_TrackerFiducialsDataNode->SetColor(color);
		m_TrackerFiducialsDataNode->SetBoolProperty("updateDataOnRender", false);

		ds->Add(m_TrackerFiducialsDataNode);
	}

	m_Controls->m_RegistrationWidget->SetTrackerFiducialsNode(m_TrackerFiducialsDataNode);
}


/** This method is called when the instrument is selected.
*  It stores the navigation data of the instrument.
*/

/*void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnInstrumentSelected()
{
	
	if (m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource().IsNotNull())
	{
		try
		{
			std::string tool_name = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationTool()->GetToolName();
			m_InstrumentNavigationData = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID());

		}

		catch (...) //todo: change to mitk::IGTException
		{
			MessageBox("Could not select a navigation tool. Please select a tool from the navigation tool list.");
			return;
		}
	}
	else
	{
		m_Controls->m_PointerNameLabel->setText("<not available>");
		return;
	}

	if (m_InstrumentNavigationData.IsNotNull())
	{
		m_Controls->m_PointerNameLabel->setText(m_InstrumentNavigationData->GetName());
	}
	else
	{
		m_Controls->m_PointerNameLabel->setText("<not available>");
	}
}*/

/** This method is called when the object marker is selected.
*  It stores the navigation data of the object marker.
*/
void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddRegistrationTrackingFiducial()
{
	mitk::NavigationData::Pointer nd = m_InstrumentNavigationData;

	std::string tool_name = m_Controls->m_ToolForPositioningComboBox->GetSelectedNode()->GetName();

	std::cout << tool_name << std::endl;
	for (int i = 0; i < m_toolStorage->GetToolCount(); i++)
	{
		std::cout << m_toolStorage->GetTool(i)->GetToolName() << std::endl;

		if (tool_name == m_toolStorage->GetTool(i)->GetToolName())
		{
			nd = m_TrackingDeviceSource->GetOutput(i);
		}
	}

	if (nd.IsNull() || !nd->IsDataValid())
	{
		QMessageBox::warning(0, "Invalid tracking data", "Navigation data is not available or invalid!", QMessageBox::Ok);
		return;
	}

	if (m_TrackerFiducialsDataNode.IsNotNull() && m_TrackerFiducialsDataNode->GetData() != NULL)
	{
		mitk::PointSet::Pointer ps = dynamic_cast<mitk::PointSet*>(m_TrackerFiducialsDataNode->GetData());
		ps->InsertPoint(ps->GetSize(), nd->GetPosition());
	}
	else
		QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "Can not access Tracker Fiducials. Adding fiducial not possible!");
}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::MessageBox(std::string s)
  {
  QMessageBox msgBox;
  msgBox.setText(s.c_str());
  msgBox.exec();
  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::UpdateTrackingTimer()
  {
	bool movement = false;
	double min_dist;
	
	if (m_ThereIsAReference)
	{
		for (int i = 0; i < m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
		{
			if (m_PermanentRegistration)
			{
				if (i != m_Reference_Index)
				{
					//If Current_pos = G * Orig_Pos => G = Current_pos * (Orig_pos)^-1 ; 
					itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
					matrix->Compose(m_Original_Reference_Orientation_Inverse);
					matrix->Compose(m_TrackingDeviceSource->GetOutput(m_Reference_Index)->GetAffineTransform3D());

					//Now we get G^-1 and we will have to apply it to the tools that are not a reference marker
					matrix->GetInverse(m_Reference_Orientation_Inverse);

					//The total transform includes the effect of the marker movement (G^-1) and the registration to the virtual world (ITKRegistrationTransform). 
					m_TotalOrientationTransform->SetIdentity();
					m_TotalOrientationTransform->Compose(m_Reference_Orientation_Inverse);
					m_TotalOrientationTransform->Compose(m_MaxillofacialTrackingLab->GetITKRegistrationTransform());
					m_ToolVisualizationFilter->SetOffset(i, m_TotalOrientationTransform);	
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
		{
			if (m_PermanentRegistration)
			{
				std::cout << "Surface " << i << ":" << m_SurfaceGeometricalTransform[i].SurfaceRelated << std::endl;
				//Locate the related surface on the right position and orientation before including the registration
				if (m_SurfaceGeometricalTransform[i].SurfaceRelated)
				{
					itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer surface_matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
					surface_matrix->SetIdentity();				
					surface_matrix->Compose(m_SurfaceGeometricalTransform[i].SurfaceToToolTransform);
					surface_matrix->Compose(m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse);
					surface_matrix->Compose(m_ToolVisualizationFilter->GetOutput(i)->GetAffineTransform3D());
					surface_matrix->Compose(m_GeneralRegistrationTransform);
		
					m_SurfaceGeometricalTransform[i].surface_node->GetData()->GetGeometry()->SetIndexToWorldTransform(surface_matrix);
					m_SurfaceGeometricalTransform[i].surface_node->GetData()->Modified();
				}
			}
		}
	}
	
	m_ToolVisualizationFilter->Update();
	m_Controls->m_TrackingToolsStatusWidget->Refresh();
	
	//TO-DO
	mitk::NavigationData::Pointer nd = m_ToolVisualizationFilter->GetOutput(0);
	mitk::NavigationData::Pointer T_Object = mitk::NavigationData::New(m_TotalOrientationTransform, false);
	nd->Compose(T_Object);
	
	if ((m_DistanceControl && m_DistanceLinePointSet.IsNotNull()) || (m_PointSetRecording && m_PSRecordingPointSet.IsNotNull()))
	{
		if (m_LastToolPosition.IsNotNull())
		{
			mitk::Point3D p = m_LastToolPosition->GetPosition();
			 
	        if (p.EuclideanDistanceTo(nd->GetPosition()) > (double)m_Controls->m_PSRecordingSpinBox->value())
			{
					movement = true;
			}
		}
		else
		{
			m_LastToolPosition = mitk::NavigationData::New();
			movement = true;
		}

		m_LastToolPosition->SetPosition(nd->GetPosition());
	}

	if (movement && m_PointSetRecording && m_PSRecordingPointSet.IsNotNull())
	{
		int size = m_PSRecordingPointSet->GetSize();
		
	    m_PSRecordingPointSet->InsertPoint(size, nd->GetPosition());
	}

	if (movement && m_DistanceControl && m_DistanceLinePointSet.IsNotNull())
	{
			Distance_Data *distance_data = m_Controls->m_RemeshingWidget->RealTimeControlOfDistance(nd->GetPosition());
			m_DistanceLinePointSet->SetPoint(0, nd->GetPosition());
			m_DistanceLinePointSet->SetPoint(1, distance_data->closest_point);
			m_DistanceLinePointSet->GetUpdatedGeometry();
			QString distancestr = QString::number(distance_data->distance);
			min_dist = distance_data->distance;
			m_Controls->m_ToolDistanceToSurfaceWidget->setText(distancestr);
	}

	if (movement && m_logging)
	{
		if (m_DistanceControl)
		{
			this->m_loggingFilter->Update(min_dist);
		}
		else
		{
			this->m_loggingFilter->Update();
		}
		m_loggedFrames = this->m_loggingFilter->GetRecordCounter();
		m_Controls->m_LoggedFramesLabel->setText("Logged Frames: " + QString::number(m_loggedFrames));

		//check if logging stopped automatically

		if ((m_loggedFrames>1) && (!m_loggingFilter->GetRecording()))
		{
			StopLogging();
		}
	}

	mitk::RenderingManager::GetInstance()->RequestUpdateAll();
	
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnChooseFileClicked()
  {
  QDir currentPath = QFileInfo(m_Controls->m_LoggingFileName->text()).dir();

  // if no path was selected (QDir would select current working dir then) or the
  // selected path does not exist -> use home directory
  if ( currentPath == QDir() || ! currentPath.exists() )
  {
    currentPath = QDir(QDir::homePath());
  }

  QString filename = QFileDialog::getSaveFileName(NULL,tr("Choose Logging File"), currentPath.absolutePath(), "*.*");
  if (filename == "") return;
  this->m_Controls->m_LoggingFileName->setText(filename);
  this->OnToggleFileExtension(this->m_Controls->m_LoggingFileName);
  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnChooseTransformFileClicked()
{
	QDir currentPath = QFileInfo(m_Controls->m_TransformFileName->text()).dir();

	// if no path was selected (QDir would select current working dir then) or the
	// selected path does not exist -> use home directory
	if (currentPath == QDir() || !currentPath.exists())
	{
		currentPath = QDir(QDir::homePath());
	}

	QString filename = QFileDialog::getSaveFileName(NULL, tr("Choose Logging File"), currentPath.absolutePath(), "*.*");
	if (filename == "") return;
	this->m_Controls->m_TransformFileName->setText(filename);
	OnToggleFileExtension(m_Controls->m_TransformFileName);
}

// bug-16470: toggle file extension after clicking on radio button
void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnToggleFileExtension(QLineEdit *filename)
{

  QString currentInputText = filename->text();
  QString currentFile = QFileInfo(currentInputText).baseName();
  QDir currentPath = QFileInfo(currentInputText).dir();
  if(currentFile.isEmpty())
  {
    currentFile = "logfile";
  }
  // Setting currentPath to default home path when currentPath is empty or it does not exist
  if(currentPath == QDir() || !currentPath.exists())
  {
    currentPath = QDir::homePath();
  }
  // check if csv radio button is clicked
  if(this->m_Controls->m_csvFormat->isChecked())
  {
    // you needn't add a seperator to the input text when currentpath is the rootpath
    if(currentPath.isRoot())
    {
		filename->setText(QDir::toNativeSeparators(currentPath.absolutePath()) + currentFile + ".csv");
    }

    else
    {
		filename->setText(QDir::toNativeSeparators(currentPath.absolutePath()) + QDir::separator() + currentFile + ".csv");
    }
  }
  // check if xml radio button is clicked
  else if(this->m_Controls->m_xmlFormat->isChecked())
  {
    // you needn't add a seperator to the input text when currentpath is the rootpath
    if(currentPath.isRoot())
    {
		filename->setText(QDir::toNativeSeparators(currentPath.absolutePath()) + currentFile + ".xml");
    }
    else
    {
		filename->setText(QDir::toNativeSeparators(currentPath.absolutePath()) + QDir::separator() + currentFile + ".xml");
    }

  }
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::StartLogging()
  {

  if (m_ToolVisualizationFilter.IsNull())
  {
    MessageBox("Cannot activate logging without a connected device. Configure and connect a tracking device first.");
    return;
  }

  if (!m_logging)
    {
    //initialize logging filter
    m_loggingFilter = mitk::NavigationDataRecorder::New();
    m_loggingFilter->SetRecordingMode(mitk::NavigationDataRecorder::NormalFile);
    if (m_Controls->m_xmlFormat->isChecked()) m_loggingFilter->SetOutputFormat(mitk::NavigationDataRecorder::xml);
    else if (m_Controls->m_csvFormat->isChecked()) m_loggingFilter->SetOutputFormat(mitk::NavigationDataRecorder::csv);
    std::string filename = m_Controls->m_LoggingFileName->text().toStdString().c_str();
    
	// this part has been changed in order to prevent crash of the  program
	if (!filename.empty())
	{
		m_loggingFilter->SetFileName(filename);
	}
	else if (filename.empty())
	{
		std::string errormessage = "File name has not been set, please set the file name";
		mitkThrowException(mitk::IGTIOException) << errormessage;
		QMessageBox::warning(NULL, "IGTPlayer: Error", errormessage.c_str());
		m_loggingFilter->SetFileName(filename);
	}

    if (m_Controls->m_LoggingLimit->isChecked()){m_loggingFilter->SetRecordCountLimit(m_Controls->m_LoggedFramesLimit->value());}

    //connect filter
	//std::cout << "Number of outputs: " << m_ToolVisualizationFilter->GetNumberOfOutputs() << endl;
    for(int i=0; i<m_ToolVisualizationFilter->GetNumberOfOutputs(); i++)
	
	{
		m_loggingFilter->AddNavigationData(m_ToolVisualizationFilter->GetOutput(i));
	}

	
    //start filter with try-catch block for exceptions
    try
    {
    m_loggingFilter->StartRecording();
    }
    catch(mitk::IGTException)
    {
    std::string errormessage = "Error during start recording. Recorder already started recording?";
    QMessageBox::warning(NULL, "IGTPlayer: Error", errormessage.c_str());
    m_loggingFilter->StopRecording();
    return;
    }

    //update labels / logging variables
    this->m_Controls->m_LoggingLabel->setText("Logging ON");
    this->m_Controls->m_LoggedFramesLabel->setText("Logged Frames: 0");
    m_loggedFrames = 0;
    m_logging = true;
    DisableLoggingButtons();
  }
  }



void QmitkMITKIGTMaxillofacialTrackingToolboxView::StopLogging()
  {
  if (m_logging)
    {
    //update label
    this->m_Controls->m_LoggingLabel->setText("Logging OFF");

    m_loggingFilter->StopRecording();
    m_logging = false;
    EnableLoggingButtons();
    }
  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddSingleTool()
  {

  QString Identifier = "Tool";
  if (m_toolStorage.IsNotNull()) Identifier += QString::number(m_toolStorage->GetToolCount());
  else Identifier += "0";
  
  m_Controls->m_NavigationToolCreationWidget->Initialize(GetDataStorage(),Identifier.toStdString());
  m_Controls->m_NavigationToolCreationWidget->SetTrackingDeviceType(m_Controls->m_configurationWidget->GetTrackingDevice()->GetType(),false);
  m_Controls->m_TrackingToolsWidget->setCurrentIndex(1);

  //disable tracking volume during tool editing
  lastTrackingVolumeState = m_Controls->m_ShowTrackingVolume->isChecked();
  if (lastTrackingVolumeState) m_Controls->m_ShowTrackingVolume->click();
  GlobalReinit();

  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddSingleToolFinished()
  {
  m_Controls->m_TrackingToolsWidget->setCurrentIndex(0);
  if (this->m_toolStorage.IsNull())
    {
    //this shouldn't happen!
    MITK_WARN << "No ToolStorage available, cannot add tool, aborting!";
    return;
    }
 
  m_toolStorage->AddTool(m_Controls->m_NavigationToolCreationWidget->GetCreatedTool());
  m_Controls->m_TrackingToolsStatusWidget->PreShowTools(m_toolStorage);
  QString toolLabel = QString("Loaded Tools: <manually added>");

  //enable tracking volume again
  if (lastTrackingVolumeState) m_Controls->m_ShowTrackingVolume->click();
  GlobalReinit();
  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddSingleToolCanceled()
  {
  m_Controls->m_TrackingToolsWidget->setCurrentIndex(0);

  //enable tracking volume again
  if (lastTrackingVolumeState) m_Controls->m_ShowTrackingVolume->click();
  GlobalReinit();
  }


void QmitkMITKIGTMaxillofacialTrackingToolboxView::GlobalReinit()
{
// get all nodes that have not set "includeInBoundingBox" to false
  mitk::NodePredicateNot::Pointer pred = mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("includeInBoundingBox", mitk::BoolProperty::New(false)));

  mitk::DataStorage::SetOfObjects::ConstPointer rs = this->GetDataStorage()->GetSubset(pred);
  // calculate bounding geometry of these nodes
  mitk::TimeGeometry::Pointer bounds = this->GetDataStorage()->ComputeBoundingGeometry3D(rs, "visible");

  // initialize the views to the bounding geometry
  mitk::RenderingManager::GetInstance()->InitializeViews(bounds);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::DisableLoggingButtons()
{
    m_Controls->m_StartLogging->setEnabled(false);
    m_Controls->m_LoggingFileName->setEnabled(false);
    m_Controls->m_ChooseFile->setEnabled(false);
    m_Controls->m_LoggingLimit->setEnabled(false);
    m_Controls->m_LoggedFramesLimit->setEnabled(false);
    m_Controls->m_csvFormat->setEnabled(false);
    m_Controls->m_xmlFormat->setEnabled(false);
    m_Controls->m_StopLogging->setEnabled(true);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::EnableLoggingButtons()
{
    m_Controls->m_StartLogging->setEnabled(true);
    m_Controls->m_LoggingFileName->setEnabled(true);
    m_Controls->m_ChooseFile->setEnabled(true);
    m_Controls->m_LoggingLimit->setEnabled(true);
    m_Controls->m_LoggedFramesLimit->setEnabled(true);
    m_Controls->m_csvFormat->setEnabled(true);
    m_Controls->m_xmlFormat->setEnabled(true);
    m_Controls->m_StopLogging->setEnabled(false);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::DisableOptionsButtons()
{
    m_Controls->m_ShowTrackingVolume->setEnabled(false);
    m_Controls->m_UpdateRate->setEnabled(false); 
    m_Controls->m_OptionsUpdateRateLabel->setEnabled(false);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::EnableOptionsButtons()
{
    m_Controls->m_ShowTrackingVolume->setEnabled(true);
    m_Controls->m_UpdateRate->setEnabled(true);
    m_Controls->m_OptionsUpdateRateLabel->setEnabled(true);
	
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::EnableTrackingConfigurationButtons()
{
   // m_Controls->m_AutoDetectTools->setEnabled(true);
    if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::NDIAurora) m_Controls->m_AddSingleTool->setEnabled(true);
    //m_Controls->m_LoadTools->setEnabled(true);
    m_Controls->m_ResetTools->setEnabled(true);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::DisableTrackingConfigurationButtons()
{
    //m_Controls->m_AutoDetectTools->setEnabled(false);
    if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::NDIAurora) m_Controls->m_AddSingleTool->setEnabled(false);
    //m_Controls->m_LoadTools->setEnabled(false);
    m_Controls->m_ResetTools->setEnabled(false);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::ReplaceCurrentToolStorage(mitk::NavigationToolStorage::Pointer newStorage, std::string newStorageName)
{
    //first: get rid of the old one
    m_toolStorage->UnLockStorage(); //only to be sure...
    m_toolStorage->UnRegisterMicroservice();
    m_toolStorage = NULL;

    //now: replace by the new one
    m_toolStorage = newStorage;
    m_toolStorage->SetName(newStorageName);
    m_toolStorage->RegisterAsMicroservice("no tracking device");
}

/** @OnPointSetRecording allows saving the trajectory of an object that is moving */

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnPointSetRecording(bool record)
{
	mitk::DataStorage* ds = this->GetDataStorage();
	mitk::DataNode::Pointer dn = mitk::DataNode::New();
	mitk::DataNode::Pointer psRecND;
	
	if(record)
	{
		if (m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID() == -1)
		{
			QMessageBox::warning(NULL, "Error", "No tool selected for point set recording!");
			m_Controls->m_PointSetRecordCheckBox->setChecked(false);
			return;
		}

		//m_PointSetRecordingNavigationData = m_Controls->m_m_TrajectoryControlToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());

		m_PointSetRecordingNavigationData = m_ToolVisualizationFilter->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());

		//initialize point set
		psRecND = ds->GetNamedNode("Recorded Points");

		if (m_PSRecordingPointSet.IsNull() || psRecND.IsNull())
		{
			m_PSRecordingPointSet = NULL;
			m_PSRecordingPointSet = mitk::PointSet::New();
			
			dn->SetName("Recorded Points");
			dn->SetColor(1, 1, 1);
			dn->SetData(m_PSRecordingPointSet);
			dn->SetBoolProperty("show points", true);
			dn->SetBoolProperty("show contour", true);
			ds->Add(dn);
		}
		
		m_PointSetRecording = true;		
	}

	else
	{
		dn = ds->GetNamedNode("Recorded Points");
		ds->RemoveNodeEvent(dn);
		m_PSRecordingPointSet->Clear();
		m_PSRecordingPointSet = NULL;
		m_PointSetRecording = false;
	}
}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnCalculateDistanceBetweenPoints()
{
	std::cout << "OnCalculateDistanceBetweenPoints" << std::endl;

	/* retrieve fiducials from data storage */
	mitk::PointSet::Pointer ReferencePointSet = dynamic_cast<mitk::PointSet*>(m_ReferencePositionDataNode->GetData());
	mitk::PointSet::Pointer MeasurementPointSet = dynamic_cast<mitk::PointSet*>(m_MeasurementPositionDataNode->GetData());

 	for (int i = 0; i<MeasurementPointSet->GetSize(); i++)
	{
		double point_reference[3] = { ReferencePointSet->GetPoint(0)[0], ReferencePointSet->GetPoint(0)[1], ReferencePointSet->GetPoint(0)[2] };
		double point_for_measurement[3] = { MeasurementPointSet->GetPoint(i)[0], MeasurementPointSet->GetPoint(i)[1], MeasurementPointSet->GetPoint(i)[2] };
		double squared_distance = pow(point_reference[0] - point_for_measurement[0],2) + pow(point_reference[1] - point_for_measurement[1],2) + pow(point_reference[2] - point_for_measurement[2],2);
		double distance = sqrt(squared_distance);
		std::cout << "Distance Between Points: " << distance << std::endl;
		m_Controls->m_DistanceCalculationBetweenPointsWidget->AddDistanceToList(distance);
	}
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddReferencePosition()
{
	mitk::NavigationData::Pointer nd = m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());

	if (nd.IsNull() || !nd->IsDataValid())
	{
		QMessageBox::warning(0, "Invalid tracking data", "Navigation data is not available or invalid!", QMessageBox::Ok);
		return;
	}

	if (m_Controls->m_DistanceCalculationBetweenPointsWidget->GetReferencePositionNode().IsNotNull() && m_Controls->m_DistanceCalculationBetweenPointsWidget->GetReferencePositionNode()->GetData() != NULL)
	{
		mitk::PointSet::Pointer ps = dynamic_cast<mitk::PointSet*>(m_Controls->m_DistanceCalculationBetweenPointsWidget->GetReferencePositionNode()->GetData());
		ps->InsertPoint(ps->GetSize(), nd->GetPosition());
	}
	else
		QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "Can not access Tracker Fiducials. Adding fiducial not possible!");
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddMeasurementPosition()
{
	mitk::NavigationData::Pointer nd = m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());


	if (nd.IsNull() || !nd->IsDataValid())
	{
		QMessageBox::warning(0, "Invalid tracking data", "Navigation data is not available or invalid!", QMessageBox::Ok);
		return;
	}

	if (m_Controls->m_DistanceCalculationBetweenPointsWidget->GetMeasurementPositionNode().IsNotNull() && m_Controls->m_DistanceCalculationBetweenPointsWidget->GetMeasurementPositionNode()->GetData() != NULL)
	{
		mitk::PointSet::Pointer ps = dynamic_cast<mitk::PointSet*>(m_Controls->m_DistanceCalculationBetweenPointsWidget->GetMeasurementPositionNode()->GetData());
		ps->InsertPoint(ps->GetSize(), nd->GetPosition());
	}
	else
		QMessageBox::warning(NULL, "IGTSurfaceTracker: Error", "Can not access Tracker Fiducials. Adding fiducial not possible!");
}


/** @OnPointSetRecording allows control of distance from the tracking tool to 
the closest point on a selected surface */

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnDistanceControl(bool distance_control)

{
	mitk::DataStorage* ds = this->GetDataStorage();

	mitk::DataNode::Pointer distanceNode = mitk::DataNode::New();
	mitk::DataNode::Pointer psDistND;

	if (distance_control)
	{
		if (m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID() == -1)
		{
			QMessageBox::warning(NULL, "Error", "No tool selected for distance control!");
			m_Controls->m_DistanceControlCheckBox->setChecked(false);
			return;
		}

		m_PointSetRecordingNavigationData = m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());
		/*m_PSRegisteredLastPoint->SetPoint(0, m_PointSetRecordingNavigationData->GetPosition());
		m_PSRegisteredLastPoint->GetUpdatedGeometry();
		m_PSRegisteredLastPoint->GetGeometry()->Compose(m_MaxillofacialTrackingLab->GetITKRegistrationTransform());*/

		//initialize line that corresponds to minimum distance
		psDistND = ds->GetNamedNode("Distance line");

		if (m_DistanceLinePointSet.IsNull() || psDistND.IsNull())
		{		
			m_Controls->m_RemeshingWidget->InitiateDistanceControl(m_Controls->m_TargetSurfaceComboBox->GetSelectedNode());
			
			m_DistanceLinePointSet = mitk::PointSet::New();

			m_DistanceLinePointSet->InsertPoint(0, m_PointSetRecordingNavigationData->GetPosition());
			m_DistanceLinePointSet->InsertPoint(1, m_PointSetRecordingNavigationData->GetPosition());
			
			distanceNode->SetData(m_DistanceLinePointSet);
			distanceNode->SetName("Distance line");
			
			distanceNode->SetBoolProperty("show points", true);
			// mapper will hide the start and end point
			distanceNode->SetBoolProperty("show contour", true);
			// more important: mapper will hide/draw a line between all points in the set	
			distanceNode->SetOpacity(0.5);

			mitk::Color color;
			color.Set(1.0f, 1.0f, 0.0f);
			
			distanceNode->SetColor(color);
			ds->Add(distanceNode);
		}	
		
		m_DistanceControl = true;

	}

	else
	{
		distanceNode = ds->GetNamedNode("Distance line");
		ds->RemoveNodeEvent(distanceNode);

		if (m_DistanceLinePointSet.IsNotNull())
		{
			m_DistanceLinePointSet->Clear();
			m_DistanceLinePointSet = NULL;
		}
		m_DistanceControl = false;
	}
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::SetFocus()
{
	//m_Controls->m_UseAsPointerButton->setFocus();
}

bool QmitkMITKIGTMaxillofacialTrackingToolboxView::IsTransformDifferenceHigh(mitk::NavigationData::Pointer transformA, mitk::NavigationData::Pointer transformB, double euclideanDistanceThreshold, double angularDifferenceThreshold)
{
	if (transformA.IsNull() || transformA.IsNull())
	{
		return false;
	}
	mitk::Point3D posA, posB;
	posA = transformA->GetPosition();
	posB = transformB->GetPosition();


	if (posA.EuclideanDistanceTo(posB) > euclideanDistanceThreshold)
	{
		return true;
	}

	double returnValue;
	mitk::Quaternion rotA, rotB;
	rotA = transformA->GetOrientation();
	rotB = transformB->GetOrientation();

	itk::Vector<double, 3> point; //caution 5D-Tools: Vector must lie in the YZ-plane for a correct result.
	point[0] = 0.0;
	point[1] = 0.0;
	point[2] = 100000.0;

	rotA.normalize();
	rotB.normalize();

	itk::Matrix<double, 3, 3> rotMatrixA;
	for (int i = 0; i<3; i++) for (int j = 0; j<3; j++) rotMatrixA[i][j] = rotA.rotation_matrix_transpose().transpose()[i][j];

	itk::Matrix<double, 3, 3> rotMatrixB;
	for (int i = 0; i<3; i++) for (int j = 0; j<3; j++) rotMatrixB[i][j] = rotB.rotation_matrix_transpose().transpose()[i][j];

	itk::Vector<double, 3> pt1 = rotMatrixA * point;
	itk::Vector<double, 3> pt2 = rotMatrixB * point;

	returnValue = (pt1[0] * pt2[0] + pt1[1] * pt2[1] + pt1[2] * pt2[2]) / (sqrt(pow(pt1[0], 2.0) + pow(pt1[1], 2.0) + pow(pt1[2], 2.0)) * sqrt(pow(pt2[0], 2.0) + pow(pt2[1], 2.0) + pow(pt2[2], 2.0)));
	returnValue = acos(returnValue);

	if (returnValue*57.3 > angularDifferenceThreshold){ return true; }

	return false;
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnSurfaceToSurfaceDisalignmentControl()
{
	if (m_Controls->m_ControlSurfaceComboBox->GetSelectedNode().IsNotNull())
	{
		m_MaxillofacialTrackingLab->SelectControlSurface(m_Controls->m_ControlSurfaceComboBox->GetSelectedNode());
	}
	if (m_Controls->m_MovingSurfaceComboBox->GetSelectedNode().IsNotNull())
	{
		m_MaxillofacialTrackingLab->SelectMovingSurface(m_Controls->m_MovingSurfaceComboBox->GetSelectedNode());
	}
	vtkMatrix4x4 * matrix = m_MaxillofacialTrackingLab->calculateLandmarkbasedWithICP();
	//m_Controls->m_MovingSurfaceComboBox->GetSelectedNode()->GetData()->GetGeometry()->Compose(matrix);
	//m_Controls->m_MovingSurfaceComboBox->GetSelectedNode()->GetData()->GetTimeGeometry()->Update();
	//m_Controls->m_ControlSurfaceComboBox->GetSelectedNode()->GetData()->GetTimeGeometry()->Update();
	mitk::RenderingManager::GetInstance()->RequestUpdateAll();
	
	double FRE = m_MaxillofacialTrackingLab->checkLandmarkError();
	std::cout << FRE << std::endl;

	m_Controls->m_SurfaceToSurfaceDisalignment->setText(std::to_string(FRE).c_str());
}