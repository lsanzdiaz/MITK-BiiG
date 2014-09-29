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

#include "MITKMaxillofacialMeshLab.h"
#include <mitkExceptionMacro.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkIsotropicDiscreteRemeshing.h>
#include <vtkMultiThreader.h>
#include <vtkPolyData.h>
#include <vtkPolyDataNormals.h>
#include <vtkSmartPointer.h>
#include <vtkSurface.h>

#include "mitkGeometryData.h"
#include "mitkPlaneGeometry.h"


struct ClustersQuadrics
{
  explicit ClustersQuadrics(int size)
    : Elements(new double*[size]),
      Size(size)
  {
    for (int i = 0; i < size; ++i)
    {
      Elements[i] = new double[9];

      for (int j = 0; j < 9; ++j)
        Elements[i][j] = 0.0;
    }
  }

  ~ClustersQuadrics()
  {
    for (int i = 0; i < Size; ++i)
      delete[] Elements[i];

    delete Elements;
  }

  double** Elements;
  int Size;

private:
  ClustersQuadrics(const ClustersQuadrics&);
  ClustersQuadrics& operator=(const ClustersQuadrics&);
};


static void ValidateSurface(mitk::Surface::ConstPointer surface, unsigned int t)
{
	if (surface.IsNull())
		mitkThrow() << "Input surface is NULL!";

	if (t >= surface->GetSizeOfPolyDataSeries())
		mitkThrow() << "Input surface doesn't have data at time step " << t << "!";

	vtkPolyData* polyData = const_cast<mitk::Surface*>(surface.GetPointer())->GetVtkPolyData(t);

	if (polyData == NULL)
		mitkThrow() << "PolyData of input surface at time step " << t << " is NULL!";

	if (polyData->GetNumberOfPolys() == 0)
		mitkThrow() << "Input surface has no polygons at time step " << t << "!";
}

mitk::ACVD::RemeshFilter::RemeshFilter()
	: m_TimeStep(0),
	m_NumVertices(0),
	m_Gradation(1.0),
	m_Subsampling(10),
	m_EdgeSplitting(0.0),
	m_OptimizationLevel(1),
	m_ForceManifold(false),
	m_BoundaryFixing(false)
{
	Surface::Pointer output = Surface::New();
	this->SetNthOutput(0, output);
}

mitk::ACVD::RemeshFilter::~RemeshFilter()
{
}

mitk::Surface::Pointer mitk::ACVD::Remesh(mitk::Surface::ConstPointer surface, unsigned int t, int numVertices, double gradation, int subsampling, double edgeSplitting, int optimizationLevel, bool forceManifold, bool boundaryFixing)
{
  ValidateSurface(surface, t);

  MITK_INFO << "Start remeshing...";

  vtkSmartPointer<vtkPolyData> surfacePolyData = vtkSmartPointer<vtkPolyData>::New();
  surfacePolyData->DeepCopy(const_cast<Surface*>(surface.GetPointer())->GetVtkPolyData(t));

  vtkSmartPointer<vtkSurface> mesh = vtkSmartPointer<vtkSurface>::New();

  mesh->CreateFromPolyData(surfacePolyData);
  mesh->GetCellData()->Initialize();
  mesh->GetPointData()->Initialize();

  mesh->DisplayMeshProperties();

  if (numVertices == 0)
    numVertices = surfacePolyData->GetNumberOfPoints();

  if (edgeSplitting != 0.0)
    mesh->SplitLongEdges(edgeSplitting);

  vtkSmartPointer<vtkIsotropicDiscreteRemeshing> remesher = vtkSmartPointer<vtkIsotropicDiscreteRemeshing>::New();

  remesher->GetMetric()->SetGradation(gradation);
  remesher->SetBoundaryFixing(boundaryFixing);
  remesher->SetConsoleOutput(1);
  remesher->SetForceManifold(forceManifold);
  remesher->SetInput(mesh);
  remesher->SetNumberOfClusters(numVertices);
  //remesher->SetNumberOfThreads(vtkMultiThreader::GetGlobalDefaultNumberOfThreads());
  remesher->SetSubsamplingThreshold(subsampling);

  remesher->Remesh();

  // Optimization: Minimize distance between input surface and remeshed surface
  if (optimizationLevel != 0)
  {
    ClustersQuadrics clustersQuadrics(numVertices);

    vtkSmartPointer<vtkIdList> faceList = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIntArray> clustering = remesher->GetClustering();
    vtkSmartPointer<vtkSurface> remesherInput = remesher->GetInput();
    int clusteringType = remesher->GetClusteringType();
    int numItems = remesher->GetNumberOfItems();
    int numMisclassifiedItems = 0;

    for (int i = 0; i < numItems; ++i)
    {
      int cluster = clustering->GetValue(i);

      if (cluster >= 0 && cluster < numVertices)
      {
        if (clusteringType != 0)
        {
          remesherInput->GetVertexNeighbourFaces(i, faceList);
          int numIds = static_cast<int>(faceList->GetNumberOfIds());

          for (int j = 0; j < numIds; ++j)
            vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], remesherInput, faceList->GetId(j), false);
		}
        else
        {
          vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], remesherInput, i, false);
		
		}
      }
      else
      {
        ++numMisclassifiedItems;
      }
    }

    if (numMisclassifiedItems != 0)
      std::cout << numMisclassifiedItems << " items with wrong cluster association" << std::endl;

    vtkSmartPointer<vtkSurface> remesherOutput = remesher->GetOutput();
    double point[3];

    for (int i = 0; i < numVertices; ++i)
    {
      remesherOutput->GetPoint(i, point);
      vtkQuadricTools::ComputeRepresentativePoint(clustersQuadrics.Elements[i], point, optimizationLevel);
      remesherOutput->SetPointCoordinates(i, point);
    }

    std::cout << "After quadrics post-processing:" << std::endl;
    remesherOutput->DisplayMeshProperties();
  }

  vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();

  normals->SetInputData(remesher->GetOutput());
  normals->AutoOrientNormalsOn();
  normals->ComputeCellNormalsOn();
  normals->ComputePointNormalsOn();
  normals->ConsistencyOff();
  normals->FlipNormalsOff();
  normals->NonManifoldTraversalOff();
  normals->SplittingOff();

  normals->Update();

  Surface::Pointer remeshedSurface = Surface::New();
  remeshedSurface->SetVtkPolyData(normals->GetOutput());

  MITK_INFO << "Finished remeshing";

  return remeshedSurface;
}

void mitk::ACVD::RemeshFilter::InsertVtkPolydata(vtkSmartPointer<vtkSurface> remesherOutput, vtkSmartPointer<vtkPolyData> MarginSurfacePolyData)
{
	std::stringstream stream;
	vtkIdType nPoly = 0, nTri = 0, nQuad = 0;
	vtkIdType NumberOfEmptySlots = 0;
	
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> vertex = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
	vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();;

	MarginSurfacePolyData->BuildCells();
	MarginSurfacePolyData->BuildLinks();
	
	MarginSurfacePolyData->SetPoints(remesherOutput->GetPoints());
	MarginSurfacePolyData->SetVerts(remesherOutput->GetVerts());
	MarginSurfacePolyData->SetPolys(remesherOutput->GetPolys());
	MarginSurfacePolyData->SetLines(remesherOutput->GetLines());
	MarginSurfacePolyData->SetStrips(remesherOutput->GetStrips());
	
	/*
	for (vtkIdType i = 0; i < remesherOutput->GetNumberOfCells(); i++) {

		if (remesherOutput->IsFaceActive(i))
		{
			remesherOutput->GetCellPoints(i, NV, Vertices);
			polys->InsertNextCell(NV, Vertices);
			switch (NV)
			{
			case 3:
				nTri++;
				//remesherOutput->GetFaceVertices(i, v1, v2, v3);
				//remesherOutput->GetPointCoordinates(v1, P1);
				//remesherOutput->GetPointCoordinates(v2, P2);
				//remesherOutput->GetPointCoordinates(v3, P3);
				//Add a point to the polydata and save its index, 
				//which we will use to create the vertex on that point.
				//create a vertex cell on the point that was just added.
				//vertices->InsertNextCell(NV, pid);
				//MarginSurfacePolyData->ReplaceCell(i, NV, Vertices);
				//MarginSurfacePolyData->Allocate();
				//MarginMesh->AddFace(v1, v2, v3);
				//MarginMesh->AddPolygon(3, Vertices);
				//vtkTriangle::ComputeNormal(P1, P2, P3, N);
				//vtkTriangle::TriangleCenter(P1, P2, P3, C);		
				//triangle->SetFaces(Vertices);			
				break;
			case 4:
				nQuad++;
				break;
			default:
				nPoly++;
				break;
			}
		}
		else
		{
			NumberOfEmptySlots++;
		}
	}
	*/
	/*
	///////// Set Point Normals ///////////

	//add the points and vertices to a polydata
	
	MarginSurfacePolyData->BuildCells();
	MarginSurfacePolyData->BuildLinks();
	//set vertex (cell) normals
	vtkSmartPointer<vtkDoubleArray> normalsArray = vtkSmartPointer<vtkDoubleArray>::New();
	normalsArray->SetNumberOfComponents(3); //3d normals (ie x,y,z)
	normalsArray->SetNumberOfTuples(MarginSurfacePolyData->GetNumberOfPoints());

	//construct the normal vectors
	double cN1[3] = { 1.0, 0.0, 0.0 };
	double cN2[3] = { 0.0, 1.0, 0.0 };
	double cN3[3] = { 0.0, 0.0, 1.0 };

	//add the data to the normals array
	normalsArray->SetTuple(0, cN1);
	normalsArray->SetTuple(1, cN2);
	normalsArray->SetTuple(2, cN3);

	//add the normals to the cells in the polydata
	MarginSurfacePolyData->GetCellData()->SetNormals(normalsArray);

	///////// Get cell normals ///////////
	vtkSmartPointer<vtkDoubleArray> cellNormalsRetrieved =
		vtkDoubleArray::SafeDownCast(MarginSurfacePolyData->GetCellData()->GetNormals());
	if (cellNormalsRetrieved)
	{
		cout << "There are " << cellNormalsRetrieved->GetNumberOfTuples() << " cell normals." << endl;

		for (vtkIdType i = 0; i < cellNormalsRetrieved->GetNumberOfTuples(); i++)
		{
			double cN[3];
			cellNormalsRetrieved->GetTuple(i, cN);
			cout << "Cell normal " << i << ": " << cN[0] << " " << cN[1] << " " << cN[2] << endl;
		}
	}
	else
	{
		cout << "No cell normals." << endl;
	}
	*/
}


unsigned int mitk::ACVD::RemeshFilter::CreateControlDistanceMatrix(mitk::Point3D P, vtkSmartPointer<vtkSurface> surface)
{
	int minima;
	
	SetSurfacePointsMatrix(surface);
	m_Surface = surface;

	SetToolPositionMatrix(P, surface->GetNumberOfPoints());
	
	m_DifferenceMatrix = new MatrixControl(surface->GetNumberOfPoints(), 3);
	
	try
	{
		m_DifferenceMatrix->ResultOfMinusOperator(m_PositionMatrix->GetMatrix(), m_SurfaceMatrix->GetMatrix());
		std::cout << "Difference matrix:" << endl;
		std::cout << m_DifferenceMatrix->GetMatrix() << endl;

		m_DifferenceMatrix->Squared();
		minima = m_DifferenceMatrix->GetMinima();
		std::cout << "Difference matrix:" << endl;
		std::cout << m_DifferenceMatrix->GetMatrix() << endl;
	}
	catch (...)
	{
		std::string warningmessage = "Surface has not being specified or does not exist. Distance control matrix will not be calculated.";
	}

	return minima;
	
}

double *mitk::ACVD::RemeshFilter::MonitorDistance(mitk::Point3D P)
{

	std::cout << "Surface Points Matrix" << endl;
	std::cout << m_SurfaceMatrix->GetMatrix() << endl;

	unsigned int minima;
	SetToolPositionMatrix(P, m_Surface->GetNumberOfPoints());

	m_DifferenceMatrix = new MatrixControl(m_Surface->GetNumberOfPoints(), 3);
	
	try
	{
		m_DifferenceMatrix->ResultOfMinusOperator(m_PositionMatrix->GetMatrix(), m_SurfaceMatrix->GetMatrix());
		m_DifferenceMatrix->Squared();
		minima = m_DifferenceMatrix->GetMinima();
		std::cout << "Distance Vector:" << endl;
		std::cout << m_DifferenceMatrix->GetDistanceVector() << endl;

	}
	catch (...)
	{
		std::string warningmessage = "Surface has not being specified or does not exist. Distance control matrix will not be calculated.";
	}

	double distance = m_DifferenceMatrix->GetDistanceVectorElement(minima);
	double *distance_data = new double[2];
	distance_data[0] = minima;
	distance_data[1] = sqrt(distance);
	return distance_data;
}

MatrixControl *mitk::ACVD::RemeshFilter::GetToolPositionMatrix()
{
	return m_PositionMatrix;
}
void mitk::ACVD::RemeshFilter::SetToolPositionMatrix(mitk::Point3D pr, double n)
{
	std::cout << "Tool position:" << endl;
	std::cout << pr << endl;
	m_PositionMatrix = new MatrixControl(n, 3);
		
	m_PositionMatrix->Fill(pr[0], pr[1], pr[2]);
		
	std::cout << "Tool Position Matrix: " << endl;
	std::cout << m_PositionMatrix->GetMatrix() << endl;
	
}

void mitk::ACVD::RemeshFilter::SetSurfacePointsMatrix(vtkSmartPointer<vtkSurface> surface)
{
	
	try
	{
		int n = surface->GetNumberOfPoints();
		std::cout << "NumberOfPoints" << std::endl;
		std::cout << n << std::endl;

		Vector Sp;
		
		m_SurfaceMatrix = new MatrixControl(surface->GetNumberOfPoints(),3);
		
		for (vtkIdType i = 0; i < surface->GetNumberOfPoints(); i++)
		{
			surface->GetPoint(i, Sp);
			m_SurfaceMatrix->SetElement(i, 0, Sp[0]);
			m_SurfaceMatrix->SetElement(i, 1, Sp[1]);
			m_SurfaceMatrix->SetElement(i, 2, Sp[2]);
			std::cout << i << endl;
			std::cout << Sp[0] << endl;
			std::cout << Sp[1] << endl;
			std::cout << Sp[2] << endl;
			std::cout << m_SurfaceMatrix->GetElement(i,0) << endl;
			std::cout << m_SurfaceMatrix->GetElement(i,1) << endl;
			std::cout << m_SurfaceMatrix->GetElement(i,2) << endl;
		}
		std::cout << "m_SurfaceMatrix has been edited" << endl;
		std::cout << m_SurfaceMatrix->GetMatrix() << endl;
		return;
	}
	catch (...)
	{
		std::string warningmessage = "Surface has not being specified or does not exist. Distance control matrix will not be calculated.";
	}
	
	return;
	
}

mitk::Surface::Pointer mitk::ACVD::RemeshFilter::GenerateMesh(mitk::Surface::ConstPointer surface, unsigned int t, int numVertices, double gradation, int subsampling, double edgeSplitting, int optimizationLevel, bool forceManifold, bool boundaryFixing)
{
	/*******************Generate First Remesher, which follows the surface************************/
	ValidateSurface(surface, t);

	MITK_INFO << "Start remeshing...";

	vtkSmartPointer<vtkPolyData> surfacePolyData = vtkSmartPointer<vtkPolyData>::New();
	surfacePolyData->DeepCopy(const_cast<Surface*>(surface.GetPointer())->GetVtkPolyData(t));

	vtkSmartPointer<vtkSurface> mesh = vtkSmartPointer<vtkSurface>::New();

	mesh->CreateFromPolyData(surfacePolyData);

	mesh->GetCellData()->Initialize();
	mesh->GetPointData()->Initialize();

	std::cout << "First mesh:" << std::endl;
	mesh->DisplayMeshProperties();

	if (numVertices == 0)
		numVertices = surfacePolyData->GetNumberOfPoints();

	if (edgeSplitting != 0.0)
		mesh->SplitLongEdges(edgeSplitting);

	vtkSmartPointer<vtkIsotropicDiscreteRemeshing> remesher = vtkSmartPointer<vtkIsotropicDiscreteRemeshing>::New();

	remesher->GetMetric()->SetGradation(gradation);
	remesher->SetBoundaryFixing(boundaryFixing);
	remesher->SetConsoleOutput(1);
	remesher->SetForceManifold(forceManifold);
	remesher->SetInput(mesh);
	remesher->SetNumberOfClusters(numVertices);
	//remesher->SetNumberOfThreads(vtkMultiThreader::GetGlobalDefaultNumberOfThreads());
	remesher->SetSubsamplingThreshold(subsampling);

	remesher->Remesh();

	vtkSmartPointer<vtkSurface> remesherOutput = remesher->GetOutput();

	// Optimization: Minimize distance between input surface and remeshed surface
	if (optimizationLevel != 0)
	{
		ClustersQuadrics clustersQuadrics(numVertices);

		vtkSmartPointer<vtkIdList> faceList = vtkSmartPointer<vtkIdList>::New();
		vtkSmartPointer<vtkIntArray> clustering = remesher->GetClustering();
		vtkSmartPointer<vtkSurface> remesherInput = remesher->GetInput();
		int clusteringType = remesher->GetClusteringType();
		int numItems = remesher->GetNumberOfItems();
		int numMisclassifiedItems = 0;

		for (int i = 0; i < numItems; ++i)
		{
			int cluster = clustering->GetValue(i);

			if (cluster >= 0 && cluster < numVertices)
			{
				if (clusteringType != 0)
				{
					remesherInput->GetVertexNeighbourFaces(i, faceList);
					int numIds = static_cast<int>(faceList->GetNumberOfIds());

					for (int j = 0; j < numIds; ++j)
						vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], remesherInput, faceList->GetId(j), false);
				}
				else
				{
					vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], remesherInput, i, false);

				}
			}
			else
			{
				++numMisclassifiedItems;
			}
		}

		if (numMisclassifiedItems != 0)
			std::cout << numMisclassifiedItems << " items with wrong cluster association" << std::endl;

		vtkSmartPointer<vtkSurface> remesherOutput = remesher->GetOutput();
		double point[3];

		for (int i = 0; i < numVertices; ++i)
		{
			remesherOutput->GetPoint(i, point);
			vtkQuadricTools::ComputeRepresentativePoint(clustersQuadrics.Elements[i], point, optimizationLevel);
			remesherOutput->SetPointCoordinates(i, point);
		}
		std::cout << "First Remesher Output :" << std::endl;
		remesherOutput->DisplayMeshProperties();

	}

	vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();

	normals->SetInputData(remesher->GetOutput());
	normals->AutoOrientNormalsOn();
	normals->ComputeCellNormalsOn();
	normals->ComputePointNormalsOn();
	normals->ConsistencyOff();
	normals->FlipNormalsOff();
	normals->NonManifoldTraversalOff();
	normals->SplittingOff();

	normals->Update();
	
	m_Surface = remesher->GetOutput();

	Surface::Pointer MeshSurface = Surface::New();
		
	MeshSurface->SetVtkPolyData(normals->GetOutput());


	MITK_INFO << "Finished remeshing";

	return MeshSurface;
}


mitk::Surface::Pointer* mitk::ACVD::RemeshFilter::GenerateMeshWithMargin(mitk::Surface::ConstPointer surface, unsigned int t, int numVertices, double gradation, int subsampling, double edgeSplitting, int optimizationLevel, bool forceManifold, bool boundaryFixing)
{
	mitk::Surface::Pointer *surfacegroup = new mitk::Surface::Pointer[2];

	/*******************Generate First Remesher, which follows the surface************************/
	ValidateSurface(surface, t);

	MITK_INFO << "Start remeshing...";

	vtkSmartPointer<vtkPolyData> surfacePolyData = vtkSmartPointer<vtkPolyData>::New();
	surfacePolyData->DeepCopy(const_cast<Surface*>(surface.GetPointer())->GetVtkPolyData(t));

	vtkSmartPointer<vtkSurface> mesh = vtkSmartPointer<vtkSurface>::New();

	mesh->CreateFromPolyData(surfacePolyData);
	
	mesh->GetCellData()->Initialize();
	mesh->GetPointData()->Initialize();

	std::cout << "First mesh:" << std::endl;
	mesh->DisplayMeshProperties();

	if (numVertices == 0)
		numVertices = surfacePolyData->GetNumberOfPoints();

	if (edgeSplitting != 0.0)
		mesh->SplitLongEdges(edgeSplitting);

	vtkSmartPointer<vtkIsotropicDiscreteRemeshing> remesher = vtkSmartPointer<vtkIsotropicDiscreteRemeshing>::New();

	remesher->GetMetric()->SetGradation(gradation);
	remesher->SetBoundaryFixing(boundaryFixing);
	remesher->SetConsoleOutput(1);
	remesher->SetForceManifold(forceManifold);
	remesher->SetInput(mesh);
	remesher->SetNumberOfClusters(numVertices);
	//remesher->SetNumberOfThreads(vtkMultiThreader::GetGlobalDefaultNumberOfThreads());
	remesher->SetSubsamplingThreshold(subsampling);

	remesher->Remesh();
	
	vtkSmartPointer<vtkSurface> remesherOutput = remesher->GetOutput();
	
	// Optimization: Minimize distance between input surface and remeshed surface
	if (optimizationLevel != 0)
	{
		ClustersQuadrics clustersQuadrics(numVertices);

		vtkSmartPointer<vtkIdList> faceList = vtkSmartPointer<vtkIdList>::New();
		vtkSmartPointer<vtkIntArray> clustering = remesher->GetClustering();
		vtkSmartPointer<vtkSurface> remesherInput = remesher->GetInput();
		int clusteringType = remesher->GetClusteringType();
		int numItems = remesher->GetNumberOfItems();
		int numMisclassifiedItems = 0;

		for (int i = 0; i < numItems; ++i)
		{
			int cluster = clustering->GetValue(i);

			if (cluster >= 0 && cluster < numVertices)
			{
				if (clusteringType != 0)
				{
					remesherInput->GetVertexNeighbourFaces(i, faceList);
					int numIds = static_cast<int>(faceList->GetNumberOfIds());

					for (int j = 0; j < numIds; ++j)
						vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], remesherInput, faceList->GetId(j), false);
				}
				else
				{
					vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], remesherInput, i, false);

				}
			}
			else
			{
				++numMisclassifiedItems;
			}
		}

		if (numMisclassifiedItems != 0)
			std::cout << numMisclassifiedItems << " items with wrong cluster association" << std::endl;

		vtkSmartPointer<vtkSurface> remesherOutput = remesher->GetOutput();
		double point[3];

		for (int i = 0; i < numVertices; ++i)
		{
			remesherOutput->GetPoint(i, point);
			vtkQuadricTools::ComputeRepresentativePoint(clustersQuadrics.Elements[i], point, optimizationLevel);
			remesherOutput->SetPointCoordinates(i, point);
		}
		std::cout << "First Remesher Output :" << std::endl;
		remesherOutput->DisplayMeshProperties();

	}

	vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();

	normals->SetInputData(remesher->GetOutput());
	normals->AutoOrientNormalsOn();
	normals->ComputeCellNormalsOn();
	normals->ComputePointNormalsOn();
	normals->ConsistencyOff();
	normals->FlipNormalsOff();
	normals->NonManifoldTraversalOff();
	normals->SplittingOff();

	normals->Update();

	surfacegroup[0] = Surface::New();
	surfacegroup[0]->SetVtkPolyData(normals->GetOutput());
	
	/*********************************************************************************************/
	/*******************Generate MarginRemesher, which will include margin************************/
	
	vtkSmartPointer<vtkPolyData> MarginSurfacePolyData = vtkSmartPointer<vtkPolyData>::New();

	/*Insert points, cells, verts from mesh output to MarginSurfacePolydata*/
	InsertVtkPolydata( remesherOutput, MarginSurfacePolyData);

	/*Create MarginMesh based on MarginSurfacePolyData*/
	vtkSmartPointer<vtkSurface> MarginMesh = vtkSmartPointer<vtkSurface>::New();
	MarginMesh->CreateFromPolyData(MarginSurfacePolyData);
	MarginMesh->GetCellData()->Initialize();
	MarginMesh->GetPointData()->Initialize();

	std::cout << "Margin mesh:" << std::endl;
	MarginMesh->DisplayMeshProperties();

	if (numVertices == 0)
		numVertices = MarginSurfacePolyData->GetNumberOfPoints();

	if (edgeSplitting != 0.0)
		MarginMesh->SplitLongEdges(edgeSplitting);
	
	/*Create MarginRemesher to perform the mesh action*/
	vtkSmartPointer<vtkIsotropicDiscreteRemeshing> MarginRemesher = vtkSmartPointer<vtkIsotropicDiscreteRemeshing>::New();

	MarginRemesher->GetMetric()->SetGradation(gradation);
	MarginRemesher->SetBoundaryFixing(boundaryFixing);
	MarginRemesher->SetConsoleOutput(1);
	MarginRemesher->SetForceManifold(forceManifold);
	MarginRemesher->SetInput(MarginMesh);
	MarginRemesher->SetNumberOfClusters(numVertices);
	MarginRemesher->SetSubsamplingThreshold(subsampling);
	MarginRemesher->Remesh();

	// Optimization: Minimize distance between input surface and remeshed surface
	if (optimizationLevel != 0)
	{
		ClustersQuadrics clustersQuadrics(numVertices);

		vtkSmartPointer<vtkIdList> faceList = vtkSmartPointer<vtkIdList>::New();
		vtkSmartPointer<vtkIntArray> clustering = MarginRemesher->GetClustering();
		vtkSmartPointer<vtkSurface> MarginRemesherInput = MarginRemesher->GetInput();
		int clusteringType = MarginRemesher->GetClusteringType();
		int numItems = MarginRemesher->GetNumberOfItems();
		int numMisclassifiedItems = 0;

		for (int i = 0; i < numItems; ++i)
		{
			int cluster = clustering->GetValue(i);

			if (cluster >= 0 && cluster < numVertices)
			{
				if (clusteringType != 0)
				{
					MarginRemesherInput->GetVertexNeighbourFaces(i, faceList);
					int numIds = static_cast<int>(faceList->GetNumberOfIds());

					for (int j = 0; j < numIds; ++j)
						vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], MarginRemesherInput, faceList->GetId(j), false);
				}
				else
				{
					vtkQuadricTools::AddTriangleQuadric(clustersQuadrics.Elements[cluster], MarginRemesherInput, i, false);

				}
			}
			else
			{
				++numMisclassifiedItems;
			}
		}

		if (numMisclassifiedItems != 0)
			std::cout << numMisclassifiedItems << " items with wrong cluster association" << std::endl;

		vtkSmartPointer<vtkSurface> MarginRemesherOutput = MarginRemesher->GetOutput();
		double point[3];

		for (int i = 0; i < numVertices; ++i)
		{
			MarginRemesherOutput->GetPoint(i, point);
			vtkQuadricTools::ComputeRepresentativePoint(clustersQuadrics.Elements[i], point, optimizationLevel);
			MarginRemesherOutput->SetPointCoordinates(i, point);
		}
		std::cout << "Margin Remesher Output :" << std::endl;
		MarginRemesherOutput->DisplayMeshProperties();

	}

	/*Create MarginRemesher normals*/
	vtkSmartPointer<vtkPolyDataNormals> MarginNormals = vtkSmartPointer<vtkPolyDataNormals>::New();

	MarginNormals->SetInputData(MarginRemesher->GetOutput());
	MarginNormals->AutoOrientNormalsOn();
	MarginNormals->ComputeCellNormalsOn();
	MarginNormals->ComputePointNormalsOn();
	MarginNormals->ConsistencyOff();
	MarginNormals->FlipNormalsOff();
	MarginNormals->NonManifoldTraversalOff();
	MarginNormals->SplittingOff();
	MarginNormals->Update();

	m_Surface = MarginRemesher->GetOutput();

	surfacegroup[1] = Surface::New();
	surfacegroup[1]->SetVtkPolyData(normals->GetOutput());

	MITK_INFO << "Finished remeshing";

	return surfacegroup;
}

vtkSmartPointer<vtkSurface> mitk::ACVD::RemeshFilter::GetMeshSurface()
{
	return this->m_Surface;
}

void mitk::ACVD::RemeshFilter::GenerateData()
{
  Surface::Pointer output = Remesh(this->GetInput(), m_TimeStep, m_NumVertices, m_Gradation, m_Subsampling, m_EdgeSplitting, m_OptimizationLevel, m_ForceManifold, m_BoundaryFixing);
  this->SetNthOutput(0, output);
}
