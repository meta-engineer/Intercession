#ifndef SCRIPT_LIBRARY_H
#define SCRIPT_LIBRARY_H

//#include "intercession_pch.h"
#include <memory>
#include <string>
#include <typeinfo>

#include "scripting/i_script_drivetrain.h"
#include "logging/pleep_log.h"

namespace pleep
{
    // A static (global) access point to coordinate memory for script objects
    //  (as multiple entities could want to use the same script)
    // Scripts are for functionality not specific enough to justify a dedicated dynamo
    // or for functionality which required components cannot be strictly defined
    // However, we still want the efficiency afforded by funneling components to a single relay.
    // ScriptComponents shouldn't create their own script object, but fetch it from a ScriptLibrary
    // which can manage their allocation in lieu of a dynamo.
    // In order to ensure all users have the same instance this will be a static singleton.
    class ScriptLibrary
    {
    protected:
        // Cannot log from this constructor because it is invoked before logger init in main
        ScriptLibrary() = default;
    public:
        ~ScriptLibrary() = default;
        // copy constructor
        ScriptLibrary(const ScriptLibrary&) = delete;

        // throws out_of_range if script type has not been registered
        // scripts == script drivetrain
        static std::shared_ptr<I_ScriptDrivetrain> fetch_scripts(std::string scriptsTypename)
        {
            // at() throws out_of_range if key is not found
            try
            {
                return ScriptLibrary::m_singleton->m_scriptMap.at(scriptsTypename);
            }
            catch(const std::exception& err)
            {
                UNREFERENCED_PARAMETER(err);
                PLEEPLOG_ERROR(err.what());
                PLEEPLOG_ERROR("Could not fetch script type: " + scriptsTypename + " from ScriptLibrary, it may not have been registered. Returning nullptr");
            }
            return nullptr;
        }

        // alternative form for aesthetics (extract typeid name from template)
        template <typename T_Scripts>
        static std::shared_ptr<I_ScriptDrivetrain> fetch_scripts()
        {
            return ScriptLibrary::fetch_scripts(typeid(T_Scripts).name());
        }

        // should be called by a CosmosContext before building a cosmos
        template <typename T_Scripts>
        static void register_script()
        {
            ScriptLibrary::m_singleton->m_scriptMap.insert({
                std::string(typeid(T_Scripts).name()),
                std::make_shared<T_Scripts>()
            });
        }

        static void clear_registery()
        {
            ScriptLibrary::m_singleton->m_scriptMap.clear();
        }

    protected:
        // string -> typeid of I_ScriptDrivetrain subclass
        std::unordered_map<std::string, std::shared_ptr<I_ScriptDrivetrain>> m_scriptMap;

        // users don't need to be able to literally 'get' this instance, so it can be unique
        // initialized in script_library.cpp
        static std::unique_ptr<ScriptLibrary> m_singleton;
    };
}

#endif // SCRIPT_LIBRARY_H