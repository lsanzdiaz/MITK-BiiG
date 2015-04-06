
#ifndef MITKAscensionMEDSAFETrackingTool_H_HEADER_INCLUDED_
#define MITKAscensionMEDSAFETrackingTool_H_HEADER_INCLUDED_

#include <mitkInternalTrackingTool.h>
#include <MitkIGTExports.h>
#include <mitkVector.h>
#include <itkFastMutexLock.h>

namespace mitk {


  /**Documentation
  * \brief implements TrackingTool interface
  *
  * This class is a complete TrackingTool implementation. It can either be used directly by
  * TrackingDevices, or be subclassed for more specific implementations.
  * mitk::MicroBirdTrackingDevice uses this class to manage its tools. Other tracking devices
  * uses specialized versions of this class (e.g. mitk::NDITrackingTool)
  *
  * \ingroup IGT
  */
  class MitkIGT_EXPORT AscensionMEDSAFETrackingTool : public InternalTrackingTool
  {
  public:
    mitkClassMacro(AscensionMEDSAFETrackingTool, InternalTrackingTool);
    friend class AscensionMEDSAFETrackingDevice;


	/**
	* \brief Set the valid flag for tracking data to true
	* @throw mitk::IGTException Throws an exception if
	*/
	void SetDataValid(bool _arg);

	/**
	* \brief Update location and orientation of the tool
	* @throw mitk::IGTException Throws an exception if
	*/
	void updateTool(int ID, double x, double y, double z, double a, double b, double c, double diff);

  protected:
    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)
    AscensionMEDSAFETrackingTool();
    virtual ~AscensionMEDSAFETrackingTool();
  };
} // namespace mitk
#endif /* MITKAscensionMEDSAFETrackingTool_H_HEADER_INCLUDED_ */
