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

#if !defined(MITKIGTMAXILLOFACIALLAB_H__INCLUDED)
#define MITKIGTMAXILLOFACIALLAB_H__INCLUDED

#include <mitkNavigationDataToPointSetFilter.h>
#include <mitkNavigationDataLandmarkTransformFilter.h>
#include <mitkNavigationDataReferenceTransformFilter.h>
#include <mitkNavigationDataObjectVisualizationFilter.h>
#include <mitkNavigationDataToPointSetFilter.h>
#include <mitkTrackingDeviceSource.h>
#include <mitkSurface.h>
#include <mitkCameraVisualization.h>

#include <QToolBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QTimer>

#include <vtkLandmarkTransform.h>
#include <vtkSmartPointer.h>

#include <MitkIGTMaxillofacialLabExports.h>

/*!
\brief MITKIGTMaxillofacialTrackingLab
*/
class MitkIGTMaxillofacialLab_EXPORT MITKIGTMaxillofacialTrackingLab
{
// this is needed for all Qt objects that should have a Qt meta-object
// (everything that derives from QObject and wants to have signal/slots)
public:

/**
\brief default constructor
*/
MITKIGTMaxillofacialTrackingLab();

/**
\brief default destructor
*/
virtual ~MITKIGTMaxillofacialTrackingLab();

/** This timer updates the IGT pipeline, when necessary:
*   1: if permanent registration is activated, then the permanent
*      registration filter has to be updated
*   2: if the camera view is on it also must be updated
*   3: point set recording is based on another filter which needs to be
*      updated when activated
*/
void UpdateTimer();

double FRE;
mitk::AffineTransform3D::Pointer RegistrationTransform;

//############## Initial Registration Step ##############

/**
\brief This method calls the initial fiducial registration.
*/
void CalculateRegistration(mitk::DataNode::Pointer m_ImageFiducialsDataNode, mitk::DataNode::Pointer m_TrackerFiducialsDataNode);

/**
\brief This method adds a new fiducial to the tracker fiducials PointSet.
*/
void AddRegistrationTrackingFiducial(mitk::DataNode::Pointer TrackerFiducialsDataNode);

/**
\brief This method initializes the registration for the FiducialRegistrationWidget.
*/
void InitializeRegistration();

/**
\brief This method returns the registration transform in the VTK format.
*/
vtkSmartPointer<vtkLandmarkTransform> GetVTKTransform();

protected:

//####################### Members for the IGT pipeline ######################################
// The IGT pipeline is basically initialized in the method OnSetupNavigation(). Further initialization
// is done in the methods OnPermanentRegistration(), OnPointSetRecording() and OnVirtualCamera().
// The pipline is updated in the method UpdateTimer(). When the complete pipeline is active, it is
// structured as follows:
//           -> m_PermanentRegistrationFilter
//          /
// m_Source -> m_Visualizer
//          \
//           -> m_VirtualView
mitk::TrackingDeviceSource::Pointer m_Source;                                         ///< source that connects to the tracking device
mitk::NavigationDataObjectVisualizationFilter::Pointer m_PermanentRegistrationFilter; ///< this filter transforms from tracking coordinates into mitk world coordinates if needed it is interconnected before the FiducialEegistrationFilter
mitk::NavigationDataObjectVisualizationFilter::Pointer m_Visualizer;                  ///< visualization filter
mitk::CameraVisualization::Pointer m_VirtualView;                                     ///< filter to update the vtk camera according to the reference navigation data
//in addition to the pipeline objects, pointers to the navigation data objects are stored for fast access:
mitk::NavigationData::Pointer m_InstrumentNavigationData;                             ///< navigation data of instrument
mitk::NavigationData::Pointer m_ObjectmarkerNavigationData;                           ///< navigation data of object marker

//members for the point set recording
mitk::NavigationData::Pointer m_PointSetRecordingNavigationData;
mitk::PointSet::Pointer m_PSRecordingPointSet;
bool m_PointSetRecording;
bool m_PermanentRegistration;
bool m_CameraView;

//members for permanent registration
mitk::PointSet::Pointer m_PermanentRegistrationSourcePoints;
mitk::NavigationData::Pointer m_T_MarkerRel;
mitk::NavigationData::Pointer m_T_ObjectReg;
mitk::AffineTransform3D::Pointer m_T_ImageReg;
mitk::AffineTransform3D::Pointer m_T_ImageGeo;
mitk::NavigationData::Pointer m_ObjectmarkerNavigationDataLastUpdate; ///< this is the position of the object marker from the last call of update(); it is used to check the difference and decide if the visualization must be updated


vtkSmartPointer<vtkLandmarkTransform> m_transform;
//######################## some internal help methods ############################
/** @brief Computes the fiducial registration error out of two sets of fiducials.
 *  The two sets must have the same size and the points must correspond to each other.
 *  @param transform        This transform is applied to the image fiducials before the FRE calculation if it is given.
 *  @return                 Returns the FRE. Returns -1 if there was an error.
 */
double ComputeFRE(mitk::PointSet::Pointer trackerFiducials, mitk::PointSet::Pointer ImageWorldFiducials, vtkSmartPointer<vtkLandmarkTransform> transform = NULL);
/**
* Checks if the difference between two given transformations is high which means the method returns
* true if the difference exeeds the given position and angular threshold.
*/
bool IsTransformDifferenceHigh(mitk::NavigationData::Pointer transformA, mitk::NavigationData::Pointer transformB, double euclideanDistanceThreshold = .8, double angularDifferenceThreshold = .8);



};

#endif // MITKIGTMaxillofacialTrackingLab_h

