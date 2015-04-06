
#ifndef ConoprobeDevice_H_HEADER_INCLUDED
#define ConoprobeDevice_H_HEADER_INCLUDED

// TODO: reference additional headers your program requires here

#include "mitkTrackingDevice.h"

#include <MitkIGTExports.h>

#include <itkMultiThreader.h>
#include <itkFastMutexLock.h>

#include <itksys/SystemTools.hxx>
#include <itkMutexLockHolder.h>
#include <vector>

#include "mitkTrackingTypes.h"
#include "mitkConoprobeTool.h"

/**
* \brief IGT Exceptions
*/
#include "mitkIGTIOException.h"
#include "mitkIGTTimeStamp.h"
#include "mitkIGTException.h"

/**
* \brief MutexHolder to keep rest of Mutex
*/
typedef itk::MutexLockHolder<itk::FastMutexLock> MutexLockHolder;

namespace mitk
{
	/** Documentation
	* \brief Class representing a tracking device which generates random positions / orientations.
	*        No hardware is needed for tracking device.
	*
	* This TrackingDevice class does not interface with a physical tracking device. It simulates
	* a tracking device by moving the tools on a randomly generated spline path.
	*
	* \ingroup IGT
	*/
	class MitkIGT_EXPORT ConoprobeDevice : public TrackingDevice
	{

	public:

		friend class ConoprobeTool;

		mitkClassMacro(ConoprobeDevice, TrackingDevice);
		itkFactorylessNewMacro(Self)
		itkCloneMacro(Self)

		void DisplayLenses();
		void SetAcquisitionParameters();
		void GetAcquisitionParameters();
	
		/**
		* @returns Returns true if the Optitrack tracker is installed on this build (means activated in CMAKE). False if not.
		*/
		virtual bool IsDeviceInstalled();

		// Define the Type of Tracker as DefinitionOfTool (MITK)
		typedef mitk::TrackingDeviceType ConoprobeDeviceType;

		/**
		* \brief Open the Connection with the Tracker. Calls LoadCalibration function and set the system up with the calibration file.
		* Remember that you have to set a calibration file first to open a correct connection to the Optical Tracking System.
		* \return Returns true if the connection is well done. Throws an exception if an error occures related to the Optitrack API messages.
		* @throw mitk::IGTException Throws an exception if InitializeCameras or LoadCalibration failed.
		*/
		virtual bool OpenConnection();

		/**
		* \brief Close the Connection with the Tracker. Also CleanUp the Optitrack variables using the API: TT_CleanUp and TT_ShutDown.
		* Sometimes API does not work properly and some problems during the Clean Up has been reported.
		* \return Returns true if the cleaning up and shutdown worked correctly. Throws an exception if an error occures related to the Optitrack API messages.
		* @throw mitk::IGTException Throws an exception if the System cannot ShutDown now or was not initialized.
		*/
		virtual bool CloseConnection();

		/**
		* \brief Start to Track the tools already defined. If no tools are defined for this tracker, it returns an error.
		* Tools can be added using either AddToolByDescriptionFile or AddToolsByConfigurationFiles
		* \return Returns true at least one tool was defined and the tracking is correct
		* @throw mitk::IGTException Throws an exception if the System is not in State Ready .
		*/
		virtual bool StartTracking();

		/**
		* \brief Stop the Tracking Thread and tools will not longer be updated.
		* \return Returns true if Tracking thread could be stopped.
		* @throw mitk::IGTException Throws an exception if System is not in State Tracking.
		*/
		virtual bool StopTracking();

		/**
		* \return Returns the number of tools which have been added to the device.
		*/
		virtual unsigned int GetToolCount() const;

		/**
		* \param toolNumber The number of the tool which should be given back.
		* \return Returns the tool which the number "toolNumber". Returns NULL, if there is
		* no tool with this number.
		*/
		TrackingTool* GetTool(unsigned int toolNumber) const;

		/**
		* \brief Return the tool pointer of the tool number toolNumber
		* \param toolNumber The number of the tool which should be given back.
		* \return Returns the tool which the number "toolNumber". Returns NULL, if there is
		* no tool with this number.
		* @throw mitk::IGTException Throws an exception if there is the required tool does not exist.
		*/
		ConoprobeTool* GetConoprobeTool(unsigned int toolNumber) const;

		/**
		* \brief Start the Tracking Thread for the tools
		* @throw mitk::IGTException Throws an exception if variable trackingDevice is NULL
		*/
		static ITK_THREAD_RETURN_TYPE ThreadStartTracking(void* data);

		/**
		* \brief Update each tool location in the list m_AllTools
		* @throw mitk::IGTException Throws an exception if the getting data operation failed for a defined tool
		*/
		void MeasurementAcquisition();

		mitk::TrackingTool* AddTool(const char* toolName);

		bool InternalAddTool(mitk::ConoprobeTool* tool);

	
	protected:
		ConoprobeDevice();
		~ConoprobeDevice();
	private:

		int m_NumOfProbes;

		// initial state of acquisition variables
		UINT16 frequency = 0;
		UINT16 laserPower = 0;
		UINT16 pulseDilution = 0;
		/**
		* \brief Described if the system was initialized at least once during execution. This is due
		* to some reported problems during the clean up , shutdown and initialization again.
		*/
		bool m_initialized = false;

		/**
		* \brief Vector of pointers pointing to all defined tools
		*/
		std::vector<mitk::ConoprobeTool::Pointer> m_AllTools;

		/**
		* \brief Mutex for coordinated access of tool container
		*/
		itk::FastMutexLock::Pointer m_ToolsMutex;

		/**
		* \brief MultiThreader that starts continuous tracking update
		*/
		itk::MultiThreader::Pointer m_MultiThreader;

		/**
		* \brief ThreadID number identification
		*/
		int m_ThreadID;

};
}//mitk
#endif /* MITKCONOPROBEDEVICE_H_HEADER_INCLUDED_ */