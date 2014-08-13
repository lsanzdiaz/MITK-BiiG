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
#include <QmitkFiducialRegistrationWidget.h>
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
#include <mitkNDITrackingDevice.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNavigationToolStorageSerializer.h>

#include "MITKIGTMaxillofacialTrackingLab.h"

// vtk
#include <vtkSphereSource.h>
#include <vtkConeSource.h>
#include <vtkMatrix4x4.h>
#include "mitkCone.h"

//for exceptions
#include <mitkIGTException.h>
#include <mitkIGTIOException.h>

//Tracking
#include "mitkVirtualTrackingDevice.h"
#include "mitkTrackingDeviceSource.h"

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
  m_MaxillofacialTrackingLab = new MITKIGTMaxillofacialTrackingLab;
  
  //Initialize registration elements
  m_ImageFiducialsDataNode = NULL;
  m_TrackerFiducialsDataNode = NULL;
  
  //Initialize PointSetRecording elements  
  m_PSRecordingPointSet = NULL;
  m_PointSetRecording = false;
  
  //Initialize camera view elements
  m_CameraView = false;
  m_VirtualTracking = false;
  m_VirtualView = NULL;
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

    //create tracking connections
    connect( m_Controls->m_LoadTools, SIGNAL(clicked()), this, SLOT(OnLoadTools()) );
    connect( m_Controls->m_Connect, SIGNAL(clicked()), this, SLOT(OnConnect()) );
    connect( m_Controls->m_Disconnect, SIGNAL(clicked()), this, SLOT(OnDisconnect()) );
    connect( m_Controls->m_StartTracking, SIGNAL(clicked()), this, SLOT(OnStartTracking()) );
    connect( m_Controls->m_StopTracking, SIGNAL(clicked()), this, SLOT(OnStopTracking()) );
    connect( m_TrackingTimer, SIGNAL(timeout()), this, SLOT(UpdateTrackingTimer()));
	connect(m_Controls->m_AddSingleTool, SIGNAL(clicked()), this, SLOT(OnAddSingleTool()));
	connect(m_Controls->m_NavigationToolCreationWidget, SIGNAL(NavigationToolFinished()), this, SLOT(OnAddSingleToolFinished()));
	connect(m_Controls->m_NavigationToolCreationWidget, SIGNAL(Canceled()), this, SLOT(OnAddSingleToolCanceled()));
	connect(m_Controls->m_configurationWidget, SIGNAL(TrackingDeviceSelectionChanged()), this, SLOT(OnTrackingDeviceChanged()));
	connect(m_Controls->m_VolumeSelectionBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(OnTrackingVolumeChanged(QString)));
	connect(m_Controls->m_ShowTrackingVolume, SIGNAL(clicked()), this, SLOT(OnShowTrackingVolumeChanged()));
	connect(m_Controls->m_AutoDetectTools, SIGNAL(clicked()), this, SLOT(OnAutoDetectTools()));
	connect(m_Controls->m_ResetTools, SIGNAL(clicked()), this, SLOT(OnResetTools()));

	//added the option of performing virtual tracking
	connect(m_Controls->m_PerformVirtualTracking, SIGNAL(toggled(bool)), this, SLOT(OnPerformVirtualTracking(bool)));

	//create connections for Registration tab
	connect(m_Controls->m_UsePermanentRegistrationToggle, SIGNAL(toggled(bool)), this, SLOT(OnApplyRegistration(bool)));
	connect(m_Controls->m_TrackingDeviceSelectionWidget, SIGNAL(NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupNavigation()));
	connect(m_Controls->m_UseAsPointerButton, SIGNAL(clicked()), this, SLOT(OnInstrumentSelected()));
	connect(m_Controls->m_UseAsObjectmarkerButton, SIGNAL(clicked()), this, SLOT(OnObjectmarkerSelected()));
	connect(m_Controls->m_RegistrationWidget, SIGNAL(AddedTrackingFiducial()), this, SLOT(OnAddRegistrationTrackingFiducial()));
	connect(m_Controls->m_RegistrationWidget, SIGNAL(PerformFiducialRegistration()), this, SLOT(OnCalculateRegistration()));
	connect(m_Controls->m_PointSetRecordCheckBox, SIGNAL(toggled(bool)), this, SLOT(OnPointSetRecording(bool)));
	connect(m_Controls->m_ActivateNeedleView, SIGNAL(toggled(bool)), this, SLOT(OnVirtualCamera(bool)));
	
	//create connection for saving transform into file
	connect(m_Controls->m_ChooseTransformFile, SIGNAL(clicked()), this, SLOT(OnChooseTransformFileClicked()));

    //create connections for logging tab
	connect( m_Controls->m_ChooseFile, SIGNAL(clicked()), this, SLOT(OnChooseFileClicked()));
    connect( m_Controls->m_StartLogging, SIGNAL(clicked()), this, SLOT(StartLogging()));
    connect( m_Controls->m_StopLogging, SIGNAL(clicked()), this, SLOT(StopLogging()));
    connect( m_Controls->m_csvFormat, SIGNAL(clicked()), this, SLOT(OnToggleFileExtension()));
    connect( m_Controls->m_xmlFormat, SIGNAL(clicked()), this, SLOT(OnToggleFileExtension()));

	//initialize widgets
    m_Controls->m_configurationWidget->EnableAdvancedUserControl(false);
    m_Controls->m_TrackingToolsStatusWidget->SetShowPositions(true);
    m_Controls->m_TrackingToolsStatusWidget->SetTextAlignment(Qt::AlignLeft);
	CreateBundleWidgets(parent);
    
	//initialize tracking volume node
    m_TrackingVolumeNode = mitk::DataNode::New();
    m_TrackingVolumeNode->SetName("TrackingVolume");
    m_TrackingVolumeNode->SetOpacity(0.25);
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
    m_Controls->m_AutoDetectTools->setVisible(false); //only visible if tracking device is Aurora

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

	//initialize Combo Boxes
	m_Controls->m_ObjectComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_ObjectComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_ObjectComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));

	m_Controls->m_ImageComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_ImageComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_ImageComboBox->SetPredicate(mitk::NodePredicateDataType::New("Image"));
  
	m_Controls->m_TargetObjectComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_TargetObjectComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_TargetObjectComboBox->SetPredicate(mitk::NodePredicateDataType::New("Surface"));

	m_Controls->m_TargetImageComboBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_TargetImageComboBox->SetAutoSelectNewItems(false);
	m_Controls->m_TargetImageComboBox->SetPredicate(mitk::NodePredicateDataType::New("Image"));
	
  }
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
  
	//check if the Virtual Tracking option is enabled. If so, perform virtual tracking
	if (m_VirtualTracking = true)
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

  //set device to rotation mode transposed becaus we are working with VNL style quaternions
  if(m_Controls->m_InverseMode->isChecked())
    trackingDevice->SetRotationMode(mitk::TrackingDevice::RotationTransposed);

  //Get Tracking Volume Data
  mitk::TrackingDeviceData data = mitk::DeviceDataUnspecified;

  QString qstr =  m_Controls->m_VolumeSelectionBox->currentText();
  if ( (! qstr.isNull()) || (! qstr.isEmpty()) ) {
    std::string str = qstr.toStdString();
    data = mitk::GetDeviceDataByName(str); //Data will be set later, after device generation
  }

  //Create Navigation Data Source with the factory class
  mitk::TrackingDeviceSourceConfigurator::Pointer myTrackingDeviceSourceFactory = mitk::TrackingDeviceSourceConfigurator::New(this->m_toolStorage,trackingDevice);
  m_TrackingDeviceSource = myTrackingDeviceSourceFactory->CreateTrackingDeviceSource(this->m_ToolVisualizationFilter);

  if ( m_TrackingDeviceSource.IsNull() )
  {
    MessageBox(std::string("Cannot connect to device: ") + myTrackingDeviceSourceFactory->GetErrorMessage());
    return;
  }

  //set filter to rotation mode transposed becaus we are working with VNL style quaternions
  if(m_Controls->m_InverseMode->isChecked())
    m_ToolVisualizationFilter->SetRotationMode(mitk::NavigationDataObjectVisualizationFilter::RotationTransposed);

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
    for (int i=0; i < toolsInNewOrder->GetToolCount(); i++) {m_toolStorage->AddTool(toolsInNewOrder->GetTool(i));}
    }

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

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnVirtualConnect()
{
	//build the Virtual IGT pipeline instead of the IGT pipeline for a real tracker device
	
	// mitk::TrackingDevice::Pointer trackingDevice = this->m_Controls->m_configurationWidget->GetTrackingDevice();

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
	//mitk::Surface::Pointer mySphere = mitk::Surface::New();
	mitk::Cone::Pointer cone = mitk::Cone::New();
	double scale[] = { 10.0, 10.0, 10.0 };
	cone->GetGeometry()->SetSpacing(scale);                       //scale it a little that so we can see something
	vtkConeSource *vtkData = vtkConeSource::New();
	vtkData->SetRadius(3.0f);
	vtkData->SetAngle(20);
	vtkData->SetCenter(0.0, 0.0, 0.0);
	vtkData->Update();
	cone->SetVtkPolyData(vtkData->GetOutput());
	vtkData->Delete();
	newNode->SetData(cone);
	newNode->SetName("Virtual Tool");
	newNode->SetColor(1.0, 0.0, 0.0);
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
	
	//Create ToolVisualizationFilter for tool "MyTool" and set m_TrackingDeviceSource as an input

	m_ToolVisualizationFilter = mitk::NavigationDataObjectVisualizationFilter::New();

	m_ToolVisualizationFilter->SetInput(0, m_TrackingDeviceSource->GetOutput());
	m_ToolVisualizationFilter->SetRepresentationObject(0, NavigationTool->GetDataNode()->GetData());

	
	//Number of tools
	MITK_INFO << "Number of tools: " << m_TrackingDeviceSource->GetNumberOfOutputs();
	
	//remove real navigation tools from toolStorage
	this->m_toolStorage->DeleteAllTools();
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
    m_Controls->m_TrackingToolsStatusWidget->AddNavigationData(m_TrackingDeviceSource->GetOutput(i));
  }
  m_Controls->m_TrackingToolsStatusWidget->ShowStatusLabels();
  if (m_Controls->m_ShowToolQuaternions->isChecked()) {m_Controls->m_TrackingToolsStatusWidget->SetShowQuaternions(true);}
  else {m_Controls->m_TrackingToolsStatusWidget->SetShowQuaternions(false);}

  //add other device: VIRTUAL
  


  //show tracking volume
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
		  m_Controls->m_AutoDetectTools->setVisible(true);
		  m_Controls->m_AddSingleTool->setEnabled(false);
	  }
	  else //Polaris or Microntracker
	  {
		  m_Controls->m_AutoDetectTools->setVisible(false);
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

	//Building up the filter pipeline
	InitializeRegistration();

}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnCalculateRegistration()
{	
	//Check for initialization
	if (!CheckRegistrationInitialization()) return;

	m_MaxillofacialTrackingLab->CalculateRegistration(m_ImageFiducialsDataNode, m_TrackerFiducialsDataNode);
	m_Controls->m_RegistrationWidget->SetQualityDisplayText("FRE: " + QString::number(m_MaxillofacialTrackingLab->FRE) + " mm");
	
	//register tool on object (surface)

	m_ToolVisualizationFilter->SetOffset(0, m_MaxillofacialTrackingLab->RegistrationTransform);


	//save transform to file
	std::ofstream myfile;
	vtkMatrix4x4 * matrix = m_MaxillofacialTrackingLab->GetVTKTransform()->GetMatrix();
	
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
		std::string warningmessage = "Tranform file has not being specified or does not exist. Registration will not be saved on file.";
		QMessageBox::warning(NULL, "IGTPlayer: Error", warningmessage.c_str());
	}	
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnApplyRegistration(bool on)
{
	if (on)
	{
		if (CheckRegistrationInitialization())
		{
			//first: apply permanent registration of tool
			m_ToolVisualizationFilter->SetOffset(0, m_MaxillofacialTrackingLab->RegistrationTransform);

			/*Test:: If we wanted to move the surface*/
     		//connect filter to source
			m_ToolVisualizationFilter->SetInput(1, this->m_ObjectmarkerNavigationData);

			//set representation object
			m_ToolVisualizationFilter->SetRepresentationObject(1, this->m_Controls->m_ObjectComboBox->GetSelectedNode()->GetData());
			m_ToolVisualizationFilter->SetOffset(1, m_MaxillofacialTrackingLab->RegistrationTransform);

		}		
	}

	else //if off = disable the permanent registration
	{
		m_ToolVisualizationFilter->SetOffset(0, NULL);
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

	/* We need to know if a surface or object has been selected, since after the registration the movement 
	  of the tool will correspond to the movement of the object or surface in the image.*/

	if (m_Controls->m_SurfaceActive->isChecked() && m_Controls->m_ObjectComboBox->GetSelectedNode().IsNull())
	{
		warningMessage = "No surface selected for registration.\nRegistration is not possible";
		initializationErrorDetected = true;
	}
	else if (m_Controls->m_ImageActive->isChecked() && m_Controls->m_ImageComboBox->GetSelectedNode().IsNull())
	{
		warningMessage = "No image selected for registration.\nRegistration is not possible";
		initializationErrorDetected = true;
	}
	else if (imageFiducials.IsNull() || trackerFiducials.IsNull())
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

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnInstrumentSelected()
{
	if (m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource().IsNotNull())
	{
		m_InstrumentNavigationData = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID());
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
}

/** This method is called when the object marker is selected.
*  It stores the navigation data of the object marker.
*/
void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnAddRegistrationTrackingFiducial()
{
	mitk::NavigationData::Pointer nd = m_InstrumentNavigationData;

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

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnObjectmarkerSelected()
{
	if (m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource().IsNotNull())
	{
		m_ObjectmarkerNavigationData = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID());
		MITK_INFO << "Objectmarker rotation: " << m_ObjectmarkerNavigationData->GetOrientation();
	}
	else
	{
		m_Controls->m_ObjectmarkerNameLabel->setText("<not available>");
		return;
	}

	if (m_ObjectmarkerNavigationData.IsNotNull())
	{
		m_Controls->m_ObjectmarkerNameLabel->setText(m_ObjectmarkerNavigationData->GetName());
	}
	else
	{
		m_Controls->m_ObjectmarkerNameLabel->setText("<not available>");
	}
}


void QmitkMITKIGTMaxillofacialTrackingToolboxView::MessageBox(std::string s)
  {
  QMessageBox msgBox;
  msgBox.setText(s.c_str());
  msgBox.exec();
  }

void QmitkMITKIGTMaxillofacialTrackingToolboxView::UpdateTrackingTimer()
  {
  m_ToolVisualizationFilter->Update();
  MITK_DEBUG << "Number of outputs ToolVisualizationFilter: " << m_ToolVisualizationFilter->GetNumberOfIndexedOutputs();
  MITK_DEBUG << "Number of inputs ToolVisualizationFilter: " << m_ToolVisualizationFilter->GetNumberOfIndexedInputs();
  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
  if (m_logging)
    {
    this->m_loggingFilter->Update();
    m_loggedFrames = this->m_loggingFilter->GetRecordCounter();
    m_Controls->m_LoggedFramesLabel->setText("Logged Frames: "+QString::number(m_loggedFrames));
    //check if logging stopped automatically
    if((m_loggedFrames>1)&&(!m_loggingFilter->GetRecording())){StopLogging();}
    }
  m_Controls->m_TrackingToolsStatusWidget->Refresh();

  if (m_CameraView && m_VirtualView.IsNotNull()) { m_VirtualView->Update(); }

  if (m_PointSetRecording && m_PSRecordingPointSet.IsNotNull())
  {
	  int size = m_PSRecordingPointSet->GetSize();
	  mitk::NavigationData::Pointer nd = m_PointSetRecordingNavigationData;

	  if (size > 0)
	  {
		  mitk::Point3D p = m_PSRecordingPointSet->GetPoint(size - 1);
		  if (p.EuclideanDistanceTo(nd->GetPosition()) > (double)m_Controls->m_PSRecordingSpinBox->value())
			  m_PSRecordingPointSet->InsertPoint(size, nd->GetPosition());
	  }
	  else
		  m_PSRecordingPointSet->InsertPoint(size, nd->GetPosition());
  }
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
    if(!filename.empty())
    m_loggingFilter->SetFileName(filename);
    else if(filename.empty()){
     std::string errormessage = "File name has not been set, please set the file name";
     mitkThrowException(mitk::IGTIOException)<<errormessage;
     QMessageBox::warning(NULL, "IGTPlayer: Error", errormessage.c_str());
     m_loggingFilter->SetFileName(filename);
    }

    if (m_Controls->m_LoggingLimit->isChecked()){m_loggingFilter->SetRecordCountLimit(m_Controls->m_LoggedFramesLimit->value());}

    //connect filter
    for(int i=0; i<m_ToolVisualizationFilter->GetNumberOfOutputs(); i++){m_loggingFilter->AddNavigationData(m_ToolVisualizationFilter->GetOutput(i));}

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
  QString Identifier = "Tool#";
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
    m_Controls->m_ShowToolQuaternions->setEnabled(false);
    m_Controls->m_OptionsUpdateRateLabel->setEnabled(false);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::EnableOptionsButtons()
{
    m_Controls->m_ShowTrackingVolume->setEnabled(true);
    m_Controls->m_UpdateRate->setEnabled(true);
    m_Controls->m_ShowToolQuaternions->setEnabled(true);
    m_Controls->m_OptionsUpdateRateLabel->setEnabled(true);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::EnableTrackingConfigurationButtons()
{
    m_Controls->m_AutoDetectTools->setEnabled(true);
    if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::NDIAurora) m_Controls->m_AddSingleTool->setEnabled(true);
    m_Controls->m_LoadTools->setEnabled(true);
    m_Controls->m_ResetTools->setEnabled(true);
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::DisableTrackingConfigurationButtons()
{
    m_Controls->m_AutoDetectTools->setEnabled(false);
    if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::NDIAurora) m_Controls->m_AddSingleTool->setEnabled(false);
    m_Controls->m_LoadTools->setEnabled(false);
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
		if (m_Controls->m_PointSetRecordingToolSelectionWidget->GetSelectedToolID() == -1)
		{
			QMessageBox::warning(NULL, "Error", "No tool selected for point set recording!");
			m_Controls->m_PointSetRecordCheckBox->setChecked(false);
			return;
		}

		m_PointSetRecordingNavigationData = m_Controls->m_PointSetRecordingToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_PointSetRecordingToolSelectionWidget->GetSelectedToolID());

		//initialize point set
		psRecND = ds->GetNamedNode("Recorded Points");

		if (m_PSRecordingPointSet.IsNull() || psRecND.IsNull())
		{
			m_PSRecordingPointSet = NULL;
			m_PSRecordingPointSet = mitk::PointSet::New();
			dn->SetName("Recorded Points");
			dn->SetColor(0.,1.,0.);
			dn->SetData(m_PSRecordingPointSet);
			ds->Add(dn);
	}

	else
	{
		m_PSRecordingPointSet->Clear();
	}
		m_PointSetRecording = true;
	}

	else
	{
	m_PointSetRecording = false;
	}

	/*****************Code for checking distance to target surface ********************/
	/*mitk::Surface::Pointer mySphere = mitk::Surface::New(); 
	vtkConeSource *vtkData = vtkConeSource::New();
	vtkData->SetRadius(2.0f);
	vtkData->SetAngle(20);
	vtkData->SetCenter(0.0, 0.0, 0.0);
	vtkData->Update();
	mySphere->SetVtkPolyData(vtkData->GetOutput());
	vtkData->Delete();
	vtkSmartPointer<vtkDistancePolyDataFilter> Normal_distances = vtkSmartPointer<vtkDistancePolyDataFilter>::New();
	Normal_distances->SetInputData(0,mySphere->GetVtkPolyData());
	Normal_distances->SetInputData(1, dynamic_cast<vtkDataObject*>(psRecND->GetData()));
	
	Normal_distances->GetOutput();
	int distance = Normal_distances->GetSignedDistance();*/
}

//* @OnVirtualCamera allows the visualization from a camera perspective */

void QmitkMITKIGTMaxillofacialTrackingToolboxView::OnVirtualCamera(bool on)
{
	if (m_Controls->m_CameraViewSelection->GetSelectedToolID() == -1)
	{
		m_Controls->m_ActivateNeedleView->setChecked(false);
		QMessageBox::warning(NULL, "Error", "No tool selected for camera view!");
		return;
	}

	if(on)
	{
		m_VirtualView = mitk::CameraVisualization::New();
		m_VirtualView->SetInput(m_Controls->m_CameraViewSelection->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_CameraViewSelection->GetSelectedToolID()));

		mitk::Vector3D viewDirection;
		viewDirection[0] = (int)(m_Controls->m_NeedleViewX->isChecked());
		viewDirection[1] = (int)(m_Controls->m_NeedleViewY->isChecked());
		viewDirection[2] = (int)(m_Controls->m_NeedleViewZ->isChecked());
		if (m_Controls->m_NeedleViewInvert->isChecked()) viewDirection *= -1;
		m_VirtualView->SetDirectionOfProjectionInToolCoordinates(viewDirection);

		mitk::Vector3D viewUpVector;
		viewUpVector[0] = (int)(m_Controls->m_NeedleUpX->isChecked());
		viewUpVector[1] = (int)(m_Controls->m_NeedleUpY->isChecked());
		viewUpVector[2] = (int)(m_Controls->m_NeedleUpZ->isChecked());
		if (m_Controls->m_NeedleUpInvert->isChecked()) viewUpVector *= -1;
		m_VirtualView->SetViewUpInToolCoordinates(viewUpVector);
		
		m_VirtualView->SetRenderer(this->GetRenderWindowPart()->GetQmitkRenderWindow("3d")->GetRenderer());
		//next line: better code when this plugin is migrated to mitk::abstractview
		//m_VirtualView->SetRenderer(mitk::BaseRenderer::GetInstance(this->GetRenderWindowPart()->GetRenderWindow("3d")->GetRenderWindow()));
		m_CameraView = true;

		//make pointer itself invisible
		m_Controls->m_CameraViewSelection->GetSelectedNavigationTool()->GetDataNode()->SetBoolProperty("visible", false);

		//disable UI elements
		m_Controls->m_ViewDirectionBox->setEnabled(false);
		m_Controls->m_ViewUpBox->setEnabled(false);
	}
	else
	{
		m_VirtualView = NULL;
		m_CameraView = false;
		m_Controls->m_CameraViewSelection->GetSelectedNavigationTool()->GetDataNode()->SetBoolProperty("visible", true);

		m_Controls->m_ViewDirectionBox->setEnabled(true);
		m_Controls->m_ViewUpBox->setEnabled(true);
	}
	
}

void QmitkMITKIGTMaxillofacialTrackingToolboxView::SetFocus()
{
	m_Controls->m_UseAsPointerButton->setFocus();
}