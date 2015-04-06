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

#include "mitkDistanceMonitoring.h"

// MITK

#include <mitkNodePredicateDataType.h>
#include <mitkVtkRepresentationProperty.h>
#include <mitkMessage.h>

// VTK
#include <vtkPolyData.h>
#include <vtkPolyDataReader.h>
#include <vtkGenericCell.h>



namespace mitk {

	mitkDistanceMonitoring::mitkDistanceMonitoring()
	{
		m_distanceFilter = vtkSmartPointer<vtkDistancePolyDataFilter>::New();
	}

	mitkDistanceMonitoring::~mitkDistanceMonitoring()
	{
	}


	void mitkDistanceMonitoring::InitiateDistanceControl(vtkPolyData *pdata)
	{
		m_CellLocator = vtkSmartPointer<vtkCellLocator>::New();
		m_CellLocator->SetDataSet(pdata);
		m_CellLocator->BuildLocator();

		return;
	}

	Distance_Data* mitkDistanceMonitoring::RealTimeControlOfDistance(double *ToolNavigationPosition)
	{
		Distance_Data *distance_data = new Distance_Data();

		double* closestCellPoint = new double[3];//the coordinates of the closest point will be returned here
		double closestCellPointDistance2; //the squared distance to the closest point will be returned here
		vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
		vtkGenericCell* cell = vtkGenericCell::New();
		int subId; //this is rarely used (in triangle strips only, I believe)

		m_CellLocator->FindClosestPoint(ToolNavigationPosition, closestCellPoint, cell, cellId, subId, closestCellPointDistance2);

		distance_data->closest_point = closestCellPoint;
		distance_data->distance = sqrt(closestCellPointDistance2);

		cell->Delete();

		std::cout << "2a->distance_data->closest_point in cell: " << endl;
		std::cout << distance_data->closest_point[0] << "; " << distance_data->closest_point[1] << "; " << distance_data->closest_point[2] << endl;
		std::cout << "2b->Minimum distance: " << endl;
		std::cout << distance_data->distance << endl;
		std::cout << endl;

		return distance_data;
	}
}