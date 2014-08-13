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

#ifndef QmitkMITKIGTMaxillofacialTrackingToolboxView_h
#define QmitkMITKIGTMaxillofacialTrackingToolboxView_h

#include <berryISelectionListener.h>

//#include <QmitkFunctionality.h>
#include <QmitkAbstractView.h>

#include "ui_QmitkMITKIGTMaxillofacialTrackingToolboxViewControls.h"
#include "MITKIGTMaxillofacialTrackingLab.h"

//mitk headers
#include <mitkNavigationToolStorage.h>
#include <mitkTrackingDeviceSource.h>
#include <mitkNavigationDataObjectVisualizationFilter.h>
#include <mitkNavigationDataRecorder.h>

//QT headers
#include <QTimer>

/*!
  \brief QmitkMITKIGTTrackingToolboxView

   This is the view of the bundle IGT Tracking Toolbox. The IGT Tracking Toolbox can be used to access tracking devices with MITK-IGT. The Tracking Toolbox can be used to log tracking data in     XML or CSV format for measurement purposes. The Tracking Toolbox further allows for visualization of tools with given surfaces in combination with the NaviagtionToolManager.

  \sa QmitkFunctionality
  \ingroup Functionalities
*/
class QmitkMITKIGTMaxillofacialTrackingToolboxView : public QmitkAbstractView
{
  // this is needed for all Qt objects that should have a Qt meta-object
  // (everything that derives from QObject and wants to have signal/slots)
  Q_OBJECT

  public:

    static const std::string VIEW_ID;

	/*Construction and destruction methods*/

	QmitkMITKIGTMaxillofacialTrackingToolboxView();
    virtual ~QmitkMITKIGTMaxillofacialTrackingToolboxView();

	/*Initialize the user interface which corresponds to this plugin*/
	virtual void CreateQtPartControl(QWidget *parent);
	virtual void SetFocus();
  protected slots:

    /** @brief changes name of the filename when switching fileextension by radio button */
    void OnToggleFileExtension(QLineEdit *filename);
    /** @brief This slot is called if the user wants to load a new tool file. A new window opens where the user can choose a file. If the chosen file is
               corrupt or not valid the user gets an error message. If the file was loaded successfully the tools are show in the tool status widget. */
    void OnLoadTools();

    /** @brief This slot connects to the device. In status "connected" configuration of the device is disabled. */
    void OnConnect();
	/** @brief This slot connects to a virtual device. */
	void OnVirtualConnect();

    /** @brief This slot disconnects from the device. */
    void OnDisconnect();

    /** @brief This slot tries to start tracking with the current device. If start tracking fails the user gets an error message and tracking stays off.*/
    void OnStartTracking();

    /** @brief This slot stops tracking. If tracking is not strated it does nothing.*/
    void OnStopTracking();

    /** @brief This slot is called if the user wants to choose a file name for logging. A new windows to navigate through the file system and choose
               a file opens.*/
    void OnChooseFileClicked();

    /** @brief This slot starts logging. Logging is only possible if a device is tracking. If not the logging mechanism start when the start tracking
               is called.*/
    void StartLogging();

    /** @brief This slot stops logging. If logging is not running it does nothing.*/
    void StopLogging();

    /** @brief This slot enables / disables UI elements depending on the tracking device after a device is changed.*/
    void OnTrackingDeviceChanged();

	/** @brief This slot enables the option of performing tracking simulation instead of using a real tracking device */
	void OnPerformVirtualTracking(bool on);

    /** @brief This slot selects the Tracking Volume appropriate for a given model */
    void OnTrackingVolumeChanged(QString qstr);

    /** @brief Shows or hides the tracking volume according to the checkboxe's state */
    void OnShowTrackingVolumeChanged();

    /** @brief This slot auto detects tools of a NDI Aurora tracking device. If tools where found they will be stored internally as a tool storage.
        The user is also asked if he wants to save this tool storage to load it later. Only call it if a Aurora device was configured because other
        devices don't support auto detection.*/
    void OnAutoDetectTools();

    /** @brief Slot for tracking timer. The timer updates the IGT pipline and also the logging filter if logging is activated.*/
    void UpdateTrackingTimer();

    /** @brief Resets the Tracking Tools: this means all tools are removed. */
    void OnResetTools();

    /** @brief Opens a dialog where a new navigation tool can be created. */
    void OnAddSingleTool();

    /** @brief This slot is called if the user finishes the creation of a new tool. */
    void OnAddSingleToolFinished();

    /** @brief This slot is called if the user cancels the creation of a new tool. */
    void OnAddSingleToolCanceled();
	
	/**************************Registration tab ************************************/
	
	/*brief This method sets up the navigation pipeline during initialization.*/
	void OnSetupNavigation();
	
	/* This method is called when an instrument or tool is selected. It stores the navigation data of the instrument.*/
	void OnInstrumentSelected();
	
	/* This method is called when the object marker is selected. It stores the navigation data of the object marker.*/
	//void OnObjectmarkerSelected();
	
	/* This method calculates the transform for the registration target -> image (tracking -> original image)*/
	void OnCalculateRegistration();

	/* This method applies the transform permanently to the tracking navigation data, 
	/* so it is automatically registered and visualized with the new coordinates */
	
	void OnApplyRegistration(bool on);

	/* This method allows taking a navigation data point from the taking device 
	in order to use it for the ICP registration */
	
	void OnAddRegistrationTrackingFiducial();

	/** @brief This slot is called if the user wants to choose a file name to save the registration transform. A new windows to navigate through the file system and choose
	a file opens.*/
	void OnChooseTransformFileClicked();


	/**************************Point set recording tab ************************************/

	/** @brief This method starts the PointSet recording.*/
	void OnPointSetRecording(bool record);

	/**************************Virtual camera view tab ************************************/
	/** @brief This method activates the virtual camera.*/
	//void OnVirtualCamera(bool on);

  protected:

    Ui::QmitkMITKIGTMaxillofacialTrackingToolboxViewControls* m_Controls;

    QmitkStdMultiWidget* m_MultiWidget;

	bool m_tracking;    ///> bool which is true if tracking is running, false if not
    bool m_logging;     ///> bool which is true if logging is running, false if not
    int m_loggedFrames; ///> stores the current number of logged frames if logging is on

	/** @brief set to true when we want to perform Virtual Tracking. */
	bool m_VirtualTracking;
	 
    mitk::NavigationToolStorage::Pointer m_toolStorage;  ///>stores the loaded tools
    mitk::DataNode::Pointer m_TrackingVolumeNode;        ///>holds the data node of the tracking volume if volume is visualized
    bool lastTrackingVolumeState;                        ///>temporary holds the state of the tracking volume (activated/not activated) during some methods

    /** @brief Shows a message box with the text given as parameter. */
    void MessageBox(std::string s);

    /** @brief reinits the view globally. */
    void GlobalReinit();

	//************************ MEMBERS FOR THE TRACKING LAB *****************************************
	/** @brief tracking lab object */
	MITKIGTMaxillofacialTrackingLab *m_MaxillofacialTrackingLab;
	
	void CreateBundleWidgets(QWidget* parent);
	
	/** @brief This method save the image points and the target points
	into the data storage object, so the registration can be made.*/
	void InitializeRegistration();


	bool CheckRegistrationInitialization();

	/** @members for initial registration*/
	mitk::DataNode::Pointer m_ImageFiducialsDataNode;
	mitk::DataNode::Pointer m_TrackerFiducialsDataNode;
	mitk::NavigationData::Pointer m_ObjectmarkerNavigationData;
	mitk::NavigationData::Pointer m_InstrumentNavigationData;

	/***************************************************************************************************/

	/** @members for the filter pipeline */
   mitk::TrackingDeviceSource::Pointer m_TrackingDeviceSource; ///> member for the source of the IGT pipeline
   mitk::TrackingDeviceData m_TrackingDeviceData; ///> stores the tracking device data as long as this is not handled by the tracking device configuration widget
   mitk::NavigationDataObjectVisualizationFilter::Pointer m_ToolVisualizationFilter; ///> holds the tool visualization filter (second filter of the IGT pipeline)
   mitk::NavigationDataRecorder::Pointer m_loggingFilter; ///> holds the logging filter if logging is on (third filter of the IGT pipeline)

  
   /** @brief This method destroys the filter pipeline.*/
   void DestroyIGTPipeline();
   
   /** @brief This timer updates the IGT pipline and also the logging filter if logging is activated.*/
   QTimer* m_TrackingTimer;

   /** @help methods for enable/disable buttons*/
   void DisableLoggingButtons();
   void EnableLoggingButtons();
   void DisableOptionsButtons();
   void EnableOptionsButtons();
   void EnableTrackingConfigurationButtons();
   void DisableTrackingConfigurationButtons();
   /** Replaces the current navigation tool storage which is stored in m_toolStorage.
    *  Basically handles the microservice stuff: unregisteres the old storage, then
    *  replaces the storage and registers the new one.
    */
   void ReplaceCurrentToolStorage(mitk::NavigationToolStorage::Pointer newStorage, std::string newStorageName);

   /*members for the point set recording*/
   mitk::NavigationData::Pointer m_PointSetRecordingNavigationData;
   mitk::PointSet::Pointer m_PSRecordingPointSet;
   bool m_PointSetRecording;
   bool m_PermanentRegistration;
   bool m_CameraView;
   mitk::CameraVisualization::Pointer m_VirtualView;
};



#endif // _QMITKMITKIGTTRACKINGTOOLBOXVIEW_H_INCLUDED
