#include <OpenSG/VRJ/Viewer/plugins/ViewpointsPlugin.h>

#include <OpenSG/VRJ/Viewer/IOV/Viewer.h>
#include <OpenSG/VRJ/Viewer/IOV/PluginCreator.h>
#include <OpenSG/VRJ/Viewer/IOV/User.h>
#include <OpenSG/VRJ/Viewer/IOV/InterfaceTrader.h>
#include <OpenSG/VRJ/Viewer/IOV/WandInterface.h>
#include <OpenSG/VRJ/Viewer/IOV/ViewPlatform.h>
#include <OpenSG/VRJ/Viewer/IOV/Util/Exceptions.h>

#include <boost/format.hpp>
#include <sstream>

#include <gmtl/Matrix.h>
#include <gmtl/Vec.h>
#include <gmtl/MatrixOps.h>
#include <gmtl/VecOps.h>
#include <gmtl/Output.h>
#include <gmtl/Generate.h>

namespace
{
   const std::string vp_plugin_elt_tkn("viewpoints_plugin");
   const std::string vp_control_button_num_tkn("control_button_num");
   const std::string vp_viewpoints_tkn("viewpoints");

   const std::string vp_vp_elt_tkn("viewpoints_vp");
   const std::string vp_pos_elt_tkn("position");
   const std::string vp_rot_elt_tkn("rotation");
}


static inf::PluginCreator sViewpointsPluginCreator(&inf::ViewpointsPlugin::create,
                                                   "Viewpoints Plugin");

extern "C"
{

/** @name Plug-in Entry points */
//@{
IOV_PLUGIN_API(void) getPluginInterfaceVersion(vpr::Uint32& majorVer,
                                               vpr::Uint32& minorVer)
{
   majorVer = INF_PLUGIN_API_MAJOR;
   minorVer = INF_PLUGIN_API_MINOR;
}

IOV_PLUGIN_API(inf::PluginCreator*) getCreator()
{
   return &sViewpointsPluginCreator;
}
//@}

}

namespace inf
{

PluginPtr ViewpointsPlugin::create()
{
   return PluginPtr(new ViewpointsPlugin);
}

void ViewpointsPlugin::init(inf::ViewerPtr viewer)
{
   // Get the wand interface
   InterfaceTrader& if_trader = viewer->getUser()->getInterfaceTrader();
   mWandInterface = if_trader.getWandInterface();

   jccl::ConfigElementPtr elt = viewer->getConfiguration().getConfigElement(vp_plugin_elt_tkn);

   if(!elt)
   {
      std::stringstream ex_msg;
      ex_msg << "Viewpoint plugin could not find its configuration.  "
             << "Looking for type: " << vp_plugin_elt_tkn;
      throw PluginException(ex_msg.str(), IOV_LOCATION);
   }

   // -- Read configuration -- //
   vprASSERT(elt->getID() == vp_plugin_elt_tkn);

   // Get the control button to use
   mControlBtnNum = elt->getProperty<unsigned>(vp_control_button_num_tkn);

   // Read in all the viewpoints
   unsigned num_vps = elt->getNum(vp_viewpoints_tkn);
   for(unsigned i=0;i<num_vps;i++)
   {
      jccl::ConfigElementPtr vp_elt =
         elt->getProperty<jccl::ConfigElementPtr>(vp_viewpoints_tkn, i);
      vprASSERT(vp_elt.get() != NULL);
      float xt = vp_elt->getProperty<float>(vp_pos_elt_tkn, 0);
      float yt = vp_elt->getProperty<float>(vp_pos_elt_tkn, 1);
      float zt = vp_elt->getProperty<float>(vp_pos_elt_tkn, 2);

      float xr = vp_elt->getProperty<float>(vp_rot_elt_tkn, 0);
      float yr = vp_elt->getProperty<float>(vp_rot_elt_tkn, 1);
      float zr = vp_elt->getProperty<float>(vp_rot_elt_tkn, 2);

      gmtl::Coord3fXYZ vp_coord;
      vp_coord.pos().set(xt,yt,zt);
      vp_coord.rot().set(gmtl::Math::deg2Rad(xr),
                         gmtl::Math::deg2Rad(yr),
                         gmtl::Math::deg2Rad(zr));

      Viewpoint vp;
      vp.mXform = gmtl::make<gmtl::Matrix44f>(vp_coord); // Set at T*R
      vp.mName = vp_elt->getName();
      mViewpoints.push_back(vp);
   }
   vprASSERT(mViewpoints.size() == num_vps);

   // Setup the internals
   // - Setup the next viewpoint to use
   if(num_vps > 0)
   {
      mNextViewpoint = 1;
   }
}

//
// Update the state of the plugin
// - If there are viewpoints and the button is pressed
//    - Get the new transform
//    - Set it on the viewplatform
//    - Get ready for the next location
//
void ViewpointsPlugin::updateState(inf::ViewerPtr viewer)
{
   gadget::DigitalInterface& control_button = mWandInterface->getButton(mControlBtnNum);

   // If we have viewpoints and the button has been pressed
   if(!mViewpoints.empty() &&
      control_button->getData() == gadget::Digital::TOGGLE_ON)
   {

      vprASSERT(mNextViewpoint < mViewpoints.size());
      Viewpoint vp = mViewpoints[mNextViewpoint];

      std::cout << boost::format("Selecting new viewpoint: [%s] %s") % mNextViewpoint % vp.mName << std::endl;

      gmtl::Coord3fXYZ new_coord = gmtl::make<gmtl::Coord3fXYZ>(vp.mXform);
      std::cout << "   New pos: " << new_coord << std::endl;

      inf::ViewPlatform& viewplatform = viewer->getUser()->getViewPlatform();
      viewplatform.setCurPos(vp.mXform);

      mNextViewpoint += 1;
      if(mNextViewpoint >= mViewpoints.size())
      {
         mNextViewpoint = 0;
      }
   }
}

void ViewpointsPlugin::run(inf::ViewerPtr viewer)
{
}


} // namespace inf

