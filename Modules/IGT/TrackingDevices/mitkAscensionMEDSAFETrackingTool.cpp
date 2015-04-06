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

#include "mitkAscensionMEDSAFETrackingTool.h"
#include <itkMutexLockHolder.h>

typedef itk::MutexLockHolder<itk::FastMutexLock> MutexLockHolder;

mitk::AscensionMEDSAFETrackingTool::AscensionMEDSAFETrackingTool()
: InternalTrackingTool()
{
  
}

mitk::AscensionMEDSAFETrackingTool::~AscensionMEDSAFETrackingTool()
{
}


//=======================================================
// SetDataValid
//=======================================================
void mitk::AscensionMEDSAFETrackingTool::SetDataValid(bool validate)
{
	MITK_DEBUG << "SetDataValid";
	// sets if the tracking data (position & Orientation) is valid
	this->m_DataValid = validate;
}

//=======================================================
// updateTool
//=======================================================
void mitk::AscensionMEDSAFETrackingTool::updateTool(int ID, double x, double y, double z, double q1, double q2, double q3, double q4)
{
	MITK_DEBUG << "updateTool";

	this->m_Position[0] = x* 1000 / 32768;
	this->m_Position[1] = y* 1000 / 32768;
	this->m_Position[2] = z* 1000 / 32768;

	this->m_Orientation.x() = q1 * 180 / 32768;
	this->m_Orientation.y() = q2 * 180 / 32768;
	this->m_Orientation.z() = q3 * 180 / 32768;
	this->m_Orientation.r() = q4 * 180 / 32768;

	this->SetDataValid(true);

	MITK_DEBUG << this->m_Position[0] << "   " << this->m_Position[1] << "   " << this->m_Position[2];
	//MITK_DEBUG << data[3] << "   " << data[4] << "   " << data[5] << "   " << data[6];
}