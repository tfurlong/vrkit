// vrkit is (C) Copyright 2005-2007
//    by Allen Bierbaum, Aron Bierbuam, Patrick Hartling, and Daniel Shipton
//
// This file is part of vrkit.
//
// vrkit is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// vrkit is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef _INF_LOGO_PLUGIN_H_
#define _INF_LOGO_PLUGIN_H_

#include <IOV/Plugin/PluginConfig.h>

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <gmtl/Matrix.h>
#include <vector>

#include <IOV/Plugin.h>
#include <IOV/WandInterfacePtr.h>

namespace inf
{

/** Plugin for loading logos into the scene.
 *
 */
class LogoPlugin
   : public inf::Plugin, public boost::enable_shared_from_this<LogoPlugin>
{
public:
   static PluginPtr create();

   virtual ~LogoPlugin()
   {;}

   virtual std::string getDescription()
   {
      return std::string("Logo Plugin");
   }

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

   LogoPlugin()
   {;}

protected:
   struct Logo
   {
      std::string           name;
      OSG::TransformNodePtr xformNode;
   };

private:
   std::vector<Logo>    mLogos;  /**< List of loaded logos. */


};

} // namespace inf


#endif