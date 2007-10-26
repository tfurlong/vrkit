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

#include <algorithm>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <OpenSG/OSGRemoteAspect.h>
#include <OpenSG/OSGGroupConnection.h>
#include <OpenSG/OSGConnectionFactory.h>
#include <OpenSG/OSGSimpleAttachments.h>
#include <OpenSG/OSGFieldContainerFactory.h>
#include <OpenSG/OSGContainerFactoryMixins.h>

#include <vpr/vpr.h>
#include <vpr/System.h>
#include <vpr/Util/FileUtils.h>
#include <jccl/Config/Configuration.h>

#include <IOV/OpenSG2Shim.h>
#include <IOV/User.h>
#include <IOV/Plugin.h>
#include <IOV/PluginCreator.h>
#include <IOV/PluginFactory.h>
#include <IOV/Viewer.h>

// Embedded plugins
#include <IOV/StatusPanelPlugin.h>


namespace fs = boost::filesystem;

namespace inf
{

Viewer::Viewer()
   : vrj::OpenSGApp(NULL)
   , mAspect(NULL)
   , mConnection(NULL)
{
   mPluginFactory = PluginFactory::create();
}


Viewer::~Viewer()
{
}

void Viewer::init()
{
   vrj::OpenSGApp::init();

   // Verify configuration has been loaded
   bool have_config = (mConfiguration.getAllConfigElements().size() > 0);
   if (!have_config)
   {
      std::cerr << "WARNING: No configuration files were provided to the "
                << "Viewer application!" << std::endl;
   }

   // Create an initialize the user
   mUser = User::create();
   mUser->init();

   // Create and initialize the base scene object
   mScene = Scene::create();
   mScene->init();

   // Load plugins embedded in library
   getPluginFactory()->registerCreator(
      new inf::PluginCreator(&inf::StatusPanelPlugin::create,
                             "StatusPanelPlugin"),
      "StatusPanelPlugin"
   );

   // Load the app configuration and then...
   // - Setup scene root for networking
   // - Configure the networking
   // - Load and initialize the plugins
   if ( have_config )
   {
      const std::string app_elt_type("infiscape_opensg_viewer");
      const std::string cluster_elt_type("iov_cluster");
      const std::string root_name_prop("root_name");

      // -- Configure core application -- //
      jccl::ConfigElementPtr app_cfg =
         mConfiguration.getConfigElement(app_elt_type);

      if ( app_cfg )
      {
         const unsigned int app_cfg_ver(3);
         if ( app_cfg->getVersion() < app_cfg_ver )
         {
            std::cerr << "WARNING: IOV config element '" << app_cfg->getName()
                      << "' is out of date!" << std::endl
                      << "         Current config element version is "
                      << app_cfg_ver << ", but this one is version "
                      << app_cfg->getVersion() << std::endl
                      << "         IOV configuration may fail or be incomplete"
                      << std::endl;
         }

         std::string root_name =
            app_cfg->getProperty<std::string>(root_name_prop);

         // This has to be done before the slave connections are received so
         // that this change is included with the initial sync.
         OSG::GroupNodePtr root_node = mScene->getSceneRoot();
         OSG::beginEditCP(root_node);
            OSG::setName(root_node.node(), root_name);
         OSG::endEditCP(root_node);

         // Setup the plugins that are configured to load
         loadAndInitPlugins(app_cfg);
      }

      OSG::commitChanges();

      // -- Configure cluster support --- //
      jccl::ConfigElementPtr cluster_cfg =
         mConfiguration.getConfigElement(cluster_elt_type);

      if ( cluster_cfg )
      {
         // Set ourselves up as a rendering master (or not depending on the
         // configuration).
         configureNetwork(cluster_cfg);
      }
   }
}

void Viewer::preFrame()
{
   std::vector<inf::PluginPtr>::iterator i;

   // First, we update the state of each plug-in.  All plug-ins will get a
   // consistent view of the run-time state of the system before being run.
   for ( i = mPlugins.begin(); i != mPlugins.end(); ++i )
   {
      (*i)->updateState(shared_from_this());
   }

   // Then, we tell each plug-in to do its thing.  Any given plug-in may
   // change the state of the system as a result of performing its task(s).
   for ( i = mPlugins.begin(); i != mPlugins.end(); ++i )
   {
      (*i)->run(shared_from_this());
   }

   // Update the user (and navigation)
   getUser()->update(shared_from_this());

   OSG::commitChanges();
}

void Viewer::latePreFrame()
{
   //static int iter_num(0);

   OSG::Connection::Channel channel;

   // If we have networking to do then do it
   if ( NULL != mConnection )
   {
      try
      {
         OSG::UInt8 finish(false);

         mConnection->signal();
         mAspect->sendSync(*mConnection, OSG::Thread::getCurrentChangeList());
         mConnection->putValue(finish);
         sendDataToSlaves(*mConnection);
         mConnection->flush();

         while(mConnection->getSelectionCount()>0)
         {
            channel = mConnection->selectChannel();
            readDataFromSlave(*mConnection);
            mConnection->subSelection(channel);
         }

         mConnection->resetSelection();
      }
      catch (OSG::Exception& ex)
      {
         std::cerr << ex.what() << std::endl;
         // XXX: How do we find out which channel caused the exception to be
         // thrown so that we can disconnect from it?
//         mConnection->disconnect();
         // XXX: We should not be dropping the connection with all nodes just
         // because one (or more) may have gone away.
         delete mConnection;
         mConnection = NULL;
      }
   }

   OSG::commitChanges();
 
   // We are using writeable change lists, so we need to clear them out
   // We do this here because it should be after anything else that the user may want to do
   OSG::Thread::getCurrentChangeList()->clear();
}

void Viewer::sendDataToSlaves(OSG::BinaryDataHandler& writer)
{
   OSG::UInt8 junk(false);
   writer.putValue(junk);
   float near_val, far_val;
   vrj::Projection::getNearFar(near_val, far_val);
   writer.putValue(near_val);
   writer.putValue(far_val);
}

void Viewer::readDataFromSlave(OSG::BinaryDataHandler& reader)
{
   OSG::UInt8 junk;
   reader.getValue(junk);
}

void Viewer::exit()
{
   // First we free up all the OpenSG resources that have been allocated
   // for this application.
   deallocate();

   // Then we call up to the base class implementation of this method, which
   // in turn tells OpenSG to exit.
   vrj::OpenSGApp::exit();
}

void Viewer::deallocate()
{
   if ( NULL != mAspect )
   {
      delete mAspect;
   }

   if ( NULL != mConnection )
   {
      delete mConnection;
   }

   mUser.reset();
   mScene.reset();
   mPlugins.clear();
   mPluginFactory.reset();

#if 0
   // Turn this off for now XXX dshipton 2/14/07
   
   // Output information about what is left over
   typedef std::vector<OSG::FieldContainerPtr> FieldContainerStore;
   typedef FieldContainerStore::const_iterator FieldContainerStoreConstIt;

   const FieldContainerStore pFCStore = OSG::FieldContainerFactory::the()->getContainerStore();
   unsigned int num_types = OSG::FieldContainerFactory::the()->getNumTypes();

   std::vector<unsigned int> type_ids;
   OSG::FieldContainerFactory::TypeMapIterator i;
   for ( i = OSG::FieldContainerFactory::the()->beginTypes(); 
	 i != OSG::FieldContainerFactory::the()->endTypes(); ++i )
   {
      if( ((*i).second != NULL) && ((*i).second)->getPrototype() != OSG::NullFC)
      {
         unsigned int type_proto_ptr_id =
            ((*i).second)->getPrototype().getFieldContainerId();
         type_ids.push_back(type_proto_ptr_id);
         //std::cout << "Protyo ptr id: " << type_proto_ptr_id << std::endl;
      }
   }

   std::cout << "Viewer::deallocate():\n"
             << "                                    num_types: " << num_types << std::endl
             << "    Total OpenSG objects allocated (w/ types): " << pFCStore->size()
             << std::endl;

   unsigned int non_null_count(0);

   for ( unsigned int i = 0; i < pFCStore->size(); ++i )
   {
      OSG::FieldContainerPtr ptr = (*pFCStore)[i];
      if(ptr != OSG::NullFC)
      {
         non_null_count += 1;
      }
   }

   std::cout << "Remaining non-null OpenSG objects (w/o types): "
             << (non_null_count-num_types) << std::endl;
#endif

// Enable this section when you want to see the names and types of the objects
// that remain.
#if 0
   std::cout << " ---- non-null objects remaining --- " << std::endl;
   for(unsigned i=0; i<pFCStore->size(); ++i)
   {
      OSG::FieldContainerPtr ptr = (*pFCStore)[i];
      if( (std::count(type_ids.begin(), type_ids.end(), i) == 0)
           && (ptr != OSG::NullFC) )
      {
         OSG::AttachmentContainerPtr acp =
            OSG::AttachmentContainerPtr::dcast(ptr);
         const char* node_name = OSG::getName(acp);
         std::string node_name_str("<NULL>");
         if(NULL != node_name)
         {
            node_name_str = std::string(node_name);
         }

         std::cout << "   " << i << ": "
                   << "  type:" << ptr->getType().getName().str() << " id:"
                   << ptr.getFieldContainerId()
                   << " name:" << node_name_str
                   << std::endl;
      }
   }
#endif

}

/**
 * See @ref SlaveCommunicationProtocol for details of the communication.
 */
void Viewer::configureNetwork(jccl::ConfigElementPtr clusterCfg)
{
   const std::string listen_addr_prop("listen_addr");
   const std::string listen_port_prop("listen_port");
   const std::string slave_count_prop("slave_count");

   const std::string listen_addr =
      clusterCfg->getProperty<std::string>(listen_addr_prop);
   const unsigned short listen_port =
      clusterCfg->getProperty<unsigned short>(listen_port_prop);
   const unsigned int slave_count =
      clusterCfg->getProperty<unsigned int>(slave_count_prop);

   // If we have a port and at least one slave, then we need to set things up
   // for the incoming slave connections.
   if ( listen_port != 0 && slave_count != 0 )
   {
      std::cout << "Setting up remote slave network:" << std::endl;
      mAspect = new OSG::RemoteAspect();
      mConnection = OSG::ConnectionFactory::the()->createGroup("StreamSock");

      // Construct the binding address to hand off to OpenSG.
      // At this point, listen_addr may be an empty string. This would give us
      // the binding address ":<listen_port>", which is fine because
      // OSG::PointSockConnection::bind() only looks at the port information.
      std::stringstream addr_stream;
      addr_stream << listen_addr << ":" << listen_port;
      std::cout << "   Attempting to bind to: " << addr_stream.str()
                << std::flush;

      // To set the IP address to which mConnection will be bound, we have to
      // do this ridiculous two-step process. If listen_addr is empty, then
      // OSG::PointSockConnection will the local host name.
      mConnection->setInterface(listen_addr);
      mConnection->bind(addr_stream.str());
      std::cout << " [OK]" << std::endl;

      mChannels.resize(slave_count);

      for ( unsigned int s = 0; s < slave_count; ++s )
      {
         std::cout << "   Waiting for slave #" << s << " to connect ..."
                   << std::flush;
         mChannels[s] = mConnection->acceptPoint();
         std::cout << "[OK]" << std::endl;
      }

      OSG::UInt8 finish(false);

      // Signal the slave nodes that we are about to send the initial sync.
      mConnection->signal();

      // Provide the slave nodes with a consistent rendering scale factor.
      mConnection->putValue(getDrawScaleFactor());
      mAspect->sendSync(*mConnection, OSG::Thread::getCurrentChangeList());
      mConnection->putValue(finish);
      mConnection->flush();

      // NOTE: We are not clearing the change list at this point
      // because that would blow away any actions taken during the
      // initialization of mScene.

      std::cout << "   All " << slave_count << " slave nodes have connected"
                << std::endl;
   }
}

void Viewer::loadAndInitPlugins(jccl::ConfigElementPtr appCfg)
{
   std::cout << "Viewer: Loading plugins" << std::endl;

   const std::string plugin_path_prop("plugin_path");
   const std::string plugin_prop("plugin");

   const std::string iov_base_dir_tkn("IOV_BASE_DIR");

   std::vector<std::string> search_path;

   // Set up two default search paths:
   //    1. Relative path to './plugins'
   //    2. IOV_BASE_DIR/lib/IOV/plugins
   search_path.push_back("plugins");

   std::string iov_base_dir;
   if ( vpr::System::getenv(iov_base_dir_tkn, iov_base_dir).success() )
   {
      fs::path iov_base_path(iov_base_dir, fs::native);
      fs::path def_iov_plugin_path = iov_base_path / "lib/IOV/plugins";

      if ( fs::exists(def_iov_plugin_path) )
      {
         std::string def_search_path =
            def_iov_plugin_path.native_directory_string();
         std::cout << "Setting default IOV plug-in path: " << def_search_path
                   << std::endl;
         search_path.push_back(def_search_path);
      }
      else
      {
         std::cerr << "Default IOV plug-in path does not exist: "
                   << def_iov_plugin_path.native_directory_string()
                   << std::endl;
      }
   }

   // Add paths from the application configuration
   const unsigned int num_paths(appCfg->getNum(plugin_path_prop));

   for ( unsigned int i = 0; i < num_paths; ++i )
   {
      std::string dir = appCfg->getProperty<std::string>(plugin_path_prop, i);
      search_path.push_back(vpr::replaceEnvVars(dir));
   }

   mPluginFactory->init(search_path);

   const unsigned int num_plugins(appCfg->getNum(plugin_prop));

   for ( unsigned int i = 0; i < num_plugins; ++i )
   {
      std::string plugin_name =
         appCfg->getProperty<std::string>(plugin_prop, i);

      try
      {
         std::cout << "   Loading plugin: " << plugin_name << " .... ";
         inf::PluginCreator* creator =
            mPluginFactory->getPluginCreator(plugin_name);

         if ( NULL != creator )
         {
            inf::PluginPtr plugin = creator->createPlugin();
            plugin->setFocused(shared_from_this(), true);
            // Initialize the plugin, and configure it.
            plugin->init(shared_from_this());
            mPlugins.push_back(plugin);
            std::cout << "[OK]" << std::endl;
         }
         else
         {
            std::cout << "[ERROR]\n   Plug-in '" << plugin_name
                      << "' has a NULL creator!" << std::endl;
         }
      }
      catch (std::runtime_error& ex)
      {
         std::cout << "[FAILED]\n   WARNING: Failed to load plug-in '"
                   << plugin_name << "': " << ex.what() << std::endl;
      }
   }
}

}