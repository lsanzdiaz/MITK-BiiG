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



// MITK Maxillofacial Tracking Lab
#include "MITKMaxillofacialTrackingLab.h"

/** @Constructor */

MITKMaxillofacialTrackingLab::MITKMaxillofacialTrackingLab()
{
	m_RegistrationTransformITK = mitk::AffineTransform3D::New();
	m_RegistrationTransformITK_Inverse = mitk::AffineTransform3D::New();
}

/** @Destructor */
MITKMaxillofacialTrackingLab::~MITKMaxillofacialTrackingLab()
{

}


/** @CalculateRegistration uses a Point-Based Algorithm to calculate the registration transform and compute the error */

void MITKMaxillofacialTrackingLab::CalculateRegistration(mitk::DataNode::Pointer m_ImageFiducialsDataNode, mitk::DataNode::Pointer m_TrackerFiducialsDataNode)
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

  m_RegistrationTransformVTK = vtkSmartPointer<vtkLandmarkTransform>::New();
  m_RegistrationTransformVTK_Inverse = m_RegistrationTransformVTK->GetLinearInverse();



  m_RegistrationTransformVTK->SetSourceLandmarks(sourcePoints);
  m_RegistrationTransformVTK->SetTargetLandmarks(targetPoints);
  m_RegistrationTransformVTK->SetModeToRigidBody();
  m_RegistrationTransformVTK->Modified();
  m_RegistrationTransformVTK->Update();


  
  /*compute FRE of transform */
  double m_FRE = ComputeFRE(trackerFiducials, imageFiducials, m_RegistrationTransformVTK);
  
  /* conversion from vtk back to itk/mitk data types */

  itk::Matrix<float,3,3> rotationFloat = itk::Matrix<float,3,3>();
  itk::Vector<float,3> translationFloat = itk::Vector<float,3>();
  itk::Matrix<double,3,3> rotationDouble = itk::Matrix<double,3,3>();
  itk::Vector<double,3> translationDouble = itk::Vector<double,3>();

  vtkSmartPointer<vtkMatrix4x4> m = m_RegistrationTransformVTK->GetMatrix();
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
  m_RegistrationTransformITK->SetMatrix(rotationDouble);
  m_RegistrationTransformITK->SetOffset(translationDouble);

  

  m = m_RegistrationTransformVTK_Inverse->GetMatrix();
  for (int k = 0; k<3; k++) for (int l = 0; l<3; l++)
  {
	  rotationFloat[k][l] = m->GetElement(k, l);
	  rotationDouble[k][l] = m->GetElement(k, l);

  }
  for (int k = 0; k<3; k++)
  {
	  translationFloat[k] = m->GetElement(k, 3);
	  translationDouble[k] = m->GetElement(k, 3);
  }
  
  m_RegistrationTransformITK_Inverse->SetMatrix(rotationDouble);
  m_RegistrationTransformITK_Inverse->SetOffset(translationDouble);

  return;
}

vtkSmartPointer<vtkLandmarkTransform> MITKMaxillofacialTrackingLab::GetVTKRegistrationTransform()
{
	return m_RegistrationTransformVTK;
}

mitk::AffineTransform3D::Pointer MITKMaxillofacialTrackingLab::GetITKRegistrationTransform()
{
	return m_RegistrationTransformITK;
}

vtkSmartPointer<vtkLinearTransform> MITKMaxillofacialTrackingLab::GetVTKRegistrationTransformInverse()
{
	return m_RegistrationTransformVTK_Inverse;
}

mitk::AffineTransform3D::Pointer MITKMaxillofacialTrackingLab::GetITKRegistrationTransformInverse()
{
	return m_RegistrationTransformITK_Inverse;
}


double MITKMaxillofacialTrackingLab::GetRegistrationFRE()
{

	return m_FRE;
}

/** @ComputeFRE calculates de error given by the application of a registration transform */

double MITKMaxillofacialTrackingLab::ComputeFRE(mitk::PointSet::Pointer trackerFiducials, mitk::PointSet::Pointer ImageWorldFiducials, vtkSmartPointer<vtkLandmarkTransform> transform)
{
	m_FRE = 0;
	if (trackerFiducials->GetSize() != ImageWorldFiducials->GetSize()) return -1;
	
	for (unsigned int i = 0; i < trackerFiducials->GetSize(); i++)
	{
		itk::Point<double> current_tracker_fiducial_point = trackerFiducials->GetPoint(i);
		if (transform != NULL)
		{
			current_tracker_fiducial_point = transform->TransformPoint(trackerFiducials->GetPoint(i)[0], trackerFiducials->GetPoint(i)[1], trackerFiducials->GetPoint(i)[2]);
		}
		double cur_error_squared = current_tracker_fiducial_point.SquaredEuclideanDistanceTo(ImageWorldFiducials->GetPoint(i));
		m_FRE += cur_error_squared;
	}

	m_FRE = sqrt(m_FRE / (double)trackerFiducials->GetSize());

	return m_FRE;
}




