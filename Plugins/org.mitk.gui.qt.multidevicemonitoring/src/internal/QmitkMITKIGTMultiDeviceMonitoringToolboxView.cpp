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
#include "QmitkMITKIGTMultiDeviceMonitoringToolboxView.h"

#include <QmitkNDIConfigurationWidget.h>
#include <mitkConoprobeDevice.h>
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
#include <vtkPlane.h>
#include <vtkPlaneSource.h>

//for exceptions
#include <mitkIGTException.h>
#include <mitkIGTIOException.h>

//Tracking
#include "mitkVirtualTrackingDevice.h"
#include "mitkTrackingDeviceSource.h"

//STL Reader

#include <mitkSTLFileReader.h>

//XML Reader

#include "tinyxml.h"
#include <istream>


#include "mitkDistanceMonitoring.h"

const std::string QmitkMITKIGTMultiDeviceMonitoringToolboxView::VIEW_ID = "org.mitk.views.QMITKMITKIGTMultiDeviceMonitoringToolbox";

QmitkMITKIGTMultiDeviceMonitoringToolboxView::QmitkMITKIGTMultiDeviceMonitoringToolboxView()
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

  //Initialize tracking lab
  m_MultiDeviceMonitoringLab = new MITKMaxillofacialTrackingLab();

  m_VirtualTimeStamp = QmitkPlotWidget::DataVector();
  m_Distance= QmitkPlotWidget::DataVector();

  m_ToolVisualizationFilterVector = std::vector<mitk::MultiDeviceMonitoringVisualizationFilter::Pointer>();
  m_TrackingDeviceSourceVector = std::vector<mitk::TrackingDeviceSource::Pointer>();
  m_TrackingDeviceDataVector = std::vector<mitk::TrackingDeviceData>();

}

QmitkMITKIGTMultiDeviceMonitoringToolboxView::~QmitkMITKIGTMultiDeviceMonitoringToolboxView()
{
//remove the tracking volume
this->GetDataStorage()->Remove(m_TrackingVolumeNode);
//remove the tool storage
m_toolStorage->UnRegisterMicroservice();
}


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::CreateQtPartControl( QWidget *parent )
{
  // build up qt view, unless already done
  if ( !m_Controls )
  {
    // create GUI widgets from the Qt Designer's .ui file
    m_Controls = new Ui::QmitkMITKIGTMultiDevicemonitoringToolboxViewControls;
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
	connect(m_Controls->m_AutoDetectTools, SIGNAL(clicked()), this, SLOT(OnAutoDetectTools()));
	connect(m_Controls->m_ResetTools, SIGNAL(clicked()), this, SLOT(OnResetTools()));
	connect(m_Controls->m_NavigationToolCreationWidget, SIGNAL(NavigationToolFinished()), this, SLOT(OnAddSingleToolFinished()));
	connect(m_Controls->m_NavigationToolCreationWidget, SIGNAL(Canceled()), this, SLOT(OnAddSingleToolCanceled()));
	connect(m_Controls->m_configurationWidget, SIGNAL(TrackingDeviceSelectionChanged()), this, SLOT(OnTrackingDeviceChanged()));

	//added the option to set whether a tool a reference marker or not
	connect(m_Controls->m_ToolIsReferenceFramework, SIGNAL(toggled(bool)), this, SLOT(OnSetAsReferenceFramework(bool)));

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
	
	connect(m_Controls->m_TrajectoryControlToolSelectionWidget, SIGNAL(NavigationDataSourceSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupToolForTrajectoryControl()));
	connect(m_Controls->m_TrajectoryControlToolSelectionWidget, SIGNAL(NavigationToolSelected(mitk::NavigationDataSource::Pointer)), this, SLOT(OnSetupTool()));


	connect(m_Controls->m_RegistrationSurfaceComboBox, SIGNAL(OnSelectionChanged(const mitk::DataNode *)), this, SLOT(m_RegistrationSurfaceChanged(const mitk::DataNode *)));
	connect(m_Controls->m_ToolToSurfaceRegistration, SIGNAL(toggled(bool)), this, SLOT(OnToolToSurfaceRegistration(bool)));
	connect(m_Controls->m_GeneralRegistration, SIGNAL(toggled(bool)), this, SLOT(OnGeneralRegistration(bool)));

    //create connections for logging tab
	connect(m_Controls->m_ChooseFile, SIGNAL(clicked()), this, SLOT(OnChooseFileClicked()));
    connect(m_Controls->m_StartLogging, SIGNAL(clicked()), this, SLOT(StartLogging()));
    connect(m_Controls->m_StopLogging, SIGNAL(clicked()), this, SLOT(StopLogging()));
    connect(m_Controls->m_csvFormat, SIGNAL(clicked()), this, SLOT(OnToggleFileExtension()));
    connect(m_Controls->m_xmlFormat, SIGNAL(clicked()), this, SLOT(OnToggleFileExtension()));

	//create connection for reading XML file and preload settings
	connect(m_Controls->m_PreloadSettingsBtn, SIGNAL(clicked()), this, SLOT(OnPreloadSettingsClicked()));
	connect(m_Controls->m_ChooseSettingsFileBtn, SIGNAL(clicked()), this, SLOT(OnChooseSettingsFileClicked()));

	// create connection for saving transform into a file
	connect(m_Controls->m_SaveRegistrationTransformBtn, SIGNAL(clicked()), this, SLOT(OnChooseTransformFileNameClicked()));

	connect(m_Controls->m_SaveCurrentSettingsBtn, SIGNAL(clicked), this, SLOT(OnSaveCurrentSettingsClicked()));

	//initialize widgets
    m_Controls->m_configurationWidget->EnableAdvancedUserControl(false);
    m_Controls->m_TrackingToolsStatusWidget->SetShowPositions(true);
    m_Controls->m_TrackingToolsStatusWidget->SetTextAlignment(Qt::AlignLeft);
	
	
	CreateBundleWidgets(parent);
   
	//Tracking volume connections. TO-DO REMOVE
	connect(m_Controls->m_VolumeSelectionBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(OnTrackingVolumeChanged(QString)));
	connect(m_Controls->m_ShowTrackingVolume, SIGNAL(clicked()), this, SLOT(OnShowTrackingVolumeChanged()));


	connect(m_Controls->m_Interpolate3DModel, SIGNAL(clicked()), this, SLOT(OnInterpolate3DModel()));

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
    m_Controls->m_AutoDetectTools->setVisible(false); //only visible if tracking device is Aurora
	//m_Controls->m_LoadTools->setEnabled(false);
	//m_Controls->m_LoadTools->setVisible(false);

    //Update List of available models for selected tool.
    std::vector<mitk::TrackingDeviceData> Compatibles = mitk::GetDeviceDataForLine(m_Controls->m_configurationWidget->GetTrackingDevice()->GetType());
    m_Controls->m_VolumeSelectionBox->clear();
    for(int i = 0; i < Compatibles.size(); i++)
    {
      m_Controls->m_VolumeSelectionBox->addItem(Compatibles[i].Model.c_str());
    }

	m_Controls->m_ToolInfoTable->resizeColumnToContents(1);
	m_Controls->m_ToolInfoTable->resizeRowsToContents();
	m_Controls->m_ToolInfoTable->setFocusPolicy(Qt::NoFocus);
	
    //initialize tool storage
    m_toolStorage = mitk::NavigationToolStorage::New(GetDataStorage());
    m_toolStorage->SetName("MultiDeviceMonitoringToolbox Default Storage");
    m_toolStorage->RegisterAsMicroservice("no tracking device");

    //set home directory as default path for logfile
    m_Controls->m_LoggingFileName->setText(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + "logfile.txt");
  
	//set home directory as default path for logfile
	m_Controls->m_TransformFileName->setText(QDir::toNativeSeparators(QDir::homePath()) + QDir::separator() + "transform.txt");


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

	m_Controls->m_PointsetSelectionBox->SetDataStorage(this->GetDataStorage());
	m_Controls->m_PointsetSelectionBox->SetAutoSelectNewItems(false);
	m_Controls->m_PointsetSelectionBox->SetPredicate(mitk::NodePredicateDataType::New("PointSet"));
	

	m_Controls->m_DistanceMonitoringWidget->SetAxisTitle(2, "Time (hh:mm:ss)");
	m_Controls->m_DistanceMonitoringWidget->SetAxisTitle(0, "Distance (mm)");
	m_Controls->m_DistanceMonitoringWidget->InsertCurve("Distance(mm)");

  }
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::LoadModel(std::string filename)
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnPreloadSettings(std::string filename)
{
	std::string CalibrationFile;
	std::string ToolName;
	std::string ToolDefinitionFile;
	std::string ToolRepresentationObject;

	TiXmlDocument doc(filename);
	
	if (doc.LoadFile())
	{
		TiXmlHandle hDoc(&doc);
		TiXmlElement *pData, *pTrackingDevice, *pToolData, *pTool;
		
		m_Controls->m_configurationWidget->SetTrackingDeviceType(3);

		//Data
		pData = doc.FirstChildElement("Data");
		if (pData)
		{

			//Tracking Device Data
			pTrackingDevice = pData->FirstChildElement("TrackingDevice");
			if (pTrackingDevice)
			{		
				CalibrationFile = pTrackingDevice->Attribute("CalibrationFile");
				m_Controls->m_configurationWidget->SetOptitrackCalibrationFile(CalibrationFile);
			}

			//Tool data
			pToolData = pData->FirstChildElement("ToolData");
			if (pToolData)
			{
				OnResetTools();
				pTool = pToolData->FirstChildElement("Tool");
				int i = 0; // for sorting the entries
				while (pTool)
				{
					ToolName = pTool->Attribute("Name");
					ToolDefinitionFile = pTool->Attribute("ToolDefinition");
					ToolRepresentationObject = pTool->Attribute("ToolRepresentation");

					//Create tool with xml parsed tool information
					this->OnAddSingleTool();
					this->m_Controls->m_NavigationToolCreationWidget->PreloadToolSettings(ToolName, ToolDefinitionFile, ToolRepresentationObject);
					this->m_Controls->m_NavigationToolCreationWidget->AddToolFinished();
					this->OnAddSingleToolFinished();

					//Go to next tool node
					pTool = pTool->NextSiblingElement("Tool");
					i++;
				}
				std::cout << "Tool Number: " << i << std::endl;
			}
		}
	}
	else
	{
		std::cout << "Could not load XML File." << std::endl;
		return;
	}


	m_Controls->m_ShowTrackingVolume->setChecked(false);

	this->OnConnect();
	this->OnStartTracking();
	
	
	//m_InstrumentNavigationData = m_TrackingDeviceSource->GetOutput(0);
	

	//TO-DO Open surface models
	std::string m_SurfaceName = "J:/External_resources/Mandibula/Jaw.stl";
	LoadModel(m_SurfaceName);

	m_SurfaceName = "J:/External_resources/Renders_craneo/Craneo2aOpcion_LabelUnica_decimate2.stl";
	LoadModel(m_SurfaceName);
	
 	GlobalReinit();

}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnLoadTools()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnResetTools()
{
  this->ReplaceCurrentToolStorage(mitk::NavigationToolStorage::New(GetDataStorage()),"MultiDeviceMonitoringToolbox Default Storage");
  m_Controls->m_TrackingToolsStatusWidget->RemoveStatusLabels();
  QString toolLabel = QString("Loaded Tools: <none>");
  m_Controls->m_toolLabel->setText(toolLabel);
  
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnConnect()
  {

	//check if everything is ready to start tracking

	mitk::TrackingDeviceType Type;

	Type = m_Controls->m_configurationWidget->GetTrackingDevice()->GetType();


  
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
  trackingDevice->SetData(TrackingDeviceData);

  m_TrackingDeviceDataVector.resize(m_TrackingDeviceDataVector.size() + 1);
  m_TrackingDeviceDataVector.push_back(TrackingDeviceData);

  //Get Tracking Volume Data
  mitk::TrackingDeviceData data = mitk::DeviceDataUnspecified;

  QString qstr =  m_Controls->m_VolumeSelectionBox->currentText();
  if ( (! qstr.isNull()) || (! qstr.isEmpty()) ) {
    std::string str = qstr.toStdString();
    data = mitk::GetDeviceDataByName(str); //Data will be set later, after device generation
  }


  //Create Navigation Data Source with the factory class
  mitk::TrackingDeviceSourceConfigurator::Pointer myTrackingDeviceSourceFactory = mitk::TrackingDeviceSourceConfigurator::New(this->m_toolStorage, trackingDevice);
  mitk::TrackingDeviceSource::Pointer TrackingDeviceSource = myTrackingDeviceSourceFactory->CreateTrackingDeviceSource();
  mitk::MultiDeviceMonitoringVisualizationFilter::Pointer ToolVisualizationFilter = mitk::MultiDeviceMonitoringVisualizationFilter::New();

  for (unsigned int i = 0; i<TrackingDeviceSource->GetNumberOfIndexedOutputs(); i++)
  {
	  mitk::NavigationTool::Pointer currentTool = this->m_toolStorage->GetTool(i);
	  std::cout << "Tool Name" << TrackingDeviceSource->GetOutput(i)->GetName() << endl;
	  if (currentTool.IsNull())
	  {
		  MessageBox("Error: did not find corresponding tool in tracking device after initialization.");
		  return;
	  }
	  ToolVisualizationFilter->SetInput(i, TrackingDeviceSource->GetOutput(i));
	  ToolVisualizationFilter->SetRepresentationObject(i, currentTool->GetDataNode()->GetData());
  }

  m_ToolVisualizationFilterVector.resize(m_ToolVisualizationFilterVector.size() + 1);
  m_ToolVisualizationFilterVector.push_back(ToolVisualizationFilter);

  if (TrackingDeviceSource.IsNull() )
  {
    MessageBox(std::string("Cannot connect to device: ") + myTrackingDeviceSourceFactory->GetErrorMessage());
    return;
  }

  //First check if the created object is valid
  if (TrackingDeviceSource.IsNull())
  {
    MessageBox(myTrackingDeviceSourceFactory->GetErrorMessage());
    return;
  }

  MITK_INFO << "Number of tools: " << TrackingDeviceSource->GetNumberOfOutputs();

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

  

  //connect to device
  try
    {
    TrackingDeviceSource->Connect();
    //Microservice registration:
    TrackingDeviceSource->RegisterAsMicroservice();
    m_toolStorage->UnRegisterMicroservice();
    m_toolStorage->RegisterAsMicroservice(TrackingDeviceSource->GetMicroserviceID());
    m_toolStorage->LockStorage();
	m_TrackingDeviceSourceVector.resize(m_TrackingDeviceSourceVector.size() + 1);
	m_TrackingDeviceSourceVector.push_back(TrackingDeviceSource);	
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

  m_Controls->m_PreloadSettingsFileName->setEnabled(false);

  DisableOptionsButtons();
  DisableTrackingConfigurationButtons();
  m_Controls->m_configurationWidget->ConfigurationFinished();

  m_Controls->m_TrackingControlLabel->setText("Status: connected");
  }


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnDisconnect()
  {
  if (m_tracking) this->OnStopTracking();

  for (int i = 0; i < m_TrackingDeviceSourceVector.size(); i++)
  {
	  m_TrackingDeviceSourceVector.at(i)->Disconnect();
	  m_TrackingDeviceSourceVector.at(i)->UnRegisterMicroservice();
  }
  m_toolStorage->UnLockStorage();

  //enable/disable Buttons
  m_Controls->m_Disconnect->setEnabled(false);
  m_Controls->m_StartTracking->setEnabled(false);
  m_Controls->m_StopTracking->setEnabled(false);
  m_Controls->m_Connect->setEnabled(true);
  m_Controls->m_PreloadSettingsFileName->setEnabled(true);
  EnableOptionsButtons();
  EnableTrackingConfigurationButtons();
  m_Controls->m_configurationWidget->Reset();
  m_Controls->m_TrackingControlLabel->setText("Status: disconnected");
  m_Controls->m_PerformVirtualTracking->setEnabled(true);

  }

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnStartTracking()
{
	for (int i = 0; i < m_TrackingDeviceSourceVector.size(); i++)
	{
		try
		{
			m_TrackingDeviceSourceVector.at(i)->StartTracking();
		}
		catch (...) //todo: change to mitk::IGTException
		{
			MessageBox("Error while starting the tracking device!");
			return;
		}
    }
 

  m_TrackingTimer->start(1000/(m_Controls->m_UpdateRate->value()));
  m_Controls->m_TrackingControlLabel->setText("Status: tracking");

  //connect the tool visualization widget
  for (int numOfTracker = 0; numOfTracker < m_TrackingDeviceSourceVector.size(); numOfTracker++)
  {
	  mitk::MultiDeviceMonitoringVisualizationFilter::Pointer ToolVisualizationFilter = m_ToolVisualizationFilterVector.at(numOfTracker);
	  for (int i = 0; i < m_TrackingDeviceSourceVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
	  {
		  m_Controls->m_TrackingToolsStatusWidget->AddNavigationData(ToolVisualizationFilter->GetOutput(i));
		  //std::cout << "AddNavigationData, tool : " << i << endl;
	  }
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnStopTracking()
{
  if (!m_tracking) return;
  m_TrackingTimer->stop();
  for (int i = 0; i < m_TrackingDeviceSourceVector.size(); i++)
  {
	  m_TrackingDeviceSourceVector.at(i)->StopTracking();
  }
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnTrackingDeviceChanged()
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
	  if (Type == mitk::NDIAurora || Type == mitk::Conoprobe) //Trackers that detect the attached tools automatically
	  {
		  m_Controls->m_AutoDetectTools->setVisible(true);
		  m_Controls->m_AddSingleTool->setEnabled(false);
	  }

	  else if (Type == mitk::AscensionMEDSAFE)
	  {
		  m_Controls->m_TrackingToolsGroupBox->setEnabled(false);
		  m_Controls->m_TrackingControlsGroupBox->setEnabled(true);
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnTrackingVolumeChanged(QString qstr)
{
 if (qstr.isNull()) return;
  if (qstr.isEmpty()) return;
  if (m_Controls->m_ShowTrackingVolume->isChecked())
  {
    mitk::TrackingVolumeGenerator::Pointer volumeGenerator = mitk::TrackingVolumeGenerator::New();

    std::string str = qstr.toStdString();

    mitk::TrackingDeviceData data = mitk::GetDeviceDataByName(str);
    TrackingDeviceData = data;

    volumeGenerator->SetTrackingDeviceData(data);
    volumeGenerator->Update();

    mitk::Surface::Pointer volumeSurface = volumeGenerator->GetOutput();

    m_TrackingVolumeNode->SetData(volumeSurface);

    GlobalReinit();
  }
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnShowTrackingVolumeChanged()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnAutoDetectTools()
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

else if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() == mitk::Conoprobe)
{
	DisableTrackingConfigurationButtons();
	
	mitk::ConoprobeDevice::Pointer currentDevice = dynamic_cast<mitk::ConoprobeDevice*>(m_Controls->m_configurationWidget->GetTrackingDevice().GetPointer());
	currentDevice->OpenConnection();
	currentDevice->StartTracking();
	mitk::NavigationToolStorage::Pointer autoDetectedStorage = mitk::NavigationToolStorage::New(this->GetDataStorage());
	for (int i = 0; i<currentDevice->GetToolCount(); i++)
	{
		//create a navigation tool with sphere as surface
		std::stringstream toolname;
		toolname << "AutoDetectedTool" << i;
		mitk::NavigationTool::Pointer newTool = mitk::NavigationTool::New();
		newTool->SetIdentifier(toolname.str());
		newTool->SetTrackingDeviceType(mitk::Conoprobe);
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
	currentDevice->StopTracking();
	//save detected tools
	this->ReplaceCurrentToolStorage(autoDetectedStorage, "Autodetected Conoprobe Storage");
	//update label
	QString toolLabel = QString("Loaded Tools: ") + QString::number(m_toolStorage->GetToolCount()) + " Tools (Auto Detected)";
	m_Controls->m_toolLabel->setText(toolLabel);
	//update tool preview
	m_Controls->m_TrackingToolsStatusWidget->RemoveStatusLabels();
	m_Controls->m_TrackingToolsStatusWidget->PreShowTools(m_toolStorage);
	
	currentDevice->CloseConnection();

	EnableTrackingConfigurationButtons();

}
}

/**
\brief This method sets up the navigation pipeline during initialization.
*/

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::CreateBundleWidgets(QWidget* parent)
{
	//initialize registration widget
	m_Controls->m_RegistrationWidget->HideStaticRegistrationRadioButton(true);
	m_Controls->m_RegistrationWidget->HideContinousRegistrationRadioButton(true);
	m_Controls->m_RegistrationWidget->HideUseICPRegistrationCheckbox(true);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnSetupNavigation()
{

	mitk::DataStorage* ds = this->GetDataStorage();

	if (ds == NULL)
	{
		MITK_WARN << "IGTSurfaceTracker: Error", "can not access DataStorage. Navigation not possible";
		return;
	}
	InitializeRegistration();

	InitializeDistanceControl();


	for (int numOfTracker = 0; numOfTracker < m_ToolVisualizationFilterVector.size(); numOfTracker++)
	{
		//Initialize elements for surface orientation
		m_SurfaceGeometricalTransform = new SurfaceGeometricalTransform[m_ToolVisualizationFilterVector.at(numOfTracker)->GetNumberOfOutputs()];

		for (int i = 0; i < m_ToolVisualizationFilterVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
		{
			m_SurfaceGeometricalTransform[i].node_name = "-";
			m_SurfaceGeometricalTransform[i].surface_node = NULL;
			m_SurfaceGeometricalTransform[i].SurfaceRelated = false;
			m_SurfaceGeometricalTransform[i].SurfaceToToolTransform = NULL;
			m_SurfaceGeometricalTransform[i].ToolPositionAtRegistrationTime_Inverse = NULL;

		}

		//Initialize tool information table

		for (int i = 0; i < m_ToolVisualizationFilterVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
		{

			QString tool_name = m_toolStorage->GetTool(i)->GetToolName().c_str();
			QTableWidgetItem *tool_item = new QTableWidgetItem();
			tool_item->setText(tool_name);
			m_Controls->m_ToolInfoTable->setItem(i, 0, tool_item);

			QTableWidgetItem *reference_item = new QTableWidgetItem();
			reference_item->setText("-");
			m_Controls->m_ToolInfoTable->setItem(i, 1, reference_item);

			QTableWidgetItem * surface_item = new QTableWidgetItem();
			surface_item->setText("-");
			m_Controls->m_ToolInfoTable->setItem(i, 2, surface_item);

			QTableWidgetItem * toolsurfacereg_item = new QTableWidgetItem();
			toolsurfacereg_item->setText("-");
			m_Controls->m_ToolInfoTable->setItem(i, 3, toolsurfacereg_item);

			QTableWidgetItem * generalreg_item = new QTableWidgetItem();
			generalreg_item->setText("Not calc.");
			m_Controls->m_ToolInfoTable->setItem(i, 4, generalreg_item);

		}
	}

}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnSetupToolForTrajectoryControl()
{

	mitk::DataStorage* ds = this->GetDataStorage();

	if (ds == NULL)
	{
		MITK_WARN << "IGTSurfaceTracker: Error", "can not access DataStorage. Navigation not possible";
		return;
	}
	InitializeDistanceControl();

}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::InitializeDistanceControl()
{
	mitk::DataStorage* ds = this->GetDataStorage();
	if (ds == NULL)
		return;

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
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnSetupTool()
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
			std::cout << "Tool is not reference framework" << std::endl;
			m_Controls->m_ToolIsReferenceFramework->setChecked(false);
			m_ReferenceFrameworkChecked = false;
		}
	}
		
	else
	{
		m_Controls->m_ToolIsReferenceFramework->setChecked(false);
		m_ReferenceFrameworkChecked = false;
	}

	/*QListWidgetItem *surface_item = m_Controls->m_Surfaces->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID());

	if (surface_item->text().toStdString() == "-")
	{
		m_Controls->m_RegisterToolWithSurface->setChecked(false);
		m_Controls->m_RegistrationSurfaceComboBox->setEnabled(false);
	}
	else
	{
		m_Controls->m_RegisterToolWithSurface->setChecked(true);
		m_Controls->m_RegistrationSurfaceComboBox->setEnabled(true);
	}*/

	QTableWidgetItem *surfacetable_item = m_Controls->m_ToolInfoTable->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID(),2);

	if (surfacetable_item->text().toStdString() == "-")
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


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnRegisterToolWithSurface(bool on)
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnSetAsReferenceFramework(bool on)
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnAcceptToolData()
{

	m_ThereIsAReference = false;
	m_Reference_Index = NULL;


	for (int index = 0; index < m_toolStorage->GetToolCount(); index++)
	{

		QTableWidgetItem *table_item = m_Controls->m_ToolInfoTable->item(index, 1);
		if (m_ReferenceFrameworkChecked)
		{
			if (index == m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID())
			{
				table_item->setText("Reference framework");
				m_ThereIsAReference = true;
				m_Reference_Index = index;
			}
			else
			{
				table_item->setText("-");
			}
		}
		else
		{
			if (index == m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID())
			{
				table_item->setText("-");
			}

		}

		if (table_item->text().compare("Reference framework") == 0)
		{
			m_ThereIsAReference = true;
			m_Reference_Index = index;
		}
		table_item = NULL;
	}

	if (m_Controls->m_RegisterToolWithSurface->isChecked())
	{
		QString surface_name = QString::fromStdString(m_Controls->m_RegistrationSurfaceComboBox->GetSelectedNode()->GetProperty("name")->GetValueAsString());

		QTableWidgetItem *surface_tableitem = m_Controls->m_ToolInfoTable->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID(),2);
		surface_tableitem->setText(surface_name);
		surface_tableitem = NULL;

		QTableWidgetItem * toolsurfacereg_item = m_Controls->m_ToolInfoTable->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID(), 3);
		toolsurfacereg_item->setText("Not calc.");
		toolsurfacereg_item = NULL;
	}

	else
	{
		QTableWidgetItem *surface_tableitem = m_Controls->m_ToolInfoTable->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID(), 2);
		surface_tableitem->setText("-");
		surface_tableitem = NULL;

		QTableWidgetItem * toolsurfacereg_item = m_Controls->m_ToolInfoTable->item(m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID(), 3);
		toolsurfacereg_item->setText("-");
		toolsurfacereg_item = NULL;
	}

}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnCalculateRegistration()
{

	int index = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID();

	//Check for initialization
	if (!CheckRegistrationInitialization()) return;

	m_MultiDeviceMonitoringLab->CalculateRegistration(m_ImageFiducialsDataNode, m_TrackerFiducialsDataNode);

	m_Controls->m_RegistrationWidget->SetQualityDisplayText("FRE: " + QString::number(m_MultiDeviceMonitoringLab->GetRegistrationFRE()) + " mm");

	vtkMatrix4x4 * matrix = m_MultiDeviceMonitoringLab->GetVTKRegistrationTransform()->GetMatrix();

	//Transform Tracker Fiducials to the position on the Image World 
	m_TrackerFiducialsDataNode->GetData()->GetGeometry()->Compose(matrix);
	m_TrackerFiducialsDataNode->GetData()->GetTimeGeometry()->Update();
	mitk::RenderingManager::GetInstance()->RequestUpdateAll();


	m_Controls->m_UsePermanentRegistrationToggle->setEnabled(true);

	//save transform to file
	std::ofstream myfile;

	std::cout << "Matrix: " << *matrix << std::endl;
	m_Controls->m_SaveRegistrationTransformBtn->setEnabled(true);

	SaveSurfaceToToolRegistrationTransform();
}


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::SaveSurfaceToToolRegistrationTransform()
{
	int index = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID();

	if (m_Controls->m_ToolToSurfaceRegistration->isChecked())
	{
		m_SurfaceGeometricalTransform[index].surface_node = m_Controls->m_RegistrationSurfaceComboBox->GetSelectedNode();
		m_SurfaceGeometricalTransform[index].node_name = m_SurfaceGeometricalTransform[index].surface_node->GetName();
		m_SurfaceGeometricalTransform[index].SurfaceRelated = true;
		itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer  matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
		m_MultiDeviceMonitoringLab->GetITKRegistrationTransform()->GetInverse(matrix);
		m_SurfaceGeometricalTransform[index].SurfaceToToolTransform = matrix;
		QTableWidgetItem * toolsurfacereg_item = m_Controls->m_ToolInfoTable->item(index, 3);
		toolsurfacereg_item->setText("Calculated");
		toolsurfacereg_item = NULL;
	}

	else

	{
		m_GeneralRegistrationTransform = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
		m_GeneralRegistrationTransform = m_MultiDeviceMonitoringLab->GetITKRegistrationTransform();
		
		for (unsigned int numOfTracker = 0; numOfTracker < m_TrackingDeviceSourceVector.size(); numOfTracker++)
		{

			for (unsigned int i = 0; i < m_TrackingDeviceSourceVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
			{
				QTableWidgetItem * generalreg_item = m_Controls->m_ToolInfoTable->item(i, 4);
				generalreg_item->setText("Calculated");
				generalreg_item = NULL;
			}
		}
	}
}


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnToolToSurfaceRegistration(bool on)
{
	m_Controls->m_GeneralRegistration->setChecked(!on);
}
void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnGeneralRegistration(bool on)
{
	m_Controls->m_ToolToSurfaceRegistration->setChecked(!on);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnApplyRegistration(bool on)
{
	
	if (on)
	{
		//some initial checks
		if (!CheckRegistrationInitialization())
		{
			m_Controls->m_UsePermanentRegistrationToggle->setChecked(false);
			return;
		}

		for (unsigned int numOfTracker = 0; numOfTracker < m_TrackingDeviceSourceVector.size(); numOfTracker++)
		{

			for (unsigned int i = 0; i < m_TrackingDeviceSourceVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
			{
				if (m_ThereIsAReference) // All the tools and surfaces must be represented with respect to the reference framework
				{
					std::cout << "Reference Index : " << m_Reference_Index << std::endl;
					m_Original_Reference_Orientation = m_ToolVisualizationFilterVector.at(numOfTracker)->GetOutput(m_Reference_Index)->GetAffineTransform3D();
					m_Original_Reference_Orientation->GetInverse(m_Original_Reference_Orientation_Inverse);

					if (i == m_Reference_Index)
					{
						m_Total_Reference_Orientation = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
						m_Total_Reference_Orientation->SetIdentity();

						//We set the full orientation and the registration now, since later we won't allow any changes in position or orientation for this tool
						m_Total_Reference_Orientation->Compose(m_Original_Reference_Orientation);
						m_Total_Reference_Orientation->Compose(m_GeneralRegistrationTransform);
						m_ToolVisualizationFilterVector.at(numOfTracker)->SetOffset(i, m_Total_Reference_Orientation);

					}
					else
					{
						m_TotalOrientationTransform->SetIdentity();
						m_TotalOrientationTransform->Compose(m_GeneralRegistrationTransform);
						m_ToolVisualizationFilterVector.at(numOfTracker)->SetOffset(i, m_TotalOrientationTransform);
					}
				}

				else // If there is no reference framework, we give priority to the tool that we are using for the registration
				{
					m_ToolVisualizationFilterVector.at(numOfTracker)->SetOffset(i, m_GeneralRegistrationTransform);
				}

				m_ToolVisualizationFilterVector.at(numOfTracker)->Update();

				QTableWidgetItem * generalreg_item = m_Controls->m_ToolInfoTable->item(i, 4);
				generalreg_item->setText("Registered");
				generalreg_item = NULL;
			}


			//From this point, we won't visualize the changes in orientation or position for the tool that represents the reference marker. Instead, the changes will
			//be applied to the rest of the context (inverted)

			if (m_ThereIsAReference)
			{
				m_ToolVisualizationFilterVector.at(numOfTracker)->SetTransformOrientation(m_Reference_Index, false);
				m_ToolVisualizationFilterVector.at(numOfTracker)->SetTransformPosition(m_Reference_Index, false);
			}

			//TO-DO fix this.
			if (m_PSRecordingPointSet.IsNotNull())
			{
				m_PSRecordingPointSet->GetGeometry()->Compose(m_MultiDeviceMonitoringLab->GetITKRegistrationTransform());
			}

			mitk::RenderingManager::GetInstance()->RequestUpdateAll();

			//Permanent registration must be applied
			m_PermanentRegistration = true;

		}
	}

	else //if off = disable the permanent registration
	{
		//stop permanent registration
		m_PermanentRegistration = false;
		
		for (unsigned int numOfTracker = 0; numOfTracker < m_TrackingDeviceSourceVector.size(); numOfTracker++)
		{
			for (unsigned int i = 0; i < m_TrackingDeviceSourceVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
			{
				m_ToolVisualizationFilterVector.at(numOfTracker)->SetOffset(i, NULL);

				QTableWidgetItem * generalreg_item = m_Controls->m_ToolInfoTable->item(i, 4);
				generalreg_item->setText("Calculated");
				generalreg_item = NULL;

			}

			m_ToolVisualizationFilterVector.at(numOfTracker)->Update();

			//Undo registration in Trajectory PointSet TO-DO

			if (m_PSRecordingPointSet.IsNotNull())
			{
				m_PSRecordingPointSet->GetGeometry()->Compose(m_MultiDeviceMonitoringLab->GetITKRegistrationTransformInverse());
			}

			//Undo registration in distance line TO-DO

			if (m_DistanceLinePointSet.IsNotNull())
			{
				m_DistanceLinePointSet->GetGeometry()->Compose(m_MultiDeviceMonitoringLab->GetITKRegistrationTransformInverse());
			}
		}
	}
}

bool QmitkMITKIGTMultiDeviceMonitoringToolboxView::CheckRegistrationInitialization()
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
void QmitkMITKIGTMultiDeviceMonitoringToolboxView::InitializeRegistration()
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


/** This method is called when the object marker is selected.
*  It stores the navigation data of the object marker.
*/
void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnAddRegistrationTrackingFiducial()
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


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::MessageBox(std::string s)
  {
  QMessageBox msgBox;
  msgBox.setText(s.c_str());
  msgBox.exec();
  }

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::UpdateTrackingTimer()
  {
	bool movement = false;
	double min_dist;
	
	for (unsigned int numOfTracker = 0; numOfTracker < m_TrackingDeviceSourceVector.size(); numOfTracker++)
	{
		if (m_ThereIsAReference)
		{
			for (int i = 0; i < m_ToolVisualizationFilterVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
			{
				if (m_PermanentRegistration)
				{
					if (i != m_Reference_Index)
					{
						//If Current_pos = G * Orig_Pos => G = Current_pos * (Orig_pos)^-1 ; 
						itk::ScalableAffineTransform<mitk::ScalarType, 3U>::Pointer matrix = itk::ScalableAffineTransform<mitk::ScalarType, 3U>::New();
						matrix->Compose(m_Original_Reference_Orientation_Inverse);
						matrix->Compose(m_TrackingDeviceSourceVector.at(numOfTracker)->GetOutput(m_Reference_Index)->GetAffineTransform3D());

						//Now we get G^-1 and we will have to apply it to the tools that are not a reference marker
						matrix->GetInverse(m_Reference_Orientation_Inverse);

						//The total transform includes the effect of the reference framework movement (inverted, G^-1) and the registration to the virtual world (ITKRegistrationTransform). 
						m_TotalOrientationTransform->SetIdentity();
						m_TotalOrientationTransform->Compose(m_Reference_Orientation_Inverse);
						m_TotalOrientationTransform->Compose(m_GeneralRegistrationTransform);
						m_ToolVisualizationFilterVector.at(numOfTracker)->SetOffset(i, m_TotalOrientationTransform);

					}
				}
			}
		}

		else
		{
			if (m_PermanentRegistration)
			{
				for (int i = 0; i < m_ToolVisualizationFilterVector.at(numOfTracker)->GetNumberOfOutputs(); i++)
				{
					//The total transform includes the effect of the reference framework movement (inverted, G^-1) and the registration to the virtual world (ITKRegistrationTransform). 
					m_TotalOrientationTransform->SetIdentity();
					m_TotalOrientationTransform->Compose(m_GeneralRegistrationTransform);
					m_ToolVisualizationFilterVector.at(numOfTracker)->SetOffset(i, m_TotalOrientationTransform);
				}
			}
		}

		m_ToolVisualizationFilterVector.at(numOfTracker)->Update();
		m_Controls->m_TrackingToolsStatusWidget->Refresh();

		//TO-DO
		mitk::NavigationData::Pointer nd;
		mitk::NavigationData::Pointer T_Object = mitk::NavigationData::New(m_TotalOrientationTransform, false);


		if ((m_DistanceControl && m_DistanceLinePointSet.IsNotNull()) || (m_PointSetRecording && m_PSRecordingPointSet.IsNotNull()))
		{
			nd = m_ToolVisualizationFilterVector.at(numOfTracker)->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());
			nd->Compose(T_Object);
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
			double *Position = new double[3];
			Position[0] = nd->GetPosition().GetElement(0);
			Position[1] = nd->GetPosition().GetElement(1);
			Position[2] = nd->GetPosition().GetElement(2);

			Distance_Data *distance_data = m_DistMonitor->RealTimeControlOfDistance(Position);
			m_DistanceLinePointSet->SetPoint(0, nd->GetPosition());
			m_DistanceLinePointSet->SetPoint(1, distance_data->closest_point);
			m_DistanceLinePointSet->GetUpdatedGeometry();
			QString distancestr = QString::number(distance_data->distance);
			min_dist = distance_data->distance;
			m_Controls->m_ToolDistanceToSurfaceWidget->setText(distancestr);

			int size = m_VirtualTimeStamp.size();

			m_VirtualTimeStamp.resize(size + 1, size);
			m_Distance.resize(size + 1, distance_data->distance);

			/*m_VirtualTimeStamp[size - 1] = size - 1;
			m_Distance[size - 1] = distance_data->distance;
			m_Distance.resize(m_PSRecordingPointSet->GetSize());
			m_VirtualTimeStamp.resize(m_PSRecordingPointSet->GetSize());*/

			m_Controls->m_DistanceMonitoringWidget->SetCurveData(0, m_VirtualTimeStamp, m_Distance);

			QPen pen(Qt::blue, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
			m_Controls->m_DistanceMonitoringWidget->SetCurvePen(0, pen);
			m_Controls->m_DistanceMonitoringWidget->Replot();

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

			if ((m_loggedFrames > 1) && (!m_loggingFilter->GetRecording()))
			{
				StopLogging();
			}
		}
	}

	mitk::RenderingManager::GetInstance()->RequestUpdateAll();
	
}


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnChooseSettingsFileClicked()
{
	QDir currentPath = QFileInfo(m_Controls->m_PreloadSettingsFileName->text()).dir();

	// if no path was selected (QDir would select current working dir then) or the
	// selected path does not exist -> use home directory
	if (currentPath == QDir() || !currentPath.exists())
	{
		currentPath = QDir(QDir::homePath());
	}

	QString filename = QFileDialog::getOpenFileName(NULL, tr("Choose Preload Settings File"), currentPath.absolutePath(), "*.xml");

	std::cout << filename.toStdString() << std::endl;
	if (filename == "") return;
	this->m_Controls->m_PreloadSettingsFileName->setText(filename);
}
void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnPreloadSettingsClicked()
{
	std::cout << m_Controls->m_PreloadSettingsFileName->text().toStdString() << std::endl;
	OnPreloadSettings(m_Controls->m_PreloadSettingsFileName->text().toStdString());
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnChooseFileClicked()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnSaveCurrentSettingsClicked()
{
	QDir currentPath = QFileInfo(m_Controls->m_TransformFileName->text()).dir();

	// if no path was selected (QDir would select current working dir then) or the
	// selected path does not exist -> use home directory
	if (currentPath == QDir() || !currentPath.exists())
	{
		currentPath = QDir(QDir::homePath());
	}

	QString filename = QFileDialog::getSaveFileName(NULL, tr("Choose name for registration Transform file"), currentPath.absolutePath(), "*.*");
	if (filename == "") return;
	this->m_Controls->m_TransformFileName->setText(filename);
	OnToggleFileExtension(m_Controls->m_TransformFileName);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnChooseTransformFileNameClicked()
{
	QDir currentPath = QFileInfo(m_Controls->m_TransformFileName->text()).dir();
// if no path was selected (QDir would select current working dir then) or the
	// selected path does not exist -> use home directory
	if (currentPath == QDir() || !currentPath.exists())
	{
		currentPath = QDir(QDir::homePath());
		QString filename = QFileDialog::getSaveFileName(NULL, tr("Choose name for registration Transform file"), currentPath.absolutePath(), ".xml");
		if (filename == "") return;
		this->m_Controls->m_TransformFileName->setText(filename);
	}

	//OnToggleFileExtension(m_Controls->m_TransformFileName);

	vtkMatrix4x4 * matrix = m_MultiDeviceMonitoringLab->GetVTKRegistrationTransform()->GetMatrix();

	//TO-DO write transform file name in settings when pressing save current settings button
	std::string transformfilename = this->m_Controls->m_TransformFileName->text().toStdString().c_str();
	try
	{	
		TiXmlDocument doc;
		TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "utf-8", "");
		
		doc.LinkEndChild(decl);

		TiXmlElement * data;
		TiXmlElement * row;

		data = new TiXmlElement("Data");
		doc.LinkEndChild(data);
		
		for (int i = 0; i < 4; i++)
		{
			row = new TiXmlElement("row");
			row->SetDoubleAttribute("col0", matrix->GetElement(i, 0));
			row->SetDoubleAttribute("col1", matrix->GetElement(i, 1));
			row->SetDoubleAttribute("col2", matrix->GetElement(i, 2));
			row->SetDoubleAttribute("col3", matrix->GetElement(i, 3));
			data->LinkEndChild(row);
		}

		TiXmlElement * error;
		error = new TiXmlElement("error");
		error->SetDoubleAttribute("value", 0);
		data->LinkEndChild(error);

		doc.SaveFile(transformfilename);

	}
	catch (...)
	{
		std::string warningmessage = "Transform file has not being specified or does not exist. Registration will not be saved on file.";
		QMessageBox::warning(NULL, "IGTPlayer: Error", warningmessage.c_str());
	}

	if (m_Controls->m_GeneralRegistration->isChecked() == true)
	{
		m_GeneralRegistrationFile = transformfilename;
	}
	else
	{
		int index = m_Controls->m_TrackingDeviceSelectionWidget->GetSelectedToolID();
		m_SurfaceGeometricalTransform[index].SurfaceToToolRegistrationFile = transformfilename;
	}
}

// bug-16470: toggle file extension after clicking on radio button
void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnToggleFileExtension(QLineEdit *filename)
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::StartLogging()
  {
  
  if (m_ToolVisualizationFilterVector.size() == 0)
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
    for(int i=0; i<m_ToolVisualizationFilterVector.at(0)->GetNumberOfOutputs(); i++)
	
	{
		m_loggingFilter->AddNavigationData(m_ToolVisualizationFilterVector.at(0)->GetOutput(i));
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



void QmitkMITKIGTMultiDeviceMonitoringToolboxView::StopLogging()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnAddSingleTool()
  {

  QString Identifier = "Sensor_";
  if (m_toolStorage.IsNotNull()) Identifier += QString::number(m_toolStorage->GetToolCount());
  else Identifier += "1";
  
  m_Controls->m_NavigationToolCreationWidget->Initialize(GetDataStorage(),Identifier.toStdString());
  m_Controls->m_NavigationToolCreationWidget->SetTrackingDeviceType(m_Controls->m_configurationWidget->GetTrackingDevice()->GetType(),false);
  m_Controls->m_TrackingToolsWidget->setCurrentIndex(1);

  //disable tracking volume during tool editing
  lastTrackingVolumeState = m_Controls->m_ShowTrackingVolume->isChecked();
  if (lastTrackingVolumeState) m_Controls->m_ShowTrackingVolume->click();
  GlobalReinit();

  }

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnAddSingleToolFinished()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnAddSingleToolCanceled()
  {
  m_Controls->m_TrackingToolsWidget->setCurrentIndex(0);

  //enable tracking volume again
  if (lastTrackingVolumeState) m_Controls->m_ShowTrackingVolume->click();
  GlobalReinit();
  }


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::GlobalReinit()
{
// get all nodes that have not set "includeInBoundingBox" to false
  mitk::NodePredicateNot::Pointer pred = mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("includeInBoundingBox", mitk::BoolProperty::New(false)));

  mitk::DataStorage::SetOfObjects::ConstPointer rs = this->GetDataStorage()->GetSubset(pred);
  // calculate bounding geometry of these nodes
  mitk::TimeGeometry::Pointer bounds = this->GetDataStorage()->ComputeBoundingGeometry3D(rs, "visible");

  // initialize the views to the bounding geometry
  mitk::RenderingManager::GetInstance()->InitializeViews(bounds);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::DisableLoggingButtons()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::EnableLoggingButtons()
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::DisableOptionsButtons()
{
    m_Controls->m_ShowTrackingVolume->setEnabled(false);
    m_Controls->m_UpdateRate->setEnabled(false); 
    m_Controls->m_OptionsUpdateRateLabel->setEnabled(false);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::EnableOptionsButtons()
{
    m_Controls->m_ShowTrackingVolume->setEnabled(true);
    m_Controls->m_UpdateRate->setEnabled(true);
    m_Controls->m_OptionsUpdateRateLabel->setEnabled(true);
	
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::EnableTrackingConfigurationButtons()
{
    m_Controls->m_AutoDetectTools->setEnabled(true);
	if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::NDIAurora && m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::Conoprobe) m_Controls->m_AddSingleTool->setEnabled(true);
    //m_Controls->m_LoadTools->setEnabled(true);
    m_Controls->m_ResetTools->setEnabled(true);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::DisableTrackingConfigurationButtons()
{
    m_Controls->m_AutoDetectTools->setEnabled(false);
	if (m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::NDIAurora && m_Controls->m_configurationWidget->GetTrackingDevice()->GetType() != mitk::Conoprobe) m_Controls->m_AddSingleTool->setEnabled(false);
    //m_Controls->m_LoadTools->setEnabled(false);
    m_Controls->m_ResetTools->setEnabled(false);
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::ReplaceCurrentToolStorage(mitk::NavigationToolStorage::Pointer newStorage, std::string newStorageName)
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

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnPointSetRecording(bool record)
{
	mitk::DataStorage* ds = this->GetDataStorage();
	mitk::DataNode::Pointer dn = mitk::DataNode::New();
	mitk::DataNode::Pointer psRecND;

	if (record)
	{
		if (m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID() == -1)
		{
			QMessageBox::warning(NULL, "Error", "No tool selected for point set recording!");
			m_Controls->m_PointSetRecordCheckBox->setChecked(false);
			return;
		}

		//m_PointSetRecordingNavigationData = m_Controls->m_m_TrajectoryControlToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());

		m_PointSetRecordingNavigationData = m_ToolVisualizationFilterVector.at(0)->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());

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
		/*dn = ds->GetNamedNode("Recorded Points");
		ds->RemoveNodeEvent(dn);
		m_PSRecordingPointSet->Clear();
		m_PSRecordingPointSet = NULL;*/
		m_PointSetRecording = false;
	}
}


/** @OnPointSetRecording allows control of distance from the tracking tool to 
the closest point on a selected surface */

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnDistanceControl(bool distance_control)

{
	mitk::DataStorage* ds = this->GetDataStorage();

	mitk::DataNode::Pointer distanceNode = mitk::DataNode::New();
	mitk::DataNode::Pointer psDistND;

	m_DistMonitor = new mitk::mitkDistanceMonitoring();

	if (distance_control)
	{
		if (m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID() == -1)
		{
			QMessageBox::warning(NULL, "Error", "No tool selected for distance control!");
			m_Controls->m_DistanceControlCheckBox->setChecked(false);
			return;
		}

		m_PointSetRecordingNavigationData = m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedNavigationDataSource()->GetOutput(m_Controls->m_TrajectoryControlToolSelectionWidget->GetSelectedToolID());
	
		
		//initialize line that corresponds to minimum distance
		psDistND = ds->GetNamedNode("Distance line");

		if (m_DistanceLinePointSet.IsNull() || psDistND.IsNull())
		{		
			mitk::Surface::Pointer surface = static_cast<mitk::Surface*>(m_Controls->m_TargetSurfaceComboBox->GetSelectedNode()->GetData());

			m_DistMonitor->InitiateDistanceControl(surface->GetVtkPolyData());
			
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

void BestFitPlane(vtkPoints *points, vtkPlaneSource *BestPlane)
{
	vtkIdType NumPoints = points->GetNumberOfPoints();

	//find the center of mass of the points
	double Center[3] = { 0.0, 0.0, 0.0 };

	for (vtkIdType i = 0; i < NumPoints; i++)
	{
		double point[3];
		points->GetPoint(i, point);

		Center[0] += point[0];
		Center[1] += point[1];
		Center[2] += point[2];
	}

	Center[0] = Center[0] / static_cast<double>(NumPoints);
	Center[1] = Center[1] / static_cast<double>(NumPoints);
	Center[2] = Center[2] / static_cast<double>(NumPoints);

	// int matrix with POINTS x (X,Y,Z)
	vnl_matrix<mitk::ScalarType> dataM(points->GetNumberOfPoints(), 3);
	int ps_total = points->GetNumberOfPoints();
	for (int i = 0; i<ps_total; i++)
	{
	mitk::Point3D p3d = points->GetPoint(i);
	dataM[i][0] = p3d[0] - Center[0];
	dataM[i][1] = p3d[1] - Center[1];
	dataM[i][2] = p3d[2] - Center[2];
	}
	// process the SVD (singular value decomposition) from ITK
	// the vector will be ordered   descending
	vnl_svd<mitk::ScalarType> svd(dataM, 0.0);
	// calculate the SVD of A
	vnl_vector<mitk::ScalarType> vPCA = svd.nullvector();
	// Avoid erratic normal sign switching when the plane changes minimally
	// by negating the vector for negative x values.
	if (vPCA[0] < 0)
	{
		vPCA = -vPCA;
	}

	/* the normal vector to descrie a plane gemoetry.*/
	mitk::Vector3D m_PlaneNormal = mitk::Vector3D();
	m_PlaneNormal[0] = vPCA[0];
	m_PlaneNormal[1] = vPCA[1];
	m_PlaneNormal[2] = vPCA[2];


	//Set the plane center to the center of mass
	BestPlane->SetCenter(Center[0], Center[1], Center[2]);

	//Set the plane normal to the smallest eigen vector
	BestPlane->SetNormal(vPCA[0], vPCA[1], vPCA[2]);
	//return BestPlane;

}


void QmitkMITKIGTMultiDeviceMonitoringToolboxView::OnInterpolate3DModel()
{
	mitk::DataStorage* ds = this->GetDataStorage();
	mitk::DataNode::Pointer dn = mitk::DataNode::New();
	mitk::DataNode::Pointer dp = mitk::DataNode::New();
	mitk::DataNode::Pointer dr = mitk::DataNode::New();
	mitk::DataNode::Pointer Model3DND;
	

	vtkPoints* Points = vtkPoints::New();

	for (int i = 0; i < m_PSRecordingPointSet->GetSize(); i++)
	{
	mitk::Point3D p = m_PSRecordingPointSet->GetPoint(i);
	std::cout << "Point3D: " << p[0] << "; " << p[1] << "; " << p[2] << std::endl;

	Points->InsertPoint(i, p[0], p[1], p[2]);
	}

	
	vtkPlaneSource* m_PlaneSource = vtkPlaneSource::New();
	m_PlaneSource->SetOrigin(0.0, 0.0, 0.0);
	m_PlaneSource->SetPoint1(0.0, 1000.0, 0.0);
	m_PlaneSource->SetPoint2(1000.0, 0.0, 0.0);
	m_PlaneSource->SetResolution(128, 128);
	m_PlaneSource->Update();

	BestFitPlane(Points, m_PlaneSource);
	
	m_PlaneSource->Update();
	
	vtkPolyData *m_PlanePolyData = vtkPolyData::New();
	m_PlanePolyData->DeepCopy(m_PlaneSource->GetOutput());
	
	mitk::Surface::Pointer mitkPlane = mitk::Surface::New();
	mitkPlane->SetVtkPolyData(m_PlanePolyData);

	dn->SetName("Reference Plane");
	dn->SetColor(1, 1, 1);
	dn->SetData(mitkPlane);
	ds->Add(dn);

	mitk::RenderingManager::GetInstance()->RequestUpdateAll();
	
}

void QmitkMITKIGTMultiDeviceMonitoringToolboxView::SetFocus()
{
	//m_Controls->m_UseAsPointerButton->setFocus();
}

bool QmitkMITKIGTMultiDeviceMonitoringToolboxView::IsTransformDifferenceHigh(mitk::NavigationData::Pointer transformA, mitk::NavigationData::Pointer transformB, double euclideanDistanceThreshold, double angularDifferenceThreshold)
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
