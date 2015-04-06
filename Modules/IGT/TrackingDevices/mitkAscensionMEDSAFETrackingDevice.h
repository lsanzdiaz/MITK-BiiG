
#ifndef AscensionMEDSAFETrackingDevice_H_HEADER_INCLUDED
#define AscensionMEDSAFETrackingDevice_H_HEADER_INCLUDED

// Next default values for new objects
// 

#define _APS_NEXT_RESOURCE_VALUE        101
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           101


// TODO: reference additional headers your program requires here

#include <MitkIGTExports.h>
#include <mitkTrackingDevice.h>
#include <windows.h>



#include <itkMultiThreader.h>
#include <mitkTrackingTypes.h>
#include <mitkIGTTimeStamp.h>
#include <itkFastMutexLock.h>
#include <itksys/SystemTools.hxx>
#include <itkMutexLockHolder.h>

#include <mitkAscensionMEDSAFETrackingTool.h>
#include <itkMultiThreader.h>

#include "itkFastMutexLock.h"


#include <stdlib.h>
#include <stdio.h>		// printf
#include <string.h>		// string handling
#include <time.h>		// needed for time functions
#include <vector>

/**
* \brief IGT Exceptions
*/
#include "mitkIGTIOException.h"
#include "mitkIGTTimeStamp.h"
#include "mitkIGTException.h"

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
	class MitkIGT_EXPORT AscensionMEDSAFETrackingDevice : public TrackingDevice
	{
	public:

		mitkClassMacro(AscensionMEDSAFETrackingDevice, TrackingDevice);
		friend class AscensionMEDSAFETrackingTool;
		itkFactorylessNewMacro(Self)
			itkCloneMacro(Self)


			/**
			* \brief Sets the refresh rate of the virtual tracking device in ms
			* \warning This refresh rate is not guaranteed. A thread is used to refresh the positions
			* of the virtual tools. However, this thread may not run at all during this refresh time period.
			* \return Sets the refresh rate of the virtual tracking device in ms
			*/
			itkSetMacro(RefreshRate, unsigned int)

			/**
			* \brief Returns the refresh rate in ms.
			* \return Returns the refresh rate in ms.
			*/
			itkGetConstMacro(RefreshRate, unsigned int)

			/**
			* \brief Starts the tracking.
			*
			* After StartTracking() is called,
			* the tools will move on their spline paths with a constant velocity that can be set with
			* SetToolSpeed(). The standard velocity is 10 seconds for one complete cycle along the spline path.
			* \warning tool speed is not yet used in the current version
			* \return Returns true if the tracking is started. Returns false if there was an error.
			*/
			virtual bool StartTracking();

		/**
		* \brief Stops the tracking.
		* \return Returns true if the tracking is stopped. Returns false if there was an error.
		*/
		virtual bool StopTracking();

		/**
		* \brief Opens the connection to the device. This have to be done before the tracking is started.
		* @throw mitk::IGTException Throws an exception if there are two less control points to start the the virtual device.
		*/
		virtual bool OpenConnection();

		/**
		* \brief Closes the connection and clears all resources.
		*/
		virtual bool CloseConnection();

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

		mitk::AscensionMEDSAFETrackingTool* GetAscensionMEDSAFETool(unsigned int toolNumber) const;

		/**
		* \brief Adds a tool to the tracking device.
		*
		* The tool will have a random path on which it will move around. The path is created with a
		* spline function and random control points inside the tracking volume.
		*
		* \param tool  The tool which will be added.
		* \return Returns true if the tool has been added, false otherwise.
		*/
		TrackingTool* AddTool(const char* toolName);

		/**
		* \brief Update each tool location in the list m_AllTools
		* @throw mitk::IGTException Throws an exception if the getting data operation failed for a defined tool
		*/
		void TrackTools();

	protected:
		AscensionMEDSAFETrackingDevice();
		~AscensionMEDSAFETrackingDevice();
		/**
		* \brief This method tracks tools as long as the variable m_Mode is set to "Tracking".
		* Tracking tools means generating random numbers for the tool position and orientation.
		* @throw mitk::IGTException Throws an mitk::IGTException if there is an error during virtual tool tracking.
		*/

		static ITK_THREAD_RETURN_TYPE ThreadStartTracking(void* data); ///< static start method for tracking thread

		//mitk::VirtualTrackingTool* GetInternalTool(unsigned int idx);

		//typedef std::vector<VirtualTrackingTool::Pointer> ToolContainer; ///< container type for tracking tools
		typedef std::vector<AscensionMEDSAFETrackingTool::Pointer> ToolContainer; ///< container type for tracking tools
		ToolContainer m_AllTools;                       ///< container for all tracking tools
		itk::FastMutexLock::Pointer m_ToolsMutex; ///< mutex for coordinated access of tool container

		itk::MultiThreader::Pointer m_MultiThreader;    ///< MultiThreader that starts continuous tracking update
		int m_ThreadID;

		unsigned int m_RefreshRate;                     ///< refresh rate of the internal tracking thread in milliseconds (NOT refreshs per second!)
		unsigned int m_NumberOfControlPoints;           ///< number of control points for the random path generation

		mitk::ScalarType m_Bounds[6];                   ///< bounding box of the tracking volume stored as {xMin, xMax, yMin, yMax, zMin, zMax}

   private:

	void errorHandler(int error);
};
}//mitk
#endif /* MITKASCENSIONMEDSAFETRACKINGDEVICE_H_HEADER_INCLUDED_ */