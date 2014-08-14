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

#if !defined(MITKMAXILLOFACIALLAB_H__INCLUDED)
#define MITKMAXILLOFACIALLAB_H__INCLUDED

//QMITK
#include <QmitkPointListWidget.h>

// VTK
#include <vtkLandmarkTransform.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkPoints.h>


#include <MitkMaxillofacialLabExports.h>

/** @brief MITKIGTMaxillofacialTrackingLab */
class MitkMaxillofacialLab_EXPORT MITKMaxillofacialTrackingLab
{

public:
/** @brief default constructor*/
MITKMaxillofacialTrackingLab();

/** @brief default destructor*/
virtual ~MITKMaxillofacialTrackingLab();

void CalculateRegistration(mitk::DataNode::Pointer m_ImageFiducialsDataNode, mitk::DataNode::Pointer m_TrackerFiducialsDataNode);

/** @brief Computes the fiducial registration error out of two sets of fiducials.
 *  The two sets must have the same size and the points must correspond to each other.
 *  @param transform        This transform is applied to the image fiducials before the FRE calculation if it is given.
 *  @return                 Returns the FRE. Returns -1 if there was an error.
 */

/** @brief This method returns the registration transform in the VTK format.*/
vtkSmartPointer<vtkLandmarkTransform> GetVTKRegistrationTransform();
/** @brief This method returns the registration transform in the ITK format.*/
mitk::AffineTransform3D::Pointer GetITKRegistrationTransform();
double GetRegistrationFRE();

private:
	vtkSmartPointer<vtkLandmarkTransform> m_RegistrationTransformVTK;
	mitk::AffineTransform3D::Pointer m_RegistrationTransformITK;
	double m_FRE;

	double ComputeFRE(mitk::PointSet::Pointer trackerFiducials, mitk::PointSet::Pointer ImageWorldFiducials, vtkSmartPointer<vtkLandmarkTransform> transform = NULL);
	/**
	* Checks if the difference between two given transformations is high which means the method returns
	* true if the difference exeeds the given position and angular threshold.
	*/
	//bool IsTransformDifferenceHigh(mitk::NavigationData::Pointer transformA, mitk::NavigationData::Pointer transformB, double euclideanDistanceThreshold = .8, double angularDifferenceThreshold = .8);

};

#endif // MITKMaxillofacialTrackingLab_h

