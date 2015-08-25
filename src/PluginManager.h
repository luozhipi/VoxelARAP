//
//  PluginManager.h
//  Preview3D
//
//

#ifndef Preview3D_PluginManager_h
#define Preview3D_PluginManager_h

#include "PreviewPlugin.h"
#include <vector>

// Keeps the lists of plugins
class _PluginManager_
{
public:  //-- SYSTEM PART------------------------------------------------------    
    _PluginManager_() {}
    friend _PluginManager_& PluginManager();
    friend class Preview3D;
	~_PluginManager_() {
		plugin_list_.clear();
	}
    
    
    
    /** Registers a new plugin. A call to this function should be
     implemented in the constructor of all classes derived from PreviewPlugin. */
    bool register_plugin(PreviewPlugin* _bl)
    {
        plugin_list_.push_back(_bl);
        return true;
    }
private:
    
    // stores registered reader modules
    std::vector<PreviewPlugin*> plugin_list_;    
};


void CreatePluginManager();
_PluginManager_& PluginManager();
void DeletePluginManager();

#endif
