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

#ifndef ConoprobeTool_H_HEADER_INCLUDED_
#define ConoprobeTool_H_HEADER_INCLUDED_

#include <MitkIGTExports.h>

#include "mitkTrackingDevice.h"
#include "mitkTrackingTypes.h"
#include "mitkTrackingTool.h"
#include <mitkInternalTrackingTool.h>


#include "mitkIGTTimeStamp.h"

#include <itkMultiThreader.h>
#include "mitkIGTException.h"

/**
* \brief Function to get the Error messages from API
*/


namespace mitk
{

	class ConoprobeDevice;
  /** Documentation:
  *   \brief An object of this class represents the a Tool tracked by Optitrack System. You can define
  *  the tool by the a definition file like in the example in ****. Remember that it will be necessary to
  *  to have a license for using the Conoprobe Device.
  *  See  http://www.naturalpoint.com/ for details.
  *  
  */
  class MitkIGT_EXPORT ConoprobeTool : public InternalTrackingTool
  {
  public:
	 friend class ConoprobeDevice;
	 mitkClassMacro(mitk::ConoprobeTool, mitk::InternalTrackingTool);
    itkNewMacro(Self);

    /**
    * \brief Ask API the next number of defined tool
    * \return Returns the next ID (int) for a new tool in the device list for API
    * @throw mitk::IGTException Throws an exception if get_IDnext failed
    */
    int get_IDnext();

    /**
    * \brief Delete the tool from the list of tools inside API Optitrack
    * \return Returns true if the deletion was correct
    * @throw mitk::IGTException Throws an exception if
    */
    bool DeleteConoprobeTool();

    /**
    * \brief Set the position to a given one
    * @throw mitk::IGTException Throws an exception if
    */
    void SetPosition(mitk::Point3D position, ScalarType eps=0.0);

    /**
    * \brief Set the orientation to a given one using a quaternion nomenclature
    * @throw mitk::IGTException Throws an exception if
    */
    void SetOrientation(mitk::Quaternion orientation, ScalarType eps=0.0);

    /**
    * \brief Get the position of the tool
    * @throw mitk::IGTException Throws an exception if
    */
    void GetPosition(mitk::Point3D& position) const;

    /**
    * \brief Get the orientation of the tool using quaternion nomenclature
    * @throw mitk::IGTException Throws an exception if
    */
    void GetOrientation(mitk::Quaternion& orientation) const;

    /**
    * \brief Set the tool enabled for tracking.
  * \return Return true if the enabling was successfull
    * @throw mitk::IGTException Throws an exception if
    */
    bool Enable();

    /**
    * \brief Set the tool disabled for tracking.
  * \return Return true if the disabling was successfull
    * @throw mitk::IGTException Throws an exception if
    */
    bool Disable();

    /**
    * \brief Check if the tool is enabled (true) or not.
  * \return Return true if the tool is enabled for tracking
    * @throw mitk::IGTException Throws an exception if
    */
    bool IsEnabled() const;

    /**
    * \brief Check if the data of the tool is valid.
  * \return Return true if location data is valid
    * @throw mitk::IGTException Throws an exception if
    */
    bool IsDataValid() const;

    /**
    * \brief Get the expectated error in the tracked tool
  * \return Return the error location
    * @throw mitk::IGTException Throws an exception if
    */
	float GetMeasurementError() const;

    /**
    * \brief Set the FLE (Fiducial Localization Error) for the tool
    * @throw mitk::IGTException Throws an exception if
    */
	void SetMeasurementError(float FLEerror);

    /**
    * \brief Set the valid flag for tracking data to true
    * @throw mitk::IGTException Throws an exception if
    */
    void SetDataValid(bool _arg);

    /**
    * \brief Update location and orientation of the tool
    * @throw mitk::IGTException Throws an exception if
    */
	void updateTool(float distance, float snr, UINT16 total);

    /**
    * \brief Constructor of the class
    */
	ConoprobeTool();

    /**
    * \brief Destructor of the class
    */
	~ConoprobeTool();

     /**
    * \brief ID number from Optitrack API
    */
    int m_ID;
	int m_MeasurementError;

  private:
	  ConoprobeTool(const ConoprobeTool&);
	  const ConoprobeTool& operator=(const ConoprobeTool&);

  };
}
#endif /* OptiTrackTrackingTool_H_HEADER_INCLUDED_ */
