#include <iostream>
#include <sstream>

#include <boost/filesystem/path.hpp>

#include <vpr/vpr.h>
#include <vpr/DynLoad/LibraryFinder.h>
#include <vpr/Util/Assert.h>

#include <OpenSG/VRJ/Viewer/IOV/Plugin.h>
#include <OpenSG/VRJ/Viewer/IOV/PluginFactory.h>


namespace fs = boost::filesystem;

namespace inf
{

void PluginFactory::init(const std::vector<std::string>& scanPath)
{
   // Determine the platform-specific file extension used for dynamically
   // loadable code.
#if defined(VPR_OS_Win32)
   const std::string driver_ext("dll");
#elif defined(VPR_OS_Darwin)
   // NOTE: The file extension "bundle" may be appropriate in some cases.
   const std::string driver_ext("dylib");
#else
   const std::string driver_ext("so");
#endif

   // Determine the build-specific part of the dynamically loadable module
   // file name to be stripped off.  For a debug build, this would be "_d.so"
   // on most UNIX-based platforms.  For an optimized build, it would simply
   // be ".so" on the same platforms.
#if defined(_DEBUG)
   const std::string strip_str = std::string("_d.") + driver_ext;
#else
   const std::string strip_str = std::string(".") + driver_ext;
#endif

   std::vector<std::string>::const_iterator i;
   for ( i = scanPath.begin(); i != scanPath.end(); ++i )
   {
      vpr::LibraryFinder finder(*i, driver_ext);
      vpr::LibraryFinder::LibraryList libs = finder.getLibraries();

      for ( unsigned int j = 0; j < libs.size(); ++j )
      {
         // Construct the platform-agnostic plug-in name by getting the name
         // of the plug-in file (leading path information is removed) and
         // then stripping off strip_str.
         fs::path lib_path(libs[j]->getName(), fs::native);
         const std::string lib_name(lib_path.leaf());
         const std::string::size_type strip_pos = lib_name.find(strip_str);

         if ( strip_pos != std::string::npos )
         {
            const std::string plugin_name(lib_name.substr(0, strip_pos));

            // Register the vpr::LibraryPtr object using the platform-agnostic
            // name so that callers of getPluginLibrary() and
            // getPluginCreator() do not have to worry about platform-specific
            // naming issues.
            mPluginLibs[plugin_name] = libs[j];
         }
         else
         {
            std::cout << "WARNING: Invalid plug-in library name encountered: '"
                      << libs[j]->getName() << "'" << std::endl;
         }
      }
   }
}

vpr::LibraryPtr PluginFactory::getPluginLibrary(const std::string& name) const
   throw(inf::NoSuchPluginException)
{
   std::map<std::string, vpr::LibraryPtr>::const_iterator lib =
      mPluginLibs.find(name);

   if ( lib != mPluginLibs.end() )
   {
      return (*lib).second;
   }
   else
   {
      std::stringstream msg_stream;
      msg_stream << "No plug-in named '" << name << "' exists";
      throw NoSuchPluginException(msg_stream.str());
   }
}

inf::PluginCreator* PluginFactory::getPluginCreator(const std::string& name)
   throw(inf::PluginLoadException, inf::NoSuchPluginException,
         inf::PluginInterfaceException)
{
   // Get the vpr::LibraryPtr for the named plug-in.  This will throw an
   // exception if name is not a valid plug-in name.
   vpr::LibraryPtr plugin_lib = getPluginLibrary(name);

   // At this point, we know that the given name must be a valid plug-in name.
   if ( plugin_lib->isLoaded() )
   {
      std::map<std::string, inf::PluginCreator*>::iterator c =
         mPluginCreators.find(name);

      if ( c == mPluginCreators.end() )
      {
         registerCreator(plugin_lib, name);
      }
   }
   else
   {
      if ( plugin_lib->load().success() )
      {
         registerCreator(plugin_lib, name);
      }
      else
      {
         std::stringstream msg_stream;
         msg_stream << "Plug-in '" << name << "' failed to load";
         throw PluginLoadException(msg_stream.str());
      }
   }

   return mPluginCreators[name];
}

void PluginFactory::registerCreator(vpr::LibraryPtr pluginLib,
                                    const std::string& name)
   throw(inf::PluginInterfaceException)
{
   vprASSERT(pluginLib->isLoaded() && "Plug-in library is not loaded");

   validatePluginInterface(pluginLib);

   const std::string get_creator_func("getCreator");
   void* creator_symbol = pluginLib->findSymbol(get_creator_func);

   if ( creator_symbol != NULL )
   {
      inf::PluginCreator* (*creator_func)();
      creator_func = (inf::PluginCreator* (*)()) creator_symbol;
      mPluginCreators[name] = (*creator_func)();
   }
   else
   {
      std::stringstream msg_stream;
      msg_stream << "Plug-in '" << pluginLib->getName()
                 << "' has no entry point function named "
                 << get_creator_func;
      throw inf::PluginInterfaceException(msg_stream.str());
   }
}

void PluginFactory::validatePluginInterface(vpr::LibraryPtr pluginLib)
   throw(inf::PluginInterfaceException)
{
   vprASSERT(pluginLib->isLoaded() && "Plug-in library is not loaded");

   const std::string get_version_func("getPluginInterfaceVersion");

   void* version_symbol = pluginLib->findSymbol(get_version_func);

   if ( version_symbol == NULL )
   {
      std::stringstream msg_stream;
      msg_stream << "Plug-in '" << pluginLib->getName()
                 << "' has no entry point function named " << get_version_func;
      throw inf::PluginInterfaceException(msg_stream.str());
   }
   else
   {
      void (*version_func)(vpr::Uint32&, vpr::Uint32&);
      version_func = (void (*)(vpr::Uint32&, vpr::Uint32&)) version_symbol;

      vpr::Uint32 major_ver;
      vpr::Uint32 minor_ver;
      (*version_func)(major_ver, minor_ver);

      if ( major_ver != INF_PLUGIN_API_MAJOR )
      {
         std::stringstream msg_stream;
         msg_stream << "Interface version mismatch: run-time does not match "
                    << "compile-time plug-in setting ("
                    << INF_PLUGIN_API_MAJOR << "." << INF_PLUGIN_API_MINOR
                    << " != " << major_ver << "." << minor_ver << ")";
         throw inf::PluginInterfaceException(msg_stream.str());
      }
   }
}

}