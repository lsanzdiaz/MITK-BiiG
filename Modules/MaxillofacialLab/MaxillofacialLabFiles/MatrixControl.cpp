/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "MatrixControl.h"

//template< typename T, unsigned int NRows = 3, unsigned int NColumns = 3 >
/** @Constructor */
MatrixControl::MatrixControl(int NRows, int NColumns)
{
	m_RowDimensions = NRows;
	m_ColumnDimensions = NColumns;
	m_Matrix = InternalMatrixType(NRows, NColumns);
	m_VectorSquaredDifference = vnl_vector<double>(NRows);
}

/** @Destructor */
MatrixControl::~MatrixControl()
{

}

void MatrixControl::Assign(InternalMatrixType matrix)
{
	this->m_Matrix = matrix;
}

void MatrixControl::ResultOfMinusOperator(InternalMatrixType matrix1, InternalMatrixType matrix2)
{
	this->m_Matrix = matrix1 - matrix2;
}

/** Return an element of the matrix. */
double MatrixControl::GetElement(unsigned int row, unsigned int col)
  {
    return m_Matrix(row, col);
  }

void MatrixControl::SetElement(unsigned int row, unsigned int col, double value)
{
	m_Matrix(row, col) = value;
}

/** Return a row of the matrix. */
double * MatrixControl::GetRow(unsigned int i)
  {
	double* vector = new double[3];
	vector[0] = m_Matrix.get_row(i).get(0);
	vector[1] = m_Matrix.get_row(i).get(1);
	vector[2] = m_Matrix.get_row(i).get(2);
	return vector;
  }

/** Return the matrix. */
InternalMatrixType MatrixControl::GetMatrix()
{
    return m_Matrix;
}

/** Return the squared matrix. */
vnl_vector<double> MatrixControl::GetDistanceVector()
{
	return m_VectorSquaredDifference;
}

double MatrixControl::GetDistanceVectorElement(unsigned int index)
{
	return m_VectorSquaredDifference[index];
}

/** Fill the matrix with a value. */
void MatrixControl::Fill(double X, double Y, double Z)
{
	m_Matrix.set_column(0, X);
	m_Matrix.set_column(1, Y);
	m_Matrix.set_column(2, Z);
}

int MatrixControl::GetMinima()
{
	return m_VectorSquaredDifference.arg_min();
}

void MatrixControl::Squared()
{
	InternalMatrixType m_SquaredMatrix = m_Matrix * m_Matrix.transpose();
	m_VectorSquaredDifference = m_SquaredMatrix.get_diagonal();
}