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
#include <qmessagebox.h>

/** @Constructor */

MITKMaxillofacialTrackingLab::MITKMaxillofacialTrackingLab(int tool_number)
{
	m_ToolNumber = tool_number;
	m_ToolSurfaceRegistration = new ToolSurfaceRegistration[m_ToolNumber];
	for (int i = 0; i < m_ToolNumber; i++)
	{
		m_ToolSurfaceRegistration[i].RegistrationTransformITK = mitk::AffineTransform3D::New();
		m_ToolSurfaceRegistration[i].RegistrationTransformITK_Inverse = mitk::AffineTransform3D::New();
	}
}

/** @Destructor */
MITKMaxillofacialTrackingLab::~MITKMaxillofacialTrackingLab()
{

}


/** @CalculateRegistration uses a Point-Based Algorithm to calculate the registration transform and compute the error */

void MITKMaxillofacialTrackingLab::CalculateRegistration(int i, mitk::DataNode::Pointer m_ImageFiducialsDataNode, mitk::DataNode::Pointer m_TrackerFiducialsDataNode)
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

  m_ToolSurfaceRegistration[i].RegistrationTransformVTK = vtkSmartPointer<vtkLandmarkTransform>::New();
  m_ToolSurfaceRegistration[i].RegistrationTransformVTK_Inverse = m_ToolSurfaceRegistration[i].RegistrationTransformVTK->GetLinearInverse();



  m_ToolSurfaceRegistration[i].RegistrationTransformVTK->SetSourceLandmarks(sourcePoints);
  m_ToolSurfaceRegistration[i].RegistrationTransformVTK->SetTargetLandmarks(targetPoints);
  m_ToolSurfaceRegistration[i].RegistrationTransformVTK->SetModeToRigidBody();
  m_ToolSurfaceRegistration[i].RegistrationTransformVTK->Modified();
  m_ToolSurfaceRegistration[i].RegistrationTransformVTK->Update();


  
  /*compute FRE of transform */
  double m_FRE = ComputeFRE(i, trackerFiducials, imageFiducials, m_ToolSurfaceRegistration[i].RegistrationTransformVTK);
  
  /* conversion from vtk back to itk/mitk data types */

  itk::Matrix<float,3,3> rotationFloat = itk::Matrix<float,3,3>();
  itk::Vector<float,3> translationFloat = itk::Vector<float,3>();
  itk::Matrix<double,3,3> rotationDouble = itk::Matrix<double,3,3>();
  itk::Vector<double,3> translationDouble = itk::Vector<double,3>();

  vtkSmartPointer<vtkMatrix4x4> m = m_ToolSurfaceRegistration[i].RegistrationTransformVTK->GetMatrix();
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
  m_ToolSurfaceRegistration[i].RegistrationTransformITK->SetMatrix(rotationDouble);
  m_ToolSurfaceRegistration[i].RegistrationTransformITK->SetOffset(translationDouble);

  m = m_ToolSurfaceRegistration[i].RegistrationTransformVTK_Inverse->GetMatrix();
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
  
  m_ToolSurfaceRegistration[i].RegistrationTransformITK_Inverse->SetMatrix(rotationDouble);
  m_ToolSurfaceRegistration[i].RegistrationTransformITK_Inverse->SetOffset(translationDouble);

  return;
}

vtkSmartPointer<vtkLandmarkTransform> MITKMaxillofacialTrackingLab::GetVTKRegistrationTransform(int i)
{
	return m_ToolSurfaceRegistration[i].RegistrationTransformVTK;
}

mitk::AffineTransform3D::Pointer MITKMaxillofacialTrackingLab::GetITKRegistrationTransform(int i)
{
	return m_ToolSurfaceRegistration[i].RegistrationTransformITK;
}

vtkSmartPointer<vtkLinearTransform> MITKMaxillofacialTrackingLab::GetVTKRegistrationTransformInverse(int i)
{
	return m_ToolSurfaceRegistration[i].RegistrationTransformVTK_Inverse;
}

mitk::AffineTransform3D::Pointer MITKMaxillofacialTrackingLab::GetITKRegistrationTransformInverse(int i)
{
	return m_ToolSurfaceRegistration[i].RegistrationTransformITK_Inverse;
}


double MITKMaxillofacialTrackingLab::GetRegistrationFRE(int i)
{

	return m_ToolSurfaceRegistration[i].FRE;
}

/** @ComputeFRE calculates de error given by the application of a registration transform */

double MITKMaxillofacialTrackingLab::ComputeFRE(int i, mitk::PointSet::Pointer trackerFiducials, mitk::PointSet::Pointer ImageWorldFiducials, vtkSmartPointer<vtkLandmarkTransform> transform)
{
	m_ToolSurfaceRegistration[i].FRE = 0;
	if (trackerFiducials->GetSize() != ImageWorldFiducials->GetSize()) return -1;
	
	for (unsigned int i = 0; i < trackerFiducials->GetSize(); i++)
	{
		itk::Point<double> current_tracker_fiducial_point = trackerFiducials->GetPoint(i);
		if (transform != NULL)
		{
			current_tracker_fiducial_point = transform->TransformPoint(trackerFiducials->GetPoint(i)[0], trackerFiducials->GetPoint(i)[1], trackerFiducials->GetPoint(i)[2]);
		}
		double cur_error_squared = current_tracker_fiducial_point.SquaredEuclideanDistanceTo(ImageWorldFiducials->GetPoint(i));
		m_ToolSurfaceRegistration[i].FRE += cur_error_squared;
	}

	m_ToolSurfaceRegistration[i].FRE = sqrt(m_ToolSurfaceRegistration[i].FRE / (double)trackerFiducials->GetSize());

	return m_ToolSurfaceRegistration[i].FRE;
}


void MITKMaxillofacialTrackingLab::SelectControlSurface(mitk::DataNode::Pointer ControlSurface)
{	
	m_FixedNode = ControlSurface;
	m_FixedSurface = dynamic_cast<mitk::Surface*>(m_FixedNode->GetData());
}

void MITKMaxillofacialTrackingLab::SelectMovingSurface(mitk::DataNode::Pointer MovingSurface)
{
	m_MovingNode = MovingSurface;
	m_MovingSurface = dynamic_cast<mitk::Surface*>(m_MovingNode->GetData());
}

double MITKMaxillofacialTrackingLab::checkLandmarkError()
{
	mitk::Point3D FixedPoint, MovingPoint, TransformedPoint;
	double TP[3], MP[3];

	mitk::Surface::Pointer m_TransformedMovingSurface = mitk::Surface::New();
	vtkSmartPointer<vtkPolyData> m_TransfPolydata = vtkPolyData::New();
	vtkSmartPointer<vtkPoints> TransfPoints = vtkPoints::New();

	//std::cout << "Matrix: " << *m_ICPTransform->GetMatrix() << std::endl;
	m_ICPTransform->TransformPoints(m_MovingSurface->GetVtkPolyData()->GetPoints(), TransfPoints);	
	m_TransfPolydata->SetPoints(TransfPoints);
	m_TransformedMovingSurface->SetVtkPolyData(m_TransfPolydata);


	//First option: point locator
	mitk::PointLocator::Pointer FixedLocator = mitk::PointLocator::New();
	mitk::PointSet::Pointer FixedSurfacePointSet = mitk::PointSet::New();
	for (int pointId = 0; pointId < m_FixedSurface->GetVtkPolyData()->GetNumberOfPoints(); ++pointId)
	{
		FixedSurfacePointSet->SetPoint(pointId, m_FixedSurface->GetVtkPolyData()->GetPoint(pointId));
	}
	//std::cout << FixedSurfacePointSet->GetSize() << std::endl;
	FixedLocator->SetPoints(FixedSurfacePointSet);

	
	/*mitk::PointLocator::Pointer MovingLocator = mitk::PointLocator::New();
	
	mitk::PointSet::Pointer MovingSurfacePointSet = mitk::PointSet::New();
	for (int pointId = 0; pointId < m_MovingSurface->GetVtkPolyData()->GetNumberOfPoints(); pointId++)
	{
		MovingSurfacePointSet->SetPoint(pointId, m_MovingSurface->GetVtkPolyData()->GetPoint(pointId));
	}

	std::cout << MovingSurfacePointSet->GetSize() << std::endl;
	MovingLocator->SetPoints(MovingSurfacePointSet);*/
	
	mitk::PointLocator::IdType PointLocatorId;
	
	mitk::PointLocator::DistanceType Distance1, Distance2, TotalDistance1, TotalDistance2;
	mitk::PointLocator::DistanceType TotalDistance;

	
	for (int TransformedPointId = 0; TransformedPointId < m_MovingSurface->GetVtkPolyData()->GetNumberOfPoints(); TransformedPointId++)
	{

		//point = m_MovingSurface->GetVtkPolyData()->GetPoint(pointId2);
		//std::cout << "Point: " << point[0] << "; " << point[1] << "; " << point[2] << std::endl;
		TransformedPoint = m_TransformedMovingSurface->GetVtkPolyData()->GetPoint(TransformedPointId);
		MovingPoint = m_MovingSurface->GetVtkPolyData()->GetPoint(TransformedPointId);
		//std::cout << "Transformed Point: " << Tpoint[0] << "; " << Tpoint[1] << "; " << Tpoint[2] << std::endl;
		FixedLocator->FindClosestPointAndDistance(TransformedPoint, &PointLocatorId, &Distance1);	
		Distance2 = TransformedPoint.EuclideanDistanceTo(MovingPoint);	
		TotalDistance1 += Distance1;
		TotalDistance2 += Distance2;
	}


	double FRE1 = sqrt(TotalDistance1 / m_FixedSurface->GetVtkPolyData()->GetNumberOfPoints());
	std::cout << FRE1 << std::endl;

	double FRE2 = sqrt(TotalDistance2 / m_FixedSurface->GetVtkPolyData()->GetNumberOfPoints());
	std::cout << FRE2 << std::endl;

	TotalDistance = TotalDistance1 + TotalDistance2;
	std::cout << TotalDistance << std::endl;

	double FRE = sqrt(TotalDistance / m_FixedSurface->GetVtkPolyData()->GetNumberOfPoints());
	
	std::cout << FRE << std::endl;
	return FRE;
}

// ICP with vtkLandmarkTransformation
vtkMatrix4x4 * MITKMaxillofacialTrackingLab::calculateLandmarkbasedWithICP()
{
	m_ICPTransform = NULL;
	
	
	m_ICPTransform = vtkIterativeClosestPointTransform::New();
	m_ICPTransform->SetCheckMeanDistance(1);
	m_ICPTransform->SetSource(m_MovingSurface->GetVtkPolyData());
	m_ICPTransform->SetTarget(m_FixedSurface->GetVtkPolyData());
	m_ICPTransform->SetMaximumNumberOfIterations(50);
	m_ICPTransform->StartByMatchingCentroidsOn();

	m_ICPTransform->GetLandmarkTransform()->SetModeToRigidBody();
	
	m_ICPTransform->Modified();
	m_ICPTransform->Update();
	
	vtkMatrix4x4 * matrix = m_ICPTransform->GetMatrix();

	std::cout << "Matrix: " << *matrix << std::endl;

	double determinant = fabs(matrix->Determinant());
	if ((determinant < mitk::eps) || (determinant > 100) || (determinant < 0.01)
		|| (determinant == itk::NumericTraits<double>::infinity())
		|| (determinant == itk::NumericTraits<double>::quiet_NaN())
		|| (determinant == itk::NumericTraits<double>::signaling_NaN())
		|| (determinant == -itk::NumericTraits<double>::infinity())
		|| (determinant == -itk::NumericTraits<double>::quiet_NaN())
		|| (determinant == -itk::NumericTraits<double>::signaling_NaN())
		|| (!(determinant <= 0) && !(determinant > 0)))
	{
		QMessageBox msgBox;
		msgBox.setText("Suspicious determinant of matrix calculated by ICP.\n"
			"Please select more points or other points!");
		msgBox.exec();
		return NULL;
	}

	
	/*
	m_MovingSurface->GetGeometry()->SetIndexToWorldTransformByVtkMatrix(matrix);

	mitk::BaseData::Pointer movingData = m_MovingNode->GetData();
	mitk::Geometry3D::Pointer movingNodeGeometry = movingData->GetGeometry(0);
	movingNodeGeometry->Compose(matrix);
	movingData->GetTimeGeometry()->Update();*/
	
	return matrix;
}


