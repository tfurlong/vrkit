#ifndef _INF_MODE_SWITCH_PLUGIN_H_
#define _INF_MODE_SWITCH_PLUGIN_H_

#include <IOV/Plugin/PluginConfig.h>

#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>

#include <vpr/vpr.h>
#include <vpr/DynLoad/Library.h>

#include <IOV/Plugin.h>
#include <IOV/WandInterfacePtr.h>
#include <IOV/Plugin/Buttons.h>


namespace inf
{

/** Mode switching plugin.
 *
 * This plugin manages a set of other plugins and switches among them
 * when the user changes "modes" in the application.
 */
class ModeSwitchPlugin
   : public inf::Plugin
   , public boost::enable_shared_from_this<ModeSwitchPlugin>
{
public:
   static inf::PluginPtr create()
   {
      return inf::PluginPtr(new ModeSwitchPlugin());
   }

   virtual ~ModeSwitchPlugin()
   {
      /* Do nothing. */ ;
   }

   virtual std::string getDescription();

   virtual void init(inf::ViewerPtr viewer);

   virtual void updateState(inf::ViewerPtr viewer);

   virtual void run(inf::ViewerPtr viewer);

   /**
    * Invokes the global scope delete operator.  This is required for proper
    * releasing of memory in DLLs on Win32.
    */
   void operator delete(void* p)
   {
      ::operator delete(p);
   }

protected:
   /**
    * Deletes this object.  This is an implementation of the pure virtual
    * inf::Plugin::destroy() method.
    */
   virtual void destroy()
   {
      delete this;
   }

   ModeSwitchPlugin()
      : mSwitchButton(-1)
      , mCurrentMode(0)
   {
      /* Do nothing. */ ;
   }

   static std::string getElementType()
   {
      return std::string("mode_switch_plugin");
   }

   WandInterfacePtr mWandInterface;
   int              mSwitchButton;

   unsigned                      mCurrentMode;     /**< Current active plugin. */
   std::vector<inf::PluginPtr>   mPlugins;
};

}


#endif
