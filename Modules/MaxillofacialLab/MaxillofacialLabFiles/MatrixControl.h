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
#ifndef __MatrixControl_h
#define __MatrixControl_h

#include "itkPoint.h"
#include "itkCovariantVector.h"
#include "vnl/vnl_matrix_fixed.txx" // Get the templates
#include "vnl/vnl_transpose.h"
#include "vnl/algo/vnl_matrix_inverse.h"
#include "vnl/vnl_matrix.h"
#include "vnl/algo/vnl_determinant.h"

typedef vnl_matrix<double> InternalMatrixType;
//template< typename T, unsigned int NRows = 3, unsigned int NColumns = 3 >
class MatrixControl
{
public:
	/** @brief default constructor*/
	MatrixControl(int NRows, int NColumns);
	  /** @brief default destructor*/
	virtual ~MatrixControl();
  /** Standard class typedefs. */
  typedef MatrixControl Self;

  /** Matrix addition.  */

  void Assign(InternalMatrixType matrix);
  void MatrixControl::ResultOfMinusOperator(InternalMatrixType matrix1, InternalMatrixType matrix2);

  double GetElement(unsigned int row, unsigned int col);
  double *GetRow(unsigned int i);
  InternalMatrixType GetMatrix();
  vnl_vector<double> GetDistanceVector();
  double GetDistanceVectorElement(unsigned int index);
  void Fill(double X, double Y, double Z);
  void SetElement(unsigned int row, unsigned int col, double value);
  int GetMinima();
  void Squared();
protected:
  int m_RowDimensions, m_ColumnDimensions;

private:
	/** Internal matrix type */
	InternalMatrixType m_Matrix;
  vnl_vector<double> m_VectorSquaredDifference;
};

#endif
