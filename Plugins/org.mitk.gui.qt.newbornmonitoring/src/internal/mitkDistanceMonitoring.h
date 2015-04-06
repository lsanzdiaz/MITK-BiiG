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

#ifndef MITKDISTANCEMONITORING_H
#define MITKDISTANCEMONITORING_H

//VTK

#include "vtkCellLocator.h"
#include <vtkSmartPointer.h>
#include <vtkDistancePolyDataFilter.h>
#include <vtkProperty.h>
#include <vtkPolyData.h>


struct Distance_Data
{
	double* closest_point;
	double distance;
};

namespace mitk {

	class mitkDistanceMonitoring
	{
	public:

		mitkDistanceMonitoring();
		~mitkDistanceMonitoring();

		void InitiateDistanceControl(vtkPolyData *pdata);

		Distance_Data* RealTimeControlOfDistance(double *ToolNavigationPosition);

	protected:

		vtkSmartPointer<vtkDistancePolyDataFilter> m_distanceFilter;
		//mitk::PointLocator::Pointer m_Locator;
		vtkSmartPointer<vtkCellLocator> m_CellLocator;
	};
}

#endif