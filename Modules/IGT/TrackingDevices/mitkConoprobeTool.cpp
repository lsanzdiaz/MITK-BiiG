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

#include "mitkConoprobeTool.h"

#ifdef MITK_USE_CONOPROBE_TRACKER

/**
* \brief API library header for Conoprobe Device
*/
#include <Smart32Interface.h>
#include <Smart32Defs.h>
#include <Smart32Api.h>
//=======================================================
// Constructor
//=======================================================
mitk::ConoprobeTool::ConoprobeTool()
  :  mitk::InternalTrackingTool(),
    m_ID(-1)
{
  MITK_DEBUG << "Creating ConoprobeTool Object";
}

//=======================================================
// Destructor
//=======================================================
mitk::ConoprobeTool::~ConoprobeTool()
{
  MITK_DEBUG << "Deleting ConoprobeTool Object";
}

//=======================================================
// DeleteConoprobeTool
//=======================================================
bool mitk::ConoprobeTool::DeleteConoprobeTool()
{
  MITK_DEBUG << "DeleteTrackable";
  int resultRemoveTrackable;
  
  return true;
}

//=======================================================
// SetPosition
//=======================================================
void mitk::ConoprobeTool::SetPosition(mitk::Point3D position, ScalarType eps)
{
  MITK_DEBUG << "SetPosition";
  // sets the position
  this->m_Position[0] = position[0];
  this->m_Position[1] = position[1];
  this->m_Position[2] = position[2];
}

//=======================================================
// SetOrientation
//=======================================================
void mitk::ConoprobeTool::SetOrientation(mitk::Quaternion orientation, ScalarType eps)
{
  MITK_DEBUG << "SetOrientation";
  // sets the orientation as a quaternion
  this->m_Orientation.x() = orientation.x();
  this->m_Orientation.y() = orientation.y();
  this->m_Orientation.z() = orientation.z();
  this->m_Orientation.r() = orientation.r();
}

//=======================================================
// GetPosition (from Lens)
//=======================================================
void mitk::ConoprobeTool::GetPosition(mitk::Point3D& positionOutput) const
{
  MITK_DEBUG << "GetPosition";
  // returns the current position of the tool as an array of three floats (in the tracking device coordinate system)
  positionOutput[0] = this->m_Position[0];
  positionOutput[1] = this->m_Position[1];
  positionOutput[2] = this->m_Position[2];
}

//=======================================================
// GetOrientation (from Lens)
//=======================================================
void mitk::ConoprobeTool::GetOrientation(mitk::Quaternion& orientation) const
{
  MITK_DEBUG << "GetOrientation";
  // returns the current orientation of the tool as a quaternion (in the tracking device coordinate system)
  orientation.x() = this->m_Orientation.x();
  orientation.y() = this->m_Orientation.y();
  orientation.z() = this->m_Orientation.z();
  orientation.r() = this->m_Orientation.r();
}

//=======================================================
// Enable
//=======================================================
bool mitk::ConoprobeTool::Enable()
{
  MITK_DEBUG << "Enable";
  // enable the tool, so that it will be tracked. Returns true if enabling was successfull
  return true;
}

//=======================================================
// Disable
//=======================================================
bool mitk::ConoprobeTool::Disable()
{
  MITK_DEBUG << "Disable";
  // disables the tool, so that it will not be tracked anymore. Returns true if disabling was successfull
  return true;
}

//=======================================================
// IsEnabled
//=======================================================
bool mitk::ConoprobeTool::IsEnabled() const
{
  MITK_DEBUG << "IsEnabled";
  // returns whether the tool is enabled or disabled
  return true;
}

//=======================================================
// IsDataValid
//=======================================================
bool mitk::ConoprobeTool::IsDataValid() const
{
  MITK_DEBUG << "IsDataValid";
  // returns true if the current position data is valid (no error during tracking, tracking error below threshold, ...)
  return this->m_DataValid;
}

//=======================================================
// GetTrackingError
//=======================================================
float mitk::ConoprobeTool::GetMeasurementError() const
{
  MITK_DEBUG << "GetTrackingError";
  // return one value that corresponds to the overall tracking error. The dimension of this value is specific to each tracking device
  return this->m_MeasurementError;
}

//=======================================================
// SetTrackingError
//=======================================================
void mitk::ConoprobeTool::SetMeasurementError(float error)
{
  MITK_DEBUG << "GetTrackingError";
  //< sets the tracking error
  //this->m_FLE = error;
  //this->UpdateError;
  this->m_TrackingError = error;
}

//=======================================================
// SetDataValid
//=======================================================
void mitk::ConoprobeTool::SetDataValid(bool validate)
{
  MITK_DEBUG << "SetDataValid";
  // sets if the tracking data (position & Orientation) is valid
  this->m_DataValid = validate;
}

//=======================================================
// updateTool
//=======================================================
void mitk::ConoprobeTool::updateTool(float distance, float snr, UINT16 total)
{
  MITK_DEBUG << "updateTool";

  if (this->IsEnabled())
  {
	  // m -> mm
	  this->m_Position[0] = 0;
	  this->m_Position[1] = 0;
	  this->m_Position[2] = distance; // The distance will be projected like the z-coordinate
	  this->SetDataValid(true);
  }
  else
  {
	  this->SetDataValid(false);
  }
}

//=======================================================
// IF Conoprobe is not installed set functions to warnings
//=======================================================
#else

//=======================================================
// Constructor
//=======================================================
mitk::ConoprobeTool::ConoprobeTool()
  :  mitk::InternalTrackingTool(),
    m_ID(-1)
{
  //MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// Destructor
//=======================================================
mitk::ConoprobeTool::~ConoprobeTool()
{
  //MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// get_IDnext
//=======================================================
int mitk::ConoprobeTool::get_IDnext()
{
 // MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
  return -1;
}

//=======================================================
// DeleteTrackable
//=======================================================
bool mitk::ConoprobeTool::DeleteTrackable()
{
 //MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
 return false;
}

//=======================================================
// SetPosition
//=======================================================
void mitk::ConoprobeTool::SetPosition(mitk::Point3D position, ScalarType eps)
{
  //MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// SetOrientation
//=======================================================
void mitk::ConoprobeTool::SetOrientation(mitk::Quaternion orientation, ScalarType eps)
{
  //MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// GetPosition
//=======================================================
void mitk::ConoprobeTool::GetPosition(mitk::Point3D& positionOutput) const
{
 // MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// GetOrientation
//=======================================================
void mitk::ConoprobeTool::GetOrientation(mitk::Quaternion& orientation) const
{
 // MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// Enable
//=======================================================
bool mitk::ConoprobeTool::Enable()
{
 // MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
  return false;
}

//=======================================================
// Disable
//=======================================================
bool mitk::ConoprobeTool::Disable()
{
	//MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
 return false;
}

//=======================================================
// IsEnabled
//=======================================================
bool mitk::ConoprobeTool::IsEnabled() const
{
	//MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
  return false;
}

//=======================================================
// IsDataValid
//=======================================================
bool mitk::ConoprobeTool::IsDataValid() const
{
	// MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
  return false;
}

//=======================================================
// GetTrackingError
//=======================================================
float mitk::ConoprobeTool::GetMeasurementError() const
{
	//MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
  return 0.0;
}

//=======================================================
// SetTrackingError
//=======================================================
void mitk::ConoprobeTool::SetMeasurementError(float error)
{
	//MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// SetDataValid
//=======================================================
void mitk::ConoprobeTool::SetDataValid(bool validate)
{
	//MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}

//=======================================================
// updateTool
//=======================================================
void mitk::ConoprobeTool::updateTool(float distance, float snr, UINT16 total)
{
	//MITK_WARN("IGT") << "Error: " << mitk::ConoprobeErrorMessages::GetConoprobeErrorMessage(100);
}


#endif
