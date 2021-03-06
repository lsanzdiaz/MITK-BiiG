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


#ifndef MITKDISPLAYPOINTSETINTERACTOR_H_HEADER_INCLUDED_C11202FF
#define MITKDISPLAYPOINTSETINTERACTOR_H_HEADER_INCLUDED_C11202FF

#include "mitkCommon.h"
#include "MitkExtExports.h"
#include "mitkNumericTypes.h"
#include <mitkInteractor.h>
#include "mitkPointSetInteractor.h"
#include <mitkBaseRenderer.h>

namespace mitk
{
  class DataNode;

  /**
   * \brief Interaction with a set of points.
   *
   * Points can be added, removed and moved.
   * \ingroup Interaction
   */

  /**
  * \deprecatedSince{2014_03} mitk::DisplayPointSetInteractor is deprecated. Needs to be updated to the new interaction-framework.
  * Refer to \see DataInteractionPage for general information about the concept of the new implementation.
  */

  class DEPRECATED() MitkExt_EXPORT DisplayPointSetInteractor : public PointSetInteractor
  {
  public:
    mitkClassMacro(DisplayPointSetInteractor, PointSetInteractor);
    mitkNewMacro3Param(Self, const char*, DataNode*, int);
    mitkNewMacro2Param(Self, const char*, DataNode*);

    Point2D GetLastDisplayCoordinates();

    BaseRenderer* GetLastRenderer();


  protected:
    /**
     * \brief Constructor with Param n for limited Set of Points
     *
     * if no n is set, then the number of points is unlimited*
     */
    DisplayPointSetInteractor(const char * type,
      DataNode* dataNode, int n = -1);

    /**
     * \brief Default Destructor
     **/
    virtual ~DisplayPointSetInteractor();

    virtual bool ExecuteAction( Action* action,
      mitk::StateEvent const* stateEvent );

    /** \brief last display-coordinates of the point
     *
     */
    Point2D m_LastDisplayCoordinates;


    mitk::BaseRenderer* m_LastRenderer;


  };
}
#endif /* MITKDisplayPointSetInteractor_H_HEADER_INCLUDED_C11202FF */
