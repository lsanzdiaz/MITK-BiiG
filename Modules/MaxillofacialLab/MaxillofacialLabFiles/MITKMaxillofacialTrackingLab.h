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


//MITK

#include <mitkSurface.h>
#include <mitkPointLocator.h>


//QMITK
#include <QmitkPointListWidget.h>

// VTK
#include <vtkLandmarkTransform.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkPoints.h>

#include <vtkIterativeClosestPointTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkMath.h>
#include "vtkPolyData.h"
#include <vtkCellArray.h>

#include "itkVersorRigid3DTransform.h"


#include <MitkMaxillofacialLabExports.h>

typedef struct ToolSurfaceRegistration
{
	vtkSmartPointer<vtkLandmarkTransform> RegistrationTransformVTK;
	//mitk::AffineTransform3D::Pointer m_RegistrationTransformITK;
	mitk::AffineTransform3D::Pointer RegistrationTransformITK_Inverse;
	mitk::AffineTransform3D::Pointer RegistrationTransformITK;
	vtkSmartPointer<vtkLinearTransform> RegistrationTransformVTK_Inverse;
	double FRE;
};


/** @brief MITKIGTMaxillofacialTrackingLab */
class MitkMaxillofacialLab_EXPORT MITKMaxillofacialTrackingLab
{

public:
/** @brief default constructor*/
MITKMaxillofacialTrackingLab(int tool_number);

/** @brief default destructor*/
virtual ~MITKMaxillofacialTrackingLab();

void CalculateRegistration(int i, mitk::DataNode::Pointer m_ImageFiducialsDataNode, mitk::DataNode::Pointer m_TrackerFiducialsDataNode);

/** @brief Computes the fiducial registration error out of two sets of fiducials.
 *  The two sets must have the same size and the points must correspond to each other.
 *  @param transform        This transform is applied to the image fiducials before the FRE calculation if it is given.
 *  @return                 Returns the FRE. Returns -1 if there was an error.
 */

/** @brief This method returns the registration transform in the VTK format.*/
vtkSmartPointer<vtkLandmarkTransform> GetVTKRegistrationTransform(int i);
/** @brief This method returns the registration transform in the ITK format.*/
mitk::AffineTransform3D::Pointer GetITKRegistrationTransform(int i);

vtkSmartPointer<vtkLinearTransform> GetVTKRegistrationTransformInverse(int i);
mitk::AffineTransform3D::Pointer GetITKRegistrationTransformInverse(int i);

double GetRegistrationFRE(int i);

void SelectControlSurface(mitk::DataNode::Pointer ControlSurface);

void SelectMovingSurface(mitk::DataNode::Pointer MovingSurface);

double checkLandmarkError();

vtkMatrix4x4 * calculateLandmarkbasedWithICP();

private:
	

	double ComputeFRE(int i, mitk::PointSet::Pointer trackerFiducials, mitk::PointSet::Pointer ImageWorldFiducials, vtkSmartPointer<vtkLandmarkTransform> transform = NULL);
	/**
	* Checks if the difference between two given transformations is high which means the method returns
	* true if the difference exeeds the given position and angular threshold.
	*/
	
	mitk::DataNode::Pointer m_FixedNode;
	mitk::PointSet::Pointer m_FixedPointSet;
	mitk::Surface::Pointer m_FixedSurface;
	mitk::DataNode::Pointer m_MovingNode;
	mitk::PointSet::Pointer m_MovingPointSet;
	mitk::Surface::Pointer m_MovingSurface;
	int m_ToolNumber;

	vtkIterativeClosestPointTransform * m_ICPTransform;

	ToolSurfaceRegistration *m_ToolSurfaceRegistration;
};

#endif // MITKMaxillofacialTrackingLab_h

