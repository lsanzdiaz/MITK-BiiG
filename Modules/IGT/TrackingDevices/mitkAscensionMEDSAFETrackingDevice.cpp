

#include "mitkAscensionMEDSAFETrackingDevice.h"
#include <ATC3DGm.h>	// PCIBIRD API

#ifdef MITK_USE_ASCENSION_TRACKER


class CSystem
{
public:
	SYSTEM_CONFIGURATION m_config;
};

class CSensor
{
public:
	SENSOR_CONFIGURATION m_config;
};

class CXmtr
{
public:
	TRANSMITTER_CONFIGURATION m_config;
};

CSystem			medSAFE;
CSensor			*pSensor;
CXmtr			*pXmtr;
int				errorCode;
int				i;
int				sensorID;
short			id;
int				records = 100;
char			output[256];
int				numberBytes;
clock_t			goal;
clock_t			wait = 10;	// 10 ms delay

//=======================================================
// Constructor
//=======================================================
mitk::AscensionMEDSAFETrackingDevice::AscensionMEDSAFETrackingDevice()
{
	// Set the MultiThread and Mutex
	this->m_MultiThreader = itk::MultiThreader::New();
	this->m_ToolsMutex = itk::FastMutexLock::New();

	//Set the mitk device information
	SetData(mitk::DeviceDataNPOptitrack);

	//Clear List of tools
	this->m_AllTools.clear();
}

mitk::AscensionMEDSAFETrackingDevice::~AscensionMEDSAFETrackingDevice()
{
}


//	ERROR HANDLER
//	=============
//
// This is a simplified error handler.
// This error handler takes the error code and passes it to the GetErrorText()
// procedure along with a buffer to place an error message string.
// This error message string can then be output to a user display device
// like the console
// Specific error codes should be parsed depending on the application.
//

void mitk::AscensionMEDSAFETrackingDevice::errorHandler(int error)
{
	char			buffer[1024];
	char			*pBuffer = &buffer[0];
	int				numberBytes;

	while(error!=BIRD_ERROR_SUCCESS)
	{
		error = GetErrorText(error, pBuffer, sizeof(buffer), SIMPLE_MESSAGE);
		numberBytes = strlen(buffer);
		buffer[numberBytes] = '\n';		// append a newline to buffer
		printf("%s", buffer);
	}
}

/**
* \brief Open the Connection with the Tracker. Calls LoadCalibration function and set the system up with the calibration file.
* Remember that you have to set a calibration file first to open a correct connection to the Optical Tracking System.
* \return Returns true if the connection is well done. Throws an exception if an error occures related to the Optitrack API messages.
* @throw mitk::IGTException Throws an exception if InitializeCameras or LoadCalibration failed.
*/
bool mitk::AscensionMEDSAFETrackingDevice::OpenConnection()
{
	std::cout << "Connection of Ascension MEDSAFE Tracking System using ATC3DGm.\n" << std::endl;

	/* Initialize the medSAFE driver and DLL
	It is always necessary to first initialize the medSAFE system. By
	"system" we mean the set of medSAFE trackers attached to the PC. All trackers
	will be initialized by a single call to InitializeBIRDSystem(). If at any time
	during operation of the system an unrecoverable error occurs then the
	first course of action should be to attempt to Recall InitializeBIRDSystem()
	if this doesn't restore normal operating conditions there is probably a
	permanent failure - contact tech support.
	A call to InitializeBIRDSystem() does not return any information*/

	std::cout << "Initializing ATC3DGm system...\n" << std::endl;
	errorCode = InitializeBIRDSystem();
	if (errorCode != BIRD_ERROR_SUCCESS) errorHandler(errorCode);


	/* GET SYSTEM CONFIGURATION

	In order to get information about the system we have to make a call to
	GetBIRDSystemConfiguration(). This call will fill a fixed size structure
	containing amongst other things the number of trackers detected and the
	number of sensors and transmitters the system can support (Note: This
	does not mean that all sensors and transmitters that can be supported
	are physically attached)*/

	std::cout << "Getting ATC3DGm system configuration...\n" << std::endl;

	errorCode = GetBIRDSystemConfiguration(&medSAFE.m_config);
	if (errorCode != BIRD_ERROR_SUCCESS) errorHandler(errorCode);


	int sensorNum = 0;
	/* GET SENSOR CONFIGURATION

	Having determined how many sensors can be supported we can dynamically
	allocate storage for the information about each sensor.
	This information is acquired through a call to GetSensorConfiguration()
	This call will fill a fixed size structure containing amongst other things
	a status which indicates whether a physical sensor is attached to this
	sensor port or not*/

	std::cout << "Number of sensors (attached and not attached)" << medSAFE.m_config.numberSensors << "\n" << std::endl;

	pSensor = new CSensor[medSAFE.m_config.numberSensors];
	for (i = 0; i<medSAFE.m_config.numberSensors; i++)
	{
		errorCode = GetSensorConfiguration(i, &(pSensor + i)->m_config);

		if ((pSensor + id)->m_config.attached)
		{
			sensorNum++;

			//Add tool
			std::stringstream toolname;
			toolname << "Sensor_" << sensorNum;
			std::string s = toolname.str();
			const char *cstr = s.c_str();
			AddTool(cstr);
		}
		if (errorCode != BIRD_ERROR_SUCCESS)
		{
			errorHandler(errorCode);
		}
	}

	/* GET TRANSMITTER CONFIGURATION

	The call to GetTransmitterConfiguration() performs a similar task to the
	GetSensorConfiguration() call. It also returns a status in the filled
	structure which indicates whether a transmitter is attached to this
	port or not. In a single transmitter system it is only necessary to
	find where that transmitter is in order to turn it on and use it.*/

	std::cout << "Number of transmitters: " << medSAFE.m_config.numberTransmitters << "\n" << std::endl;

	pXmtr = new CXmtr[medSAFE.m_config.numberTransmitters];
	for (i = 0; i<medSAFE.m_config.numberTransmitters; i++)
	{
		errorCode = GetTransmitterConfiguration(i, &(pXmtr + i)->m_config);
		if (errorCode != BIRD_ERROR_SUCCESS)
		{
			errorHandler(errorCode);
		}
	}

	/* Search for the first attached transmitter and turn it on*/

	for (id = 0; id<medSAFE.m_config.numberTransmitters; id++)
	{
		if ((pXmtr + id)->m_config.attached)
		{
			// Transmitter selection is a system function.
			// Using the SELECT_TRANSMITTER parameter we send the id of the
			// transmitter that we want to run with the SetSystemParameter() call
			errorCode = SetSystemParameter(SELECT_TRANSMITTER, &id, sizeof(id));
			if (errorCode != BIRD_ERROR_SUCCESS) errorHandler(errorCode);
			break;
		}
	}

	this->SetState(mitk::TrackingDevice::Ready);

	return true;
}

/**
* \brief Close the Connection with the Tracker. Also CleanUp the Optitrack variables using the API: TT_CleanUp and TT_ShutDown.
* Sometimes API does not work properly and some problems during the Clean Up has been reported.
* \return Returns true if the cleaning up and shutdown worked correctly. Throws an exception if an error occures related to the Optitrack API messages.
* @throw mitk::IGTException Throws an exception if the System cannot ShutDown now or was not initialized.
*/
bool mitk::AscensionMEDSAFETrackingDevice::CloseConnection()

{
	/* Turn off the transmitter before exiting
	// We turn off the transmitter by "selecting" a transmitter with an id of "-1"*/

	id = -1;
	errorCode = SetSystemParameter(SELECT_TRANSMITTER, &id, sizeof(id));
	if (errorCode != BIRD_ERROR_SUCCESS) errorHandler(errorCode);

	/*Free memory allocations before exiting*/
	delete[] pSensor;
	delete[] pXmtr;

	std::cout << "Transmitter has been turned off.\n" << std::endl;
	return true;

}

/**
* \brief Start to Track the tools already defined. If no tools are defined for this tracker, it returns an error.
* Tools can be added using either AddToolByDescriptionFile or AddToolsByConfigurationFiles
* \return Returns true at least one tool was defined and the tracking is correct
* @throw mitk::IGTException Throws an exception if the System is not in State Ready .
*/
bool mitk::AscensionMEDSAFETrackingDevice::StartTracking()
{

	MITK_DEBUG << "StartTracking";
	bool resultIsTrackableTracked;

	if (this->GetState() != mitk::TrackingDevice::Ready)
	{
		MITK_INFO << "System is not in State Ready -> Cannot StartTracking";
		mitkThrowException(mitk::IGTException) << "System is not in State Ready -> Cannot StartTracking";
		return false;
	}

	this->SetState(mitk::TrackingDevice::Tracking);

	// Change the m_StopTracking Variable to false
	this->m_StopTrackingMutex->Lock();
	this->m_StopTracking = false;
	this->m_StopTrackingMutex->Unlock();

	m_TrackingFinishedMutex->Unlock(); // transfer the execution rights to tracking thread

	// Launch multiThreader using the Function ThreadStartTracking that executes the TrackTools() method
	m_ThreadID = m_MultiThreader->SpawnThread(this->ThreadStartTracking, this);    // start a new thread that executes the TrackTools() method

	return true;
}

/**
* \brief Stop the Tracking Thread and tools will not longer be updated.
* \return Returns true if Tracking thread could be stopped.
* @throw mitk::IGTException Throws an exception if System is not in State Tracking.
*/
bool mitk::AscensionMEDSAFETrackingDevice::StopTracking()
{
	MITK_DEBUG << "StopTracking";

	if (this->GetState() == mitk::TrackingDevice::Tracking) // Only if the object is in the correct state
	{
		//Change the StopTracking value
		m_StopTrackingMutex->Lock();  // m_StopTracking is used by two threads, so we have to ensure correct thread handling
		m_StopTrackingMutex->Unlock();
		this->SetState(mitk::TrackingDevice::Ready);
	}
	else
	{
		MITK_INFO << "System is not in State Tracking -> Cannot StopTracking";
		mitkThrowException(mitk::IGTException) << "System is not in State Tracking -> Cannot StopTracking";
		return false;
	}

	/******************************************************************************
	###############################################################################
	TODO: check the timestamp from the Optitrack API
	###############################################################################
	******************************************************************************/
	mitk::IGTTimeStamp::GetInstance()->Stop(this);

	m_TrackingFinishedMutex->Lock();
	return true;
	return true;
}


//=======================================================
// ThreadStartTracking
//=======================================================
ITK_THREAD_RETURN_TYPE mitk::AscensionMEDSAFETrackingDevice::ThreadStartTracking(void* pInfoStruct)
{
	MITK_DEBUG << "ThreadStartTracking";


	/* extract this pointer from Thread Info structure */
	struct itk::MultiThreader::ThreadInfoStruct * pInfo = (struct itk::MultiThreader::ThreadInfoStruct*)pInfoStruct;

	if (pInfo == NULL)
	{
		return ITK_THREAD_RETURN_VALUE;
	}

	if (pInfo->UserData == NULL)
	{
		return ITK_THREAD_RETURN_VALUE;
	}

	AscensionMEDSAFETrackingDevice *trackingDevice = static_cast<AscensionMEDSAFETrackingDevice*>(pInfo->UserData);

	if (trackingDevice != NULL)
	{
		// Call the TrackTools function in this thread
		trackingDevice->TrackTools();

	}
	else
	{
		mitkThrowException(mitk::IGTException) << "In ThreadStartTracking(): trackingDevice is NULL";
	}

	trackingDevice->m_ThreadID = -1; // reset thread ID because we end the thread here
	return ITK_THREAD_RETURN_VALUE;

}


//=======================================================
// GetAscensionMEDSAFETool
//=======================================================
mitk::AscensionMEDSAFETrackingTool* mitk::AscensionMEDSAFETrackingDevice::GetAscensionMEDSAFETool(unsigned int toolNumber) const
{
	MITK_DEBUG << "ThreadStartTracking";
	mitk::AscensionMEDSAFETrackingTool* t = nullptr;

	MutexLockHolder toolsMutexLockHolder(*m_ToolsMutex); // lock and unlock the mutex
	if (toolNumber < m_AllTools.size())
	{
		t = m_AllTools.at(toolNumber);
	}
	else
	{
		MITK_INFO << "The tool numbered " << toolNumber << " does not exist";
		mitkThrowException(mitk::IGTException) << "The tool numbered " << toolNumber << " does not exist";
	}
	return t;
}

//=======================================================
// TrackTools
//=======================================================
void mitk::AscensionMEDSAFETrackingDevice::TrackTools()
{
	MITK_DEBUG << "TrackTools";

	Point3D position;
	ScalarType t = 0.0;

	try
	{
		bool localStopTracking;       // Because m_StopTracking is used by two threads, access has to be guarded by a mutex. To minimize thread locking, a local copy is used here
		this->m_StopTrackingMutex->Lock();  // update the local copy of m_StopTracking
		localStopTracking = this->m_StopTracking;

		/* lock the TrackingFinishedMutex to signal that the execution rights are now transfered to the tracking thread */
		if (!localStopTracking)
		{
			m_TrackingFinishedMutex->Lock();
		}

		this->m_StopTrackingMutex->Unlock();
		while ((this->GetState() == mitk::TrackingDevice::Tracking) && (localStopTracking == false))
		{
			// For each Tracked Tool update the position and orientation

			/* Collect data from all birds
			Loop through all sensors and get a data record if the sensor is attached.
			Print result to screen
			Note: The default data format is DOUBLE_POSITION_ANGLES. We can use this
			format without first setting it.*/


			//DOUBLE_POSITION_ANGLES_TIME_STAMP_RECORD record, *pRecord = &record;

			SHORT_POSITION_QUATERNION_RECORD record, *pRecord = &record;

			std::cout << "Setting data format...\n" << std::endl;
			// Set format to double position angles time to match above record.
			DATA_FORMAT_TYPE mode = SHORT_POSITION_QUATERNION;// DOUBLE_POSITION_ANGLES_TIME_STAMP;
			
			for (sensorID = 0; sensorID<medSAFE.m_config.numberSensors; sensorID++)
				SetSensorParameter(sensorID, DATA_FORMAT, &mode, sizeof(mode));


			// Set up time delay for first loop
			// It only makes sense to request a data record from the sensor once per
			// measurement cycle. Therefore we set up a 10ms loop and request a record
			// only after at least 10ms have elapsed.

			std::cout << "Requesting data from connected sensors (sensor->m_config.attached)\n" << std::endl;

			goal = wait + clock();

			double oldtime = (double)(clock());

			// collect as many records as specified in the command line
			for (i = 0; i<records; i++)
			{
				// delay 10ms between collecting data
				// wait till time delay expires
				while (goal>clock());
				// set up time delay for next loop
				goal = wait + clock();

				// scan the sensors and request a record if the sensor is physically attached
				for (sensorID = 0; sensorID<medSAFE.m_config.numberSensors; sensorID++)
				{
					if ((pSensor + sensorID)->m_config.attached)
					{
						// sensor attached so get record
						errorCode = GetAsynchronousRecord(sensorID, pRecord, sizeof(record));
						if (errorCode != BIRD_ERROR_SUCCESS) {errorHandler(errorCode); }

						//double diff = (record.time - oldtime) * 1000;


						mitk::AscensionMEDSAFETrackingTool* currentTool = this->GetAscensionMEDSAFETool(sensorID);
						if (currentTool != nullptr)
						{
							currentTool->updateTool(sensorID,record.x,record.y,record.z, record.q[1], record.q[2], record.q[3], record.q[4]);
							std::cout << "Tool number " << sensorID << " updated position" << std::endl;
							MITK_DEBUG << "Tool number " << sensorID << " updated position";
						}
						else
						{
							std::cout << "Get data from tool number " << sensorID << " failed" << std::endl;
							MITK_DEBUG << "Get data from tool number " << sensorID << " failed";
							mitkThrowException(mitk::IGTException) << "Get data from tool number " << i << " failed";
						}

						// send output to console
						/*sprintf(output, "[%d] %8.3f %8.3f %8.3f: %8.2f %8.2f %8.2f # %6.1f\n",
							sensorID,
							record.x,
							record.y,
							record.z,
							record.a,
							record.e,
							record.r,
							diff
							);*/


						numberBytes = strlen(output);
						printf("%s", output);
						//oldtime = record.time;
					}
				}
			}

			std::cout << "Finished collecting data from sensors.\n" << std::endl;

			/* Update the local copy of m_StopTracking */
			this->m_StopTrackingMutex->Lock();
			localStopTracking = m_StopTracking;
			this->m_StopTrackingMutex->Unlock();
			Sleep(10);
		} // tracking ends if we pass this line

		m_TrackingFinishedMutex->Unlock(); // transfer control back to main thread
	}
	catch (...)
	{
		m_TrackingFinishedMutex->Unlock();
		this->StopTracking();
		mitkThrowException(mitk::IGTException) << "Error while trying to track tools. Thread stopped.";
	}
}

unsigned int mitk::AscensionMEDSAFETrackingDevice::GetToolCount() const
{
	MutexLockHolder lock(*m_ToolsMutex); // lock and unlock the mutex
	std::cout << this->m_AllTools.size() << std::endl;
	return static_cast<unsigned int>(this->m_AllTools.size());
}


mitk::TrackingTool* mitk::AscensionMEDSAFETrackingDevice::GetTool(unsigned int toolNumber) const
{
	MutexLockHolder lock(*m_ToolsMutex); // lock and unlock the mutex
	if (toolNumber < m_AllTools.size())
		return this->m_AllTools.at(toolNumber);
	return NULL;
}

mitk::TrackingTool* mitk::AscensionMEDSAFETrackingDevice::AddTool(const char* toolName)
{
	if (this->GetState() == Tracking)
	{
	  return NULL;
	}
	
	mitk::AscensionMEDSAFETrackingTool::Pointer t = mitk::AscensionMEDSAFETrackingTool::New();
	t->SetToolName(toolName);
	
	MutexLockHolder lock(*m_ToolsMutex); // lock and unlock the mutex
	m_AllTools.push_back(t);
	return t;
}

#endif