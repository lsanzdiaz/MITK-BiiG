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


// Blueberry
//#include <berryISelectionService.h>
//#include <berryIWorkbenchWindow.h>

// Qmitk
#include "MITKIGTMaxillofacialTrackingLab.h"
#include "QmitkStdMultiWidget.h"

#include <QmitkNDIConfigurationWidget.h>
#include <QmitkFiducialRegistrationWidget.h>
#include <QmitkUpdateTimerWidget.h>
#include <QmitkToolSelectionWidget.h>
#include <QmitkToolTrackingStatusWidget.h>


#include <mitkCone.h>
#include <mitkIGTException.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateDataType.h>
#include <itkVector.h>

#include <vtkConeSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkPoints.h>

// Qt
#include <QMessageBox>
#include <QIcon>
#include <QPushButton>

// vtk
#include <mitkVtkResliceInterpolationProperty.h>

/** @Constructor */

MITKIGTMaxillofacialTrackingLab::MITKIGTMaxillofacialTrackingLab()
{
	m_Source = NULL;
	m_PermanentRegistrationFilter = NULL;
	m_Visualizer = NULL;
	m_VirtualView = NULL;
	m_PSRecordingPointSet = NULL;
	m_PointSetRecording = false;
	m_PermanentRegistration = false;
	m_CameraView = false;
	m_PermanentRegistrationSourcePoints = NULL;
}

/** @Destructor */
MITKIGTMaxillofacialTrackingLab::~MITKIGTMaxillofacialTrackingLab()
{

}


/** @CalculateRegistration uses a Point-Based Algorithm to calculate the registration transform and compute the error */

void MITKIGTMaxillofacialTrackingLab::CalculateRegistration(mitk::DataNode::Pointer m_ImageFiducialsDataNode, mitk::DataNode::Pointer m_TrackerFiducialsDataNode)
{
   /* retrieve fiducials from data storage */
  mitk::PointSet::Pointer imageFiducials = dynamic_cast<mitk::PointSet*>(m_ImageFiducialsDataNode->GetData());
  mitk::PointSet::Pointer trackerFiducials = dynamic_cast<mitk::PointSet*>(m_TrackerFiducialsDataNode->GetData());

   /* conversion to vtk data types (we will use the vtk landmark based transform) and
  convert point sets to vtk poly data*/
  vtkSmartPointer<vtkPoints> sourcePoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkPoints> targetPoints = vtkSmartPointer<vtkPoints>::New();
  for (int i=0; i<imageFiducials->GetSize(); i++)
    {
    double point_sources[3] = {trackerFiducials->GetPoint(i)[0],trackerFiducials->GetPoint(i)[1],trackerFiducials->GetPoint(i)[2]};
    sourcePoints->InsertNextPoint(point_sources);
	double point_targets[3] = { imageFiducials->GetPoint(i)[0], imageFiducials->GetPoint(i)[1], imageFiducials->GetPoint(i)[2] };
	targetPoints->InsertNextPoint(point_targets);
    }

  /*here, the actual transform is computed */

  m_transform = vtkSmartPointer<vtkLandmarkTransform>::New();
  m_transform->SetSourceLandmarks(sourcePoints);
  m_transform->SetTargetLandmarks(targetPoints);
  m_transform->SetModeToRigidBody();
  m_transform->Modified();
  m_transform->Update();
  
  /*compute FRE of transform */
  double FRE = ComputeFRE(trackerFiducials,imageFiducials,m_transform);
  
  /* conversion from vtk back to itk/mitk data types */

  itk::Matrix<float,3,3> rotationFloat = itk::Matrix<float,3,3>();
  itk::Vector<float,3> translationFloat = itk::Vector<float,3>();
  itk::Matrix<double,3,3> rotationDouble = itk::Matrix<double,3,3>();
  itk::Vector<double,3> translationDouble = itk::Vector<double,3>();

  vtkSmartPointer<vtkMatrix4x4> m = m_transform->GetMatrix();
  for(int k=0; k<3; k++) for(int l=0; l<3; l++)
  {
    rotationFloat[k][l] = m->GetElement(k,l);
    rotationDouble[k][l] = m->GetElement(k,l);

  }
  for(int k=0; k<3; k++)
  {
    translationFloat[k] = m->GetElement(k,3);
    translationDouble[k] = m->GetElement(k,3);
  }
  /*Create affine transform 3D surface, accesible from other modules or plugins */

  RegistrationTransform = mitk::AffineTransform3D::New();
  RegistrationTransform->SetMatrix(rotationDouble);
  RegistrationTransform->SetOffset(translationDouble);
  
  return;
}

vtkSmartPointer<vtkLandmarkTransform> MITKIGTMaxillofacialTrackingLab::GetVTKTransform()
{
	return m_transform;
}


/** @ComputeFRE calculates de error given by the application of a registration transform */

double MITKIGTMaxillofacialTrackingLab::ComputeFRE(mitk::PointSet::Pointer trackerFiducials, mitk::PointSet::Pointer ImageWorldFiducials, vtkSmartPointer<vtkLandmarkTransform> transform)
{
	FRE = 0;
	if (trackerFiducials->GetSize() != ImageWorldFiducials->GetSize()) return -1;
	
	for (unsigned int i = 0; i < trackerFiducials->GetSize(); i++)
	{
		itk::Point<double> current_tracker_fiducial_point = trackerFiducials->GetPoint(i);
		if (transform != NULL)
		{
			current_tracker_fiducial_point = transform->TransformPoint(trackerFiducials->GetPoint(i)[0], trackerFiducials->GetPoint(i)[1], trackerFiducials->GetPoint(i)[2]);
		}
		double cur_error_squared = current_tracker_fiducial_point.SquaredEuclideanDistanceTo(ImageWorldFiducials->GetPoint(i));
		FRE += cur_error_squared;
	}

	FRE = sqrt(FRE / (double)trackerFiducials->GetSize());

	return FRE;
}


bool MITKIGTMaxillofacialTrackingLab::IsTransformDifferenceHigh(mitk::NavigationData::Pointer transformA, mitk::NavigationData::Pointer transformB, double euclideanDistanceThreshold, double angularDifferenceThreshold)
{
  if(transformA.IsNull() || transformA.IsNull())
    {return false;}
  mitk::Point3D posA,posB;
  posA = transformA->GetPosition();
  posB = transformB->GetPosition();


  if(posA.EuclideanDistanceTo(posB) > euclideanDistanceThreshold)
    {return true;}

  double returnValue;
  mitk::Quaternion rotA,rotB;
  rotA = transformA->GetOrientation();
  rotB = transformB->GetOrientation();

  itk::Vector<double,3> point; //caution 5D-Tools: Vector must lie in the YZ-plane for a correct result.
  point[0] = 0.0;
  point[1] = 0.0;
  point[2] = 100000.0;

  rotA.normalize();
  rotB.normalize();

  itk::Matrix<double,3,3> rotMatrixA;
  for(int i=0; i<3; i++) for(int j=0; j<3; j++) rotMatrixA[i][j] = rotA.rotation_matrix_transpose().transpose()[i][j];

  itk::Matrix<double,3,3> rotMatrixB;
  for(int i=0; i<3; i++) for(int j=0; j<3; j++) rotMatrixB[i][j] = rotB.rotation_matrix_transpose().transpose()[i][j];

  itk::Vector<double,3> pt1 = rotMatrixA * point;
  itk::Vector<double,3> pt2 = rotMatrixB * point;

  returnValue = (pt1[0]*pt2[0]+pt1[1]*pt2[1]+pt1[2]*pt2[2]) / ( sqrt(pow(pt1[0],2.0)+pow(pt1[1],2.0)+pow(pt1[2],2.0)) * sqrt(pow(pt2[0],2.0)+pow(pt2[1],2.0)+pow(pt2[2],2.0)));
  returnValue = acos(returnValue);

  if(returnValue*57.3 > angularDifferenceThreshold){return true;}

  return false;
}



