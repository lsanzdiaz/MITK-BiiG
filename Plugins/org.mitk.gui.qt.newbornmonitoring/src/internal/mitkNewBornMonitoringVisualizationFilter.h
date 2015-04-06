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


#ifndef MITKNewBornMonitoringVISUALIZATIONFILTER_H_HEADER_INCLUDED_
#define MITKNewBornMonitoringVISUALIZATIONFILTER_H_HEADER_INCLUDED_

#include <mitkNavigationDataObjectVisualizationFilter.h>
#include <mitkNavigationData.h>
#include <mitkBaseData.h>

namespace mitk {

  /**
  * \brief Class that reads NavigationData from input and transfers the information to the geometry of the associated BaseData
  *
  * Derived from NavigationDataObjectVisualizationFilter
  *
  * \ingroup IGT
  */
  class NewBornMonitoringVisualizationFilter : public NavigationDataObjectVisualizationFilter
  {
  public:
	  mitkClassMacro(NewBornMonitoringVisualizationFilter, NavigationDataObjectVisualizationFilter);

	  itkFactorylessNewMacro(Self)
		  itkCloneMacro(Self)
		  /**
		  * \brief Set the representation object of the input
		  *
		  * \param data The BaseData to be associated to the index
		  * \param index the index with which data will be associated
		  */
		  void SetAssociatedSurfaceModel(unsigned int index, BaseData* data);
		  void SetSurfaceTransform(int index, mitk::AffineTransform3D::Pointer surface_transform);

		  mitk::AffineTransform3D::Pointer GetSurfaceTransform(int index);

  protected:
    /**
    * \brief Constructor
    **/
	  NewBornMonitoringVisualizationFilter();

    /**
    * \brief Destructor
    **/
	  ~NewBornMonitoringVisualizationFilter();


	  

	  /**
	  * \brief Get the representation object associated with the index idx
	  *
	  * \param idx the corresponding input number with which the BaseData is associated
	  * \return Returns the desired BaseData if it exists for the given input; Returns NULL
	  *         if no BaseData was found.
	  */
	  const BaseData* GetAssociatedSurfaceModel(unsigned int idx);

	  /*
	  * \brief Transfer the information from the input to the associated BaseData. This method changes with respect to the method of the base class.
	  */
	  virtual void GenerateData();


	  virtual void SetRotationMode(RotationMode r);

	  

	  OffsetPointerMap m_SurfaceTransformList;

  private:

	  /**
	  * \brief An array of the BaseData which represent the surface model associated with the tool.
	  */
	  RepresentationPointerMap m_AssociatedSurfaceModelList;
	  

	  RotationMode m_RotationMode; ///< defines the rotation mode Standard or Transposed, Standard is default

	 
  };
} // namespace mitk
#endif /* MITKNewBornMonitoringVISUALIZATIONFILTER_H_HEADER_INCLUDED_ */

