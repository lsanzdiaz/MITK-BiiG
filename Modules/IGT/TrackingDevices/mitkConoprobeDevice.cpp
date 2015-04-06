/*------------------------------------------------------------------------------*/
/*       Optimet Smart32 Sample Software					*/
/*		 Smart32 API Tester Program												*/
/*       Copyright (c) 2008 Optimet Ltd.										*/
/*------------------------------------------------------------------------------*/

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <MitkIGTExports.h>

#include "mitkConoprobeDevice.h"

#include <Smart32Interface.h>
#include <Smart32Defs.h>
#include <Smart32Api.h>

#ifdef MITK_USE_CONOPROBE_TRACKER

ISmart **m_smartDevice;

//=======================================================
// Constructor
//=======================================================
mitk::ConoprobeDevice::ConoprobeDevice()
{
	
}

mitk::ConoprobeDevice::~ConoprobeDevice()
{
	/* stop tracking and disconnect from tracking device */
	if (GetState() == Tracking)
	{
		this->StopTracking();
	}
	if (GetState() == Ready)
	{
		this->CloseConnection();
	}
	/* cleanup tracking thread */
	if ((m_ThreadID != 0) && (m_MultiThreader.IsNotNull()))
	{
		m_MultiThreader->TerminateThread(m_ThreadID);
	}
	m_MultiThreader = NULL;
	m_ToolsMutex = NULL;
	m_StopTrackingMutex = NULL;

}


void  mitk::ConoprobeDevice::GetAcquisitionParameters()
{
	AcquisitionMode acquisitionMode = AcquisitionMode::TimeAcquisitionMode;
	
	UINT16 delayBetweenMeasurements = 10;
	UINT16 frequency = 1000;

	int test = sizeof(AcquisitionMode);
	m_smartDevice[0]->GetAcquisitionParams(&acquisitionMode, &frequency, &laserPower, &delayBetweenMeasurements);
	
	
	unsigned short coarsepower = laserPower >> 6;
	unsigned short finePower = laserPower & 0x003F;
	unsigned short compositePower = coarsepower;
	compositePower <<= 6;
	compositePower |= finePower;
	
	printf("Current acquisition parameters:\n");
	printf("coarse power  = %d  fine power = %d  freq = %d, dilution = %d  mode = %d\n",
		coarsepower, finePower, frequency, pulseDilution, acquisitionMode);
}

void mitk::ConoprobeDevice::SetAcquisitionParameters()
{
	AcquisitionMode acquisitionMode = AcquisitionMode::TimeAcquisitionMode;
	UINT16 delayBetweenMeasurements = 1;
	UINT16 frequency = 1000;

	//laserPower = 11;
	unsigned short coarsepower = 13;
	unsigned short finePower = 0;
	unsigned short compositePower = coarsepower;
	compositePower <<= 6;
	compositePower |= finePower;
	

	for (int i = 0; i < this->GetToolCount(); i++)
	{
		m_smartDevice[i]->SetAcquisitionParams(acquisitionMode, frequency, compositePower, delayBetweenMeasurements);
	}
}

void mitk::ConoprobeDevice::DisplayLenses()
{
	int lenscount = m_smartDevice[0]->GetLensCount();
	printf("Number of Lens: %d\n", lenscount);
	
	Lens *lens = new Lens();

	for (int i = 0; i< lenscount; i++)
	{
		m_smartDevice[0]->GetLens(i, lens);
		printf("Details of lens %d%c\n", lens->FocalDistance, lens->Type);
		printf("Focal length: %d\n", lens->FocalDistance);
		printf("Min Distance: %f\n", lens->DistanceMin);
		printf("Max Distance: %f\n\n", lens->DistanceMax);
	}

	int Lens_Index = m_smartDevice[0]->GetActiveLensIndex();
	printf("Active Lens Index: %d\n", Lens_Index);
}

//=======================================================
// Static method: IsDeviceInstalled
//=======================================================
bool mitk::ConoprobeDevice::IsDeviceInstalled()
{
	return true;
}

//=======================================================
// OpenConnection
//=======================================================
bool mitk::ConoprobeDevice::OpenConnection()
{

	m_MultiThreader = itk::MultiThreader::New();
	m_ToolsMutex = itk::FastMutexLock::New();
	//Clear List of tools
	this->m_AllTools.clear();

	DWORD hostADDR = 16909065; // %IP address 1.2.3.9
	timeval timeOut;
	timeOut.tv_sec = 1;
	timeOut.tv_usec = 0;
	
	DiscoveryInfo *discInfo;

	// Not initialize the system twice.
	if (!m_initialized)
	{

		m_NumOfProbes = ISmart::DiscoverProbes(hostADDR, timeOut);

		// create the device objects
		m_smartDevice = new ISmart*[m_NumOfProbes];

		for (int i = 0; i <m_NumOfProbes; i++)
			m_smartDevice[i] = ISmart::Create();

		DiscoveryInfo probeInfo;
		for (int i = 0; i< m_NumOfProbes; i++)
		{
			ISmart::DiscoveredProbeInfo(i, &probeInfo);
			std::string toolname;
			toolname = i;
			this->AddTool(toolname.c_str());
			m_smartDevice[i]->Init(probeInfo.ProbeAddress, hostADDR, 10000);
			
		}

		//GetAcquisitionParameters();
		SetAcquisitionParameters();
		DisplayLenses();
	
		MITK_DEBUG << "Initialize Conoprobe Tracking System";
		m_initialized = true;
		this->SetState(mitk::TrackingDevice::Ready);
	}
	//this->LoadCalibration();
	return m_initialized;
}


//=======================================================
// CloseConnection
//=======================================================
bool mitk::ConoprobeDevice::CloseConnection()
{
	MITK_DEBUG << "CloseConnection";
	int resultStop, resultShutdown;

	if (m_initialized) // Close connection if the System was initialized first
	{
		if (this->GetState() == mitk::TrackingDevice::Tracking)
		{
			MITK_DEBUG << "Device state: Tracking -> Stoping the Tracking";
			resultStop = this->StopTracking(); //Stop tracking on close
		}

		this->SetState(mitk::ConoprobeDevice::Setup);

		m_smartDevice[0]->Destroy(m_smartDevice[0]);
		this->~ConoprobeDevice();
	}
	return false;
}

//=======================================================
// StartTracking
//=======================================================
bool mitk::ConoprobeDevice::StartTracking()
{
	if (this->GetState() != Ready)
		return false;

	this->SetState(Tracking);      // go to mode Tracking
	this->m_StopTrackingMutex->Lock();  // update the local copy of m_StopTracking
	this->m_StopTracking = false;
	this->m_StopTrackingMutex->Unlock();

	m_TrackingFinishedMutex->Unlock(); // transfer the execution rights to tracking thread

	m_ThreadID = m_MultiThreader->SpawnThread(this->ThreadStartTracking, this);    // start a new thread that executes the TrackTools() method
	mitk::IGTTimeStamp::GetInstance()->Start(this);
	return true;

	/******************************************************************************
	###############################################################################
	TODO: check the timestamp from the Conoprobe API
	###############################################################################
	******************************************************************************/
	//mitk::IGTTimeStamp::GetInstance()->Start(this);

	// Launch multiThreader using the Function ThreadStartTracking that executes the MeasurementAcquisition() method
	//m_ThreadID = m_MultiThreader->SpawnThread(this->ThreadStartTracking, this);    // start a new thread that executes the MeasurementAcquisition() method

}


bool mitk::ConoprobeDevice::InternalAddTool(mitk::ConoprobeTool* tool)
{
	if (tool == NULL)
		return false;
	ConoprobeTool::Pointer p = tool;
	
		/* now that the tool is added to the device, add it to list too */
		m_ToolsMutex->Lock();
		this->m_AllTools.push_back(p);
		m_ToolsMutex->Unlock();
		this->Modified();
		return true;
}


//=======================================================
// GetTool
//=======================================================
mitk::TrackingTool* mitk::ConoprobeDevice::AddTool(const char* toolName)
{
	mitk::ConoprobeTool::Pointer t = mitk::ConoprobeTool::New();
	
	t->SetToolName(toolName);

	if (this->InternalAddTool(t) == false)
		return NULL;
	
	return t.GetPointer();
}

//=======================================================
// GetTool
//=======================================================
mitk::TrackingTool* mitk::ConoprobeDevice::GetTool(unsigned int toolNumber) const
{
	return static_cast<mitk::TrackingTool*>(GetConoprobeTool(toolNumber));
}

/**
* \return Returns the number of tools which have been added to the device.
*/
unsigned int mitk::ConoprobeDevice::GetToolCount() const
{
	MutexLockHolder toolsMutexLockHolder(*m_ToolsMutex); // lock and unlock the mutex
	return m_AllTools.size();
}


void mitk::ConoprobeDevice::MeasurementAcquisition()
{
	MITK_DEBUG << "Conoprobe Tool Monitoring";

	if (this->GetState() != Tracking)
		return;

	/* lock the TrackingFinishedMutex to signal that the execution rights are now transfered to the tracking thread */
	MutexLockHolder trackingFinishedLockHolder(*m_TrackingFinishedMutex); // keep lock until end of scope

	try
	{
		/* Update the local copy of m_StopTracking */
		bool localStopTracking;
		this->m_StopTrackingMutex->Lock();	
		localStopTracking = m_StopTracking;
		this->m_StopTrackingMutex->Unlock();

		// Stream meausrements
		printf("\nStream Measurements\n");
		m_smartDevice[0]->StreamStartMeasuring(0);
	

		while (this->GetState() == mitk::TrackingDevice::Tracking && (localStopTracking == false))
		{
			for (int i = 0; i < this->GetToolCount(); i++)
			{
				Measurement m = m_smartDevice[0]->GetSingleMeasurement();
				printf("Distance: %f Frequency: %f Snr: %hd BitSet: %hd\n", m.Distance, m.Frequency, m.Snr, m.BitSet);
				ConoprobeTool* currentTool = this->GetConoprobeTool(i);
				if (currentTool != nullptr)
				{
					currentTool->updateTool(m.Distance, m.Snr, m.Total);
					std::cout << "Conoprobe Tool updated position" << std::endl;
				}
			}
			Sleep(1);
		} // tracking ends if we pass this line

		/* Update the local copy of m_StopTracking */
		this->m_StopTrackingMutex->Lock();
		localStopTracking = m_StopTracking;
		this->m_StopTrackingMutex->Unlock();
		
	}
	catch (...)
	{
		//m_TrackingFinishedMutex->Lock();
		mitkThrowException(mitk::IGTException) << "Error while trying to track tools. Thread stopped.";
	}
}
//=======================================================
// StopTracking
//=======================================================
bool mitk::ConoprobeDevice::StopTracking()
{
	MITK_DEBUG << "StopTracking";


	if (this->GetState() == mitk::TrackingDevice::Tracking) // Only if the object is in the correct state
	{
		//Change the StopTracking value
		m_StopTrackingMutex->Lock();  // m_StopTracking is used by two threads, so we have to ensure correct thread handling
		this->SetState(mitk::TrackingDevice::Ready);
		m_smartDevice[0]->StopMeasuring();
		m_StopTrackingMutex->Unlock();
		
	}
	else
	{
		MITK_INFO << "System is not in State Tracking -> Cannot StopTracking";
		mitkThrowException(mitk::IGTException) << "System is not in State Tracking -> Cannot StopTracking";
		return false;
	}

	m_TrackingFinishedMutex->Unlock(); // transfer control back to main thread

	/******************************************************************************
	###############################################################################
	TODO: check the timestamp from the Optitrack API
	###############################################################################
	******************************************************************************/
	mitk::IGTTimeStamp::GetInstance()->Stop(this);

	
	return true;
}

//=======================================================
// ThreadStartTracking
//=======================================================
ITK_THREAD_RETURN_TYPE mitk::ConoprobeDevice::ThreadStartTracking(void* pInfoStruct)
{
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
	ConoprobeDevice *conopDevice = (ConoprobeDevice*)pInfo->UserData;
	if (conopDevice != NULL)
	{
		conopDevice->MeasurementAcquisition();
	}
    conopDevice->m_ThreadID = -1;  // erase thread id, now that this thread will end.
	return ITK_THREAD_RETURN_VALUE;	
}

//=======================================================
// GetConoprobeTool
//=======================================================
mitk::ConoprobeTool* mitk::ConoprobeDevice::GetConoprobeTool(unsigned int toolNumber) const
{
	MITK_DEBUG << "ThreadStartTracking";
	ConoprobeTool* t = nullptr;

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

#endif

