// Copyright (C) Infiscape Corporation 2005-2007

#include <OpenSG/OSGConfig.h>

#include <IOV/PluginCreator.h>
#include <IOV/Viewer.h>
#include <IOV/Version.h>
#include <IOV/Plugin/Info.h>

#include "VolumeDrawingPlugin.h"


static inf::PluginCreator<inf::Plugin> sPluginCreator(
   &inf::VolumeDrawingPlugin::create
);

extern "C"
{

/** @name Plug-in Entry Points */
//@{
IOV_PLUGIN_API(inf::plugin::Info) getPluginInfo()
{
   std::vector<unsigned int> version(3);
   version[0] = IOV_VERSION_MAJOR;
   version[1] = IOV_VERSION_MINOR;
   version[2] = IOV_VERSION_PATCH;

   return inf::plugin::Info("com.infiscape", "VolumeDrawingPlugin", version);
}

IOV_PLUGIN_API(void) getPluginInterfaceVersion(vpr::Uint32& majorVer,
                                               vpr::Uint32& minorVer)
{
   majorVer = INF_PLUGIN_API_MAJOR;
   minorVer = INF_PLUGIN_API_MINOR;
}

IOV_PLUGIN_API(inf::PluginCreatorBase*) getCreator()
{
   return &sPluginCreator;
}
//@}
}

namespace inf
{

VolumeDrawingPlugin::VolumeDrawingPlugin()
   : inf::Plugin()
{
   /* Do nothing. */ ;
}

PluginPtr VolumeDrawingPlugin::init(inf::ViewerPtr)
{
   return shared_from_this();
}

void VolumeDrawingPlugin::contextInit(inf::ViewerPtr viewer)
{
   viewer->getRenderAction()->setVolumeDrawing(isFocused());
}

void VolumeDrawingPlugin::update(inf::ViewerPtr)
{
   /* Do nothing. */ ;
}

}
