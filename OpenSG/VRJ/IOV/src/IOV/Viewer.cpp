#include <algorithm>

#include <OpenSG/OSGConnectionFactory.h>
#include <OpenSG/OSGSimpleAttachments.h>

#include <vpr/vpr.h>
#include <vpr/DynLoad/LibraryLoader.h>
#include <vpr/IO/Socket/InetAddr.h>
#include <jccl/Config/Configuration.h>

#include <OpenSG/VRJ/Viewer/IOV/User.h>
#include <OpenSG/VRJ/Viewer/IOV/Plugin.h>
#include <OpenSG/VRJ/Viewer/IOV/PluginCreator.h>
#include <OpenSG/VRJ/Viewer/IOV/PluginFactory.h>
#include <OpenSG/VRJ/Viewer/IOV/Viewer.h>


namespace inf
{

Viewer::~Viewer()
{
   if ( NULL != mAspect )
   {
      delete mAspect;
   }

   if ( NULL != mConnection )
   {
      delete mConnection;
   }
}

void Viewer::init()
{
   // This has to be called before OSG::osgInit(), which is done by
   // vrj::OpenSGApp::init().
   OSG::ChangeList::setReadWriteDefault();

   vrj::OpenSGApp::init();

   // Verify configuration has been loaded
   bool have_config = (mConfiguration.getAllConfigElements().size() > 0);
   if (!have_config)
   {
      std::cerr << "WARNING: System has no configuration files loaded!" << std::endl;
   }

   // Create an initialize the user
   mUser = User::create();
   mUser->init();

   // Create and initialize the base scene object
   mScene = Scene::create();
   mScene->init();

   // Load the app configuration and then...
   // - Setup scene root for networking
   // - Configure the networking
   // - Load and initialize the plugins
   if ( have_config )
   {
      const std::string app_elt_type("infiscape_opensg_viewer");
      const std::string root_name_prop("root_name");

      jccl::ConfigElementPtr app_cfg = mConfiguration.getConfigElement(app_elt_type);

      if ( app_cfg )
      {
         std::string root_name =
            app_cfg->getProperty<std::string>(root_name_prop);

         // This has to be done before the slave connections are received so
         // that this change is included with the initial sync.
         CoredGroupPtr root_node = mScene->getSceneRoot();
         OSG::beginEditCP(root_node);
            OSG::setName(root_node.node(), root_name);
         OSG::endEditCP(root_node);

         // Set ourselves up as a rendering master (or not depending on the
         // configuration).
         configureNetwork(app_cfg);

         loadAndInitPlugins(app_cfg);

         /*
         if ( ! all_elts.empty() )
         {
            std::cout << "Unconsumed config elements from viewer configuration: " << std::endl;

            std::vector<jccl::ConfigElementPtr>::iterator i;
            for ( i = all_elts.begin(); i != all_elts.end(); ++i )
            {
               std::cout << "\t" << (*i)->getName() << "\n";
            }

            std::cout << std::flush;
         }
         */
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

   if ( NULL != mConnection )
   {
      try
      {
         OSG::UInt8 finish(false);

         mConnection->signal();
         mAspect->sendSync(*mConnection, OSG::Thread::getCurrentChangeList());
         mConnection->putValue(finish);
         mConnection->flush();
         mConnection->wait();

         OSG::Thread::getCurrentChangeList()->clearAll();
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
}

void Viewer::configureNetwork(jccl::ConfigElementPtr appCfg)
{
   const std::string listen_port_prop("listen_port");
   const std::string slave_count_prop("slave_count");

   const unsigned short listen_port =
      appCfg->getProperty<unsigned short>(listen_port_prop);
   const unsigned int slave_count =
      appCfg->getProperty<unsigned int>(slave_count_prop);

   if ( listen_port != 0 && slave_count != 0 )
   {
      vpr::InetAddr local_host_addr;
      if ( vpr::InetAddr::getLocalHost(local_host_addr).success() )
      {
         mAspect = new OSG::RemoteAspect();
         mConnection =
            OSG::ConnectionFactory::the().createGroup("StreamSock");
         local_host_addr.setPort(listen_port);
         std::stringstream addr_stream;
         addr_stream << local_host_addr.getAddressString() << ":"
                     << listen_port;
         mConnection->bind(addr_stream.str());
      }

      mChannels.resize(slave_count);

      for ( unsigned int s = 0; s < slave_count; ++s )
      {
         std::cout << "Waiting for slave #" << s << " to connect ..."
                   << std::endl;
         mChannels[s] = mConnection->acceptPoint();
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

      std::cout << "All " << slave_count << " nodes have connected"
                << std::endl;
   }
}

void Viewer::loadAndInitPlugins(jccl::ConfigElementPtr appCfg)
{
   std::cout << "Loading plugins" << std::endl;

   const std::string plugin_path_prop("plugin_path");
   const std::string plugin_prop("plugin");

   std::vector<std::string> search_path;
   search_path.push_back("plugins");

   const unsigned int num_paths(appCfg->getNum(plugin_path_prop));

   for ( unsigned int i = 0; i < num_paths; ++i )
   {
      search_path.push_back(
         appCfg->getProperty<std::string>(plugin_path_prop, i)
      );
   }

   mPluginFactory = PluginFactory::create();
   mPluginFactory->init(search_path);

   const unsigned int num_plugins(appCfg->getNum(plugin_prop));

   for ( unsigned int i = 0; i < num_plugins; ++i )
   {
      std::string plugin_name =
         appCfg->getProperty<std::string>(plugin_prop, i);

      try
      {
         std::cout << "   Loading plugin: " << plugin_name << " .... " << std::endl;
         inf::PluginCreator* creator =
            mPluginFactory->getPluginCreator(plugin_name);

         if ( NULL != creator )
         {
            inf::PluginPtr plugin = creator->createPlugin();
            plugin->setFocused(true);
            plugin->init(shared_from_this());                     // Initialize the plugin, and configure it
            mPlugins.push_back(plugin);
         }
         else
         {
            std::cout << "[ERROR]\n   Plug-in '" << plugin_name
                      << "' has a NULL creator!" << std::endl;
         }
         std::cout << "[OK]" << std::endl;
      }
      catch (std::runtime_error& ex)
      {
         std::cout << "[FAILED]\n   WARNING: Failed to load plug-in '"
                   << plugin_name << "': " << ex.what() << std::endl;
      }
   }
}

}