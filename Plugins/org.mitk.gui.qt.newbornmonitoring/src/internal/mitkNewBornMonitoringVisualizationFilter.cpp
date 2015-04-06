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

#include "mitkNewBornMonitoringVisualizationFilter.h"

mitk::NewBornMonitoringVisualizationFilter::NewBornMonitoringVisualizationFilter()
	: NavigationDataObjectVisualizationFilter(),
	m_AssociatedSurfaceModelList(), m_RotationMode(RotationStandard)
{

}

mitk::NewBornMonitoringVisualizationFilter::~NewBornMonitoringVisualizationFilter()
{
	m_RepresentationList.clear();
	m_OffsetList.clear();
	m_AssociatedSurfaceModelList.clear();
}

void mitk::NewBornMonitoringVisualizationFilter::SetAssociatedSurfaceModel(unsigned int idx, BaseData* data)
{
	m_AssociatedSurfaceModelList[idx] = RepresentationPointer(data);
}

/**
* \brief Get the representation object associated with the index idx
*
* \param idx the corresponding input number with which the BaseData is associated
* \return Returns the desired BaseData if it exists for the given input; Returns NULL
*         if no BaseData was found.
*/
const mitk::BaseData* mitk::NewBornMonitoringVisualizationFilter::GetAssociatedSurfaceModel(unsigned int idx)
{
	RepresentationPointerMap::const_iterator iter = m_AssociatedSurfaceModelList.find(idx);
	if (iter != m_AssociatedSurfaceModelList.end())
		return iter->second;

	return NULL;
}

void mitk::NewBornMonitoringVisualizationFilter::SetRotationMode(RotationMode r)
{
	m_RotationMode = r;
}

/*
* \brief Transfer the information from the input to the associated BaseData. This method changes with respect to the method of the base class.
*/
void mitk::NewBornMonitoringVisualizationFilter::GenerateData()
{
	/*get each input, lookup the associated BaseData and transfer the data*/
	DataObjectPointerArray inputs = this->GetInputs(); //get all inputs
	std::cout << "number of inputs: " << this->GetNumberOfInputs() << std::endl;
	std::cout << "number of indexed inputs: " << this->GetNumberOfIndexedInputs() << std::endl;
	
	for (unsigned int index = 0; index < this->GetNumberOfIndexedInputs(); index++)
	{
		//get the needed variables
		const mitk::NavigationData *nd = this->GetInput(index);
		assert(nd);

		mitk::NavigationData* output = this->GetOutput(index);
		assert(output);

		//check if the data is valid
		if (!nd->IsDataValid())
		{

			output->SetDataValid(false);
			continue;
		}
		output->Graft(nd); // copy all information from input to output

		//Get representation data
		const mitk::BaseData* data = this->GetRepresentationObject(index);
		if (data == NULL)
		{
			MITK_WARN << "No BaseData associated with input " << index;
			continue;
		}

		//get the transform from data
		mitk::AffineTransform3D::Pointer affineTransform = data->GetGeometry()->GetIndexToWorldTransform();
		if (affineTransform.IsNull())
		{
			MITK_WARN << "AffineTransform IndexToWorldTransform not initialized!";
			continue;
		}

		//check for offset
		mitk::AffineTransform3D::Pointer offset = this->GetOffset(index);

		//store the current scaling to set it after transformation
		mitk::Vector3D spacing = data->GetGeometry()->GetSpacing();
		//clear spacing of data to be able to set it again afterwards
		ScalarType scale[] = { 1.0, 1.0, 1.0 };
		data->GetGeometry()->SetSpacing(scale);

		/*now bring quaternion to affineTransform by using vnl_Quaternion*/
		affineTransform->SetIdentity();

		/*if (this->GetTransformOrientation(index) == true)
		{
			mitk::NavigationData::OrientationType orientation = nd->GetOrientation();

			/* because of an itk bug, the transform can not be calculated with float data type.
			To use it in the mitk geometry classes, it has to be transfered to mitk::ScalarType which is float 
			static AffineTransform3D::MatrixType m;*/

			//convert quaternion to rotation matrix depending on the rotation mode
			
		/*	if (m_RotationMode == RotationStandard)
			{
				//calculate the transform from the quaternions
				static itk::QuaternionRigidTransform<double>::Pointer quatTransform = itk::QuaternionRigidTransform<double>::New();
				// convert mitk::ScalarType quaternion to double quaternion because of itk bug
				vnl_quaternion<double> doubleQuaternion(orientation.x(), orientation.y(), orientation.z(), orientation.r());
				quatTransform->SetIdentity();
				quatTransform->SetRotation(doubleQuaternion);
				quatTransform->Modified();
				mitk::TransferMatrix(quatTransform->GetMatrix(), m);
			}

			else if (m_RotationMode == RotationTransposed)
			{
				vnl_matrix_fixed<mitk::ScalarType, 3, 3> rot = orientation.rotation_matrix_transpose();
				for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) m[i][j] = rot[i][j];
			}
			affineTransform->SetMatrix(m);
		}*/

		if (this->GetTransformPosition(index) == true)
		{
			///*set the offset by convert from itkPoint to itkVector and setting offset of transform*/
			mitk::Vector3D pos;
			pos.SetVnlVector(nd->GetPosition().GetVnlVector());
			affineTransform->SetOffset(pos);
		}
		affineTransform->Modified();

		//set the transform to data
		if (offset.IsNotNull()) //first use offset if there is one.
		{
			mitk::AffineTransform3D::Pointer overallTransform = mitk::AffineTransform3D::New();
			overallTransform->SetIdentity();
			overallTransform->Compose(affineTransform);
			overallTransform->Compose(offset);
			data->GetGeometry()->SetIndexToWorldTransform(overallTransform);
		}
		else
		{
			data->GetGeometry()->SetIndexToWorldTransform(affineTransform);
		}

		//set the original spacing to keep scaling of the geometrical object
		data->GetGeometry()->SetSpacing(spacing);
		data->GetGeometry()->TransferItkToVtkTransform(); // update VTK Transform for rendering too
		data->GetGeometry()->Modified();
		data->Modified();

		output->SetDataValid(true); // operation was successful, therefore data of output is valid.
	}
}

/*
* \brief Transfer the information from the input to the associated BaseData. This method changes with respect to the method of the base class.
*/
void mitk::NewBornMonitoringVisualizationFilter::SetSurfaceTransform(int index, mitk::AffineTransform3D::Pointer surface_transform)
{
	m_SurfaceTransformList[index] = surface_transform;
}

mitk::AffineTransform3D::Pointer mitk::NewBornMonitoringVisualizationFilter::GetSurfaceTransform(int index)
{
	OffsetPointerMap::const_iterator iter = m_SurfaceTransformList.find(index);
	if (iter != m_SurfaceTransformList.end())
		return iter->second;
	return NULL;
}